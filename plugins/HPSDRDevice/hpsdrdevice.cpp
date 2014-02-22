#include "hpsdrdevice.h"
#include <QMessageBox>
#include <QProgressDialog>

#if 0
References
Metis: http://www.w7ay.net/site/Software/Metis%20Framework/index.html

#endif

HPSDRDevice::HPSDRDevice():DeviceInterfaceBase()
{
	InitSettings("HPSDR-USB");

	ozyFX2FW.clear();
	ozyFW=0;
	mercuryFW=0;
	penelopeFW=0;
	metisFW=0;
	janusFW=0;

	ReadSettings();
	if (discovery == USE_OZY)
		connectionType = OZY;
	else if (discovery == USE_METIS)
		connectionType = METIS;
	else
		connectionType = UNKNOWN;

	optionUi = NULL;
	inputBuffer = NULL;
	outputBuffer = NULL;
	producerFreeBufPtr = NULL;
	forceReload = false;

}

HPSDRDevice::~HPSDRDevice()
{
	WriteSettings();

	if (connectionType == OZY && usbUtil.IsUSBLoaded()) {
		usbUtil.ReleaseInterface(0);
		usbUtil.CloseDevice();
	}
}

bool HPSDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	ProcessIQData = _callback;
	framesPerBuffer = _framesPerBuffer;

	//Frames are 512 bytes, defined by HPSDR protocol
	//Each frame has 3 sync and 5 Command and control bytes, leaving 504 for data
	//Each sample is 3 bytes(24 bits) x 2 for I and Q plus 2 bytes for Mic-Line
	//Experiment with different values here
	//Lets start with makeing sure every fetch gets at least 2048 samples
	inputBufferSize = OZY_FRAME_SIZE * qRound(framesPerBuffer / 63.0 * 0.5);

	if (outputBuffer != NULL)
		delete[] outputBuffer;
	outputBuffer = new unsigned char[OZY_FRAME_SIZE]; //Max #bytes we can send to Ozy

	if (inputBuffer != NULL)
		delete[] inputBuffer;
	inputBuffer = new unsigned char[inputBufferSize];

	connected = false;
	numProducerBuffers = 50;
	readBufferSize = framesPerBuffer * sizeof(CPX);
	producerConsumer.Initialize(std::bind(&HPSDRDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&HPSDRDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);

	return true;
}

bool HPSDRDevice::ConnectUsb()
{	connectionType = OZY;

	//USB specific below
	if (!usbUtil.IsUSBLoaded()) {
		//Explicit load.  DLL may not exist on users system, in which case we can only suppoprt non-USB devices like SoftRock Lite
		if (!usbUtil.LoadUsb()) {
			QMessageBox::information(NULL,"Pebble","libusb0 could not be loaded.");
			return false;
		} else {
			usbUtil.InitUSB();
		}

	}

	//Must be called after Initialize
	//Sample rate and size must be in consistent units - bytes
	producerConsumer.SetPollingInterval(sampleRate*sizeof(CPX),inputBufferSize);

	if (!Open())
		return false;
	connected = true;

	//This setting allows the user to load and manage the firmware directly if Pebble has a problem for some reason
	//If no init flag, just send Ozy config
	//If no firmware is loaded this will return error
	if (sNoInit) {
		return SendConfig();
	}


	//Check firmware version to see if already loaded
	ozyFX2FW = GetFirmwareInfo();
	if (!forceReload && ozyFX2FW!=NULL) {
		//Firmware already loaded
		qDebug()<<"Firmware String: "<<ozyFX2FW;
		if (!SendConfig()) {
			return false;
		}
	} else {
		qDebug()<<"Firmware String not found, initializing";

		QProgressDialog progress("Loading FX2 and FPGA firmware...", NULL, 0, 10);
		progress.setWindowModality(Qt::WindowModal);
		progress.show();
		progress.setValue(1);

		//Firmware and FPGA code are lost on when Ozy loses power
		//We reload on every connect to make sure
		if (!ResetCPU(true))
			return false;
		progress.setValue(2);
		if (!LoadFirmware(sFW))
			return false;
		progress.setValue(3);
		if (!ResetCPU(false))
			return false;
		progress.setValue(4);
		if (!Close())
			return false;
		progress.setValue(5);

		//Give Ozy a chance to process firmware
		QThread::sleep(5);
		progress.setValue(6);

		if (!Open())
			return false;
		progress.setValue(7);

		if(!SetLED(1,true))
			return false;
		if(!LoadFpga(sFpga))
			return false;
		progress.setValue(8);
		if(!SetLED(1,false))
			return false;

		if (!Close())
			return false;

		if (!Open())
			return false;
		progress.setValue(9);

		//Ozy needs a few Command and Control frames to select the desired clock sources before data can be received
		//So send C&C a few times
		for (int i=0; i<6; i++) {
			if (!SendConfig()) {
				return false;
			}
		}

		ozyFX2FW = GetFirmwareInfo();
		if (ozyFX2FW!=NULL) {
			qDebug()<<"New Firmware FX2FW version :"<<ozyFX2FW;
		} else {
			qDebug()<<"Firmware upload failed";
			return false;
		}

		progress.close();
	}

	//        1E 00 - Reset chip
	//        12 01 - set digital interface active
	//        08 15 - D/A on, mic input, mic 20dB boost
	//        08 14 - ditto but no mic boost
	//        0C 00 - All chip power on
	//        0E 02 - Slave, 16 bit, I2S
	//        10 00 - 48k, Normal mode
	//        0A 00 - turn D/A mute off

	//char foo[] = {0x1e,0x00};
	//int count = WriteI2C(0x1a,foo,2);

	return true;

}
bool HPSDRDevice::ConnectTcp()
{
	bool result;
	connectionType = METIS;
	//Shortest valid address is "0.0.0.0"
	if (metisAddress.length() < 7)
		result = hpsdrNetwork.Init(this, NULL, 0);
	else
		result = hpsdrNetwork.Init(this, metisAddress, metisPort);
	if (result) {
		connected = true;
		return true;
	}
	return false;

}

