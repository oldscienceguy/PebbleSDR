#include "sdrplaydevice.h"

SDRPlayDevice::SDRPlayDevice():DeviceInterfaceBase()
{
	InitSettings("SDRPlay");
	optionUi = NULL;

	samplesPerPacket = 0;
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
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(short); //SDRPlay returns shorts
	readBufferSize = framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	producerIBuf = new short[framesPerBuffer * 2]; //2X what we need so we have overflow space
	producerQBuf = new short[framesPerBuffer * 2];
	producerIndex = 0;

	consumerBuffer = CPXBuf::malloc(framesPerBuffer);

	producerConsumer.Initialize(std::bind(&SDRPlayDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&SDRPlayDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	producerConsumer.SetProducerInterval(sampleRate,readBufferSize);
	producerConsumer.SetConsumerInterval(sampleRate,readBufferSize);

#endif

	return true;
}

bool SDRPlayDevice::errorCheck(mir_sdr_ErrT err)
{
	switch (err) {
		case mir_sdr_Success:
			return true;
			break;
		case mir_sdr_Fail:
			return false;
			break;
		case mir_sdr_InvalidParam:
			qDebug()<<"SDRPLay InvalidParam error";
			break;
		case mir_sdr_OutOfRange:
			qDebug()<<"SDRPlay OutOfRange error";
			break;
		case mir_sdr_GainUpdateError:
			qDebug()<<"SDRPlay GainUpdate error";
			break;
		case mir_sdr_RfUpdateError:
			qDebug()<<"SDRPlay RfUpdate error";
			break;
		case mir_sdr_FsUpdateError:
			qDebug()<<"SDRPlay FsUpdate error";
			break;
		case mir_sdr_HwError:
			qDebug()<<"SDRPlay Hw error";
			break;
		case mir_sdr_AliasingError:
			qDebug()<<"SDRPlay Aliasing error";
			return true; //Ignore for now
			break;
		case mir_sdr_AlreadyInitialised:
			qDebug()<<"SDRPlay AlreadyInitialized error";
			break;
		case mir_sdr_NotInitialised:
			qDebug()<<"SDRPlay NotInitialized error";
			break;
		default:
			qDebug()<<"Unknown SDRPlay error";
			break;
	}
	return false;
}

bool SDRPlayDevice::Command(DeviceInterface::STANDARD_COMMANDS _cmd, QVariant _arg)
{
	//From API doc, DAB appropriate
	int gainReduction = 40; //Initial 40db from example
	double sampleRateMhz = 1.536; //sampleRate / 1000000.0; //Sample rate in Mhz, NOT Hz
	double centerFreqMhz = 10.0; //WWV @ 10Mhz
	//Receiver bandwidth, sampleRate must be >=
	mir_sdr_Bw_MHzT bandwidthMhz = mir_sdr_BW_1_536;
	mir_sdr_If_kHzT ifKhz = mir_sdr_IF_Zero;
	switch (_cmd) {
		case CmdConnect:
			DeviceInterfaceBase::Connect();
			//Device specific code follows
			//Check version
			float ver;
			if (!errorCheck(mir_sdr_ApiVersion(&ver)))
				return false;
			qDebug()<<"SDRPLay Version: "<<ver;

			if (!errorCheck(mir_sdr_Init(gainReduction, sampleRateMhz, centerFreqMhz, bandwidthMhz ,ifKhz , &samplesPerPacket)))
					return false;

			connected = true;
			return true;

		case CmdDisconnect:
			DeviceInterfaceBase::Disconnect();
			//Device specific code follows
			if (!errorCheck(mir_sdr_Uninit()))
				return false;
			connected = false;
			return true;

		case CmdStart:
			DeviceInterfaceBase::Start();
			//Device specific code follows
			running = true;
			producerConsumer.Start(true,true);

			return true;

		case CmdStop:
			DeviceInterfaceBase::Stop();
			//Device specific code follows
			running = false;
			producerConsumer.Stop();
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
	QStringList sl;

	switch (_key) {
		case PluginName:
			return "SDRPlay";
			break;
		case PluginDescription:
			return "SDRPlay (Mirics chips)";
			break;
		case DeviceName:
			return "SDRPlay";
		case DeviceType:
			return IQ_DEVICE;
		case DeviceSampleRates:
			//These correspond to SDRPlay IF Bandwidth options
			sl<<"200000";
			sl<<"300000";
			sl<<"600000";
			sl<<"1536000";
			sl<<"5000000";
			sl<<"6000000";
			sl<<"7000000";
			sl<<"8000000";
			return sl;
			break;

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
	unsigned firstSampleNumber;
	//0 = no change, 1 = changed
	int gainReductionChanged;
	int rfFreqChanged;
	int sampleFreqChanged;

	short *producerFreeBufPtr; //Treat as array of shorts

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;

		case cbProducerConsumerEvents::Run:
			if (!connected || !running)
				return;

			//Returns one packet (samplesPerPacket) of data int I and Q buffers
			//We want to read enough data to fill producerIbuf and producerQbuf with framesPerBuffer
			while(running) {
				//Read all the I's into 1st part of buffer, and Q's into 2nd part
				if (!errorCheck(mir_sdr_ReadPacket(&producerIBuf[producerIndex], &producerQBuf[producerIndex], &firstSampleNumber, &gainReductionChanged,
					&rfFreqChanged, &sampleFreqChanged))) {
					return; //Handle error
				}
				producerIndex += samplesPerPacket;
				if (producerIndex >= framesPerBuffer) {
					if ((producerFreeBufPtr = (short *)producerConsumer.AcquireFreeBuffer()) == NULL)
						return;
					for (int i=0, j=0; i<framesPerBuffer; i++, j+=2) {
						//Store in alternating I/Q
						producerFreeBufPtr[j] = producerIBuf[i];
						producerFreeBufPtr[j+1] = producerQBuf[i];
					}
					//Start new buffer, but producerIndex is not even multiple of framesPerBuffer
					//ie if samplesPerPacket = 252, then we will have some samples left over
					//after we process a frame.
					//So don't set to 0, just decrement what we handled
					producerIndex -= framesPerBuffer;
					producerConsumer.ReleaseFilledBuffer();
					//qDebug()<<"Released filled buffer";
					return;
				}
			}
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void SDRPlayDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	short *consumerFilledBufferPtr;
	double re;
	double im;

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			if (!connected || !running)
				return;

			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (running && producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((consumerFilledBufferPtr = (short *)producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//Process data in filled buffer and convert to Pebble format in consumerBuffer
				//Filled buffers always have framesPerBuffer I/Q samples
				for (int i=0,j=0; i<framesPerBuffer; i++, j+=2) {
					//Fill consumerBuffer with normalized -1 to +1 data
					//SDRPlay I/Q data is 16bit integers 0 - 65535
					re = (consumerFilledBufferPtr[j] - 32767) / 32767.0;
					consumerBuffer[i].re = re;
					im = (consumerFilledBufferPtr[j+1] - 32767) / 32767.0;
					consumerBuffer[i].im = im;
					//qDebug()<<re<<" "<<im;
				}

				//perform.StartPerformance("ProcessIQ");
				ProcessIQData(consumerBuffer,framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				producerConsumer.ReleaseFreeBuffer();

			}
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}
