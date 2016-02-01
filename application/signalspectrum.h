#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include "demod.h"
#include <QMutex>
#include "settings.h"
#include "goertzel.h"
#include "fftw.h"
#include "windowfunction.h"

class SignalSpectrum :
	public SignalProcessing
{
    Q_OBJECT

public:
	SignalSpectrum(int _sampleRate, quint32 _hiResSampleRate, int _numSamples);
	~SignalSpectrum(void);
	void SetHiRes(bool _on) {useHiRes = _on;}
	//Pass in soundcard buffer under/overflow counts for display
	void Unprocessed(CPX * in, double inUnder, double inOver, double outUnder, double outOver, int _numSamples);
	void MakeSpectrum(FFT *fft, CPX *in, double *out, int _numSamples, CPX *_window); //Use if we just have CPX samples
	void MakeSpectrum(FFT *fft, double *out); //Used if we already have FFT

	//Used when we already have spectrum, typically from dsp server or device
	//Just copies spectrum into unprocessed
	void SetSpectrum(double *in);

    bool MapFFTToScreen(qint32 maxHeight, qint32 maxWidth,
                                    double maxdB, double mindB,
                                    qint32 startFreq, qint32 stopFreq,
                                    qint32* outBuf );

    bool MapFFTZoomedToScreen(qint32 maxHeight, qint32 maxWidth, double maxdB, double mindB, double zoom, int modeOffset, qint32 *outBuf);

	int BinCount() {return numSpectrumBins;}
	double *GetUnprocessed() {return unprocessedSpectrum;}
	void Zoomed(CPX *in, int _numSamples);
    CPX *RawIQ() {return rawIQ;}

	double inBufferUnderflowCount;
	double inBufferOverflowCount;
	double outBufferUnderflowCount;
	double outBufferOverflowCount;

    //New technique from CuteSdr to ignore spectrum data between updates
    void SetUpdatesPerSec(int updatespersec);
    bool displayUpdateComplete; //Wrap in access func after testing
    int displayUpdateOverrun; //temp counter

	quint32 getHiResSampleRate() {return hiResSampleRate;}

	void SetSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate);

    qint32 emitFftCounter; //Testing to see if we're getting more paints than signals

public slots:

signals:
    void newFftData(); //New spectrum data to display
	void newHiResFftData(); //Not used yet, will allow us to update hires zoom independently if we want to


private:
	quint32 hiResSampleRate;

	QMutex mutex;
	int numSpectrumBins;
	int numHiResSpectrumBins;

	//Spectrum data at different steps in receive chaing
	CPX *rawIQ;
	double *unprocessedSpectrum;
	double *hiResSpectrum; //Post bandpass spectrum with more resolution around signal
	WindowFunction *windowFunction;
	bool useHiRes;

    CPX *tmp_cpx;
	FFT *fftUnprocessed;
	FFT *fftHiRes; //Different sample rate, we might be able to re-use fft, but keep separate for now

    float dbOffset; //Used to calibrate power to db calculations

    int updatesPerSec; //Refresh rate per second
    int skipFfts; //How many samples should we skip to sync with rate
	int skipFftsHiRes; //How many samples should we skip to sync with rate
    int skipFftsCounter; //Keep count of samples we've skipped
	int skipFftsHiResCounter; //Keep zoomed counter separate in case we want to update more frequently

};
