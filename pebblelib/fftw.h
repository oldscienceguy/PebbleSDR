#ifndef FFTW_H
#define FFTW_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "fft.h"
#include "cpx.h"
#include "../fftw-3.3.3/api/fftw3.h"

class PEBBLELIBSHARED_EXPORT FFTfftw : public FFT
{
public:
    FFTfftw();
    ~FFTfftw();
	void FFTParams(quint32 _size, double _dBCompensation, double sampleRate);
    void FFTForward(CPX * in, CPX * out, int size);
    void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
    void FFTInverse(CPX * in, CPX * out, int size);
    void FFTSpectrum(CPX *in, double *out, int size);

private:
    fftw_plan plan_fwd;
    fftw_plan plan_rev;
    CPX *buf;
    int half_sz;
};


#endif // FFTW_H
