#include "softrocksdrdevice.h"

SoftrockSDRDevice::SoftrockSDRDevice()
{

}

SoftrockSDRDevice::~SoftrockSDRDevice()
{

}

bool SoftrockSDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	return true;
}

bool SoftrockSDRDevice::Connect()
{
	return true;
}

bool SoftrockSDRDevice::Disconnect()
{
	return true;
}

void SoftrockSDRDevice::Start()
{

}

void SoftrockSDRDevice::Stop()
{

}

void SoftrockSDRDevice::ReadSettings()
{

}

void SoftrockSDRDevice::WriteSettings()
{

}

QVariant SoftrockSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "Softrock family";
			break;
		case PluginDescription:
			return "Softrock devices";
			break;
		case DeviceName:
			return "Softrock plugin";
		default:
			return DeviceInterface::Get(_key, _option);
	}
}

bool SoftrockSDRDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		default:
		return DeviceInterface::Set(_key, _value, _option);
	}
}

void SoftrockSDRDevice::SetupOptionUi(QWidget *parent)
{

}

void SoftrockSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{

}

void SoftrockSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{

}
