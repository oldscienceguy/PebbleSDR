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
	Presets *getPresets() {return m_presets;}
	bool getPowerOn() {return m_powerOn;}

	Settings * getSettings() {return m_settings;}
	void processIQData(CPX *in, quint16 numSamples);
	void processBandscopeData(quint8 *in, quint16 numPoints);
	void processAudioData(CPX *in, quint16 numSamples);
	SignalStrength *getSignalStrength() {return m_signalStrength;}
	SignalSpectrum *getSignalSpectrum() {return m_signalSpectrum;}
	IQBalance *getIQBalance(){return m_iqBalance;}

	DigitalModemInterface *getDigitalModem() {return m_iDigitalModem;}
	void setDigitalModem(QString _name, QWidget *_parent);

	QList<PluginInfo> getModemPluginInfo() {return m_plugins->GetModemPluginInfo();}
	QList<PluginInfo> getDevicePluginInfo() {return m_plugins->GetDevicePluginInfo();}

	Demod *getDemod() {return m_demod;}


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
		void demodModeChanged(DeviceInterface::DEMODMODE _demodMode);
		void audioGainChanged(int g);
		void agcThresholdChanged(int g);
		void squelchChanged(double s);
		void mixerChanged(int f);
		void filterChanged(int lo, int hi);
		void anfChanged(bool b);
		void nb1Changed(bool b);
		void nb2Changed(bool b);
		void agcModeChanged(AGC::AGCMODE _mode);
		void muteChanged(bool b);

private:
	ReceiverWidget *m_receiverWidget;

	Plugins *m_plugins;
	SdrOptions *m_sdrOptions;
	QMenuBar *m_mainMenu;
	QMenu *m_developerMenu;
	QMenu *m_helpMenu;
	QWebEngineView *m_readmeView;
	QWebEngineView *m_gplView;

    //Test bench profiles we can output data to test bench
    enum TestBenchProfiles {
		TB_RAW_IQ = 1,
		TB_POST_MIXER,
		TB_POST_BP,
		TB_POST_DEMOD,
		TB_POST_DECIMATE
    };

	bool m_mute;
	QMutex m_mutex;
	bool m_powerOn;
	Settings *m_settings;
	Presets *m_presets;
	QMainWindow *m_mainWindow;
	DeviceInterface *m_sdr;
	Audio *m_audioOutput;
	Demod *m_demod;
	Mixer *m_mixer;
	NoiseBlanker *m_noiseBlanker;
	NoiseFilter *m_noiseFilter;
	SignalStrength *m_signalStrength;
	SignalSpectrum *m_signalSpectrum;
	AGC *m_agc;
	IQBalance *m_iqBalance;
	DigitalModemInterface *m_iDigitalModem; //Active digital modem if any
	bool m_isRecording;
	QString m_recordingFileName;
	QString m_recordingPath;
	WavFile m_recordingFile;

	double m_frequency; //Current LO frequency (not mixed)
	double m_mixerFrequency;
	double m_demodFrequency; //frequency + mixerFrequency
	double m_lastDemodFrequency;

	int m_sampleRate;
	int m_framesPerBuffer; //#samples in each callback
	//sample rate and buffer size after down sampling step
	int m_demodFrames;
	int m_downSample2Frames;
	FIRFilter *m_downSampleFilter;

    //Trying cuteSdr downsample code
	CFractResampler m_fractResampler; //To get to final audio rate
	CDownConvert m_downConvert1; //Get to reasonable rate for demod and following
	CDownConvert m_downConvertWfm1; //Special to get to 300k
	//Trying new Decimator code
	Decimator *m_demodDecimator;
	Decimator *m_demodWfmDecimator;
	bool m_useDemodDecimator;
	bool m_useDemodWfmDecimator;

	int m_audioOutRate;
	int m_demodSampleRate;
	int m_demodWfmSampleRate;
	CPX *m_workingBuf;
	CPX *m_sampleBuf; //Used to accumulate post mixer/downconvert samples to get a full buffer
	CPX *m_audioBuf; //Used for final audio output processing
	quint16 m_sampleBufLen;

	double *m_dbSpectrumBuf; //Used when spectrum is set by remote

    //Use CuteSDR decimation filters for downsampling
	CDecimateBy2 *m_decimate1;
	int m_decimate2SampleRate;
	CDecimateBy2 *m_decmiate2;
	int m_decimate3SampleRate;
	CDecimateBy2 *m_decimate3;
	int m_decimate4SampleRate;
	CDecimateBy2 *m_decimate4;

	float m_gain;
	double m_squelchDb;

	BandPassFilter *m_bpFilter; //Current BandPass filter
	FIRFilter *m_usbFilter;
	FIRFilter *m_lsbFilter;
	FIRFilter *m_amFilter;

	bool m_converterMode;
	double m_converterOffset;
	bool m_dcRemoveMode;
	DCRemoval *m_dcRemove;

	double m_avgDb; //Average signal strength, used for squelch

};