bool HPSDRDevice::Connect()
{
	if (discovery == USE_AUTO_DISCOVERY) {
		//Try USB first
		if (!ConnectUsb())
			return ConnectTcp();
		else
			return true;

	} else if (discovery == USE_OZY) {
		return ConnectUsb();
	} else if (discovery == USE_METIS){
		return ConnectTcp();
	} else
		return false;
}

bool HPSDRDevice::Disconnect()
{
	if (connectionType == METIS) {
		connected = false;
		connectionType = UNKNOWN;
		return true;
	} else if (connectionType == OZY){
		connected = false;
		connectionType = UNKNOWN;
		return Close();
	} else
		return false;
}

void HPSDRDevice::Start()
{
	sampleCount = 0;
	if (connectionType == METIS) {
		if (!hpsdrNetwork.SendStart())
			return;
		//Metis won't listen to any commands before Start, so send config right after
		SendConfig();
		//TCP does not need producer thread because it uses QUDPSocket readyRead signal
		producerConsumer.Start(false,true);
	} else if (connectionType == OZY) {
		//USB needs producer thread to poll USB
		producerConsumer.Start(true,true);
	}
}

void HPSDRDevice::Stop()
{
	if (connectionType == METIS) {
		if (!hpsdrNetwork.SendStop())
			return;
		QThread::msleep(500); //Short delay for data to stop
	}
	producerConsumer.Stop();
}

//Review and rename common in device base
void HPSDRDevice::ReadSettings()
{
	//Set defaults before we call base ReadSettings
	startupFrequency = 10000000;
	lowFrequency = 150000;
	highFrequency = 33000000;
	startupDemodMode = dmAM;
	iqGain = 1.0;

	DeviceInterfaceBase::ReadSettings();
	//Determine sSpeed from sampleRate
	switch (sampleRate) {
	case 48000:
		sSpeed = C1_SPEED_48KHZ;
		break;
	case 96000:
		sSpeed = C1_SPEED_96KHZ;
		break;
	case 192000:
		sSpeed = C1_SPEED_192KHZ;
		break;
	case 384000:
		sSpeed = C1_SPEED_384KHZ;
		break;
	}


	sGainPreampOn = qSettings->value("GainPreampOn",3.0).toDouble();
	sGainPreampOff = qSettings->value("GainPreampOff",20.0).toDouble();
	sPID = qSettings->value("PID",0x0007).toInt();
	sVID = qSettings->value("VID",0xfffe).toInt();
	//NOTE: I had problems with various combinations of hex and rbf files
	//The most current ones I found were the 'PennyMerge' version which can be found here
	//svn://64.245.179.219/svn/repos_sdr_windows/PowerSDR/branches/kd5tfd/PennyMerge
	sFpga = qSettings->value("FPGA","ozy_janus.rbf").toString();
	sFW = qSettings->value("FW","ozyfw-sdr1k.hex").toString();

	//SDR-WIDGET
	//Skips firmware check and upload
	sNoInit = qSettings->value("NoInit",false).toBool();

	//Harware options
	//sSpeed = qSettings->value("Speed",C1_SPEED_48KHZ).toInt();
	sPTT = qSettings->value("PTT",C0_MOX_INACTIVE).toInt();
	s10Mhz = qSettings->value("10Mhz",C1_10MHZ_MERCURY).toInt();
	s122Mhz = qSettings->value("122Mhz",C1_122MHZ_MERCURY).toInt();
	sConfig = qSettings->value("AtlasConfig",C1_CONFIG_MERCURY).toInt();
	sMic = qSettings->value("MicSource",C1_MIC_PENELOPE).toInt();
	sMode = qSettings->value("Mode",C2_MODE_OTHERS).toInt();
	sPreamp = qSettings->value("LT2208Preamp",C3_LT2208_PREAMP_ON).toInt();
	sDither = qSettings->value("LT2208Dither",C3_LT2208_DITHER_OFF).toInt();
	sRandom = qSettings->value("LT2208Random",C3_LT2208_RANDOM_OFF).toInt();
	discovery = (DISCOVERY)qSettings->value("Discovery",HPSDRDevice::USE_AUTO_DISCOVERY).toInt();
	metisAddress = qSettings->value("MetisAddress","").toString();
	metisPort = qSettings->value("MetisPort",1024).toUInt();
}

void HPSDRDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
	qSettings->setValue("GainPreampOn",sGainPreampOn);
	qSettings->setValue("GainPreampOff",sGainPreampOff);
	qSettings->setValue("PID",sPID);
	qSettings->setValue("VID",sVID);
	qSettings->setValue("FPGA",sFpga);
	qSettings->setValue("FW",sFW);
	qSettings->setValue("NoInit",sNoInit);
	qSettings->setValue("PTT",sPTT);
	qSettings->setValue("10Mhz",s10Mhz);
	qSettings->setValue("122Mhz",s122Mhz);
	qSettings->setValue("AtlasConfig",sConfig);
	qSettings->setValue("MicSource",sMic);
	qSettings->setValue("Mode",sMode);
	qSettings->setValue("LT2208Preamp",sPreamp);
	qSettings->setValue("LT2208Dither",sDither);
	qSettings->setValue("LT2208Random",sRandom);
	qSettings->setValue("Discovery",discovery);
	qSettings->setValue("MetisAddress",metisAddress);
	qSettings->setValue("MetisPort",metisPort);

	qSettings->sync();
}

QVariant HPSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "HPSDR";
			break;
		case PluginDescription:
			return "HPSDR Devices";
			break;
		case DeviceName:
			if (connectionType == OZY)
				return "HPSDR-OZY";
			else if (connectionType == METIS)
				return "HPSDR-METIS";
			else
				return "HPSDR";
		case DeviceType:
			return INTERNAL_IQ;
		case DeviceSampleRates:
			return QStringList()<<"48000"<<"96000"<<"192000"<<"384000";
		case CustomKey1:
			if (sPreamp == C3_LT2208_PREAMP_ON)
				return sGainPreampOn;
			else
				return sGainPreampOff;

		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool HPSDRDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case DeviceFrequency: {
			char cmd[5];
			quint32 freq = _value.toDouble();

			cmd[0] = C0_RX1_FREQ; //Bits set receiver# (if > 1) are we setting for
			cmd[1] = freq >> 24;
			cmd[2] = freq >> 16;
			cmd[3] = freq >> 8;
			cmd[4] = freq;

			if (connectionType == METIS) {
				if (!hpsdrNetwork.SendCommand(cmd)) {
					qDebug()<<"HPSDR-IP failed to update frequency";
					return deviceFrequency;
				}
			} else if (connectionType == OZY){
				if (!WriteOutputBuffer(cmd)) {
					qDebug()<<"HPSDR-USB failed to update frequency";
					return deviceFrequency;
				}
			} else {
				return deviceFrequency;
			}
			deviceFrequency = freq;
			lastFreq = freq;
			return freq;
		}
		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

