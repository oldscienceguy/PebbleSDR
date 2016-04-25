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
	static FFT* factory(QString _label); //Returns instance based on USE_FFT, USE_FFTCUTE, etc

	const quint32 m_maxFFTSize = 65535;
	const quint32 m_minFFTSize = 2048;
	//Maximum value of input samples -1 to +1
	const double m_ampMax = 1.0;
	const double m_overLimit = 0.9;	//limit for detecting over ranging inputs

    //Keep separate from constructor so we can change on the fly eventually
    //cutesdr usage
	virtual void fftParams(quint32 _fftSize, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
						   WindowFunction::WINDOWTYPE _windowType);
    //Reset to init state, same parameters
	virtual void resetFFT();


    //If in==NULL, use whatever is in timeDomain buffer
    //If out==NULL, leave result in freqDomain buffer and don't copy to out
	virtual void fftForward(CPX * in, CPX * out, int numSamples) = 0;
	virtual bool fftSpectrum(CPX *in, double * out, int numSamples) = 0; //Replacing FFTMagnForward in most uses

    //If in==NULL, use whatever is in freqDomain buffer
    //If out==NULL, then leave result in timeDomain buffer and don't copy to out
	virtual void fftInverse(CPX * in, CPX * out, int numSamples) = 0;

    //Not virtual and common to all FFT implementations
	void freqDomainToMagnitude(CPX * freqBuf, double baseline, double correction, double *fbr);
	void overlapAdd(CPX *out, int numSamples);

    //WIP Calculate m_pFFTPwrAveBuf for any FFT.  Heavily embedded in cuteSDR
    //Compare with cuteSDR to see if we got it right
	void calcPowerAverages(CPX* in, double *out, int numSamples);

	bool mapFFTToScreen(double *inBuf, qint32 yPixels, qint32 xPixels,
									double maxdB, double mindB,
									qint32 startFreq, qint32 stopFreq,
									qint32 *outBuf);

	int getFFTSize() {return m_fftSize;}
	CPX *getFreqDomain() {return m_freqDomain;}
	CPX *getTimeDomain() {return m_timeDomain;}
	//Fractional bin width in hz
	double getBinWidth() {return m_binWidth;}
	bool getOverload() {return m_isOverload;}

protected:
    //Utility
	CPX *m_timeDomain; //Should always be counted on to have last time domain results
	CPX *m_freqDomain; //Should always be counted on to have last freqency domain results
	CPX *m_workingBuf; //Used internally and should never be counted on for anything
	CPX *m_overlap;
	bool m_fftParamsSet; //Use to make sure base class calls FFT to init variables

	qint32 m_fftSize;
	double m_sampleRate;
	double m_binWidth;

	bool m_fftInputOverload;

    //For calculating power averages
	bool m_isAveraged;
	bool m_isOverload; //True if any input values exceed overloadLimit

    //This should replace m_mutex in fftcute
	QMutex m_fftMutex; //Used to sync threads calling FFT and display calling Screen mapping

	void m_unfoldInOrder(CPX *inBuf, CPX *outBuf);
	WindowFunction *m_windowFunction;
	WindowFunction::WINDOWTYPE m_windowType;
	int m_samplesPerBuffer; //Not the same as fftSize, which may be larger than sample buffers
	double m_maxBinPower; //Maximum value we should see in any bin, maxAmp * samplesPerBuffer

	double *m_fftPower; //Raw power calc
	double *m_fftAmplitude;
	double *m_fftPhase; //Calculate phase for future plotting

	bool m_applyWindow(const CPX *in, int numSamples);
};

#endif // FFT_H
