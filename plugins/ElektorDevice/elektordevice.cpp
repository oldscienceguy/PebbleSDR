#include "elektordevice.h"

ElektorDevice::ElektorDevice():DeviceInterfaceBase()
{
	InitSettings("Elektor");

}

ElektorDevice::~ElektorDevice()
{

}

bool ElektorDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	ProcessIQData = _callback;
	framesPerBuffer = _framesPerBuffer;
	connected = false;
	numProducerBuffers = 50;
	return true;
}

bool ElektorDevice::Connect()
{
	return true;
}

bool ElektorDevice::Disconnect()
{
	return true;
}

void ElektorDevice::Start()
{

}

void ElektorDevice::Stop()
{

}

void ElektorDevice::ReadSettings()
{
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
}

void ElektorDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
}

QVariant ElektorDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "Elektor";
			break;
		case PluginDescription:
			return "Elektor";
			break;
		case DeviceName:
			return "Elektor";
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool ElektorDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void ElektorDevice::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
}

void ElektorDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void ElektorDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}
