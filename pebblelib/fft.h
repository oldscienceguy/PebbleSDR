#ifndef FFT_H
#define FFT_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "cpx.h"
#include "db.h"
#include "QMutex"
#include "windowfunction.h"

//New base class for multiple FFT variations
//This will eventually let us switch usage or at least document the various options
//Code will eventually go back to using FFT everywhere, including imported code from other projects
class PEBBLELIBSHARED_EXPORT FFT
{
public:
    FFT();
    virtual ~FFT();
	static FFT* Factory(QString _label); //Returns instance based on USE_FFT, USE_FFTCUTE, etc

	const quint32 maxFFTSize = 65535;
	const quint32 minFFTSize = 2048;
	//Maximum value of input samples -1 to +1
	const double ampMax = 1.0;
	const double overLimit = 0.9;	//limit for detecting over ranging inputs

    //Keep separate from constructor so we can change on the fly eventually
    //cutesdr usage
	virtual void FFTParams(quint32 _fftSize, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
						   WindowFunction::WINDOWTYPE _windowType);
    //Reset to init state, same parameters
    virtual void ResetFFT();


    //If in==NULL, use whatever is in timeDomain buffer
    //If out==NULL, leave result in freqDomain buffer and don't copy to out
	virtual void FFTForward(CPX * in, CPX * out, int numSamples) = 0;
	virtual bool FFTSpectrum(CPX *in, double * out, int numSamples) = 0; //Replacing FFTMagnForward in most uses

    //If in==NULL, use whatever is in freqDomain buffer
    //If out==NULL, then leave result in timeDomain buffer and don't copy to out
	virtual void FFTInverse(CPX * in, CPX * out, int numSamples) = 0;

    //Not virtual and common to all FFT implementations
	void FreqDomainToMagnitude(CPX * freqBuf, double baseline, double correction, double *fbr);
	void OverlapAdd(CPX *out, int numSamples);

    //WIP Calculate m_pFFTPwrAveBuf for any FFT.  Heavily embedded in cuteSDR
    //Compare with cuteSDR to see if we got it right
	void CalcPowerAverages(CPX* in, double *out, int numSamples);

	bool MapFFTToScreen(double *inBuf, qint32 yPixels, qint32 xPixels,
									double maxdB, double mindB,
									qint32 startFreq, qint32 stopFreq,
									qint32 *outBuf);

    int getFFTSize() {return fftSize;}
    CPX *getFreqDomain() {return freqDomain;}
    CPX *getTimeDomain() {return timeDomain;}
	//Fractional bin width in hz
	double getBinWidth() {return binWidth;}
	bool getOverload() {return isOverload;}

protected:
    //Utility
    CPX *timeDomain; //Should always be counted on to have last time domain results
    CPX *freqDomain; //Should always be counted on to have last freqency domain results
    CPX *workingBuf; //Used internally and should never be counted on for anything
    CPX *overlap;
    bool fftParamsSet; //Use to make sure base class calls FFT to init variables

    qint32 fftSize;
    double sampleRate;
	double binWidth;

    bool fftInputOverload;

    //For calculating power averages
	bool isAveraged;
	bool isOverload; //True if any input values exceed overloadLimit

    //This should replace m_mutex in fftcute
    QMutex fftMutex; //Used to sync threads calling FFT and display calling Screen mapping

	void unfoldInOrder(CPX *inBuf, CPX *outBuf);
	WindowFunction *windowFunction;
	WindowFunction::WINDOWTYPE windowType;
	int samplesPerBuffer; //Not the same as fftSize, which may be larger than sample buffers
	double maxBinPower; //Maximum value we should see in any bin, maxAmp * samplesPerBuffer

	double *fftPower; //Raw power calc
	double *fftAmplitude;
	double *fftPhase; //Calculate phase for future plotting

	bool applyWindow(const CPX *in, int numSamples);
};

#endif // FFT_H
