#include "bandpassfilter.h"

BandPassFilter::BandPassFilter(quint32 _sampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	m_useFastFIR = true;

	m_lowFreq = 0;
	m_highFreq = 0;

	if (!m_useFastFIR) {
		//Testing, time intensive for large # taps, ie @512 we lose chunks of signal
		//Check post-bandpass spectrum and make just large enough to be effective
		//64 is too small, 128 is good, ignored if useFFT arg == true
		//bpFilter = new FIRFilter(demodSampleRate, demodFrames,true, 128);
		m_bpFilter1 = new FIRFilter(_sampleRate, _bufferSize,true, 128);
		m_bpFilter1->setEnabled(true);
		m_bpFilter2 = NULL;
	} else {
		m_bpFilter2 = new CFastFIR();
		m_bpFilter1 = NULL;
	}

}

BandPassFilter::~BandPassFilter()
{
	if (m_bpFilter1 != NULL)
		delete m_bpFilter1;

	if (m_bpFilter2 != NULL)
		delete m_bpFilter2;
}

void BandPassFilter::setBandPass(float _low, float _high)
{
	m_lowFreq = _low;
	m_highFreq = _high;

	if (m_bpFilter1 != NULL) {
		m_bpFilter1->SetBandPass(m_lowFreq, m_highFreq);
	} else if (m_bpFilter2 != NULL) {
		m_bpFilter2->SetupParameters(m_lowFreq, m_highFreq, 0, sampleRate);
	}
}

CPX *BandPassFilter::process(CPX *in, quint32 _numSamples)
{
	if (m_bpFilter1 != NULL) {
		return m_bpFilter1->ProcessBlock(in);
	}
	if (m_bpFilter2 != NULL) {
		int postNumSamples;
		postNumSamples = m_bpFilter2->ProcessData(_numSamples,in, out);
		return out;
	}
	return in;

}