//USB Only
void HPSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
	if (!connected)
		return;

	//We're using blocking I/O, so BulkTransfer will wait until we get size
	int actual;
	if (usbUtil.BulkTransfer(IN_ENDPOINT6,inputBuffer,inputBufferSize,&actual,OZY_TIMEOUT)) {
		int remainingBytes = actual;
		int len;
		int bufPtr = 0;
		while (remainingBytes > 0){
			len = (remainingBytes < OZY_FRAME_SIZE) ? remainingBytes : OZY_FRAME_SIZE;
			ProcessInputFrame(&inputBuffer[bufPtr],len);
			bufPtr += len;
			remainingBytes -= len;
		}

	} else {
		qDebug()<<"Error or timeout in usb_bulk_read";
	}

}
//Processes each 512 byte frame from inputBuffer
//len is usually BUFSIZE
bool HPSDRDevice::ProcessInputFrame(unsigned char *buf, int len)
{
	if(len < OZY_FRAME_SIZE)
		qDebug()<<"Partial frame received";

	int b=0; //Buffer index
	if (buf[b++]==OZY_SYNC && buf[b++]==OZY_SYNC && buf[b++]==OZY_SYNC){
		unsigned char ctrlBuf[5];
		ctrlBuf[0] = buf[b++];
		ctrlBuf[1] = buf[b++];
		ctrlBuf[2] = buf[b++];
		ctrlBuf[3] = buf[b++];
		ctrlBuf[4] = buf[b++];
		//Extract info from control bytes,  bit 3 has type
		switch ((ctrlBuf[0]>>3) & 0x1f) {
		case 0:
			//Status information
			//int lt2208ADCOverflow = ctrlBuf[0]&0x01;
			//int HermesIO1 = ctrlBuf[0]&0x02;
			//int HermesIO2 = ctrlBuf[0]&0x04;
			//int HermesIO3 = ctrlBuf[0]&0x08;
			//Firmware versions
			//1 bit of Mercury version indicates ADC overflow, so versions run 0-127
			//http://lists.openhpsdr.org/pipermail/hpsdr-openhpsdr.org/2012-January/016571.html
			//But makes no sense per doc, when we install 3.4 rbf, reports as 3.3 here
			//mercuryFW = ctrlBuf[2];

			//We're getting FW value for Penelope of 255 with no board, assume that's an invalid
			if (ctrlBuf[3] < 255)
				penelopeFW = ctrlBuf[3];
			if (connectionType == OZY)
				ozyFW = ctrlBuf[4];
			else if (connectionType == METIS)
				metisFW = ctrlBuf[4];
			break;
		case 0x01:
			//Forward Power
			break;
		case 0x02:
			//Reverse Power
			break;
		case 0x03:
			//AIN4 & AIN6
			break;
		//Support for multiple mercury receivers
		case 0x04: //100xxx
			mercuryFW = (ctrlBuf[1] >> 1);
			//mercury2FW = ctrlBuf[2];
			//mercury3FW = ctrlBuf[3];
			//mercury4FW = ctrolBuf[4];
			break;
		}
		//Extract I/Q and add to buffer
		CPX cpx;
		double mic;
		while (b < len) {
			if (sampleCount == 0) {
				//Get a new buffer
				producerFreeBufPtr = (CPX*)producerConsumer.AcquireFreeBuffer();
				if (producerFreeBufPtr == NULL)
					return false;
			}

			//24bit samples
			//NOTE: The (signed char) and (unsigned char) casts are critical in order for the bit shift to work correctly
			//Otherwise the MSB won't have the sign set correctly
			cpx.re = (signed char)buf[b++]<<16;
			cpx.re += (unsigned char)buf[b++]<<8;
			cpx.re += (unsigned char)buf[b++];
			cpx.re /= 8388607.0; //Convert to +/- float from 24bit int

			cpx.im = (signed char)buf[b++]<<16;
			cpx.im += (unsigned char)buf[b++]<<8;
			cpx.im += (unsigned char)buf[b++];
			cpx.im /= 8388607.0; //Convert to +/- float

			//Skip mic-line sample for now
			mic = (signed char)buf[b++]<<8;
			mic += (unsigned char)buf[b++];
			mic /= 32767.0;

			//Add to dataBuf and see if we have enough samples
			producerFreeBufPtr[sampleCount++] = cpx;
			if (sampleCount == framesPerBuffer) {
				sampleCount = 0;
				producerFreeBufPtr = NULL;
				producerConsumer.ReleaseFilledBuffer(); //Triggers signal that calls consumer
			}
		}
		return true;
	}
	qDebug()<<"Invalid sync in data frame";
	return false; //Invalid sync
}

void HPSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
	if (!connected)
		return;

	//Wait for data from the producer thread
	unsigned char *dataBuf = producerConsumer.AcquireFilledBuffer();

	//Got data, process
	ProcessIQData((CPX *)dataBuf,2048); //!!Check size, CPX samples?

	producerConsumer.ReleaseFreeBuffer();
}

