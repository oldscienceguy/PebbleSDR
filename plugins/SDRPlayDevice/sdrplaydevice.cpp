#include "sdrplaydevice.h"

SDRPlayDevice::SDRPlayDevice():DeviceInterfaceBase()
{
	InitSettings("SDRPlay");
	optionUi = NULL;

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
	mir_sdr_ErrT err;
	//From API doc, DAB appropriate
	int gainReduction = 40; //Initial 40db from example
	double sampleRateMhz = 2.048; //Sample rate in Mhz, NOT Hz
	double centerFreqMhz = 222.064;
	mir_sdr_Bw_MHzT bandwidthMhz = mir_sdr_BW_1_536;
	mir_sdr_If_kHzT ifKhz = mir_sdr_IF_Zero;
	int samplesPerPacket;
	switch (_cmd) {
		case CmdConnect:
			DeviceInterfaceBase::Connect();
			//Device specific code follows
			//Check version
			float ver;
			err = mir_sdr_ApiVersion(&ver);
			if (err != mir_sdr_Success)
				return false;
			qDebug()<<"SDRPLay Version: "<<ver;


			err = mir_sdr_Init(gainReduction, sampleRateMhz, centerFreqMhz, bandwidthMhz ,ifKhz , &samplesPerPacket);
			if (err != mir_sdr_Success)
				return false;

			return true;

		case CmdDisconnect:
			DeviceInterfaceBase::Disconnect();
			//Device specific code follows
			err = mir_sdr_Uninit();
			if (err != mir_sdr_Success)
				return false;
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
