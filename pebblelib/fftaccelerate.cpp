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

void FFTAccelerate::fftParams(quint32 _size, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
							  WindowFunction::WINDOWTYPE _windowType)
{
	//Must call FFT base to properly init
	FFT::fftParams(_size, _dBCompensation, _sampleRate, _samplesPerBuffer, _windowType);

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

void FFTAccelerate::fftForward(CPX *in, CPX *out, int numSamples)
{
	if (!m_fftParamsSet)
		return;

	if (in != NULL) {
		m_applyWindow(in,numSamples);
	}

	//Copy timeDomain to splitComplex
	vDSP_Stride ic = 2; //Step factor/2 for in[i], must be multiple of 2
	vDSP_Stride iz = 1; //Step factor for splitComplex[i]
	vDSP_ctozD((DSPDoubleComplex *)m_timeDomain, ic, &splitComplex, iz, m_fftSize);

	vDSP_Stride stride = 1; //1=Process every element
	//OSX 10 and later
	//In place 1D complex
	//Todo: Try vDSP_DFT_Execute with vDSP_DFT_zop_CreateSetup for faster performance
	//vDSP_fft_zipD(fftSetupD, &splitComplex, stride, fftSizeLog2n, kFFTDirection_Forward);
	//Uses temporary buffers for faster performance
	vDSP_fft_ziptD(fftSetupD, &splitComplex, stride, &splitComplexTemp, fftSizeLog2n, kFFTDirection_Forward);

	//Maintain freqDomain with results by copying from splitComplex back to our CPX*
	vDSP_ztocD(&splitComplex,iz,(DSPDoubleComplex *)m_freqDomain,ic,m_fftSize);

	//If out == NULL, just leave result in freqDomain buffer and let caller get it
	if (out != NULL)
		CPX::copyCPX(out, m_freqDomain, m_fftSize);

}

void FFTAccelerate::fftInverse(CPX *in, CPX *out, int numSamples)
{
	if (!m_fftParamsSet)
		return;

	//If in==NULL, use whatever is in freqDomain buffer
	if (in != NULL ) {
		if (numSamples < m_fftSize)
			CPX::clearCPX(m_freqDomain,m_fftSize);
		//Put the data in properly aligned FFTW buffer
		CPX::copyCPX(m_freqDomain, in, numSamples);
	}

	//Copy freqDomain to splitComplex
	vDSP_Stride ic = 2; //Step factor/2 for in[i], must be multiple of 2
	vDSP_Stride iz = 1; //Step factor for splitComplex[i]
	vDSP_ctozD((DSPDoubleComplex *)m_freqDomain, ic, &splitComplex, iz, m_fftSize);

	vDSP_Stride stride = 1; //1=Process every element
	//In place 1D complex
	//vDSP_fft_zipD(fftSetupD, &splitComplex, stride, fftSizeLog2n, kFFTDirection_Inverse);
	//Uses temporary buffers for faster performance
	vDSP_fft_ziptD(fftSetupD, &splitComplex, stride, &splitComplexTemp, fftSizeLog2n, kFFTDirection_Inverse);

	//Maintain timeDomain with results by copying from splitComplex back to our CPX*
	vDSP_ztocD(&splitComplex,iz,(DSPDoubleComplex *)m_timeDomain,ic,m_fftSize);

	//If out == NULL, just leave result in freqDomain buffer and let caller get it
	if (out != NULL)
		CPX::copyCPX(out, m_timeDomain, m_fftSize);

}

bool FFTAccelerate::fftSpectrum(CPX *in, double *out, int numSamples)
{
	if (!m_fftParamsSet)
		return false;

	fftForward(in,m_workingBuf,numSamples); //No need to copy to out, leave in freqDomain

	m_unfoldInOrder(m_workingBuf, m_freqDomain);

	calcPowerAverages(m_freqDomain, out, m_fftSize);

	return m_isOverload;

}
