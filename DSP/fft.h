#ifndef FFT_H
#define FFT_H
#include "global.h"
#include "cpx.h"
#include "SignalProcessing.h"
#include "QMutex"

#define MAX_FFT_SIZE 65536
#define MIN_FFT_SIZE 512

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
    virtual void FFTParams(qint32 _size, bool _invert, double _dBCompensation, double _sampleRate);

    //If in==NULL, use whatever is in timeDomain buffer
    //If out==NULL, leave result in freqDomain buffer and don't copy to out
    virtual void FFTForward(CPX * in, CPX * out, int size);
    virtual void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);

    //If in==NULL, use whatever is in freqDomain buffer
    //If out==NULL, then leave result in timeDomain buffer and don't copy to out
    virtual void FFTInverse(CPX * in, CPX * out, int size);

    //Not virtual and common to all FFT implementations
    void FreqDomainToMagnitude(CPX * freqBuf, int size, double baseline, double correction, double *fbr);
    void OverlapAdd(CPX *out, int size);

    //WIP Calculate m_pFFTPwrAveBuf for any FFT.  Heavily embedded in cuteSDR
    //Compare with cuteSDR to see if we got it right
    void CalcPowerAverages(CPX* in, int size);

    bool GetScreenIntegerFFTData(qint32 MaxHeight, qint32 MaxWidth,
                                    double MaxdB, double MindB,
                                    qint32 StartFreq, qint32 StopFreq,
                                    qint32* OutBuf );

    int getFFTSize() {return fftSize;}
    CPX *getFreqDomain() {return freqDomain;}
    CPX *getTimeDomain() {return timeDomain;}

protected:
    //Utility
    CPX *timeDomain;
    CPX *freqDomain;
    CPX *overlap;
    bool fftParamsSet; //Use to make sure base class calls FFT to init variables

    qint32 fftSize;
    double sampleRate;
    bool invert; //forward or reverse in process

    //Used by GetScreenIntegerFFTData
    qint32 *plotTranslateTable;
    qint32 m_BinMin;
    qint32 m_BinMax;
    qint32 lastStartFreq;
    qint32 lastStopFreq;
    qint32 lastPlotWidth;

    //For calculating power averages
    quint32 bufferCnt; //# buffers we've processed
    quint16 averageCnt;
    double* FFTPwrAvgBuf;
    double* FFTAvgBuf;
    double* FFTPwrSumBuf;
    double K_C;
    double K_B;
    double dBCompensation;

    //This should replace m_mutex in fftcute
    QMutex fftMutex; //Used to sync threads calling FFT and display calling Screen mapping



};

#endif // FFT_H
