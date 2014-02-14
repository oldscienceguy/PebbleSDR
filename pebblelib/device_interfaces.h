#ifndef DEVICE_INTERFACES_H
#define DEVICE_INTERFACES_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
 * WIP to develop an extio.dll equivalent for Pebble, leveraging what we learned from digital modem plugins
 * First example will be to move File device  to this
 *
 * We want devices to be as ignorant as possible about caller so there is no tight coupling to Pebble Receiver class.
 * This allows devices to be used in other programs eventually.
 *
 * We want to support the producer/consumer thread model to support high sample rate devices
 *
 */
#include <QtCore>
#include "pebblelib_global.h"
//Don't use tr1, part of older pre c++ 11 standard
//Use of std::function requires CONFIG += c++11 in .pro file
#include <functional> //xcode header on mac, may be other include on Win

class CPX;

//using std::placeholders;
//ProcessIQData callback: Call with CPX buffer and number of samples
typedef std::function<void(CPX *, quint16)> cbProcessIQData;

class PEBBLELIBSHARED_EXPORT DeviceInterface
{
friend class SDR; //Temp while we're transitioning from internal to plugins to sdr has access to protected

public:
    typedef enum IQORDER {IQ,QI,IONLY,QONLY} IQORDER;
	typedef enum STARTUP_TYPE {SETFREQ = 0, LASTFREQ, DEFAULTFREQ} STARTUP_TYPE;
	//Does device generate IQ or rely on Sound Card (not in plugin)
	typedef enum DEVICE_TYPE{INTERNAL_IQ, AUDIO_IQ} DEVICE_TYPE;
	//These enums can only be extended, not changed, once released.  Otherwise will break existing plugins that are not rebuilt
	enum STANDARD_KEYS {
		PluginName,				//RO QString Name of plugin device was found in
		PluginDescription,		//RO QString Description of plugin device was found in
		PluginNumDevices,		//RO How many unique devices does plugin support, see DeviceNumber for correlation
		DeviceName,				//RO QString Actual device name, may be more than one device per plugin
		DeviceDescription,		//RO QString Actual device description
		DeviceNumber,			//RW Optional index for plugins that support multiple devices
		DeviceType,				//RO int (enum DEVICE_TYPE)
		DeviceSampleRate,		//RW quint32
		DeviceSampleRates,		//RO QStringList Sample rates supported by device
		DeviceFrequency,		//RW double Device center (LO) frequency
		InputDeviceName,		//RW QString Plugins manage settings - OS name for selected Audio input device, if any
		OutputDeviceName,		//RW QString
		HighFrequency,			//RO Highest frequency device supports
		LowFrequency,			//RO Lowest frequency device supports
		FrequencyCorrection,	//RW ???What's the universal format for this?  int ppm?
		IQGain,					//RW double User adjustable to normalize levels among devices
		StartupType,			//RW int (enum STARTUP_TYPE)
		StartupDemodMode,		//RO int (enum DEMODMODE) Default mode for device if not otherwise specified
		StartupSpectrumMode,	//RO Not used yet
		StartupFrequency,		//RO Default frequency for device if not otherwise specified
		LastDemodMode,			//RW int (enum) Mode in use when power off
		LastSpectrumMode,		//RW int (enum) Last spectrum selection
		LastFrequency,			//RW Frequency displayed with power off
		UserDemodMode,			// not used yet int (enum) User specified demod mode
		UserSpectrumMode,		//not used yet
		UserFrequency,			//User specified startup frequency
		IQOrder,				//Enum
		IQBalanceEnabled,		//bool
		IQBalanceGain,			//double
		IQBalancePhase			//double
	};

