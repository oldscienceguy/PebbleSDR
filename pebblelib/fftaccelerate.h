#ifndef FFTACCELERATE_H
#define FFTACCELERATE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "fft.h"

//Mac only FFT library using Accelerate DSP library
class PEBBLELIBSHARED_EXPORT FFTAccelerate : public FFT
{
public:
	FFTAccelerate();
	~FFTAccelerate();

	void FFTParams(quint32 _size, bool _invert, double _dBCompensation, double sampleRate);
	void FFTForward(CPX * in, CPX * out, int size);
	void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
	void FFTInverse(CPX * in, CPX * out, int size);
	void FFTSpectrum(CPX *in, double *out, int size);

};

#endif // FFTACCELERATE_H
