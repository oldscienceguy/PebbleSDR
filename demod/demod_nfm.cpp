#include "demod_nfm.h"

Demod_NFM::Demod_NFM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{

}

Demod_NFM::~Demod_NFM()
{
    fmIPrev = 0.0;
    fmQPrev = 0.0;

    //FMN PLL config
    fmLoLimit = -6000 * TWOPI/sampleRate;
    fmHiLimit = 6000 * TWOPI/sampleRate;
    fmBandwidth = 5000;

    //These change with demod mode, move
    pllAlpha = 0.3 * fmBandwidth * TWOPI/sampleRate;
    pllBeta = pllAlpha * pllAlpha * 0.25;
    fmDCOffset = 0.0;
    pllPhase = 0.0;  //Tracks ref sig in PLL
    pllFrequency = 0.0;




}

void Demod_NFM::ProcessBlock(CPX *in, CPX *out, int demodSamples)
{
    //SimpleFM(in,out);
    SimpleFM2(in,out, demodSamples); //5/12 Working well for NFM
    //PllFMN(in,out, bufSize); //6/8/12 Scaled output, now sounds better than SimpleFM2

}

void Demod_NFM::SimplePhase(CPX *in, CPX *out, int _numSamples)
{
    float tmp;
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
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

void Demod_NFM::SimpleFM(CPX *in, CPX *out, int demodSamples)
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
void Demod_NFM::SimpleFM2(CPX *in, CPX *out, int demodSamples)
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
        pllFrequency += fmPllBeta * phaseError / 100;  //Scale down to avoid overlaod

        //Keep the PLL within our limits
        if (pllFrequency < fmPllLoLimit)
            pllFrequency = fmPllLoLimit;
        if (pllFrequency > fmPllHiLimit)
            pllFrequency = fmPllHiLimit;

        //Next value for NCO
        pllPhase += pllFrequency + fmPllAlpha * phaseError;

        //LP filter the NCO frequency term to get DC offset value
        fmDCOffset = (1.0 - fmDcAlpha) * fmDCOffset + fmDcAlpha * pllFrequency;
        //Change in frequency - dc term is our demodulated output
        out[i].re = out[i].im = (pllFrequency - fmDCOffset);

    }
    //fmod will keep pllPhase <= TWOPI
    pllPhase = fmod(pllPhase, TWOPI);

}
