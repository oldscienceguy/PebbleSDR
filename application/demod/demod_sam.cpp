#include "demod_sam.h"

Demod_SAM::Demod_SAM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{

    //natural frequency - loop bandwidth
    int pllBandwidth = 100.0; //Was 300;
    float pllZeta = 0.707; //PLL loop damping factor
    int pllLimit = 1000.0; //PLL 'tuning' range

    pllAlpha = 2.0 * pllZeta * pllBandwidth * TWOPI/sampleRate;
    pllBeta = pllAlpha * pllAlpha / (4.0 * pllZeta * pllZeta);
    //Pll range:  PLL will 'auto-tune' within this range
    pllLowLimit = -pllLimit * TWOPI/sampleRate;
    pllHighLimit = pllLimit * TWOPI/sampleRate;

    pllPhase = 0.0;  //Tracks ref sig in PLL
    pllFrequency = 0.0;


    amDcRe = amDcReLast = 0.0;
    amDcIm = amDcImLast = 0.0;

    //create complex lowpass filter pair with LP cuttoff of 5000Hz and transition width of 1000Hz
    // 40dB stop band attenuation
    bpFilter.InitLPFilter(0, 1.0, 40.0, 4500, 5500,sampleRate);
    //apply transform to shift the LP filter 5000Hz so now the filter is
    // a 0 to 10000Hz Hilbert bandpass filter with 90 degree phase shift
    bpFilter.GenerateHBFilter(5000.0);
}

Demod_SAM::~Demod_SAM()
{

}

//dttsp PLL algorithm
CPX Demod_SAM::PLL(CPX sig, float loLimit, float hiLimit)
{
    CPX z;
    CPX pllSample;
    float difference;

    //Todo: See if we can use NCO here
    //This is the generated signal to sync with
    z.re = cos(pllPhase);
    z.im = sin(pllPhase);

    //Complex mult of incoming signal and NCO
    pllSample = z * sig;

    // For SAM we need the magnitude (hypot)
    // For FM we need the freq difference
    difference = sig.mag() * pllSample.phase();
    //difference = pllSample.phase();

    pllFrequency += pllBeta * difference;

    //Keep the PLL within our limits
    if (pllFrequency < loLimit)
        pllFrequency = loLimit;
    if (pllFrequency > hiLimit)
        pllFrequency = hiLimit;

    pllPhase += pllFrequency + pllAlpha * difference;

    //Next reference signal
    while (pllPhase >= TWOPI)
        pllPhase -= TWOPI;
    while (pllPhase < 0)
        pllPhase += TWOPI;

    return pllSample;
}

//dttsp & cuteSDR algorithm with DC removal
#define DC_ALPHA 0.9999f	//ALPHA for DC removal filter ~20Hz Fcut with 15625Hz Sample Rate

void Demod_SAM::ProcessBlock(  CPX * in, CPX * out, int demodSamples )
{
    //Power magnitude for each sample sqrt(re^2 + im^2)
    CPX pllSample;

    for (int i = 0; i < demodSamples; i++) {
        pllSample = PLL(in[i],pllLowLimit,pllHighLimit);

        //CuteSDR description of filter for reference
        //High pass filter(DC removal) with IIR filter
        // H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)

        //Basic am demod but using delay instead of mag() for sample
        amDcRe = (DC_ALPHA * amDcReLast) + pllSample.re;
        amDcIm = (DC_ALPHA * amDcImLast) + pllSample.im;
        out[i].re = (amDcRe - amDcReLast) * 0.5; //Adj for half power so == USB
        out[i].im = (amDcIm - amDcImLast) * 0.5;
        amDcReLast = amDcRe;
        amDcImLast = amDcIm;
    }
    //process I/Q with bandpass filter with 90deg phase shift between the I and Q filters
    bpFilter.ProcessFilter(demodSamples, out, out);
    CPX tmp;
    for(int i=0; i<demodSamples; i++)
    {
        tmp = out[i];
        //Monoaural sound
        out[i].im = tmp.re - tmp.im;	//send upper sideband to (right)channel
        out[i].re = tmp.re + tmp.im;	//send lower sideband to (left)channel
    }
}
