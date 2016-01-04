#include "hackrfdevice.h"
#include "db.h"

HackRFDevice::HackRFDevice():DeviceInterfaceBase()
{
	InitSettings("HackRF");
	optionUi = NULL;
}

HackRFDevice::~HackRFDevice()
{

}

bool HackRFDevice::Initialize(cbProcessIQData _callback,
								  cbProcessBandscopeData _callbackBandscope,
								  cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	//numProducerBuffers = 50;
	numProducerBuffers = 128; //We get 64 buffers worth of data at a time, so this needs to be large enough
	producerFreeBufPtr = NULL;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(double);
	readBufferSize = framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	producerConsumer.Initialize(std::bind(&HackRFDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&HackRFDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	producerConsumer.SetProducerInterval(sampleRate,framesPerBuffer);
	producerConsumer.SetConsumerInterval(sampleRate,framesPerBuffer);

	//Start this immediately, before connect, so we don't miss any data
	producerConsumer.Start(false,true); //Just consumer

#endif

	return true;
}

void HackRFDevice::ReadSettings()
{
	//Set defaults before calling DeviceInterfaceBase
	//Set for LNA 24db and VGA 6db
	normalizeIQGain = DB::dbToAmplitude(15);
	highFrequency = 6000000000;
	lowFrequency = 1000000;
	DeviceInterfaceBase::ReadSettings();
	lnaGain = qSettings->value("lnaGain",24).toUInt();
	vgaGain = qSettings->value("vgaGain",6).toUInt();
}

void HackRFDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	qSettings->setValue("lnaGain",lnaGain);
	qSettings->setValue("vgaGain",vgaGain);
	qSettings->sync();
}

bool HackRFDevice::apiCheck(int result, const char* api)
{
	if (result != HACKRF_SUCCESS) {
		qDebug("%s failed: %s (%d)\n",api, hackrf_error_name((hackrf_error)result), result);
		return false;
	}
	return true;
}

bool HackRFDevice::Command(DeviceInterface::STANDARD_COMMANDS _cmd, QVariant _arg)
{
	quint32 baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw(sampleRate);

	switch (_cmd) {
		case CmdConnect: {
				DeviceInterfaceBase::Connect();
				//Device specific code follows
				hackrf_device_list_t *list;
				if (!apiCheck(hackrf_init(),"init"))
					return false;

				list = hackrf_device_list();

				if (list->devicecount < 1 ) {
					qDebug("No HackRF boards found.\n");
					return false;
				}
				//We only support 1 (first) device
				hackrfDevice = NULL;
				if (!apiCheck(hackrf_device_list_open(list, 0, &hackrfDevice),"list_open"))
					return false;

				if (!apiCheck(hackrf_board_id_read(hackrfDevice, &hackrfBoardId),"id_read"))
					return false;
				qDebug("Board ID Number: %d (%s)\n", hackrfBoardId, hackrf_board_id_name((hackrf_board_id)hackrfBoardId));

				if (!apiCheck(hackrf_version_string_read(hackrfDevice, &hackrfVersion[0], 255),"id_name"))
					return false;
				qDebug("Firmware Version: %s\n", hackrfVersion);

				hackrf_device_list_free(list);
			}
			return true;

		case CmdDisconnect:
			DeviceInterfaceBase::Disconnect();
			//Device specific code follows
			if (!apiCheck(hackrf_close(hackrfDevice),"close"))
				return false;
			hackrf_exit();
			return true;

		case CmdStart:
			DeviceInterfaceBase::Start();
			//Device specific code follows

			//Testing
			if (!apiCheck(hackrf_set_sample_rate(hackrfDevice,sampleRate),"set_sample_rate"))
				return false;

			//5000000 default filter bandwidth 1750000 to 28000000
			//1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz
			if (!apiCheck(hackrf_set_baseband_filter_bandwidth(hackrfDevice,baseband_filter_bw_hz),"set_filter_bandwidth"))
				return false;
			//baseband rx gain 0-62dB, 2dB steps
			if (!apiCheck(hackrf_set_vga_gain(hackrfDevice, vgaGain),"set_vga_gain"))
				return false;
			//0 to 40db 8db steps
			if (!apiCheck(hackrf_set_lna_gain(hackrfDevice, lnaGain),"set_lan_gain"))
				return false;

			if (!apiCheck(hackrf_start_rx(hackrfDevice, (hackrf_sample_block_cb_fn)&HackRFDevice::rx_callback, this),"start_rx"))
				return false;

			return true;

		case CmdStop:
			DeviceInterfaceBase::Stop();
			//Device specific code follows
			if (!apiCheck(hackrf_stop_rx(hackrfDevice),"stop_rx"))
				return false;
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
			optionUi = new Ui::HackRFOptions();
			optionUi->setupUi(parent);
			parent->setVisible(true);

			//Set up controls, connection, initial display, etc
			//0 to 40db 8db steps
			optionUi->lnaSlider->setMinimum(0);
			optionUi->lnaSlider->setMaximum(5);
			optionUi->lnaSlider->setValue(lnaGain / 8);
			optionUi->lnaValue->setText(QString::number(lnaGain)+"dB");
			connect(optionUi->lnaSlider,SIGNAL(valueChanged(int)),this,SLOT(lnaChanged(int)));

			//baseband rx gain 0-62dB, 2dB steps
			optionUi->vgaSlider->setMinimum(0);
			optionUi->vgaSlider->setMaximum(31);
			optionUi->vgaSlider->setValue(vgaGain / 2);
			optionUi->vgaValue->setText(QString::number(vgaGain)+"dB");
			connect(optionUi->vgaSlider,SIGNAL(valueChanged(int)),this,SLOT(vgaChanged(int)));

			return true;
		}
		default:
			return false;
	}
}

