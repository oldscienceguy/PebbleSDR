//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "fftw.h"

FFTfftw::FFTfftw() : FFT()
{
}

FFTfftw::~FFTfftw()
{
    fftw_destroy_plan(plan_fwd);
    fftw_destroy_plan(plan_rev);

	if (buf) free(buf);
}

void FFTfftw::fftParams(quint32 _size, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
						WindowFunction::WINDOWTYPE _windowType)
{
    //Must call FFT base to properly init
	FFT::fftParams(_size, _dBCompensation, _sampleRate, _samplesPerBuffer, _windowType);

    half_sz = m_fftSize / 2;
	plan_fwd = fftw_plan_dft_1d(m_fftSize , (fftw_complex*)m_timeDomain, (fftw_complex*)m_freqDomain, FFTW_FORWARD, FFTW_MEASURE);
	plan_rev = fftw_plan_dft_1d(m_fftSize , (fftw_complex*)m_freqDomain, (fftw_complex*)m_timeDomain, FFTW_BACKWARD, FFTW_MEASURE);
	buf = CPX::memalign(m_fftSize);
	CPX::clearCPX(buf, m_fftSize);
}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFTfftw::fftForward(CPX * in, CPX * out, int numSamples)
{
    if (!m_fftParamsSet)
        return;

    //If in==NULL, use whatever is in timeDomain buffer
    if (in != NULL ) {
		m_applyWindow(in,numSamples);
    }

    fftw_execute(plan_fwd);

    //If out == NULL, just leave result in freqDomain buffer and let caller get it
    if (out != NULL)
		CPX::copyCPX(out, m_freqDomain, m_fftSize);
}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFTfftw::fftInverse(CPX * in, CPX * out, int numSamples)
{
    if (!m_fftParamsSet)
        return;

    //If in==NULL, use whatever is in freqDomain buffer
    if (in != NULL) {
		if (numSamples < m_fftSize)
            //Make sure that buffer which does not have samples is zero'd out
			CPX::clearCPX(m_freqDomain,m_fftSize);

		CPX::copyCPX(m_freqDomain, in, numSamples);
    }
    fftw_execute(plan_rev);

    if (out != NULL)
		CPX::copyCPX(out, m_timeDomain, m_fftSize);

}

//size is the number of samples, not the size of fft
bool FFTfftw::fftSpectrum(CPX *in, double *out, int numSamples)
{
    if (!m_fftParamsSet)
		return false;
	fftForward(in,m_workingBuf,numSamples);

	m_unfoldInOrder(m_workingBuf, m_freqDomain);

	calcPowerAverages(m_freqDomain, out, m_fftSize);

	return m_isOverload;
}



