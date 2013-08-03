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
    void FFTParams( qint32 size, bool invert, double dBCompensation, double sampleRate);
    void FFTForward(CPX * in, CPX * out, int size);
    void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
    void FFTInverse(CPX * in, CPX * out, int size);

    void OverlapAdd(CPX *out, int size);

    CPX *timeDomain;
    CPX *freqDomain;
    int fftSize;

private:
    fftw_plan plan_fwd;
    fftw_plan plan_rev;
    CPX *buf;
    CPX *overlap;
    int half_sz;
};


#endif // FFTW_H