QVariant HackRFDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "HackRFDevice";
			break;
		case PluginDescription:
			return "HackRFDevice";
			break;
		case DeviceName:
			return "HackRF";
		case DeviceType:
			return DeviceInterfaceBase::IQ_DEVICE;
		case DeviceSampleRates:
			//Move to device options page and return empty list
			//Sample rates below 8m work, but are not supported by HackRF due to limited alias filtering
			return QStringList()<<"8000000"<<"10000000"<<"12500000"<<"16000000"<<"20000000";

		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool HackRFDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);
	switch (_key) {
		case DeviceFrequency:
			if (!apiCheck(hackrf_set_freq(hackrfDevice, _value.toDouble()),"set_freq"))
				return false;
			return true; //Must be handled by device
		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void HackRFDevice::lnaChanged(int _value)
{
	//0 to 40db 8db steps
	qint16 db = _value * 8;
	if (!apiCheck(hackrf_set_lna_gain(hackrfDevice, db),"set_lan_gain"))
		optionUi->lnaValue->setText("Error");
	else
		optionUi->lnaValue->setText(QString::number(db)+"dB");
	lnaGain = db;
	qSettings->sync();
}

void HackRFDevice::vgaChanged(int _value)
{
	//baseband rx gain 0-62dB, 2dB steps
	qint16 db = _value * 2;
	if (!apiCheck(hackrf_set_vga_gain(hackrfDevice, db),"set_vga_gain"))
		optionUi->vgaValue->setText("Error");
	else
		optionUi->vgaValue->setText(QString::number(db)+"dB");
	vgaGain = db;
	qSettings->sync();
}

//typedef int(*hackrf_sample_block_cb_fn)(hackrf_transfer*transfer)
//static callback function
int HackRFDevice::rx_callback(hackrf_transfer*transfer)
{
	//qDebug()<<"rx_callback "<<transfer->valid_length;
	HackRFDevice *hackRf = (HackRFDevice *)transfer->rx_ctx;
	hackRf->mutex.lock();
	//buffer_length 262144
	//Even though transfer->buffer is quint8, actual samples are qint8, so we have to convert
	qint8* buf = (qint8*)transfer->buffer;
	//buffer_length vs valid_length?
	for (int i=0; i<transfer->buffer_length; i+=2) {
		if (hackRf->producerFreeBufPtr == NULL) {
			if ((hackRf->producerFreeBufPtr = (CPX *)hackRf->producerConsumer.AcquireFreeBuffer()) == NULL) {
				qDebug()<<"No free buffers available.  producerIndex = "<<hackRf->producerIndex <<
						  "samplesPerPacket = "<<transfer->buffer_length;
				hackRf->mutex.unlock();
				return 0; //Returning -1 stops transfers
			}
			hackRf->producerIndex = 0;
		}

		hackRf->normalizeIQ(&hackRf->producerFreeBufPtr[hackRf->producerIndex], buf[i], buf[i+1]);
		hackRf->producerIndex++;
		if (hackRf->producerIndex >= hackRf->framesPerBuffer) {
			hackRf->producerIndex = 0;
			hackRf->producerFreeBufPtr = NULL;
			hackRf->producerConsumer.ReleaseFilledBuffer();

			//We process 262144 bytes per call, at 2 bytes per sample and a 2048 sample buffer,
			//this is 64 buffers of data each call.  We may need to increase the number of producer
			//buffers beyond standard 50, but yieldCurrentThread() may let consumer run in between
			//ReleaseFilledBuffer() calls
			QThread::yieldCurrentThread();
		}
	}
	hackRf->mutex.unlock();
	return 0; //-1 if error
}

//Producer not used, see callback()
void HackRFDevice::producerWorker(cbProducerConsumerEvents _event)
{
	unsigned char *producerFreeBufPtr;
#if 0
	//For verifying device data format min/max so we can normalize later
	static short maxSample = 0;
	static short minSample = 0;
#endif
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			if ((producerFreeBufPtr = producerConsumer.AcquireFreeBuffer()) == NULL)
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

			//producerConsumer.ReleaseFilledBuffer();
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void HackRFDevice::consumerWorker(cbProducerConsumerEvents _event)
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
				ProcessIQData((CPX *)consumerFilledBufferPtr,framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				producerConsumer.ReleaseFreeBuffer();

			}
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}
