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
	void setHiRes(bool _on) {m_useHiRes = _on;}
	//Pass in soundcard buffer under/overflow counts for display
	void unprocessed(CPX * in, int _numSamples);
	void makeSpectrum(FFT *fft, CPX *in, double *out, int _numSamples); //Use if we just have CPX samples

	//Used when we already have spectrum, typically from dsp server or device
	//Just copies spectrum into unprocessed
	void setSpectrum(double *in);

	bool mapFFTToScreen(qint32 maxHeight, qint32 maxWidth,
                                    double maxdB, double mindB,
                                    qint32 startFreq, qint32 stopFreq,
                                    qint32* outBuf );

	bool mapFFTZoomedToScreen(qint32 maxHeight, qint32 maxWidth, double maxdB, double mindB, double zoom, int modeOffset, qint32 *outBuf);

	int binCount() {return m_numSpectrumBins;}
	double *getUnprocessed() {return m_unprocessedSpectrum;}
	void zoomed(CPX *in, int _numSamples);
	CPX *rawIQ() {return m_rawIQ;}

	void setUpdatesPerSec(int updatespersec);
	bool m_displayUpdateComplete; //Wrap in access func after testing
	int m_displayUpdateOverrun; //temp counter

	quint32 getHiResSampleRate() {return m_hiResSampleRate;}

	void setSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate);

	qint32 m_emitFftCounter; //Testing to see if we're getting more paints than signals

	bool getOverload() {return m_isOverload;}

public slots:

signals:
    void newFftData(); //New spectrum data to display
	void newHiResFftData(); //Not used yet, will allow us to update hires zoom independently if we want to


private:
	bool m_isOverload;

	quint32 m_hiResSampleRate;

	QMutex m_mutex;
	int m_numSpectrumBins;
	int m_numHiResSpectrumBins;

	//Spectrum data at different steps in receive chaing
	CPX *m_rawIQ;
	double *m_unprocessedSpectrum;
	double *m_hiResSpectrum; //Post bandpass spectrum with more resolution around signal
	bool m_useHiRes;

	CPX *m_tmp_cpx;
	FFT *m_fftUnprocessed;
	FFT *m_fftHiRes; //Different sample rate, we might be able to re-use fft, but keep separate for now

	float m_dbOffset; //Used to calibrate power to db calculations

	int m_updatesPerSec; //Refresh rate per second
	//Replaces old method of skipping ffts based on count with one that uses elapsed time
	//Simpler and more flexible
	QElapsedTimer m_spectrumTimer;
	qint64 m_spectrumTimerUpdate; //How many ms to wait before updating spectrum
	QElapsedTimer m_hiResTimer;
	qint64 m_hiResTimerUpdate;

};
