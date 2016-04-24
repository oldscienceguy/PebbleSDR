#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include "demod.h"
#include <QMutex>
#include "settings.h"
#include "goertzel.h"
#include "fftw.h"
#include "windowfunction.h"

class SignalSpectrum :
	public ProcessStep
{
    Q_OBJECT

public:
	SignalSpectrum(quint32 _sampleRate, quint32 _hiResSampleRate, quint32 _bufferSize);
	~SignalSpectrum(void);
	void SetHiRes(bool _on) {useHiRes = _on;}
	//Pass in soundcard buffer under/overflow counts for display
	void Unprocessed(CPX * in, int _numSamples);
	void MakeSpectrum(FFT *fft, CPX *in, double *out, int _numSamples); //Use if we just have CPX samples

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

    void SetUpdatesPerSec(int updatespersec);
    bool displayUpdateComplete; //Wrap in access func after testing
    int displayUpdateOverrun; //temp counter

	quint32 getHiResSampleRate() {return hiResSampleRate;}

	void SetSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate);

    qint32 emitFftCounter; //Testing to see if we're getting more paints than signals

	bool getOverload() {return isOverload;}

public slots:

signals:
    void newFftData(); //New spectrum data to display
	void newHiResFftData(); //Not used yet, will allow us to update hires zoom independently if we want to


private:
	bool isOverload;

	quint32 hiResSampleRate;

	QMutex mutex;
	int numSpectrumBins;
	int numHiResSpectrumBins;

	//Spectrum data at different steps in receive chaing
	CPX *rawIQ;
	double *unprocessedSpectrum;
	double *hiResSpectrum; //Post bandpass spectrum with more resolution around signal
	bool useHiRes;

    CPX *tmp_cpx;
	FFT *fftUnprocessed;
	FFT *fftHiRes; //Different sample rate, we might be able to re-use fft, but keep separate for now

    float dbOffset; //Used to calibrate power to db calculations

    int updatesPerSec; //Refresh rate per second
	//Replaces old method of skipping ffts based on count with one that uses elapsed time
	//Simpler and more flexible
	QElapsedTimer spectrumTimer;
	qint64 spectrumTimerUpdate; //How many ms to wait before updating spectrum
	QElapsedTimer hiResTimer;
	qint64 hiResTimerUpdate;

};
