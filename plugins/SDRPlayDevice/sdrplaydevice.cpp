#include "sdrplaydevice.h"
#include <dlfcn.h> //For dlopen() declarations

SDRPlayDevice::SDRPlayDevice():DeviceInterfaceBase()
{
	InitSettings("SDRPlay");
	optionUi = NULL;

#if 1
	//Explict path to libmir_sdr.so which is installed in applications frameworks sub directory
	QString path = QCoreApplication::applicationDirPath ();
#ifdef Q_OS_MAC
	//Pebble.app/contents/macos = 25
	path.chop(5); //macos
	path += "Frameworks/libmir_sdr"; //We need suffix since this is not a normal mac library suffix
#endif
	api = new QLibrary(path);
	//void *dll = dlopen(path.toUtf8(), RTLD_LAZY);
#endif
	float v;
	//mir_sdr_ApiVersion(&v);
}

SDRPlayDevice::~SDRPlayDevice()
{

}

bool SDRPlayDevice::Initialize(cbProcessIQData _callback,
								  cbProcessBandscopeData _callbackBandscope,
								  cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	numProducerBuffers = 50;
	return true;
}


bool SDRPlayDevice::Command(DeviceInterface::STANDARD_COMMANDS _cmd, QVariant _arg)
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
			optionUi = new Ui::SDRPlayOptions();
			optionUi->setupUi(parent);
			parent->setVisible(true);
			return true;
		}
		default:
			return false;
	}
}

QVariant SDRPlayDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "SDRPlay";
			break;
		case PluginDescription:
			return "SDRPlay (Mirics chips)";
			break;
		case DeviceName:
			return "SDRPlay";
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool SDRPlayDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case DeviceFrequency:
			return true; //Must be handled by device

		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void SDRPlayDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void SDRPlayDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}
