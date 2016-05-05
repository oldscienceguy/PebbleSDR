//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "demod_sam.h"

Demod_SAM::Demod_SAM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{

    //natural frequency - loop bandwidth
    int pllBandwidth = 100.0; //Was 300;
    float pllZeta = 0.707; //PLL loop damping factor
    int pllLimit = 1000.0; //PLL 'tuning' range

    m_pllAlpha = 2.0 * pllZeta * pllBandwidth * TWOPI/sampleRate;
    m_pllBeta = m_pllAlpha * m_pllAlpha / (4.0 * pllZeta * pllZeta);
    //Pll range:  PLL will 'auto-tune' within this range
    m_pllLowLimit = -pllLimit * TWOPI/sampleRate;
    m_pllHighLimit = pllLimit * TWOPI/sampleRate;

    m_pllPhase = 0.0;  //Tracks ref sig in PLL
    m_pllFrequency = 0.0;


    m_amDcRe = m_amDcReLast = 0.0;
    m_amDcIm = m_amDcImLast = 0.0;

    //create complex lowpass filter pair with LP cuttoff of 5000Hz and transition width of 1000Hz
    // 40dB stop band attenuation
    m_bpFilter.InitLPFilter(0, 1.0, 40.0, 4500, 5500,sampleRate);
    //apply transform to shift the LP filter 5000Hz so now the filter is
    // a 0 to 10000Hz Hilbert bandpass filter with 90 degree phase shift
    m_bpFilter.GenerateHBFilter(5000.0);
}

Demod_SAM::~Demod_SAM()
{

}

//dttsp PLL algorithm
CPX Demod_SAM::pll(CPX sig, float loLimit, float hiLimit)
{
    CPX z;
    CPX pllSample;
    float difference;

    //Todo: See if we can use NCO here
    //This is the generated signal to sync with
    z.re = cos(m_pllPhase);
    z.im = sin(m_pllPhase);

    //Complex mult of incoming signal and NCO
    pllSample = z * sig;

    // For SAM we need the magnitude (hypot)
    // For FM we need the freq difference
    difference = sig.mag() * pllSample.phase();
    //difference = pllSample.phase();

    m_pllFrequency += m_pllBeta * difference;

    //Keep the PLL within our limits
    if (m_pllFrequency < loLimit)
        m_pllFrequency = loLimit;
    if (m_pllFrequency > hiLimit)
        m_pllFrequency = hiLimit;

    m_pllPhase += m_pllFrequency + m_pllAlpha * difference;

    //Next reference signal
    while (m_pllPhase >= TWOPI)
        m_pllPhase -= TWOPI;
    while (m_pllPhase < 0)
        m_pllPhase += TWOPI;

    return pllSample;
}

//dttsp & cuteSDR algorithm with DC removal
#define DC_ALPHA 0.9999f	//ALPHA for DC removal filter ~20Hz Fcut with 15625Hz Sample Rate

void Demod_SAM::processBlock(  CPX * in, CPX * out, int demodSamples )
{
    //Power magnitude for each sample sqrt(re^2 + im^2)
    CPX pllSample;

    for (int i = 0; i < demodSamples; i++) {
        pllSample = pll(in[i],m_pllLowLimit,m_pllHighLimit);

        //CuteSDR description of filter for reference
        //High pass filter(DC removal) with IIR filter
        // H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)

        //Basic am demod but using delay instead of mag() for sample
        m_amDcRe = (DC_ALPHA * m_amDcReLast) + pllSample.re;
        m_amDcIm = (DC_ALPHA * m_amDcImLast) + pllSample.im;
		out[i].re = (m_amDcRe - m_amDcReLast);
		out[i].im = (m_amDcIm - m_amDcImLast);
        m_amDcReLast = m_amDcRe;
        m_amDcImLast = m_amDcIm;
    }
    //process I/Q with bandpass filter with 90deg phase shift between the I and Q filters
    m_bpFilter.ProcessFilter(demodSamples, out, out);
    CPX tmp;
    for(int i=0; i<demodSamples; i++)
    {
        tmp = out[i];
        //Monoaural sound
        out[i].im = tmp.re - tmp.im;	//send upper sideband to (right)channel
        out[i].re = tmp.re + tmp.im;	//send lower sideband to (left)channel
    }
}
