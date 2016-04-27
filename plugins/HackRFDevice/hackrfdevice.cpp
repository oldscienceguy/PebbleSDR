#include "hackrfdevice.h"
#include "db.h"

HackRFDevice::HackRFDevice():DeviceInterfaceBase()
{
	InitSettings("HackRF");
	optionUi = NULL;
	useSynchronousAPI = true; //Testing
	useSignals = false;
	producerBuf = NULL;
	decimatorBuf = NULL;
	hackrfDevice = NULL;
	decimator = NULL;
	hackrfVersion[0] = '\0';
	hackrfBoardId = 0;
}

HackRFDevice::~HackRFDevice()
{
	if (producerBuf != NULL)
		free (producerBuf);
	if (decimatorBuf != NULL)
		free (decimatorBuf);
	if (decimator != NULL)
		delete decimator;
}

//Sample rate must be set before this is called
bool HackRFDevice::Initialize(cbProcessIQData _callback,
								  cbProcessBandscopeData _callbackBandscope,
								  cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	//If we are decimating, we need to collect more samples
	deviceSamplesPerBuffer = framesPerBuffer * decimateFactor;
	producerBuf = (CPX8*)new qint8[deviceSamplesPerBuffer * sizeof(CPX8)];
	decimatorBuf = CPX::memalign(deviceSamplesPerBuffer);

	decimator = new Decimator(deviceSampleRate, deviceSamplesPerBuffer);
	setSampleRate(deviceSampleRate); //Builds decimation chain

	if (useSynchronousAPI)
		numProducerBuffers = 50;
	else
		numProducerBuffers = 128; //We get 64 buffers worth of data at a time, so this needs to be large enough
	producerFreeBufPtr = NULL;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	readBufferSize = deviceSamplesPerBuffer * sizeof(CPX);

	producerConsumer.Initialize(std::bind(&HackRFDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&HackRFDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	//Producer checks at device rate
	producerConsumer.SetProducerInterval(deviceSampleRate,deviceSamplesPerBuffer);
	//Consumer checks at decimated rate set in setSampleRate()
	producerConsumer.SetConsumerInterval(sampleRate,framesPerBuffer);

	if (useSignals)
		connect(this,SIGNAL(newIQData()),this,SLOT(consumerSlot()));

#endif

	return true;
}

void HackRFDevice::ReadSettings()
{
	//Set defaults before calling DeviceInterfaceBase
	highFrequency = 6000000000;
	//lowFrequency = 1000000;
	//1mHz is spec, but rest of AM band seems ok
	lowFrequency = 500000;
	sampleRate = 8000000;
	deviceSampleRate = sampleRate;
	decimateFactor = 2; //We can't keep up with full device sample rate yet
	DeviceInterfaceBase::ReadSettings();
	//Recommended defaults from hackRf wiki
	rfGain = qSettings->value("rfGain",false).toBool();
	lnaGain = qSettings->value("lnaGain",16).toUInt();
	vgaGain = qSettings->value("vgaGain",16).toUInt();

}

void HackRFDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	qSettings->setValue("rfGain",rfGain);
	qSettings->setValue("lnaGain",lnaGain);
	qSettings->setValue("vgaGain",vgaGain);
	qSettings->sync();
}

void HackRFDevice::setSampleRate(quint32 _sampleRate)
{
	deviceSampleRate = _sampleRate;

	if (decimateFactor == 1) {
		sampleRate = deviceSampleRate;
	} else {
		sampleRate = decimator->buildDecimationChain(deviceSampleRate,200000,deviceSampleRate / decimateFactor);
	}
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
				//qDebug("Board ID Number: %d (%s)\n", hackrfBoardId, hackrf_board_id_name((hackrf_board_id)hackrfBoardId));

				if (!apiCheck(hackrf_version_string_read(hackrfDevice, &hackrfVersion[0], 255),"id_name"))
					return false;
				//qDebug("Firmware Version: %s\n", hackrfVersion);

				hackrf_device_list_free(list);
			}
			return true;

		case CmdDisconnect:
			DeviceInterfaceBase::Disconnect();
			if (hackrfDevice == NULL)
				return true;

			//Device specific code follows
			if (!apiCheck(hackrf_close(hackrfDevice),"close"))
				return false;
			hackrf_exit();
			return true;

		case CmdStart: {

			DeviceInterfaceBase::Start();
			//Device specific code follows

			//If sampleRateDecimate ==4, then set actual device sample rate 4x what user specifies
			if (!apiCheck(hackrf_set_sample_rate(hackrfDevice,deviceSampleRate),"set_sample_rate"))
				return false;

			//5000000 default filter bandwidth 1750000 to 28000000
			//1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz
			//Auto calc filter, we can add manual selection later if needed
			quint32 baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw(deviceSampleRate);
			if (!apiCheck(hackrf_set_baseband_filter_bandwidth(hackrfDevice,baseband_filter_bw_hz),"set_filter_bandwidth"))
				return false;
			//baseband rx gain 0-62dB, 2dB steps
			if (!apiCheck(hackrf_set_vga_gain(hackrfDevice, vgaGain),"set_vga_gain"))
				return false;
			//0 to 40db 8db steps
			if (!apiCheck(hackrf_set_lna_gain(hackrfDevice, lnaGain),"set_lan_gain"))
				return false;
			if (!apiCheck(hackrf_set_amp_enable(hackrfDevice,rfGain),"set amp_enable"))
				return false;

			//Antenna bias enable on/off
			//WARNING: If we don't set this explicitly, it seems to default to on
			//Symptom is high noise and I/Q are offset from each other in Testbench time display
			if (!apiCheck(hackrf_set_antenna_enable(hackrfDevice,false),"set antenna_enable"))
				return false;

			if (useSignals)
				producerConsumer.Start(useSynchronousAPI,false); //Just consumer unless synchronous API
			else
				producerConsumer.Start(useSynchronousAPI,true);


			if (useSynchronousAPI) {
				if (!synchronousStartRx())
					return false;
			} else {
				if (!apiCheck(hackrf_start_rx(hackrfDevice, (hackrf_sample_block_cb_fn)&HackRFDevice::rx_callback, this),"start_rx"))
					return false;
			}

			running = true;
			return true;
		}
		case CmdStop:
			DeviceInterfaceBase::Stop();
			//Device specific code follows
			running = false;
			producerConsumer.Stop();

			if (useSynchronousAPI) {
				return synchronousStopRx();
			} else {
			if (!apiCheck(hackrf_stop_rx(hackrfDevice),"stop_rx"))
				return false;
			}
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
			optionUi->rfGain->setChecked(rfGain);
			if (rfGain)
				optionUi->rfValue->setText("14db");
			else
				optionUi->rfValue->setText("0db");
			connect(optionUi->rfGain,SIGNAL(clicked(bool)),this,SLOT(rfGainChanged(bool)));

			//0 to 40db 8db steps
			optionUi->lnaSlider->setMinimum(0);
			optionUi->lnaSlider->setMaximum(5);
			optionUi->lnaSlider->setValue(lnaGain / 8);
			optionUi->lnaValue->setText(QString::number(lnaGain)+"dB");
			connect(optionUi->lnaSlider,SIGNAL(valueChanged(int)),this,SLOT(lnaGainChanged(int)));

			//baseband rx gain 0-62dB, 2dB steps
			optionUi->vgaSlider->setMinimum(0);
			optionUi->vgaSlider->setMaximum(31);
			optionUi->vgaSlider->setValue(vgaGain / 2);
			optionUi->vgaValue->setText(QString::number(vgaGain)+"dB");
			connect(optionUi->vgaSlider,SIGNAL(valueChanged(int)),this,SLOT(vgaGainChanged(int)));

			optionUi->decimationBox->addItem("Off",1);
			optionUi->decimationBox->addItem("2",2);
			optionUi->decimationBox->addItem("4",4);
			optionUi->decimationBox->addItem("8",8);
			optionUi->decimationBox->addItem("16",16);
			int item = optionUi->decimationBox->findData(decimateFactor);
			optionUi->decimationBox->setCurrentIndex(item);
			connect(optionUi->decimationBox,SIGNAL(currentIndexChanged(int)),this,SLOT(
						decimationChanged(int)));

			QString fwString;
			fwString = fwString.sprintf("FW: %s ",(char *)hackrfVersion);
			optionUi->fwLabel->setText(fwString);

			return true;
		}
		default:
			return false;
	}
}