//Gets called several times from connect
bool HPSDRDevice::Open()
{
	if (connectionType == METIS) {
		return true;
	}

	//usbUtil.ListDevices();
	//Keep dev for later use, it has all the descriptor information
	if (!usbUtil.FindAndOpenDevice(sPID,sVID,0))
		return false;
	// Default is config #0, Select config #1 for SoftRock
	// Can't claim interface if config != 1
	if (!usbUtil.SetConfiguration(1))
		return false;

	// Claim interface #0.
	if (!usbUtil.Claim_interface(0))
		return false;
	//Some comments say that altinterface needs to be set to use usb_bulk_read, doesn't appear to be the case
	//int result = usb_set_altinterface(hDev,0);
	//if (result < 0)
	//	return false;

	return true;
}

bool HPSDRDevice::Close()
{
	if (connectionType == METIS) {
		return true;
	}
	int result;
	//Attempting to release interface caused error next time we open().  Why?
	//result = usb_release_interface(hDev,0);
	result = usbUtil.CloseDevice();
	return true;
}


//Configuration bytes are always sent as a group, ie you can't change just one
//This is the only place we do this to avoid inconsistencies
bool HPSDRDevice::SendConfig()
{
	char cmd[5];
	cmd[0] = sPTT;
	cmd[1] = sSpeed | s10Mhz | s122Mhz | sConfig | sMic;
	cmd[2] = sMode;
	cmd[3] = sPreamp | sDither | sRandom;
	//C4_DUPLEX_ON must be ON with 2.5 fw otherwise setting config has unpredictable results while receiver is sending samples
	cmd[4] = C4_DUPLEX_ON |  C4_1RECEIVER;

	if (connectionType == METIS) {
		return hpsdrNetwork.SendCommand(cmd);
	} else if (connectionType == OZY) {
		return WriteOutputBuffer(cmd);
	} else
		return false;
}

void HPSDRDevice::preampChanged(bool b)
{

	//Weird gcc bug, can't use static consts in this construct
	//sPreamp = b ? C3_LT2208_PREAMP_ON : C3_LT2208_PREAMP_OFF;
	if(b)
		sPreamp = C3_LT2208_PREAMP_ON;
	else
		sPreamp = C3_LT2208_PREAMP_OFF;

	//Save to hpsdr.ini
	WriteSettings();

	//Change immediately
	SendConfig();
}
void HPSDRDevice::ditherChanged(bool b)
{
	if(b)
		sDither = C3_LT2208_DITHER_ON;
	else
		sDither = C3_LT2208_DITHER_OFF;

	//Save to hpsdr.ini
	WriteSettings();

	//Change immediately
	SendConfig();
}

void HPSDRDevice::randomChanged(bool b)
{
	if(b)
		sRandom = C3_LT2208_RANDOM_ON;
	else
		sRandom = C3_LT2208_RANDOM_OFF;

	//Save to hpsdr.ini
	WriteSettings();

	//Change immediately
	SendConfig();
}

void HPSDRDevice::optionsAccepted()
{
	//Save settings are are not immediate mode

	//Save to hpsdr.ini
	WriteSettings();

}

void HPSDRDevice::useDiscoveryChanged(bool b)
{
	if (b)
		discovery = USE_AUTO_DISCOVERY;
	optionUi->ipInput->setEnabled(false);
	optionUi->portInput->setEnabled(false);
	WriteSettings();
}

void HPSDRDevice::useOzyChanged(bool b)
{
	if (b)
		discovery = USE_OZY;
	optionUi->ipInput->setEnabled(false);
	optionUi->portInput->setEnabled(false);
	WriteSettings();
}

void HPSDRDevice::useMetisChanged(bool b)
{
	if (b)
		discovery = USE_METIS;
	optionUi->ipInput->setEnabled(true);
	optionUi->portInput->setEnabled(true);
	WriteSettings();
}

void HPSDRDevice::metisAddressChanged()
{
	metisAddress = optionUi->ipInput->text();
	WriteSettings();
}

void HPSDRDevice::metisPortChanged()
{
	metisPort = optionUi->portInput->text().toInt();
	WriteSettings();
}

