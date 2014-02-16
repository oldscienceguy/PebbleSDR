#include "hpsdrdevice.h"

HPSDRDevice::HPSDRDevice():DeviceInterfaceBase()
{
	InitSettings("HPSDR");

}

HPSDRDevice::~HPSDRDevice()
{

}

bool HPSDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callback);
	Q_UNUSED(_framesPerBuffer);
	return true;
}

bool HPSDRDevice::Connect()
{
	return true;
}

bool HPSDRDevice::Disconnect()
{
	return true;
}

void HPSDRDevice::Start()
{

}

void HPSDRDevice::Stop()
{

}

void HPSDRDevice::ReadSettings()
{
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
}

void HPSDRDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
}

QVariant HPSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "HPSDR Device";
			break;
		case PluginDescription:
			return "HPSDR Devices";
			break;
		case DeviceName:
			return "HPSDRDevice";
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool HPSDRDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void HPSDRDevice::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
}

void HPSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void HPSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}
