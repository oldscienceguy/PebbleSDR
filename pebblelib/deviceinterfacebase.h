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
	virtual bool Connect();
	virtual bool Disconnect();
	virtual void Start();
	virtual void Stop();

	//Display device option widget in settings dialog
	virtual void SetupOptionUi(QWidget *parent);

	virtual void ReadSettings();
	virtual void WriteSettings();

	cbProcessIQData ProcessIQData;
	cbProcessBandscopeData ProcessBandscopeData;
	cbProcessAudioData ProcessAudioData;

	//Defaults so devices only have to handle what they need to
	virtual QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	virtual bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);

protected:
	virtual void InitSettings(QString fname);

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

	double iqGain; //Normalize device so incoming IQ levels are consistent
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

};

#endif // DEVICEINTERFACEBASE_H