void HPSDRDevice::SetupOptionUi(QWidget *parent)
{
	if (optionUi != NULL)
		delete optionUi;
	optionUi = new Ui::HPSDROptions();
	optionUi->setupUi(parent);
	parent->setVisible(true);

#if 0
	QFont smFont = settings->smFont;
	QFont medFont = settings->medFont;
	QFont lgFont = settings->lgFont;

	optionUi->enableDither->setFont(medFont);
	optionUi->enablePreamp->setFont(medFont);
	optionUi->enableRandom->setFont(medFont);
	optionUi->hasExcalibur->setFont(medFont);
	optionUi->hasJanus->setFont(medFont);
	optionUi->hasMercury->setFont(medFont);
	optionUi->hasMetis->setFont(medFont);
	optionUi->hasOzy->setFont(medFont);
	optionUi->hasPenelope->setFont(medFont);
	optionUi->mercuryLabel->setFont(medFont);
	optionUi->ozyFX2Label->setFont(medFont);
	optionUi->ozyLabel->setFont(medFont);
	optionUi->penelopeLabel->setFont(medFont);
#endif

	connect(optionUi->enablePreamp,SIGNAL(clicked(bool)),this,SLOT(preampChanged(bool)));
	connect(optionUi->enableDither,SIGNAL(clicked(bool)),this,SLOT(ditherChanged(bool)));
	connect(optionUi->enableRandom,SIGNAL(clicked(bool)),this,SLOT(randomChanged(bool)));

	if (sPreamp == C3_LT2208_PREAMP_ON)
		optionUi->enablePreamp->setChecked(true);
	if (sDither == C3_LT2208_DITHER_ON)
		optionUi->enableDither->setChecked(true);
	if (sRandom == C3_LT2208_RANDOM_ON)
		optionUi->enableRandom->setChecked(true);

	if (connectionType == OZY && ozyFW > 0) {
		optionUi->hasOzy->setChecked(true);
		optionUi->hasOzy->setText("Ozy-USB FW: " + QString::number(ozyFW/10.0) + " FX: " + ozyFX2FW);
	} else {
		optionUi->hasOzy->setChecked(false);
		optionUi->hasOzy->setText("Ozy-USB");
	}

	if (connectionType == METIS && metisFW > 0) {
		optionUi->hasMetis->setChecked(true);
		optionUi->hasMetis->setText("Metis-TCP FW: " + QString::number(metisFW/10.0));
	} else {
		optionUi->hasMetis->setChecked(false);
		optionUi->hasMetis->setText("Metis-TCP");
	}

	if (mercuryFW > 0) {
		optionUi->hasMercury->setChecked(true);
		optionUi->hasMercury->setText("Mercury-Rx FW: " + QString::number(mercuryFW/10.0));
	} else {
		optionUi->hasMercury->setChecked(false);
		optionUi->hasMercury->setText("Mercury-Rx");
	}

	if (penelopeFW > 0) {
		optionUi->hasPenelope->setChecked(true);
		optionUi->hasPenelope->setText("Penelope-Tx FW: " + QString::number(penelopeFW/10.0));
	} else {
		optionUi->hasPenelope->setText("Penelope-Tx");
		optionUi->hasPenelope->setChecked(false);
	}

	if (janusFW > 0) {
		optionUi->hasJanus->setChecked(true);
		optionUi->hasJanus->setText("Janus-audio FW: " + QString::number(penelopeFW/10.0));
	} else {
		optionUi->hasJanus->setText("Janus-audio");
		optionUi->hasJanus->setChecked(false);
	}

	if (discovery == USE_OZY || discovery == USE_AUTO_DISCOVERY) {
		optionUi->ipInput->setEnabled(false);
		optionUi->portInput->setEnabled(false);
	} else if (discovery == USE_METIS){
		optionUi->ipInput->setEnabled(true);
		optionUi->ipInput->setText(metisAddress);
		optionUi->portInput->setEnabled(true);
		optionUi->portInput->setText(QString::number(metisPort));
	}

	if (discovery == USE_AUTO_DISCOVERY)
		optionUi->enableAutoDiscovery->setChecked(true);
	else if (discovery == USE_OZY)
		optionUi->enableUSB->setChecked(true);
	else
		optionUi->enableMetis->setChecked(true);

	connect(optionUi->enableAutoDiscovery,SIGNAL(clicked(bool)),this,SLOT(useDiscoveryChanged(bool)));
	connect(optionUi->enableUSB,SIGNAL(clicked(bool)),this,SLOT(useOzyChanged(bool)));
	connect(optionUi->enableMetis,SIGNAL(clicked(bool)),this,SLOT(useMetisChanged(bool)));

	connect(optionUi->ipInput,&QLineEdit::editingFinished,this,&HPSDRDevice::metisAddressChanged);
	connect(optionUi->portInput,&QLineEdit::editingFinished,this,&HPSDRDevice::metisPortChanged);

}

