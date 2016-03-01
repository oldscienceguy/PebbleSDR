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
	virtual bool Initialize(cbProcessIQData _callback,
							cbProcessBandscopeData _callbackBandscope,
							cbProcessAudioData _callbackAudio,
							quint16 _framesPerBuffer);

	virtual bool Command(STANDARD_COMMANDS _cmd, QVariant _arg);

	//Defaults so devices only have to handle what they need to
	virtual QVariant Get(STANDARD_KEYS _key, QVariant _option = 0);
	virtual bool Set(STANDARD_KEYS _key, QVariant _value, QVariant _option = 0);

protected:
	//Used to be in public DeviceInterface, made private as a transition so we don't have to immediately re-write all the plugins
	virtual bool Connect();
	virtual bool Disconnect();
	virtual void Start();
	virtual void Stop();
	virtual void ReadSettings();
	virtual void WriteSettings();
	//Display device option widget in settings dialog
	virtual void SetupOptionUi(QWidget *parent);


	virtual void InitSettings(QString fname);

	void AudioProducer(float *samples, quint16 numSamples);
	cbProcessIQData ProcessIQData;
	cbProcessBandscopeData ProcessBandscopeData;
	cbProcessAudioData ProcessAudioData;


	//Todo: Flag which of these is just a convenience for Pebble, vs required for the interface
	quint16 framesPerBuffer;

	int lastSpectrumMode; //Spectrum, waterfall, etc
	STARTUP_TYPE startupType;
	double userFrequency;
	double deviceFrequency; //Current device frequency
	double startupFrequency;
	double highFrequency;
	double lowFrequency;
	QString inputDeviceName;
	QString outputDeviceName;
	quint32 sampleRate;

	double userIQGain; //Normalize device so incoming IQ levels are consistent
	double normalizeIQGain; //Per device to normalize device levels at userGain == 1

	IQORDER iqOrder;
	//Image rejection (iqbalance) factors for this device
	double iqBalanceGain;
	double iqBalancePhase;
	bool iqBalanceEnable;

	double lastFreq;
	int startupDemodMode;
	int lastDemodMode;

	//Device needs to manage QSettings since it knows its own settings file name
	QSettings *qSettings;

	int deviceNumber; //For plugins that support multiple devices
	bool connected;
	bool running;

	Perform perform;

	//Not used if we're not using ProducerConsumer
	ProducerConsumer producerConsumer;
	quint16 numProducerBuffers; //For faster sample rates, may need more producer buffers to handle
	int readBufferSize; //Producer buffer size in bytes (not CPX)

	Audio *audioInput;
	quint16 audioOutputSampleRate;
	CPX *audioInputBuffer;

	//Moved responsibility from receiver.cpp ProcessIQ to device
	void normalizeIQ(CPX *cpx, float I, float Q);
	void normalizeIQ(CPX *cpx, qint16 I, qint16 Q); //-32767 to +32767 samples like SDRPlay
	void normalizeIQ(CPX *cpx, quint8 I, quint8 Q); //0 to 255 samples like rtl2832
	void normalizeIQ(CPX *cpx, qint8 I, qint8 Q); //-127 to +127 samples
	void normalizeIQ(CPX *cpx, CPX iq); //Already in cpx format, just swap and gain

	//Used for up or down converters
	bool converterMode;
	double converterOffset;

	quint32 decimateFactor;
	bool removeDC;
};

#endif // DEVICEINTERFACEBASE_H
