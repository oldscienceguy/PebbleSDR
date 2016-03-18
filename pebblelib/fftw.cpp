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

void FFTfftw::FFTParams(quint32 _size, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
						WindowFunction::WINDOWTYPE _windowType)
{
    //Must call FFT base to properly init
	FFT::FFTParams(_size, _dBCompensation, _sampleRate, _samplesPerBuffer, _windowType);

    half_sz = fftSize / 2;
	plan_fwd = fftw_plan_dft_1d(fftSize , (fftw_complex*)timeDomain, (fftw_complex*)freqDomain, FFTW_FORWARD, FFTW_MEASURE);
	plan_rev = fftw_plan_dft_1d(fftSize , (fftw_complex*)freqDomain, (fftw_complex*)timeDomain, FFTW_BACKWARD, FFTW_MEASURE);
	buf = CPX::memalign(fftSize);
	CPX::clearCPX(buf, fftSize);
}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFTfftw::FFTForward(CPX * in, CPX * out, int numSamples)
{
    if (!fftParamsSet)
        return;

    //If in==NULL, use whatever is in timeDomain buffer
    if (in != NULL ) {
		applyWindow(in,numSamples);
    }

    fftw_execute(plan_fwd);

    //If out == NULL, just leave result in freqDomain buffer and let caller get it
    if (out != NULL)
		CPX::copyCPX(out, freqDomain, fftSize);
}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFTfftw::FFTInverse(CPX * in, CPX * out, int numSamples)
{
    if (!fftParamsSet)
        return;

    //If in==NULL, use whatever is in freqDomain buffer
    if (in != NULL) {
		if (numSamples < fftSize)
            //Make sure that buffer which does not have samples is zero'd out
			CPX::clearCPX(freqDomain,fftSize);

		CPX::copyCPX(freqDomain, in, numSamples);
    }
    fftw_execute(plan_rev);

    if (out != NULL)
		CPX::copyCPX(out, timeDomain, fftSize);

}

//size is the number of samples, not the size of fft
bool FFTfftw::FFTSpectrum(CPX *in, double *out, int numSamples)
{
    if (!fftParamsSet)
		return false;
	FFTForward(in,workingBuf,numSamples);

	unfoldInOrder(workingBuf, freqDomain);

	CalcPowerAverages(freqDomain, out, fftSize);

	return isOverload;
}



