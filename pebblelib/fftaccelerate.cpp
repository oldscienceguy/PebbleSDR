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
	if (splitComplexTemp.realp != NULL)
		delete splitComplexTemp.realp;
	if (splitComplexTemp.imagp != NULL)
		delete splitComplexTemp.imagp;
}

void FFTAccelerate::FFTParams(quint32 _size, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
							  WindowFunction::WINDOWTYPE _windowType)
{
	//Must call FFT base to properly init
	FFT::FFTParams(_size, _dBCompensation, _sampleRate, _samplesPerBuffer, _windowType);

	fftSizeLog2n = log2f(_size);
	//n = 1 << log2n; //???

	fftSetupD = vDSP_create_fftsetupD(fftSizeLog2n,FFT_RADIX2);

	//Set up splitComplex buffers
	splitComplex.realp = new double[_size];
	splitComplex.imagp = new double[_size];
	quint32 tempMinSize = 16384;
	quint32 tempSize = qMax(4*_size, tempMinSize);
	splitComplexTemp.realp = new double[tempSize]; //min 16384
	splitComplexTemp.imagp = new double[tempSize];
}

void FFTAccelerate::FFTForward(CPX *in, CPX *out, int numSamples)
{
	if (!fftParamsSet)
		return;

	//If in==NULL, use whatever is in timeDomain buffer
	if (in != NULL ) {
		if (windowType != WindowFunction::NONE && numSamples == samplesPerBuffer) {
			//Smooth the input data with our window
			CPX::multCPX(timeDomain, in, windowFunction->windowCpx, samplesPerBuffer);
			//Zero pad remainder of buffer if needed
			for (int i = samplesPerBuffer; i<fftSize; i++) {
				timeDomain[i] = 0;
			}
		} else {
			//Make sure that buffer which does not have samples is zero'd out
			//We can pad samples in the time domain because it does not impact frequency results in FFT
			CPX::clearCPX(timeDomain,fftSize);
			//Put the data in properly aligned FFTW buffer
			CPX::copyCPX(timeDomain, in, numSamples);
		}
	}

	//Copy timeDomain to splitComplex
	vDSP_Stride ic = 2; //Step factor/2 for in[i], must be multiple of 2
	vDSP_Stride iz = 1; //Step factor for splitComplex[i]
	vDSP_ctozD((DSPDoubleComplex *)timeDomain, ic, &splitComplex, iz, fftSize);

	vDSP_Stride stride = 1; //1=Process every element
	//OSX 10 and later
	//In place 1D complex
	//Todo: Try vDSP_DFT_Execute with vDSP_DFT_zop_CreateSetup for faster performance
	//vDSP_fft_zipD(fftSetupD, &splitComplex, stride, fftSizeLog2n, kFFTDirection_Forward);
	//Uses temporary buffers for faster performance
	vDSP_fft_ziptD(fftSetupD, &splitComplex, stride, &splitComplexTemp, fftSizeLog2n, kFFTDirection_Forward);

	//Maintain freqDomain with results by copying from splitComplex back to our CPX*
	vDSP_ztocD(&splitComplex,iz,(DSPDoubleComplex *)freqDomain,ic,fftSize);

	//If out == NULL, just leave result in freqDomain buffer and let caller get it
	if (out != NULL)
		CPX::copyCPX(out, freqDomain, fftSize);

}

void FFTAccelerate::FFTInverse(CPX *in, CPX *out, int numSamples)
{
	if (!fftParamsSet)
		return;

	//If in==NULL, use whatever is in freqDomain buffer
	if (in != NULL ) {
		if (numSamples < fftSize)
			CPX::clearCPX(freqDomain,fftSize);
		//Put the data in properly aligned FFTW buffer
		CPX::copyCPX(freqDomain, in, numSamples);
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
		CPX::copyCPX(out, timeDomain, fftSize);

}

void FFTAccelerate::FFTSpectrum(CPX *in, double *out, int numSamples)
{
	if (!fftParamsSet)
		return;

	FFTForward(in,workingBuf,numSamples); //No need to copy to out, leave in freqDomain

	unfoldInOrder(workingBuf, freqDomain, true);

	CalcPowerAverages(freqDomain, out, fftSize);

}
