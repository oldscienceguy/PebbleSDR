#ifndef FFTW_H
#define FFTW_H

#include "global.h"
#include "fft.h"
#include "cpx.h"
#include "../../fftw-3.3.1/api/fftw3.h"

class FFTfftw : public FFT
{
public:
    FFTfftw();
    ~FFTfftw();
    void FFTParams(qint32 _size, bool _invert, double _dBCompensation, double sampleRate);
    void FFTForward(CPX * in, CPX * out, int size);
    void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
    void FFTInverse(CPX * in, CPX * out, int size);
    void FFTSpectrum(CPX *in, int size);

private:
    fftw_plan plan_fwd;
    fftw_plan plan_rev;
    CPX *buf;
    int half_sz;
};


#endif // FFTW_H
