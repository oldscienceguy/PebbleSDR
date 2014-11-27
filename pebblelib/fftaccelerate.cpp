#include "fftaccelerate.h"

FFTAccelerate::FFTAccelerate() : FFT()
{
	splitComplex.realp = NULL;
	splitComplex.imagp = NULL;
	fftSizeLog2n = 0;
}

FFTAccelerate::~FFTAccelerate()
{
	if (splitComplex.realp != NULL)
		delete splitComplex.realp;
	if (splitComplex.imagp != NULL)
		delete splitComplex.imagp;
}

void FFTAccelerate::FFTParams(quint32 _size, bool _invert, double _dBCompensation, double _sampleRate)
{
	//Must call FFT base to properly init
	FFT::FFTParams(_size, _invert, _dBCompensation, _sampleRate);

	fftSizeLog2n = log2f(_size);
	//n = 1 << log2n; //???

	fftSetupD = vDSP_create_fftsetupD(fftSizeLog2n,FFT_RADIX2);

	//Set up splitComplex buffers
	splitComplex.realp = new double[_size];
	splitComplex.imagp = new double[_size];
}

void FFTAccelerate::FFTForward(CPX *in, CPX *out, int size)
{
	if (!fftParamsSet)
		return;

	//If in==NULL, use whatever is in timeDomain buffer
	if (in != NULL ) {
		if (size < fftSize)
			//Make sure that buffer which does not have samples is zero'd out
			//We can pad samples in the time domain because it does not impact frequency results in FFT
			CPXBuf::clear(timeDomain,fftSize);

		//Put the data in properly aligned FFTW buffer
		CPXBuf::copy(timeDomain, in, size);
	}

	//Copy timeDomain to splitComplex
	vDSP_Stride ic = 2; //Step factor/2 for in[i], must be multiple of 2
	vDSP_Stride iz = 1; //Step factor for splitComplex[i]
	vDSP_ctozD((DSPDoubleComplex *)timeDomain, ic, &splitComplex, iz, fftSize);

	vDSP_Stride stride = 1; //1=Process every element
	//In place !D complex
	vDSP_fft_zipD(fftSetupD,&splitComplex, stride, fftSizeLog2n, kFFTDirection_Forward);

	//Maintain freqDomain with results by copying from splitComplex back to our CPX*
	vDSP_ztocD(&splitComplex,iz,(DSPDoubleComplex *)freqDomain,ic,fftSize);

	//If out == NULL, just leave result in freqDomain buffer and let caller get it
	if (out != NULL)
		CPXBuf::copy(out, freqDomain, fftSize);

}

void FFTAccelerate::FFTMagnForward(CPX *in, int size, double baseline, double correction, double *fbr)
{
	Q_UNUSED (in);
	Q_UNUSED (size);
	Q_UNUSED (baseline);
	Q_UNUSED (correction);
	Q_UNUSED (fbr);
	//Not used anywhere
}

void FFTAccelerate::FFTInverse(CPX *in, CPX *out, int size)
{
	if (!fftParamsSet)
		return;

	//If in==NULL, use whatever is in freqDomain buffer
	if (in != NULL ) {
		if (size < fftSize)
			CPXBuf::clear(freqDomain,fftSize);
		//Put the data in properly aligned FFTW buffer
		CPXBuf::copy(freqDomain, in, size);
	}

	//Copy freqDomain to splitComplex
	vDSP_Stride ic = 2; //Step factor/2 for in[i], must be multiple of 2
	vDSP_Stride iz = 1; //Step factor for splitComplex[i]
	vDSP_ctozD((DSPDoubleComplex *)freqDomain, ic, &splitComplex, iz, fftSize);

	vDSP_Stride stride = 1; //1=Process every element
	//In place !D complex
	vDSP_fft_zipD(fftSetupD,(DSPDoubleSplitComplex *) &splitComplex, stride, fftSizeLog2n, kFFTDirection_Inverse);

	//Maintain timeDomain with results by copying from splitComplex back to our CPX*
	vDSP_ztocD(&splitComplex,iz,(DSPDoubleComplex *)timeDomain,ic,fftSize);

	//If out == NULL, just leave result in freqDomain buffer and let caller get it
	if (out != NULL)
		CPXBuf::copy(out, timeDomain, fftSize);

}

void FFTAccelerate::FFTSpectrum(CPX *in, double *out, int size)
{
	if (!fftParamsSet)
		return;

	FFTForward(in,workingBuf,size); //No need to copy to out, leave in freqDomain

	//Now in freq domain we need to work with fftSize, not sample size
	for( int unfolded = 0, folded = fftSize/2 ; folded < fftSize; unfolded++, folded++) {
		freqDomain[unfolded] = workingBuf[folded]; //folded = 1024 to 2047 unfolded = 0 to 1023
	}
	// FFT output index 0 to N/2-1 is frequency output 0 to +Fs/2 Hz  (positive frequencies)
	for( int unfolded = fftSize/2, folded = 0; unfolded < fftSize; unfolded++, folded++) {
		freqDomain[unfolded] = workingBuf[folded]; //folded = 0 to 1023 unfolded = 1024 to 2047
	}

	CalcPowerAverages(freqDomain, out, fftSize);

}