bool HPSDRDevice::WriteOutputBuffer(char * commandBuf)
{
	if (connectionType == METIS) {
		return true;
	}

	if (!connected)
		return false;

	//We're not sending audio back to Ozy, so zero out all the data buffers
	memset(outputBuffer,0x00,OZY_FRAME_SIZE);

	outputBuffer[0] = OZY_SYNC;
	outputBuffer[1] = OZY_SYNC;
	outputBuffer[2] = OZY_SYNC;

	//Just handle commands for now
	outputBuffer[3] = commandBuf[0];
	outputBuffer[4] = commandBuf[1];
	outputBuffer[5] = commandBuf[2];
	outputBuffer[6] = commandBuf[3];
	outputBuffer[7] = commandBuf[4];

	int actual;
	if (!usbUtil.BulkTransfer(OUT_ENDPOINT2,outputBuffer,OZY_FRAME_SIZE,&actual,OZY_TIMEOUT)) {
		qDebug()<<"Error writing output buffer";
		return false;
	}
	return true;
}

int HPSDRDevice::WriteRam(int fx2StartAddr, unsigned char *buf, int count)
{
	if (connectionType == METIS) {
		return 0;
	}
	int bytesWritten = 0;
	int bytesRemaining = count;
	while (bytesWritten < count) {
		bytesRemaining = count - bytesWritten;
		//Chunk into blocks of MAX_PACKET_SIZE
		int numBytes = bytesRemaining > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : bytesRemaining;
		int result = usbUtil.ControlMsg(VENDOR_REQ_TYPE_OUT,OZY_WRITE_RAM,
			fx2StartAddr+bytesWritten, 0, buf+bytesWritten, numBytes, OZY_TIMEOUT);
		if (result >= 0){
			bytesWritten += result;
		} else {
			return bytesWritten; //Timeout or error, return what was written
		}
	}
	return bytesWritten;
}

//Turns reset mode on/off
bool HPSDRDevice::ResetCPU(bool reset)
{
	if (connectionType == METIS) {
		return true;
	}
	unsigned char writeBuf;
	writeBuf = reset ? 1:0;
	int bytesWritten = WriteRam(0xe600,&writeBuf,1);
	return bytesWritten == 1 ? true:false;
}

bool HPSDRDevice::LoadFpga(QString filename)
{
	if (connectionType == METIS) {
		return true;
	}
	qDebug()<<"Loading FPGA";
	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
		//Pebble.app/contents/macos = 25
		path.chop(25);
#endif

	QFile rbfFile(path + "/PebbleData/" + filename);
	//QT assumes binary unless we add QIODevice::Text
	if (!rbfFile.open(QIODevice::ReadOnly)) {
		qDebug()<<"FPGA file not found";
		return false;
	}
	//Tell Ozy loading FPGA (no data sent)
	if (usbUtil.ControlMsg(VENDOR_REQ_TYPE_OUT, OZY_LOAD_FPGA, 0, FL_BEGIN, NULL, 0, OZY_TIMEOUT) < 0) {
		rbfFile.close();
		return false;
	}

	//Read file and send to Ozy 64bytes at a time
	int bytesRead = 0;
	unsigned char buf[MAX_PACKET_SIZE];
	while((bytesRead = rbfFile.read((char*)buf,sizeof(buf))) > 0) {

		if (usbUtil.ControlMsg(VENDOR_REQ_TYPE_OUT, OZY_LOAD_FPGA, 0, FL_XFER,  buf, bytesRead, OZY_TIMEOUT) < 0) {
			rbfFile.close();
			return false;
		}
	}
	rbfFile.close();
	//Tell Ozy done with FPGA load
	if (usbUtil.ControlMsg(VENDOR_REQ_TYPE_OUT, OZY_LOAD_FPGA, 0, FL_END, NULL, 0, OZY_TIMEOUT) < 0)
		return false;
	qDebug()<<"FPGA upload complete";
	return true;
}

//NOTE: Similar functions are available in QString and QByteArray
int HPSDRDevice::HexByteToUInt(char c)
{
	c = tolower(c);
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}
	else if ( c >= 'a' && c <= 'f' ) {
		return 10 + (c - 'a');
	}
	return -1; //Not hex
}

int HPSDRDevice::HexBytesToUInt(char * buf, int len)
{
	unsigned result = 0;
	int value;
	for ( int i = 0; i < len; i++ ) {
		value = HexByteToUInt(buf[i]);
		if (value < 0)
			return -1;
		//Bytes are in LSB->MSB order?
		result *= 16;
		result += value;
	}
	return result;

}

