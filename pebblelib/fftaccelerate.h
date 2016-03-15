#ifndef FFTACCELERATE_H
#define FFTACCELERATE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "fft.h"
#include <Accelerate/Accelerate.h>

//Mac only FFT library using Accelerate DSP library
class PEBBLELIBSHARED_EXPORT FFTAccelerate : public FFT
{
public:
	FFTAccelerate();
	~FFTAccelerate();

	void FFTParams(quint32 _fftSize, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
				   WindowFunction::WINDOWTYPE _windowType);
	void FFTForward(CPX * in, CPX * out, int numSamples);
	void FFTInverse(CPX * in, CPX * out, int numSamples);
	void FFTSpectrum(CPX *in, double *out, int numSamples);

private:
	// FFTSetup fftSetup; //Single Precision
	FFTSetupD fftSetupD; //Double precision

	vDSP_Length fftSizeLog2n; //vDSP doesn't just take FFT len, it has to be converted to log2N

	//Accelerate fw doesn't work with interleaved CPX (re,im,re,im,...).
	//Instead is uses Split Complex which is an array of reals and an array of imag
	DSPDoubleSplitComplex splitComplex;
	DSPDoubleSplitComplex splitComplexTemp; //For faster FFT
};

#endif // FFTACCELERATE_H
