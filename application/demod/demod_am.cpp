//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "demod_am.h"

Demod_AM::Demod_AM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{
	m_amDc = m_amDcLast = 0.0;
	setBandwidth(16000); //For testing
}

Demod_AM::~Demod_AM()
{

}

void Demod_AM::setBandwidth(double bandwidth)
{
    //create a LP filter with passband the same as the main filter bandwidth for post audio filtering
	m_lpFilter.InitLPFilter(0, 1.0, 50.0, bandwidth, bandwidth*1.8, sampleRate);//initialize LP FIR filter
}

//WARNING: demodSamples here is NOT the same as numSamples in the base class due to decimation.
void Demod_AM::processBlock(CPX *in, CPX *out, int demodSamples)
{
    double amOut;
    for (int i=0;i<demodSamples;i++)
    {
        //Just return the magnitude of each sample
		amOut = DB::amplitude(in[i]);
		out[i].real(amOut);
		out[i].imag(amOut);
    }
}
//dttsp & cuteSDR algorithm with DC removal
#define DC_ALPHA 0.9999f	//ALPHA for DC removal filter ~20Hz Fcut with 15625Hz Sample Rate

//WARNING: demodSamples here is NOT the same as numSamples in the base class due to decimation.
//Todo: Clean up so we stay in sync
void Demod_AM::processBlockFiltered(CPX *in, CPX *out, int demodSamples)
{
    //Power magnitude for each sample
    double mag; //sqrt(re^2 + im^2)
    double amOut;

    for (int i = 0; i < demodSamples; i++)
    {
		mag = DB::amplitude(in[i]);

        //CuteSDR description of filter for reference
        //High pass filter(DC removal) with IIR filter
        // H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)
		m_amDc = (DC_ALPHA * m_amDcLast) + mag;
		amOut = m_amDc - m_amDcLast;
		m_amDcLast = m_amDc;

		out[i].real(amOut);
		out[i].imag(amOut);
    }

    //post filter AM audio to limit high frequency noise
	m_lpFilter.ProcessFilter(demodSamples, out, out);

}