QVariant HackRFDevice::Get(DeviceInterface::STANDARD_KEYS _key, QVariant _option)
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
			//Sample rates may be decimated by user selection for CPU usage
			return QStringList()<<"8000000"<<"10000000"<<"12500000"<<"16000000"<<"20000000";
		case SampleRate:
			//Default in deviceInterfaceBase is to return deviceSampleRate
			//We may be decimated and over-ride default to return post-decimated sample rate
			return sampleRate;
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool HackRFDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);
	switch (_key) {
		case DeviceFrequency:
			if (!apiCheck(hackrf_set_freq(hackrfDevice, _value.toDouble()),"set_freq"))
				return false;
			return true; //Must be handled by device
		case DeviceSampleRate:
			setSampleRate(_value.toUInt());
			return true;
			break;
		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void HackRFDevice::rfGainChanged(bool _value)
{
	rfGain = _value;
	if (rfGain) {
		optionUi->rfValue->setText("14db");
	} else {
		optionUi->rfValue->setText("0db");
	}
	if (hackrfDevice != NULL)
		apiCheck(hackrf_set_amp_enable(hackrfDevice, rfGain),"amp enable");
	qSettings->sync();
}

int HackRFDevice::hackrf_set_transceiver_mode(hackrf_transceiver_mode value)
{	
	if (hackrfDevice == NULL)
		return HACKRF_SUCCESS;

	int result;
	result = libusb_control_transfer(
		hackrfDevice->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE,
		value,
		0,
		NULL,
		0,
		0
	);

	if( result != 0 )
	{
		return HACKRF_ERROR_LIBUSB;
	} else {
		return HACKRF_SUCCESS;
	}
}

bool HackRFDevice::synchronousStartRx()
{
	if (hackrfDevice == NULL)
		return true;

	int result;
	result = this->hackrf_set_transceiver_mode(HACKRF_TRANSCEIVER_MODE_RECEIVE);
	return result == HACKRF_SUCCESS;
}
bool HackRFDevice::synchronousStopRx()
{
	if (hackrfDevice == NULL)
		return true;

	int result;
	result = this->hackrf_set_transceiver_mode(HACKRF_TRANSCEIVER_MODE_OFF);
	return result == HACKRF_SUCCESS;
}

//Spec for MAX5864 and sample programs for HackRF say sample format is Offset Binary
//but using quint8 for data type doesn't work and results in aliases both sides of 0
//Treating samples as 2's complement (qint8) works and is what other programs seem to do
//Mystery for now
bool HackRFDevice::synchronousRead(CPX8 *_buf, int _bufLen)
{
	if (hackrfDevice == NULL)
		return true;

	const uint8_t endpoint_address = LIBUSB_ENDPOINT_IN | 1;
	//Get enough data so we get framesPerBuffer after sample rate decimation
	int bytesRead;
	//Set timeout to 2x what it should take to get a complete frame, based on sampleRate
	quint16 timeout = deviceSamplesPerBuffer / deviceSampleRate * 1000; //ms per frame
	timeout *= 2;

	int ret = libusb_bulk_transfer(hackrfDevice->usb_device, endpoint_address,
		(unsigned char*)_buf, _bufLen, &bytesRead, timeout);
	if (ret == 0) {
		if (bytesRead != _bufLen) {
			qDebug()<<"libusb bulk transfer partial read: "<<bytesRead;
			return false;
		}
		return true;
	} else if (ret == LIBUSB_ERROR_TIMEOUT) {
		qDebug()<<"libusb bulk transfer timeout";
		return false;
	} else if (ret == LIBUSB_ERROR_OVERFLOW) {
		qDebug()<<"libusb bulk transfer overflow";
		return false;
	}
	return false;
}

void HackRFDevice::lnaGainChanged(int _value)
{
	//0 to 40db 8db steps
	qint16 db = _value * 8;
	if (hackrfDevice != NULL) {
		if (!apiCheck(hackrf_set_lna_gain(hackrfDevice, db),"set_lan_gain")) {
			optionUi->lnaValue->setText("Error");
			return;
		}
	}
	optionUi->lnaValue->setText(QString::number(db)+"dB");
	lnaGain = db;
	qSettings->sync();
}

void HackRFDevice::vgaGainChanged(int _value)
{
	//baseband rx gain 0-62dB, 2dB steps
	qint16 db = _value * 2;
	if (hackrfDevice != NULL) {
		if (!apiCheck(hackrf_set_vga_gain(hackrfDevice, db),"set_vga_gain")) {
			optionUi->vgaValue->setText("Error");
			return;
		}
	}
	optionUi->vgaValue->setText(QString::number(db)+"dB");
	vgaGain = db;
	qSettings->sync();
}

void HackRFDevice::decimationChanged(int _index)
{
	Q_UNUSED(_index);
	decimateFactor = optionUi->decimationBox->currentData().toUInt();
	qSettings->sync();
}

//typedef int(*hackrf_sample_block_cb_fn)(hackrf_transfer*transfer)
//static callback function
int HackRFDevice::rx_callback(hackrf_transfer*transfer)
{
	//qDebug()<<"rx_callback "<<transfer->valid_length;
	HackRFDevice *hackRf = (HackRFDevice *)transfer->rx_ctx;
	hackRf->mutex.lock();
	//Even though transfer->buffer is quint8, actual samples are qint8, so we have to convert
	qint8* buf = (qint8*)transfer->buffer;
	//buffer_length is size of buffer, valid_length is actual bytes transferred
	for (int i=0; i<transfer->valid_length; i+=2) {
		if (hackRf->producerFreeBufPtr == NULL) {
			if ((hackRf->producerFreeBufPtr = (CPX *)hackRf->producerConsumer.AcquireFreeBuffer()) == NULL) {
				qDebug()<<"No free buffers available.  producerIndex = "<<hackRf->producerIndex <<
						  "samplesPerPacket = "<<transfer->valid_length;
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
			//qDebug()<<"producer release";
			if (hackRf->useSignals)
				emit(hackRf->newIQData());
			//perform.StopPerformance(1000);
			//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
			hackRf->producerConsumer.ReleaseFreeBuffer();

			//We process 262144 bytes per call, at 2 bytes per sample and a 2048 sample buffer,
			//this is 64 buffers of data each call.  We may need to increase the number of producer
			//buffers beyond standard 50
		}
	}
	hackRf->mutex.unlock();
	return 0; //-1 if error
}

//Producer is used for testing synchronous API, see callback() for asynchronous API
void HackRFDevice::producerWorker(cbProducerConsumerEvents _event)
{
	quint32 numDecimatedSamples = 0;

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			while (running) {

				if ((producerFreeBufPtr = (CPX*)producerConsumer.AcquireFreeBuffer()) == NULL) {
					qDebug()<<"No free producer buffer";
					return;
				}

				//Get data from device and put into producerFreeBufPtr
				//Return and wait for next producer time slice
				if (!synchronousRead(producerBuf, deviceSamplesPerBuffer * sizeof(CPX8))) {
					//Put back buffer
					producerConsumer.PutbackFreeBuffer();
					return;
				}

				if (decimateFactor > 1) {
					normalizeIQ(decimatorBuf, producerBuf, deviceSamplesPerBuffer);
					numDecimatedSamples = decimator->process(decimatorBuf, producerFreeBufPtr, deviceSamplesPerBuffer);
				} else {
					//No decimation, direct to producer/consumer buffer
					normalizeIQ(producerFreeBufPtr, producerBuf, framesPerBuffer);
				}

				producerConsumer.ReleaseFilledBuffer();
				if (useSignals)
					emit(newIQData());
			}
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void HackRFDevice::consumerSlot()
{
	unsigned char *consumerFilledBufferPtr;
	while (producerConsumer.GetNumFilledBufs() > 0) {
		//Wait for data to be available from producer
		if ((consumerFilledBufferPtr = producerConsumer.AcquireFilledBuffer()) == NULL) {
			qDebug()<<"No filled buffer available";
			return;
		}

		//Process data in filled buffer and convert to Pebble format in consumerBuffer

		//perform.StartPerformance("ProcessIQ");
		ProcessIQData((CPX *)consumerFilledBufferPtr,framesPerBuffer);
		//perform.StopPerformance(1000);
		//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
		producerConsumer.ReleaseFreeBuffer();
	}
}

void HackRFDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			consumerSlot();
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}
