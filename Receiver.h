#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QMainWindow>
#include <QtCore/QVariant>
#include <QMutex>
#include "softrock.h"
#include "audio.h"
#include "soundcard.h"
#include "cpx.h"
#include "demod.h"
#include "mixer.h"
#include "smeterwidget.h"
#include "receiverwidget.h"
#include "settings.h"
#include "sdr.h"
#include "presets.h"
#include "signalprocessing.h"
#include "noiseblanker.h"
#include "noisefilter.h"
#include "signalstrength.h"
#include "signalspectrum.h"
#include "firfilter.h"
#include "agc.h"
#include "iqbalance.h"
#include "morse.h"

#include "sdr_iq.h"

class Receiver:public QObject
{
	Q_OBJECT

public:
	Receiver(ReceiverWidget *rw, QMainWindow *main);
	~Receiver(void);
	bool On();
	bool Off();
	void Close(); //Shutdown, just before destructor
	bool Power(bool on);
	//Called with a requested frequency and the last frequency
	//Calculates next higher or lower (actual), sets it and return to widget for display
	double SetFrequency(double fRequested, double fCurrent);
	void SetMode(DEMODMODE m);
	void SetGain(int g);
	void SetAgcGainTop(int g);
	void SetSquelch(int s);
	void SetMixer(int f);
	void SetFilter(int lo, int hi);
	void SetAnfEnabled(bool b);
	void SetNbEnabled(bool b);
	void SetNb2Enabled(bool b);
	void SetAgcMode(AGC::AGCMODE m);
	void SetLpfEnabled(bool b);
	void SetMute(bool b);
	void ShowPresets();
	Settings * GetSettings() {return settings;}
	void ProcessBlock(CPX *in, CPX *out, int frameCount);
	void ProcessBlockTimeDomain(CPX *in, CPX *out, int frameCount);
	void ProcessBlockFreqDomain(CPX *in, CPX *out, int frameCount);
	SignalStrength *GetSignalStrength() {return signalStrength;}
	SignalSpectrum *GetSignalSpectrum() {return signalSpectrum;}
	IQBalance *GetIQBalance(){return iqBalance;}
	public slots:
		void Restart();
		void ShowIQBalance(bool b);
		void ShowSettings(bool b);
		void ShowSdrSettings(bool b);

private:
	FFT *fft;
	int fftSize;
	bool useFreqDomainChain;

	bool mute;
	QMutex mutex;
	bool powerOn;
	Settings *settings;
	Presets *presets;
	//SoftRock *softRock;
	ReceiverWidget *receiverWidget;
	QMainWindow *mainWindow;
	SDR *sdr;
	Audio *audioOutput;
	Demod *demod;
	Mixer *mixer;
	NoiseBlanker *noiseBlanker;
	NoiseFilter *noiseFilter;
	SignalStrength *signalStrength;
	SignalSpectrum *signalSpectrum;
	AGC *agc;
	IQBalance *iqBalance;
    Morse *morse;


	double frequency; //Current LO frequency (not mixed)
	int sampleRate;
	int framesPerBuffer; //#samples in each callback
	//sample rate and buffer size after down sampling step
	int downSampleFactor;
	int downSampleRate;
	int downSampleFrames;
	FIRFilter *downSampleFilter;

	float gain;
	double sdrGain;
	int squelch;

	FIRFilter *bpFilter; //Current BandPass filter
	FIRFilter *lpFilter;
	FIRFilter *usbFilter;
	FIRFilter *lsbFilter;
	FIRFilter *amFilter;

	SDR_IQThread *sdrThread;

};
