#ifndef FFT_H
#define FFT_H
#include "global.h"
#include "cpx.h"
#include "SignalProcessing.h"

//New base class for multiple FFT variations
//This will eventually let us switch usage or at least document the various options
//Code will eventually go back to using FFT everywhere, including imported code from other projects
class FFT
{
public:
    FFT();
    virtual ~FFT();
    //Keep separate from constructor so we can change on the fly eventually
    //cutesdr usage
    virtual void FFTParams( qint32 size, bool invert, double dBCompensation, double sampleRate) = 0;
    virtual void FFTForward(CPX * in, CPX * out, int size) = 0;
    virtual void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr) = 0;
    virtual void FFTInverse(CPX * in, CPX * out, int size) = 0;

    void FreqDomainToMagnitude(CPX * freqBuf, int size, double baseline, double correction, double *fbr);
    //void OverlapAdd(CPX *out, int size);

protected:
    //Utility
    CPX *timeDomain;
    CPX *freqDomain;
    int fftSize;
};

#endif // FFT_H
