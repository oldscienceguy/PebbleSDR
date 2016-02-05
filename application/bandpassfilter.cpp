#include "bandpassfilter.h"

BandPassFilter::BandPassFilter(quint32 _sampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	useFastFIR = true;

	if (!useFastFIR) {
		//Testing, time intensive for large # taps, ie @512 we lose chunks of signal
		//Check post-bandpass spectrum and make just large enough to be effective
		//64 is too small, 128 is good, ignored if useFFT arg == true
		//bpFilter = new FIRFilter(demodSampleRate, demodFrames,true, 128);
		bpFilter1 = new FIRFilter(_sampleRate, _bufferSize,true, 128);
		bpFilter1->setEnabled(true);
	} else {
		bpFilter2 = new CFastFIR();
	}

}

BandPassFilter::~BandPassFilter()
{
	if (!useFastFIR) {
		delete bpFilter1;
	} else {
		delete bpFilter2;
	}
}

void BandPassFilter::setBandPass(float _low, float _high)
{
	if (!useFastFIR) {
		bpFilter1->SetBandPass(_low, _high);
	} else {
		bpFilter2->SetupParameters(_low, _high, 0, sampleRate);
	}
}

CPX *BandPassFilter::process(CPX *in, quint32 _numSamples)
{
	if (!useFastFIR) {
		return bpFilter1->ProcessBlock(in);
	} else {
		int postNumSamples;
		postNumSamples = bpFilter2->ProcessData(_numSamples,in, out);
		return out;
	}

}
