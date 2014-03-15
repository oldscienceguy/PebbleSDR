#include "examplesdrdevice.h"

ExampleSDRDevice::ExampleSDRDevice():DeviceInterfaceBase()
{
	InitSettings("ExampleSDR");

}

ExampleSDRDevice::~ExampleSDRDevice()
{

}

bool ExampleSDRDevice::Initialize(cbProcessIQData _callback,
								  cbProcessBandscopeData _callbackBandscope,
								  cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	numProducerBuffers = 50;
	return true;
}

bool ExampleSDRDevice::Connect()
{
	return true;
}

bool ExampleSDRDevice::Disconnect()
{
	return true;
}

void ExampleSDRDevice::Start()
{

}

void ExampleSDRDevice::Stop()
{

}

void ExampleSDRDevice::ReadSettings()
{
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
}

void ExampleSDRDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
}

QVariant ExampleSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "Example Device";
			break;
		case PluginDescription:
			return "Template for Devices";
			break;
		case DeviceName:
			return "ExampleDevice";
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool ExampleSDRDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void ExampleSDRDevice::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
}

void ExampleSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void ExampleSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}
