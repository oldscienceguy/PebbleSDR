#include "demod_am.h"

Demod_AM::Demod_AM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{
    amDc = amDcLast = 0.0;
    amSmooth = 0.0;
    SetBandwidth(16000); //For testing
}

Demod_AM::~Demod_AM()
{

}

void Demod_AM::SetBandwidth(double bandwidth)
{
    //create a LP filter with passband the same as the main filter bandwidth for post audio filtering
    lpFilter.InitLPFilter(0, 1.0, 50.0, bandwidth, bandwidth*1.8, sampleRate);//initialize LP FIR filter
}

//WARNING: demodSamples here is NOT the same as numSamples in the base class due to decimation.
void Demod_AM::ProcessBlock(CPX *in, CPX *out, int demodSamples)
{
    double amOut;
    for (int i=0;i<demodSamples;i++)
    {
        //Just return the magnitude of each sample
        //tmp = sqrt(in[i].re * in[i].re + in[i].im * in[i].im);
        //10/11/10: putting 50% of mag in re and im yields the same signal power
        //as USB or LSB.  Without this, we get AM distortion at the same gain
        amOut = in[i].mag() * 0.5;
        out[i].re = out[i].im = amOut;
    }
}
//dttsp & cuteSDR algorithm with DC removal
#define DC_ALPHA 0.9999f	//ALPHA for DC removal filter ~20Hz Fcut with 15625Hz Sample Rate

//WARNING: demodSamples here is NOT the same as numSamples in the base class due to decimation.
//Todo: Clean up so we stay in sync
void Demod_AM::ProcessBlockFiltered(CPX *in, CPX *out, int demodSamples)
{
    //Power magnitude for each sample
    double mag; //sqrt(re^2 + im^2)
    double amOut;

    for (int i = 0; i < demodSamples; i++)
    {
        mag = in[i].mag();

        //CuteSDR description of filter for reference
        //High pass filter(DC removal) with IIR filter
        // H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)
        amDc = (DC_ALPHA * amDcLast) + mag;
        amOut = amDc - amDcLast;
        amDcLast = amDc;

        //amCout *= .5 so we keep overall signal magnitude unchanged compared to SSB with half the power
        out[i].re = out[i].im = amOut * 0.5;
    }

    //post filter AM audio to limit high frequency noise
    lpFilter.ProcessFilter(demodSamples, out, out);

}
