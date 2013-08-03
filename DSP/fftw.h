#ifndef FFTW_H
#define FFTW_H

#include "global.h"
#include "cpx.h"
#include "../../fftw-3.3.1/api/fftw3.h"
#include "SignalProcessing.h"

class fftw
{
public:
    fftw(int size);
    ~fftw();
    void DoFFTWForward(CPX * in, CPX * out, int size);
    void DoFFTWMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
    void DoFFTWInverse(CPX * in, CPX * out, int size);
    void FreqDomainToMagnitude(CPX * freqBuf, int size, double baseline, double correction, double *fbr);
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
