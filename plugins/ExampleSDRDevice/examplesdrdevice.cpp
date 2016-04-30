#include "examplesdrdevice.h"
#include "db.h"

//Plugin constructors are called indirectly when the plugin is loaded in Receiver
//Be careful not to access objects that are not initialized yet, do that in Initialize()
ExampleSDRDevice::ExampleSDRDevice():DeviceInterfaceBase()
{
	initSettings("ExampleSDR");
	optionUi = NULL;
}

//Called when the plugins object is deleted in the ~Receiver()
//Be careful not to access objects that may already be destroyed
ExampleSDRDevice::~ExampleSDRDevice()
{

}

bool ExampleSDRDevice::initialize(CB_ProcessIQData _callback,
								  CB_ProcessBandscopeData _callbackBandscope,
								  CB_ProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	m_numProducerBuffers = 50;
	producerFreeBufPtr = NULL;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(double);
	m_readBufferSize = framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	m_producerConsumer.Initialize(std::bind(&ExampleSDRDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&ExampleSDRDevice::consumerWorker, this, std::placeholders::_1),m_numProducerBuffers, m_readBufferSize);
	//Must be called after Initialize
	m_producerConsumer.SetProducerInterval(m_deviceSampleRate,framesPerBuffer);
	m_producerConsumer.SetConsumerInterval(m_deviceSampleRate,framesPerBuffer);

	//Start this immediately, before connect, so we don't miss any data
	m_producerConsumer.Start(true,true);

#endif

	return true;
}

void ExampleSDRDevice::readSettings()
{
	// +/- db gain required to normalize to fixed level input
	// Default is 0db gain, or a factor of 1.0
	m_normalizeIQGain = DB::dBToAmplitude(0);

	//Set defaults before calling DeviceInterfaceBase
	DeviceInterfaceBase::readSettings();
}

void ExampleSDRDevice::writeSettings()
{
	DeviceInterfaceBase::writeSettings();
}

bool ExampleSDRDevice::command(DeviceInterface::StandardCommands _cmd, QVariant _arg)
{
	switch (_cmd) {
		case Cmd_Connect:
			DeviceInterfaceBase::connectDevice();
			//Device specific code follows
			return true;

		case Cmd_Disconnect:
			DeviceInterfaceBase::disconnectDevice();
			//Device specific code follows
			return true;

		case Cmd_Start:
			DeviceInterfaceBase::startDevice();
			//Device specific code follows
			return true;

		case Cmd_Stop:
			DeviceInterfaceBase::stopDevice();
			//Device specific code follows
			return true;

		case Cmd_ReadSettings:
			DeviceInterfaceBase::readSettings();
			//Device specific settings follow
			readSettings();
			return true;

		case Cmd_WriteSettings:
			DeviceInterfaceBase::writeSettings();
			//Device specific settings follow
			writeSettings();
			return true;

		case Cmd_DisplayOptionUi: {
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

QVariant ExampleSDRDevice::get(DeviceInterface::StandardKeys _key, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_PluginName:
			return "Example Device";
			break;
		case Key_PluginDescription:
			return "Template for Devices";
			break;
		case Key_DeviceName:
			return "ExampleDevice";
		case Key_DeviceType:
			return DeviceInterfaceBase::DT_AUDIO_IQ_DEVICE;
		default:
			return DeviceInterfaceBase::get(_key, _option);
	}
}

bool ExampleSDRDevice::set(DeviceInterface::StandardKeys _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_DeviceFrequency:
			return true; //Must be handled by device

		default:
			return DeviceInterfaceBase::set(_key, _value, _option);
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
			if ((producerFreeBufPtr = (CPX*)m_producerConsumer.AcquireFreeBuffer()) == NULL)
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

			m_producerConsumer.ReleaseFilledBuffer();
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
			while (m_producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((consumerFilledBufferPtr = m_producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//Process data in filled buffer and convert to Pebble format in consumerBuffer

				//perform.StartPerformance("ProcessIQ");
				processIQData(consumerBuffer,framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				m_producerConsumer.ReleaseFreeBuffer();

			}
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}
