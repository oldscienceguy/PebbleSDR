#ifndef DEVICE_INTERFACES_H
#define DEVICE_INTERFACES_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
/*
 * This defines the public interface for all Pebble compatable device plugins.
 * See ExampleSDRDevice in Pebble source tree for a template example that can be re-used
 *
 * Note There are no explicit dependencies in this interface, except for a CPX class/struct CPX {double re; double im};
 * So it should be usable by any program that wants to re-use these device plugins
 *
 */

#include <QtCore>

//Don't use tr1, part of older pre c++ 11 standard
//Use of std::function requires CONFIG += c++11 in .pro file
#include <functional> //xcode header on mac, may be other include on Win

#if defined(PEBBLELIB_LIBRARY)
#  define PEBBLELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define PEBBLELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

//using std::placeholders;
//Used by audio devices to call back to device with new sample data
typedef std::function<void(float *, quint16)> CB_AudioProducer;

//ProcessIQData callback: Call with CPX buffer of I/Q unprocessed samples and number of samples
typedef std::function<void(CPX *, quint16)> CB_ProcessIQData;

//ProcessBandscopeData callback: Call with buffer of spectrum values and number of data points
typedef std::function<void(quint8 *, quint16)> CB_ProcessBandscopeData;

//ProcessAudioData callback: Call with CPX buffer with left/right audio values and number of samples
typedef std::function<void(CPX *, quint16)> CB_ProcessAudioData;

//Device (HW) options are usually saved to device .ini file
//Application (App) options may or may not be saved
class DeviceInterface
{
public:
	//These enums can only be extended, not changed, once released.  Otherwise will break existing plugins that are not rebuilt
	enum StandardKeys {
		Key_PluginName = 0,			//RO QString Name of plugin device was found in
		Key_PluginDescription,		//RO QString Description of plugin device was found in
		Key_PluginNumDevices,		//RO How many unique devices does plugin support, see DeviceNumber for correlation
		Key_InputDeviceName,		//RW QString Plugins manage settings - OS name for selected Audio input device, if any
		Key_OutputDeviceName,		//RW QString
		Key_SampleRate,				//RW Application I/Q processing sample rate
		Key_HighFrequency,			//RO Highest frequency device supports
		Key_LowFrequency,			//RO Lowest frequency device supports
		Key_IQGain,					//RW double User adjustable to normalize levels among devices
		Key_StartupType,			//RW int (enum STARTUP_TYPE)
		Key_StartupDemodMode,		//RO int (enum DeviceInterface::DEMODMODE) Default mode for device if not otherwise specified
		Key_StartupSpectrumMode,	//RO Not used yet
		Key_StartupFrequency,		//RO Default frequency for device if not otherwise specified
		Key_LastDemodMode,			//RW int (enum) Mode in use when power off
		Key_LastSpectrumMode,		//RW int (enum) Last spectrum selection
		Key_LastFrequency,			//RW Frequency displayed with power off
		Key_UserDemodMode,			// not used yet int (enum) User specified demod mode
		Key_UserSpectrumMode,		//not used yet
		Key_UserFrequency,			//User specified startup frequency
		Key_IQOrder,				//Enum
		Key_IQBalanceEnabled,		//bool
		Key_IQBalanceGain,			//double
		Key_IQBalancePhase,			//double

		//Remote servers may control things like audio output rate
		Key_AudioOutputSampleRate,	//RO quint32

		Key_SettingsFile,			//RO returns .ini file name used by device
		Key_ConverterMode,			//RW returns whether device is using an up/down converter
		Key_ConverterOffset,		//RW Offset to device frequency to use if converter is active
		Key_Setting,				//RW Direct access to device settings file. Settings name in _option
		Key_DecimateFactor,			//RW If device plugin or device supports decimation
		Key_RemoveDC,				//RW Filter to remove dc component in device or plugin

