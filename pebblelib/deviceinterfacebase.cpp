#include "deviceinterfacebase.h"

DeviceInterfaceBase::DeviceInterfaceBase()
{
}

DeviceInterfaceBase::~DeviceInterfaceBase()
{

}

bool DeviceInterfaceBase::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callback);
	Q_UNUSED(_framesPerBuffer);
	return true;
}

bool DeviceInterfaceBase::Connect()
{
	return true;
}

bool DeviceInterfaceBase::Disconnect()
{
	return true;
}

void DeviceInterfaceBase::Start()
{
	return;
}

void DeviceInterfaceBase::Stop()
{
	return;
}

void DeviceInterfaceBase::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
	return;
}

void DeviceInterfaceBase::WriteSettings()
{
	return;
}

QVariant DeviceInterfaceBase::Get(STANDARD_KEYS _key, quint16 _option) {
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

bool DeviceInterfaceBase::Set(QString _key, QVariant _value) {
	Q_UNUSED(_key);
	Q_UNUSED(_value);
	return false;
}

bool DeviceInterfaceBase::Set(STANDARD_KEYS _key, QVariant _value, quint16 _option) {
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

QVariant DeviceInterfaceBase::Get(QString _key, quint16 _option)
{
	Q_UNUSED(_key);
	Q_UNUSED(_option);
	return QVariant();
}

void DeviceInterfaceBase::ReadSettings()
{
	return;
}
