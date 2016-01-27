#ifndef FFT_H
#define FFT_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "cpx.h"
#include "db.h"
#include "QMutex"

//New base class for multiple FFT variations
//This will eventually let us switch usage or at least document the various options
//Code will eventually go back to using FFT everywhere, including imported code from other projects
class PEBBLELIBSHARED_EXPORT FFT
{
public:
    FFT();
    virtual ~FFT();
	static FFT* Factory(); //Returns instance based on USE_FFT, USE_FFTCUTE, etc

    const bool useIntegerFFT; //Used as we switch cuteSDR code to +/-1

	const quint32 maxFFTSize = 65535;
	const quint32 minFFTSize = 512;
    double ampMax;	//maximum sin wave Pk for 16 bit input data
    double overLimit;	//limit for detecting over ranging inputs

    //Keep separate from constructor so we can change on the fly eventually
    //cutesdr usage
	virtual void FFTParams(quint32 _size, double _dBCompensation, double _sampleRate);
    //Reset to init state, same parameters
    virtual void ResetFFT();


    //If in==NULL, use whatever is in timeDomain buffer
    //If out==NULL, leave result in freqDomain buffer and don't copy to out
    virtual void FFTForward(CPX * in, CPX * out, int size) = 0;
    virtual void FFTSpectrum(CPX *in, double * out, int size) = 0; //Replacing FFTMagnForward in most uses

    //If in==NULL, use whatever is in freqDomain buffer
    //If out==NULL, then leave result in timeDomain buffer and don't copy to out
    virtual void FFTInverse(CPX * in, CPX * out, int size) = 0;

    //Not virtual and common to all FFT implementations
    void FreqDomainToMagnitude(CPX * freqBuf, int size, double baseline, double correction, double *fbr);
    void OverlapAdd(CPX *out, int size);

    //WIP Calculate m_pFFTPwrAveBuf for any FFT.  Heavily embedded in cuteSDR
    //Compare with cuteSDR to see if we got it right
    void CalcPowerAverages(CPX* in, double *out, int size);
	void SetMovingAvgLimit(quint32 ave);

	bool MapFFTToScreen(double *inBuf, qint32 yPixels, qint32 xPixels,
									double maxdB, double mindB,
									qint32 startFreq, qint32 stopFreq,
									qint32 *outBuf);

    int getFFTSize() {return fftSize;}
    CPX *getFreqDomain() {return freqDomain;}
    CPX *getTimeDomain() {return timeDomain;}
	//Fractional bin width in hz
	double getBinWidth() {return sampleRate / fftSize;}

protected:
    //Utility
    CPX *timeDomain; //Should always be counted on to have last time domain results
    CPX *freqDomain; //Should always be counted on to have last freqency domain results
    CPX *workingBuf; //Used internally and should never be counted on for anything
    CPX *overlap;
    bool fftParamsSet; //Use to make sure base class calls FFT to init variables

    qint32 fftSize;
    double sampleRate;

    bool fftInputOverload;

    //Used by MapFFTToScreen
	qint32 *fftBinToX;

    //For calculating power averages
	quint32 movingAvgLimit; //How many time to do moving avg before exponential avg
    quint32 bufferCnt; //# buffers we've processed
    quint16 averageCnt;
    double* FFTPwrAvgBuf;
    double* FFTAvgBuf;
    double* FFTPwrSumBuf;
    double dBCompensation;

    //This should replace m_mutex in fftcute
    QMutex fftMutex; //Used to sync threads calling FFT and display calling Screen mapping

	void unfoldInOrder(CPX *inBuf, CPX *outBuf);
};

#endif // FFT_H
