#include "demod_nfm.h"

// Collection of NFM experiments, original Pebble, dttsp, CuteSDR

#define FMPLL_RANGE 15000.0	//maximum deviation limit of PLL
#define VOICE_BANDWIDTH 3000.0

#define FMPLL_BW VOICE_BANDWIDTH	//natural frequency ~loop bandwidth
#define FMPLL_ZETA .707				//PLL Loop damping factor

#define FMDC_ALPHA 0.001	//time constant for DC removal filter

//This is used to calc a gain in Init() but wayyyy too large
//Look into it later, but for now just use fixed gain that sounds about right
#define MAX_FMOUT 100000.0

#define SQUELCH_MAX 8000.0		//roughly the maximum noise average with no signal
#define SQUELCHAVE_TIMECONST .02
#define SQUELCH_HYSTERESIS 50.0

#define DEMPHASIS_TIME 80e-6

Demod_NFM::Demod_NFM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{
    fmIPrev = 0.0;
    fmQPrev = 0.0;

    //FMN PLL config
    pllFreqErrorDC = 0.0;
    pllPhase = 0.0; //Tracks ref sig in PLL
    ncoFrequency = 0.0;

    fmBandwidth = 5000;
    Init(sampleRate);
}

Demod_NFM::~Demod_NFM()
{
}

void Demod_NFM::Init(double samplerate)
{
    sampleRate = samplerate;

    double norm = TWOPI/sampleRate;	//to normalize Hz to radians

    //initialize the PLL
    ncoLLimit = -FMPLL_RANGE * norm;		//clamp FM PLL NCO
    ncoHLimit = FMPLL_RANGE * norm;
    pllAlpha = 2.0 * FMPLL_ZETA * FMPLL_BW * norm;
    pllBeta = (pllAlpha * pllAlpha) / (4.0 * FMPLL_ZETA * FMPLL_ZETA);

    //Check this, gain is like 33k!
    //pllOutGain = MAX_FMOUT / ncoHLimit;	//audio output level gain value
    pllOutGain = 0.0005; //Same as FM2

    //DC removal filter time constant
    pllDcAlpha = (1.0 - exp(-1.0 / (sampleRate * FMDC_ALPHA)) );

    lpFilter.InitLPFilter(0,1.0,50.0,VOICE_BANDWIDTH, 1.6*VOICE_BANDWIDTH, sampleRate);

    //Pick up noise squelch from CuteSDR sometime
}

void Demod_NFM::SimplePhase(CPX *in, CPX *out, int demodSamples)
{
    float tmp;

    for (int i=0;i<demodSamples;i++)
    {
        tmp = tan(in[i].re  / in[i].im);
        out[i].re = out[i].im = tmp;
    }
}


//Doug Smith Eq22.  I think this is what SDRadio also used
//Note: Not sure about divisor, Doug uses I^2 + Q^2, Ireland uses I*Ip + Q*Qp
/*
These 3 formulas are all derivations of the more complex phase delta algorithm (SimpleFM2) and
are easier to compute.
Ip & Qp are the previous sample, I & Q are the current sample
1) (Q * Ip - I * Qp) / (I * Ip + Q * Qp)
2) (Q * Ip - I * Qp) / (I^2 + Q^2)
3) (Q * Ip + I * Qp) / (I^2 + Q^2) //COMMAND by Andy Talbot RSGB, haven't tested adding vs sub yet
*/

/*
  Another comment on FM from compDSP
    That is a pretty heavyweight method. Here is more efficient way:
    FM Output = I*dQ/dt - Q*dI/dt
    You may have to take care about the input amplitude limiting.
*/

void Demod_NFM::ProcessBlockFM1(CPX *in, CPX *out, int demodSamples)
{
    float tmp;
    float I,Q; //To make things more readable

    for (int i=0;i<demodSamples;i++)
    {
        I = in[i].re;
        Q = in[i].im;

        tmp = (Q * fmIPrev - I * fmQPrev) / (I * fmIPrev + Q * fmQPrev); //This seems to work better, less static, better audio
        //tmp = (Q * Ip - I * Qp) / (I * I + Q * Q);

        //Output volume is very low, scaling by even 100 doesn't do anything?
        out[i].re = out[i].im = tmp * .0005;
        fmIPrev = I;
        fmQPrev = Q;
    }

}

//From Doug Smith QEX article, Eq 21,  based on phase delta
//From Erio Blossom FM article in Linux Journal Sept 2004
//5/12 Working well for NFM
void Demod_NFM::ProcessBlockFM2(CPX *in, CPX *out, int demodSamples)
{
    CPX prod;

    //Based on phase delta between samples, so we always need last sample from previous run
    static CPX lastCpx(0,0);
    for (int i=0; i < demodSamples; i++)
    {
        //The angle between to subsequent samples can be calculated by multiplying one by the complex conjugate of the other
        //and then calculating the phase (arg() or atan()) of the complex product
        prod = in[i] * lastCpx.conj();
        //Scale demod output to match am, usb, etc range
        out[i].re = out[i].im = prod.phase() *.0005;
        lastCpx = in[i];
    }
}

