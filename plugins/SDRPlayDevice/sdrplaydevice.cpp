#include "sdrplaydevice.h"

ExampleSDRDevice::ExampleSDRDevice():DeviceInterfaceBase()
{
	InitSettings("ExampleSDR");
	optionUi = NULL;
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


bool ExampleSDRDevice::Command(DeviceInterface::STANDARD_COMMANDS _cmd, QVariant _arg)
{
	switch (_cmd) {
		case CmdConnect:
			DeviceInterfaceBase::Connect();
			//Device specific code follows
			return true;

		case CmdDisconnect:
			DeviceInterfaceBase::Disconnect();
			//Device specific code follows
			return true;

		case CmdStart:
			DeviceInterfaceBase::Start();
			//Device specific code follows
			return true;

		case CmdStop:
			DeviceInterfaceBase::Stop();
			//Device specific code follows
			return true;

		case CmdReadSettings:
			DeviceInterfaceBase::ReadSettings();
			//Device specific settings follow
			return true;

		case CmdWriteSettings:
			DeviceInterfaceBase::WriteSettings();
			//Device specific settings follow
			return true;

		case CmdDisplayOptionUi: {
			//Add ui header file include
			//Add private uiOptions

			//Arg is QWidget *parent
			//Use QVariant::fromValue() to pass, and value<type passed>() to get back
			QWidget *parent = _arg.value<QWidget*>();
			if (optionUi != NULL)
				delete optionUi;

			//Change .h and this to correct class name for ui
			optionUi = new Ui::ExampleSdrOptions();
			optionUi->setupUi(parent);
			parent->setVisible(true);
			return true;
		}
		default:
			return false;
	}
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
		case DeviceFrequency:
			return true; //Must be handled by device

		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void ExampleSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void ExampleSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}
