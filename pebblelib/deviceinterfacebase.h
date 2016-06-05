#ifndef DEVICEINTERFACEBASE_H
#define DEVICEINTERFACEBASE_H
#include "device_interfaces.h"
#include "perform.h"
#include "audio.h"
#include "producerconsumer.h"

//Cross platform structure packing
#ifdef _WIN32
	//gcc and clang use __attribute, define it for windows as noop
	#define packStruct
	//Use within struct definition
	#define packStart pack(push, 1)
	#define packEnd pragma pack(pop)
#else
	#define packStart
	#define packEnd
	//Use at end of struct definition
	#define packStruct __attribute__((packed))
#endif

class PEBBLELIBSHARED_EXPORT DeviceInterfaceBase : public DeviceInterface
{
public:
	DeviceInterfaceBase();
	virtual ~DeviceInterfaceBase();
	virtual bool initialize(CB_ProcessIQData _callback,
							CB_ProcessBandscopeData _callbackBandscope,
							CB_ProcessAudioData _callbackAudio,
							quint16 _framesPerBuffer);

	virtual bool command(StandardCommands _cmd, QVariant _arg);

	//Defaults so devices only have to handle what they need to
	virtual QVariant get(StandardKeys _key, QVariant _option = 0);
	virtual bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

protected:
	//Used to be in public DeviceInterface, made private as a transition so we don't have to immediately re-write all the plugins
	virtual bool connectDevice();
	virtual bool disconnectDevice();
	virtual void startDevice();
	virtual void stopDevice();
	virtual void readSettings();
	virtual void writeSettings();
	//Display device option widget in settings dialog
	virtual void setupOptionUi(QWidget *parent);


	virtual void initSettings(QString fname);

	void audioProducer(float *samples, quint16 numSamples);
	CB_ProcessIQData processIQData;
	CB_ProcessBandscopeData processBandscopeData;
	CB_ProcessAudioData processAudioData;


	//Todo: Flag which of these is just a convenience for Pebble, vs required for the interface
	quint16 m_framesPerBuffer;

	int m_lastSpectrumMode; //Spectrum, waterfall, etc
	StartupType m_startupType;
	double m_userFrequency;
	double m_deviceFrequency; //Current device frequency
	double m_startupFrequency;
	double m_highFrequency;
	double m_lowFrequency;
	QString m_inputDeviceName;
	QString m_outputDeviceName;
	quint32 m_sampleRate;
	quint32 m_deviceSampleRate;

	double m_userIQGain; //Normalize device so incoming IQ levels are consistent
	double m_normalizeIQGain; //Per device to normalize device levels at userGain == 1

	IQOrder m_iqOrder;
	//Image rejection (iqbalance) factors for this device
	double m_iqBalanceGain;
	double m_iqBalancePhase;
	bool m_iqBalanceEnable;

	double m_lastFreq;
	int m_startupDemodMode;
	int m_lastDemodMode;

	//Device needs to manage QSettings since it knows its own settings file name
	QSettings *m_settings;

	int m_deviceNumber; //For plugins that support multiple devices
	bool m_connected;
	bool m_running;

	Perform m_perform;

	//Not used if we're not using ProducerConsumer
	ProducerConsumer m_producerConsumer;
	quint16 m_numProducerBuffers; //For faster sample rates, may need more producer buffers to handle
	int m_readBufferSize; //Producer buffer size in bytes (not CPX)

	Audio *m_audioInput;
	quint16 m_audioOutputSampleRate;
	CPX *m_audioInputBuffer;

	//Moved responsibility from receiver.cpp ProcessIQ to device
	void normalizeIQ(CPX *cpx, float I, float Q);
	void normalizeIQ(CPX *cpx, qint16 I, qint16 Q); //-32767 to +32767 samples like SDRPlay
	void normalizeIQ(CPX *cpx, quint8 I, quint8 Q); //0 to 255 samples like rtl2832
	void normalizeIQ(CPX *cpx, qint8 I, qint8 Q); //-127 to +127 samples
	void normalizeIQ(CPX *cpx, CPX iq); //Already in cpx format, just swap and gain

	void normalizeIQ(CPX *_out, CPX8 *_in, quint32 _numSamples, bool _reverse = false);
	void normalizeIQ(CPX *_out, CPXU8 *_in, quint32 _numSamples, bool _reverse = false);
	void normalizeIQ(CPX *_out, CPX16 *_in, quint32 _numSamples, bool _reverse = false);
	void normalizeIQ(CPX *_out, CPX *_in, quint32 _numSamples, bool _reverse = false);
	void normalizeIQ(CPX *_out, CPXFLOAT *_in, quint32 _numSamples, bool _reverse = false);
	//SDRPlay uses separate arrays of I and Q, like splitComplex in vDSP
	void normalizeIQ(CPX *_out, short *_inI, short *_inQ, quint32 _numSamples, bool _reverse = false);

	//Used for up or down converters
	bool m_converterMode;
	double m_converterOffset;

	quint32 m_decimateFactor;
	bool m_removeDC;
};

#endif // DEVICEINTERFACEBASE_H
