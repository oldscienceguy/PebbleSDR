#include "ghpsdr3device.h"

Ghpsdr3Device::Ghpsdr3Device():DeviceInterfaceBase()
{
	InitSettings("Ghpsdr3");

}

Ghpsdr3Device::~Ghpsdr3Device()
{

}

bool Ghpsdr3Device::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	ProcessIQData = _callback;
	framesPerBuffer = _framesPerBuffer;
	connected = false;
	numProducerBuffers = 50;
	return true;
}

bool Ghpsdr3Device::Connect()
{
	return true;
}

bool Ghpsdr3Device::Disconnect()
{
	return true;
}

void Ghpsdr3Device::Start()
{

}

void Ghpsdr3Device::Stop()
{

}

void Ghpsdr3Device::ReadSettings()
{
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
}

void Ghpsdr3Device::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
}

QVariant Ghpsdr3Device::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "Ghpsdr3Server";
			break;
		case PluginDescription:
			return "Ghpsdr3Server";
			break;
		case DeviceName:
			return "Ghpsdr3Server";
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool Ghpsdr3Device::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void Ghpsdr3Device::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
}

void Ghpsdr3Device::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void Ghpsdr3Device::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}