/*
  FM Peak Deviation is max freq change above and below the carrier
  FM applications use peak deviations based on channel spacing
  75 kHz (200 kHz spacing) FM Broadcast leaves 25khz between channel on hi and lo side of carrier
  5 kHz (25 kHz spacing) NBFM
  2.25 kHz (12.5 kHz spacing) NBFM like FRS, Amateur, etc
  2 kHz (8.33 kHz spacing) NBFM
*/
//Uses Doug Smith phase delta equations
//PLL algorithm from dttsp
//See CuteSDR by Moe Wheatley for similar examples
/*
  I think this is the way this works
  PLL tries to follow the changing frequency of the input signal
  The amount of correction for each sample is the frequency change
*/
//Reference, not used or updated
void Demod_NFM::PllFMN(  CPX * in, CPX * out, int demodSamples )
{
    int ns = demodSamples;

    //All these can be calculated once, not each call
    //time constant for DC removal filter
    const float fmDcAlpha = (1.0 - exp(-1.0 / (sampleRate * 0.001)) );
    const float fmPllZeta = .707;  //PLL Loop damping factor
    const float fmPllBW = fmBandwidth /2;// 3000.0;
    float norm = TWOPI/sampleRate;	//to normalize Hz to radians

    //Original alpha/beta calculations
    //pllAlpha = 0.3 * fmBandwidth * TWOPI/sampleRate;
    //pllBeta = pllAlpha * pllAlpha * 0.25;

    const float fmPllAlpha = 2.0 * fmPllZeta * fmPllBW * norm;
    const float fmPllBeta = (fmPllAlpha * fmPllAlpha) / (4.0 * fmPllZeta * fmPllZeta);

    const float fmPllRange = fmPllBW; //15000.0;	//maximum deviation limit of PLL
    const float fmPllLoLimit = -fmPllRange * norm; //PLL will not be allowed to exceed this range
    const float fmPllHiLimit = fmPllRange * norm;


    CPX pllNCO; //PLL NCO current value
    CPX delay;
    float phaseError;

    for (int i = 0; i < ns; i++) {
        //Todo: See if we can use NCO here
        //This is the generated signal to sync with (NCO)
        pllNCO.re = cos(pllPhase);
        pllNCO.im = sin(pllPhase);

        //Shift signal by PLL.  Should be same as CPX operator * ie pll * in[i]
        delay.re = pllNCO.re * in[i].re - pllNCO.im * in[i].im;
        delay.im = pllNCO.re * in[i].im + pllNCO.im * in[i].re;

        // same as -atan2(tmp.im, tmp.re), but with special handling in cpx class
        phaseError = -delay.phase();
        //phaseError = -atan2(delay.im,delay.re);

        //phaseError is the delta from last sample, ie demod value.  Rest is cleanup
        ncoFrequency += fmPllBeta * phaseError / 100;  //Scale down to avoid overlaod

        //Keep the PLL within our limits
        if (ncoFrequency < fmPllLoLimit)
            ncoFrequency = fmPllLoLimit;
        if (ncoFrequency > fmPllHiLimit)
            ncoFrequency = fmPllHiLimit;

        //Next value for NCO
        pllPhase += ncoFrequency + fmPllAlpha * phaseError;

        //LP filter the NCO frequency term to get DC offset value
        pllFreqErrorDC = (1.0 - fmDcAlpha) * pllFreqErrorDC + fmDcAlpha * ncoFrequency;
        //Change in frequency - dc term is our demodulated output
        out[i].re = out[i].im = (ncoFrequency - pllFreqErrorDC);

    }
    //fmod will keep pllPhase <= TWOPI
    pllPhase = fmod(pllPhase, TWOPI);

}

//CuteSDR PLL version that works
void Demod_NFM::ProcessBlockNCO(CPX* in, CPX* out, int demodSamples)
{
    CPX tmp;
    //Pick up noise squelch from CuteSDR sometime

    for(int i=0; i<demodSamples; i++)
    {
        double ncoSin = sin(pllPhase);
        double ncoCos = cos(pllPhase);
        //complex multiply input sample by NCO's  sin and cos
        tmp.re = ncoCos * in[i].re - ncoSin * in[i].im;
        tmp.im = ncoCos * in[i].im + ncoSin * in[i].re;
        //find current sample phase after being shifted by NCO frequency
        double phzerror = -atan2(tmp.im, tmp.re);

        ncoFrequency += (pllBeta * phzerror);		//  radians per sampletime
        //clamp NCO frequency so doesn't drift out of lock range
        if(ncoFrequency > ncoHLimit)
            ncoFrequency = ncoHLimit;
        else if(ncoFrequency < ncoLLimit)
            ncoFrequency = ncoLLimit;
        //update NCO phase with new value
        pllPhase += (ncoFrequency + pllAlpha * phzerror);
        //LP filter the NCO frequency term to get DC offset value
        pllFreqErrorDC = (1.0 - pllDcAlpha) * pllFreqErrorDC + pllDcAlpha * ncoFrequency;
        //subtract out DC term to get FM audio
        out[i] = (ncoFrequency - pllFreqErrorDC) * pllOutGain;
    }
    pllPhase = fmod(pllPhase, TWOPI);	//keep radian counter bounded
    //lpFilter is part of squelch logic
    lpFilter.ProcessFilter(demodSamples, out, out);

}
