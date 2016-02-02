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
    buf = CPXBuf::malloc(fftSize);
    CPXBuf::clear(buf, fftSize);
}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFTfftw::FFTForward(CPX * in, CPX * out, int numSamples)
{
    if (!fftParamsSet)
        return;

    //If in==NULL, use whatever is in timeDomain buffer
    if (in != NULL ) {
		if (windowType != WindowFunction::NONE && numSamples == samplesPerBuffer) {
			//Smooth the input data with our window
			CPXBuf::mult(timeDomain, in, windowFunction->windowCpx, samplesPerBuffer);
			//Zero pad remainder of buffer if needed
			for (int i = samplesPerBuffer; i<fftSize; i++) {
				timeDomain[i] = 0;
			}
		} else {
			//Make sure that buffer which does not have samples is zero'd out
			//We can pad samples in the time domain because it does not impact frequency results in FFT
			CPXBuf::clear(timeDomain,fftSize);
			//Put the data in properly aligned FFTW buffer
			CPXBuf::copy(timeDomain, in, numSamples);
		}

    }

    fftw_execute(plan_fwd);

    //If out == NULL, just leave result in freqDomain buffer and let caller get it
    if (out != NULL)
        CPXBuf::copy(out, freqDomain, fftSize);
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
            CPXBuf::clear(freqDomain,fftSize);

		CPXBuf::copy(freqDomain, in, numSamples);
    }
    fftw_execute(plan_rev);

    if (out != NULL)
        CPXBuf::copy(out, timeDomain, fftSize);

}

//size is the number of samples, not the size of fft
void FFTfftw::FFTSpectrum(CPX *in, double *out, int numSamples)
{
    if (!fftParamsSet)
        return;
	FFTForward(in,workingBuf,numSamples);

	unfoldInOrder(workingBuf, freqDomain, true);

	CalcPowerAverages(freqDomain, out, fftSize);


}