		//Hardware options
		Key_DeviceName,				//RO QString Actual device name, may be more than one device per plugin
		Key_DeviceDescription,		//RO QString Actual device description
		Key_DeviceNumber,			//RW Optional index for plugins that support multiple devices
		Key_DeviceType,				//RO int (enum DEVICE_TYPE)
		Key_DeviceSampleRate,		//RW quint32
		Key_DeviceSampleRates,		//RO QStringList Sample rates supported by device
		Key_DeviceFrequency,		//RW double Device center (LO) frequency
		Key_DeviceHealthValue,		//RO quin16 0-100 where 0 = throwing away data and 100 = within expected tollerances
		Key_DeviceHealthString,		//RO QString explaining last DeviceHealth returned value
		Key_DeviceDemodMode,		//RW quint16 enum DeviceInterface::DEMODMODE
		Key_DeviceOutputGain,		//RW quint16
		Key_DeviceFilter,			//RW QString "lowFilter:highFilter"
		Key_DeviceAGC,				//RW quint16
		Key_DeviceANF,				//RW quint16
		Key_DeviceNB,				//RW quint16
		Key_DeviceSlave,			//RO bool true if device is controled by somthing other than Pebble
		Key_DeviceFreqCorrectionPpm,	//RW If device supports frequency correction in ppm

		//Expansion room if needed
		Key_CustomKey1 = 200,		//Devices can implement custom keys, as long as they start with this
		Key_CustomKey2,
		Key_CustomKey3,
		Key_CustomKey4,
		Key_CustomKey5,
		Key_CustomKey6,
		Key_CustomKey7,
		Key_CustomKey8,
		Key_CustomKey9,
		Key_CustomKey10,
	};

	enum StandardCommands {
		Cmd_Connect,
		Cmd_Disconnect,
		Cmd_Start,
		Cmd_Stop,
		Cmd_ReadSettings,
		Cmd_WriteSettings,
		//Display device option widget in settings dialog
		Cmd_DisplayOptionUi,						//Arg is QWidget *parent
	};

	enum DemodMode {
		dmAM,
		dmSAM,
		dmFMN,
		dmFMM,
		dmFMS,
		dmDSB,
		dmLSB,
		dmUSB,
		dmCWL,
		dmCWU,
		dmDIGL,
		dmDIGU,
		dmNONE
	};

	enum IQOrder {
		IQO_IQ = 0,		//Normal order
		IQO_QI,		//Reversed
		IQO_IONLY,	//Normally used for testing, ignores the Q sample
		IQO_QONLY	//Normally used for testing, ignores the I sample
	};

	enum StartupType {
		ST_SETFREQ = 0,	//Device is started with a specified frequency and mode
		ST_LASTFREQ,		//Device is started with the last used frequency and mode
		ST_DEFAULTFREQ		//Device is started with a frequency and mode defined by device plugin
	};

	//Does device generate IQ, rely on Sound Card for IQ (not in plugin) or return processed audio
	enum DeviceType {
		DT_IQ_DEVICE,			//IQ is generated by device
		DT_AUDIO_IQ_DEVICE,	//IQ is generated by audio handled outside of device
		DT_DSP_DEVICE			//Device is responsible for all DSP, returns audio and spectrum data directly
		};


    //Interface must be all pure virtual functions
	virtual ~DeviceInterface() = 0;

	virtual bool initialize(CB_ProcessIQData _callback,
							CB_ProcessBandscopeData _callbackBandscope,
							CB_ProcessAudioData _callbackAudio,
							quint16 _framesPerBuffer) = 0;

	virtual bool command(StandardCommands _cmd, QVariant _arg) = 0;

    //Allows us to get/set any device specific data
	virtual QVariant get(StandardKeys _key, QVariant _option = 0) = 0;
	virtual bool set(StandardKeys _key, QVariant _value, QVariant _option = 0) = 0;
};

//How best to encode version number in interface
#define DeviceInterface_iid "N1DDY.Pebble.DeviceInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DeviceInterface, DeviceInterface_iid)


#endif // DEVICE_INTERFACES_H