bool HPSDRDevice::LoadFirmware(QString filename)
{
	if (connectionType == METIS) {
		return true;
	}
	qDebug()<<"Loading firmware";

	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
		//Pebble.app/contents/macos = 25
		path.chop(25);
#endif

	QFile rbfFile(path + "/PebbleData/" + filename);
	if (!rbfFile.open(QIODevice::ReadOnly))
		return false;
	//Read file and send to Ozy 64bytes at a time
	int bytesRead = 0;
	char buf[1030];
	unsigned char writeBuf[256];
	int numLine = 0;
	int dataTuples = 0;
	int address = 0;
	int type = 0;
	int checksum = 0;
	int value = 0;
	int result = 0;
	int index = 0;
	//Read one line at a time
	while((bytesRead = rbfFile.readLine(buf,sizeof(buf))) > 0) {
		numLine++;
		if (buf[0] != ':') {
			qDebug() << "Bad hex record in " << filename << "line " << numLine;
			rbfFile.close();
			return false;
		}
		//Example
		//: 09 0080 00 02 00 6B 00 02 00 6B 00 02 9B
		//  L  ADR  TY DATA..................DATA CS
		//0:0	= :
		//1:2	= L: # data tuples in hex (09 or 18 bytes)
		//3:6	= ADR: address (0080)
		//7:8	= TY: type (00 = record)
		//9:..	= data tuples, 2 bytes each
		//EOL	= CS: checksum (02)

		dataTuples = HexBytesToUInt(&buf[1],2);
		address = HexBytesToUInt(&buf[3],4);
		type = HexBytesToUInt(&buf[7],2);
		if (dataTuples <0 || address < 0 || type < 0){
			qDebug() << "Invalid length, address or type in record";
			rbfFile.close();
			return false;
		}
		switch (type){
		case 0:
			checksum = (unsigned char)(dataTuples + (address & 0xff) + ((address >> 8) + type));

			index = 9;
			for (int i=0; i< dataTuples; i++) {
				value = HexBytesToUInt(&buf[index],2);
				if (value < 0) {
					qDebug()<<"Invalid hex in line "<<numLine;
					rbfFile.close();
					return false;
				}
				writeBuf[i] = value;
				checksum += value;
				index += 2;
			}
			if (((checksum + HexBytesToUInt(&buf[index],2)) & 0xff) != 0) {
				qDebug()<<"Invalid Checksum in line "<<numLine;
				return false;
			}
			//qDebug()<<"Address "<<address;
			result = WriteRam(address, writeBuf, dataTuples);

			if (result < 0) {
				qDebug()<<"Error in WriteRam() uploading firmware";
				rbfFile.close();
				return false;
			}
			break;
		case 1:
			//EOF, Complete
			break;
		default:
			qDebug()<<"Invalid record type";
			return false;
		}
	}
	rbfFile.close();
	qDebug()<<"Firmware update complete";
	return true;
}



QString HPSDRDevice::GetFirmwareInfo()
{
	if (connectionType == METIS) {
		return "";
	}
	int index = 0;
	unsigned char bytes[9]; //Version string is 8 char + null
	int size = 8;
	int result = usbUtil.ControlMsg(VENDOR_REQ_TYPE_IN, OZY_SDR1K_CTL, SDR1K_CTL_READ_VERSION,index, bytes, size, OZY_TIMEOUT);
	if (result >= 0) {
		bytes[result] = 0x00; //Null terminate string
		return (char*)bytes;
	} else {
		return NULL;
	}
}
int HPSDRDevice::WriteI2C(int i2c_addr, unsigned char byte[], int length)
{
	if (connectionType == METIS) {
		return 0;
	}
	if (length < 1 || length > MAX_PACKET_SIZE) {
		return 0;
	} else {
		int result = usbUtil.ControlMsg(
		   VENDOR_REQ_TYPE_OUT,
		   OZY_I2C_WRITE,
		   i2c_addr,
		   0,
		   byte,
		   length,
		   OZY_TIMEOUT
		   );
		if (result >= 0)
			return result;
	}
	return 0;
}

//Ozy LEDs: #1 used to indicate FPGA upload in process
bool HPSDRDevice::SetLED(int which, bool on)
{
	if (connectionType == METIS) {
		return true;
	}
	int val;
	val = on ? 1:0;
	return usbUtil.ControlMsg(VENDOR_REQ_TYPE_OUT, OZY_SET_LED, val, which,  NULL, 0, OZY_TIMEOUT) >= 0;
}
