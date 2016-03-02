#include "examplesdrdevice.h"
#include "db.h"

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
	producerFreeBufPtr = NULL;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(double);
	readBufferSize = framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	producerConsumer.Initialize(std::bind(&ExampleSDRDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&ExampleSDRDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	producerConsumer.SetProducerInterval(deviceSampleRate,framesPerBuffer);
	producerConsumer.SetConsumerInterval(deviceSampleRate,framesPerBuffer);

	//Start this immediately, before connect, so we don't miss any data
	producerConsumer.Start(true,true);

#endif

	return true;
}

void ExampleSDRDevice::ReadSettings()
{
	// +/- db gain required to normalize to fixed level input
	// Default is 0db gain, or a factor of 1.0
	normalizeIQGain = DB::dbToAmplitude(0);

	//Set defaults before calling DeviceInterfaceBase
	DeviceInterfaceBase::ReadSettings();
}

void ExampleSDRDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
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
			ReadSettings();
			return true;

		case CmdWriteSettings:
			DeviceInterfaceBase::WriteSettings();
			//Device specific settings follow
			WriteSettings();
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

QVariant ExampleSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, QVariant _option)
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
		case DeviceType:
			return DeviceInterfaceBase::AUDIO_IQ_DEVICE;
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool ExampleSDRDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, QVariant _option)
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
#if 0
	//For verifying device data format min/max so we can normalize later
	static short maxSample = 0;
	static short minSample = 0;
#endif
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			if ((producerFreeBufPtr = (CPX*)producerConsumer.AcquireFreeBuffer()) == NULL)
				return;
#if 0
			while (running) {
				//This ignores producer thread slices and runs as fast as possible to get samples
				//May be used for sample rates where thread slice is less than 1ms
				//Get data from device and put into producerFreeBufPtr
			}
#else
			//Get data from device and put into producerFreeBufPtr
			//Return and wait for next producer time slice
#endif
#if 0
			//For testing device sample format
			if (producerIBuf[i] > maxSample) {
				maxSample = producerIBuf[i];
				qDebug()<<"New Max sample "<<maxSample;
			}
			if (producerQBuf[i] > maxSample) {
				maxSample = producerQBuf[i];
				qDebug()<<"New Max sample "<<maxSample;
			}
			if (producerIBuf[i] < minSample) {
				minSample = producerIBuf[i];
				qDebug()<<"New Min sample "<<minSample;
			}
			if (producerQBuf[i] < minSample) {
				minSample = producerQBuf[i];
				qDebug()<<"New Min sample "<<minSample;
			}
#endif

			producerConsumer.ReleaseFilledBuffer();
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void ExampleSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	unsigned char *consumerFilledBufferPtr;

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((consumerFilledBufferPtr = producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//Process data in filled buffer and convert to Pebble format in consumerBuffer

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
