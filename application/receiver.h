#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include <QMainWindow>
#include <QtCore/QVariant>
#include <QMutex>
#include "audio.h"
#include "audiopa.h"
#include "cpx.h"
#include "demod.h"
#include "mixer.h"
#include "smeterwidget.h"
#include "receiverwidget.h"
#include "settings.h"
#include "presets.h"
#include "processstep.h"
#include "noiseblanker.h"
#include "noisefilter.h"
#include "signalstrength.h"
#include "signalspectrum.h"
#include "bandpassfilter.h"
#include "agc.h"
#include "iqbalance.h"
#include "fir.h"
#include "fractresampler.h"
#include "wavfile.h"
#include "sdroptions.h"
#include "fft.h"
#include "dcremoval.h"
#include "decimator.h"

#include "plugins.h"
#include <QMenuBar>
#include <QWebEngineView>

class Receiver:public QObject
{
	Q_OBJECT
	friend class SdrOptions; //Avoid lots of needless access methods for now

public:
	Receiver(ReceiverWidget *rw, QMainWindow *main);
	~Receiver(void);
	bool turnPowerOn();
	bool turnPowerOff();
	bool togglePower(bool _on);

	void close(); //Shutdown, just before destructor
	//Called with a requested frequency and the last frequency
	//Calculates next higher or lower (actual), sets it and return to widget for display
	double setSDRFrequency(double fRequested, double fCurrent);
	void setMode(DeviceInterface::DEMODMODE m);
	void setGain(int g);
	void setAgcThreshold(int g);
	void setSquelch(int s);
	void setMixer(int f);
	void setFilter(int lo, int hi);
	void setAnfEnabled(bool b);
	void setNbEnabled(bool b);
	void setNb2Enabled(bool b);
	int setAgcMode(AGC::AGCMODE m);
	void setMute(bool b);
	void showPresets();
	Presets *getPresets() {return presets;}
	bool getPowerOn() {return powerOn;}

	Settings * getSettings() {return settings;}
	void processIQData(CPX *in, quint16 numSamples);
	void processBandscopeData(quint8 *in, quint16 numPoints);
	void processAudioData(CPX *in, quint16 numSamples);
	SignalStrength *getSignalStrength() {return signalStrength;}
	SignalSpectrum *getSignalSpectrum() {return signalSpectrum;}
	IQBalance *getIQBalance(){return iqBalance;}

    ReceiverWidget *receiverWidget;

    DigitalModemInterface *getDigitalModem() {return iDigitalModem;}
	void setDigitalModem(QString _name, QWidget *_parent);

    QList<PluginInfo> getModemPluginInfo() {return plugins->GetModemPluginInfo();}
    QList<PluginInfo> getDevicePluginInfo() {return plugins->GetDevicePluginInfo();}

    Demod *getDemod() {return demod;}


    public slots:
		void restart();
		void recToggled(bool on);
		void sdrOptionsPressed();
		void closeSdrOptions();
		void setWindowTitle();
		void openTestBench();
		void openAboutBox();
		void openDeviceAboutBox();
		void openReadMeWindow();
		void openGPLWindow();

private:
    Plugins *plugins;
	SdrOptions *sdrOptions;
	QMenuBar *mainMenu;
	QMenu *developerMenu;
	QMenu *helpMenu;
	QWebEngineView *readmeView;
	QWebEngineView *gplView;

    //Test bench profiles we can output data to test bench
    enum TestBenchProfiles {
		TB_RAW_IQ = 1,
		TB_POST_MIXER,
		TB_POST_BP,
		TB_POST_DEMOD,
		TB_POST_DECIMATE
    };

	bool mute;
	QMutex mutex;
	bool powerOn;
	Settings *settings;
	Presets *presets;
	QMainWindow *mainWindow;
	DeviceInterface *sdr;
	Audio *audioOutput;
	Demod *demod;
	Mixer *mixer;
	NoiseBlanker *noiseBlanker;
	NoiseFilter *noiseFilter;
	SignalStrength *signalStrength;
	SignalSpectrum *signalSpectrum;
	AGC *agc;
	IQBalance *iqBalance;
    DigitalModemInterface *iDigitalModem; //Active digital modem if any
    bool isRecording;
    QString recordingFileName;
    QString recordingPath;
    WavFile recordingFile;

	double frequency; //Current LO frequency (not mixed)
    double mixerFrequency;
	double demodFrequency; //frequency + mixerFrequency
	double lastDemodFrequency;

	int sampleRate;
	int framesPerBuffer; //#samples in each callback
	//sample rate and buffer size after down sampling step
    int demodFrames;
    int downSample2Frames;
    FIRFilter *downSampleFilter;

    //Trying cuteSdr downsample code
    CFractResampler fractResampler; //To get to final audio rate
    CDownConvert downConvert1; //Get to reasonable rate for demod and following
    CDownConvert downConvertWfm1; //Special to get to 300k
	//Trying new Decimator code
	Decimator *demodDecimator;
	Decimator *demodWfmDecimator;
	bool useDemodDecimator;
	bool useDemodWfmDecimator;

    int audioOutRate;
    int demodSampleRate;
    int demodWfmSampleRate;
    CPX *workingBuf;
    CPX *sampleBuf; //Used to accumulate post mixer/downconvert samples to get a full buffer
    CPX *audioBuf; //Used for final audio output processing
    quint16 sampleBufLen;

	double *dbSpectrumBuf; //Used when spectrum is set by remote

    //Use CuteSDR decimation filters for downsampling
    CDecimateBy2 *decimate1;
    int decimate2SampleRate;
    CDecimateBy2 *decmiate2;
    int decimate3SampleRate;
    CDecimateBy2 *decimate3;
    int decimate4SampleRate;
    CDecimateBy2 *decimate4;

	float gain;
	int squelch;

	BandPassFilter *bpFilter; //Current BandPass filter
	FIRFilter *usbFilter;
	FIRFilter *lsbFilter;
	FIRFilter *amFilter;

	bool converterMode;
	double converterOffset;
	bool dcRemoveMode;
	DCRemoval *dcRemove;

};