    DeviceInterface() {};
    virtual ~DeviceInterface() {};
    //Interface must be all pure virtual functions
    virtual bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer) = 0;
    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

    //Display device option widget in settings dialog
    virtual void SetupOptionUi(QWidget *parent) = 0;

    virtual void ReadSettings() = 0;
    virtual void WriteSettings() = 0;

    cbProcessIQData ProcessIQData;

    //Allows us to get/set any device specific data
    //Standard keys will be defined, but any key can be passed
	virtual QVariant Get(QString _key, quint16 _option = 0) {
		Q_UNUSED(_key);
		Q_UNUSED(_option);
		return QVariant();
	}
	//Defaults so devices only have to handle what they need to
	virtual QVariant Get(STANDARD_KEYS _key, quint16 _option = 0) {
		Q_UNUSED(_option);
		switch (_key) {
			case PluginName:
				break;
			case PluginDescription:
				break;
			case PluginNumDevices:
				return 1;
				break;
			case DeviceName:
				break;
			case DeviceDescription:
				break;
			case DeviceNumber:
				return deviceNumber;
				break;
			case DeviceType:
				return AUDIO_IQ;
				break;
			case DeviceSampleRate:
				return sampleRate;
				break;
			case DeviceSampleRates:
				return QStringList();
				break;
			case DeviceFrequency:
				return deviceFrequency;
				break;
			case InputDeviceName:
				return inputDeviceName;
				break;
			case OutputDeviceName:
				return outputDeviceName;
				break;
			case HighFrequency:
				break;
			case LowFrequency:
				break;
			case FrequencyCorrection:
				return 0;
				break;
			case IQGain:
				return iqGain;
				break;
			case StartupType:
				return startupType;
				break;
			case StartupDemodMode:
				break;
			case StartupSpectrumMode:
				break;
			case StartupFrequency:
				break;
			case LastDemodMode:
				return lastDemodMode;
				break;
			case LastSpectrumMode:
				return lastSpectrumMode;
				break;
			case LastFrequency:
				//If freq is outside of mode we are in return default
				if (lastFreq > Get(DeviceInterface::HighFrequency).toDouble() || lastFreq < Get(DeviceInterface::LowFrequency).toDouble())
					return Get(DeviceInterface::StartupFrequency).toDouble();
				else
					return lastFreq;
				break;
			case UserDemodMode:
				break;
			case UserSpectrumMode:
				break;
			case UserFrequency:
				break;
			case IQOrder:
				return iqOrder;
				break;
			case IQBalanceEnabled:
				return iqBalanceEnable;
				break;
			case IQBalanceGain:
				return iqBalanceGain;
				break;
			case IQBalancePhase:
				return iqBalancePhase;
				break;
			default:
				break;
		}
		return QVariant();
	}

	virtual bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0) {
		Q_UNUSED(_value);
		Q_UNUSED(_option);
		switch (_key) {
			case PluginName:
				Q_UNREACHABLE(); //Read only key
				break;
			case PluginDescription:
				Q_UNREACHABLE();
				break;
			case PluginNumDevices:
				Q_UNREACHABLE();
				break;
			case DeviceName:
				Q_UNREACHABLE();
				break;
			case DeviceDescription:
				Q_UNREACHABLE();
				break;
			case DeviceNumber:
				deviceNumber = _value.toInt();
				break;
			case DeviceType:
				Q_UNREACHABLE();
				break;
			case DeviceSampleRate:
				sampleRate = _value.toInt();
				break;
			case DeviceSampleRates:
				Q_UNREACHABLE();
				break;
			case DeviceFrequency:
				Q_UNREACHABLE(); //Must be handled by device
				break;
			case InputDeviceName:
				inputDeviceName = _value.toString();
				break;
			case OutputDeviceName:
				outputDeviceName = _value.toString();
				break;
			case HighFrequency:
				Q_UNREACHABLE();
				break;
			case LowFrequency:
				Q_UNREACHABLE();
				break;
			case FrequencyCorrection:
				break;
			case IQGain:
				iqGain = _value.toDouble();
				break;
			case StartupType:
				startupType = (STARTUP_TYPE)_value.toInt();
				break;
			case StartupDemodMode:
				Q_UNREACHABLE();
				break;
			case StartupSpectrumMode:
				Q_UNREACHABLE();
				break;
			case StartupFrequency:
				Q_UNREACHABLE();
				break;
			case LastDemodMode:
				lastDemodMode = _value.toInt();
				break;
			case LastSpectrumMode:
				lastSpectrumMode = _value.toInt();
				break;
			case LastFrequency:
				lastFreq = _value.toDouble();
				break;
			case UserDemodMode:
				Q_UNREACHABLE(); //Future
				break;
			case UserSpectrumMode:
				Q_UNREACHABLE(); //Future
				break;
			case UserFrequency:
				userFrequency = _value.toDouble();
				break;
			case IQOrder:
				iqOrder = (IQORDER)_value.toInt();
				break;
			case IQBalanceEnabled:
				iqBalanceEnable = _value.toBool();
				break;
			case IQBalanceGain:
				iqBalanceGain = _value.toDouble();
				break;
			case IQBalancePhase:
				iqBalancePhase = _value.toDouble();
				break;
			default:
				break;
		}
		return false;
	}

	virtual bool Set(QString _key, QVariant _value) {
		Q_UNUSED(_key);
		Q_UNUSED(_value);
		return false;
	}


protected:
    //Todo: Flag which of these is just a convenience for Pebble, vs required for the interface
    quint16 framesPerBuffer;

	int lastSpectrumMode; //Spectrum, waterfall, etc
	STARTUP_TYPE startupType;
	double userFrequency;
	double deviceFrequency; //Current device frequency
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
	int lastDemodMode;

    //Device needs to manage QSettings since it knows its own settings file name
    QSettings *qSettings;

    int deviceNumber; //For plugins that support multiple devices

};

//How best to encode version number in interface
#define DeviceInterface_iid "N1DDY.Pebble.DeviceInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DeviceInterface, DeviceInterface_iid)


#endif // DEVICE_INTERFACES_H
