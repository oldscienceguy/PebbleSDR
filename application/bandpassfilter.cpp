#include "bandpassfilter.h"

BandPassFilter::BandPassFilter(quint32 _sampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	//Testing, time intensive for large # taps, ie @512 we lose chunks of signal
	//Check post-bandpass spectrum and make just large enough to be effective
	//64 is too small, 128 is good, ignored if useFFT arg == true
	//bpFilter = new FIRFilter(demodSampleRate, demodFrames,true, 128);
	bpFilter = new FIRFilter(_sampleRate, _bufferSize,true, 128);
	bpFilter->setEnabled(true);

}

BandPassFilter::~BandPassFilter()
{
	delete bpFilter;
}

void BandPassFilter::setBandPass(float _low, float _high)
{
	bpFilter->SetBandPass(_low, _high);
}

CPX *BandPassFilter::process(CPX *in, quint32 _numSamples)
{

	return bpFilter->ProcessBlock(in);

}
