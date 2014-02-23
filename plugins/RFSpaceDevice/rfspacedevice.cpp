#include "rfspacedevice.h"
#include "rfsfilters.h"
#include <QMessageBox>

RFSpaceDevice::RFSpaceDevice():DeviceInterfaceBase()
{
	InitSettings("RFSpace");
	ReadSettings();

	//optionUi = NULL;
	inBuffer = NULL;
	outBuffer = NULL;
	readBuf = readBufProducer = NULL;
	usbUtil = new USBUtil(USBUtil::FTDI_D2XX);
	ad6620 = new AD6620(usbUtil);

	if (!usbUtil->IsUSBLoaded()) {
		if (!usbUtil->LoadUsb()) {
			QMessageBox::information(NULL,"Pebble","USB not loaded.  Elektor communication is disabled.");
			return;
		}
	}

	//Todo: SDR-IQ has fixed block size 2048, are we going to support variable size or just force
	inBufferSize = 2048; //settings->framesPerBuffer;
	inBuffer = CPXBuf::malloc(inBufferSize);
	outBuffer = CPXBuf::malloc(inBufferSize);
	//Max data block we will ever read is dataBlockSize
	readBuf = new BYTE[dataBlockSize];
	readBufProducer = new BYTE[dataBlockSize];

	//Instead of getting BW from SDR_IQ options dialog, we now get it directly from settings dialog
	//because we're passing GetSampleRates() during dialog setup
	//So sampleRate is actually bandwidth
	if (sampleRate==190000)
		sBandwidth = AD6620::BW190K;
	else if (sampleRate==150000)
		sBandwidth = AD6620::BW150K;
	else if (sampleRate==100000)
		sBandwidth = AD6620::BW100K;
	else
		sBandwidth = AD6620::BW50K;


}

RFSpaceDevice::~RFSpaceDevice()
{
	WriteSettings();
	usbUtil->CloseDevice();

	if (inBuffer != NULL)
		CPXBuf::free(inBuffer);
	if (outBuffer != NULL)
		CPXBuf::free(outBuffer);
	if (readBuf != NULL)
		free (readBuf);
	if (readBufProducer != NULL)
		free (readBufProducer);

}

bool RFSpaceDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	ProcessIQData = _callback;
	framesPerBuffer = _framesPerBuffer;
	connected = false;
	numProducerBuffers = 50;

	//!! Callbacks not set
	producerConsumer.Initialize(NULL,NULL,numProducerBuffers,4096 * sizeof(short));

	return true;
}

bool RFSpaceDevice::Connect()
{
	if (!usbUtil->FindDevice("SDR-IQ",false))
		return false;

	//Open device
	if (!usbUtil->OpenDevice())
		return false; // FT_Open failed

	usbUtil->ResetDevice();
	//Make sure driver buffers are empty
	usbUtil->Purge();
	usbUtil->SetTimeouts(500,500);
	//Testing: Increase size of internal buffers from default 4096
	//ftStatus = FT_SetUSBParameters(ftHandle,8192,8192);


	//SDR sends a 0 on startup, clear it out of buffer
	if (!usbUtil->Read(readBuf,1))
		return false;

	connected = true;
	return true;
}

bool RFSpaceDevice::Disconnect()
{
	connected = false;
	return true;
}

void RFSpaceDevice::Start()
{
	producerConsumer.Start(true,true);
	//Set bandwidth first, takes a while and returns lots of garbage ACKs
	ad6620->SetBandwidth(sBandwidth);

	//We're going to get back over 256 ACKs, eat em before continuing
	//Otherwise we fill up data buffers while processing acks
	FlushDataBlocks();
	//Get basic Device Information for later use, name, version, etc
	bool result = RequestTargetName();
	result = RequestTargetSerialNumber();
	result = RequestFirmwareVersion();
	result = RequestBootcodeVersion();
	result = RequestInterfaceVersion();

	//Set IF Gain, 0,+6, +12,+18,+24
	SetIFGain(sIFGain);

	//RFGain MUST be set to something when we power up or we get no data in buffer
	//RFGain is actually an attenuator
	SetRFGain(sRFGain);

	//Finally ready to start getting data samples
	StartCapture();
}

void RFSpaceDevice::Stop()
{
	StopCapture();
	producerConsumer.Stop();
	FlushDataBlocks();
}

void RFSpaceDevice::ReadSettings()
{
	lowFrequency = 150000;
	highFrequency = 33000000;
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
	sBandwidth = qSettings->value("Bandwidth",3).toInt(); //Enum
	sRFGain = qSettings->value("RFGain",0).toInt();
	sIFGain = qSettings->value("IFGain",18).toInt();

}

void RFSpaceDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
	qSettings->setValue("Bandwidth",sBandwidth); //Enum
	qSettings->setValue("RFGain",sRFGain);
	qSettings->setValue("IFGain",sIFGain);

	qSettings->sync();
}

QVariant RFSpaceDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "RFSPace";
			break;
		case PluginDescription:
			return "RFSpace SDR-IQ and SDR-IP";
			break;
		case DeviceName:
			return "RFSpace";
		case DeviceSampleRates:
			return QStringList()<<"50000"<<"100000"<<"150000"<<"190000";
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool RFSpaceDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void RFSpaceDevice::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
	if (optionUi != NULL)
		delete optionUi;
	optionUi = new Ui::SDRIQOptions;
	optionUi->setupUi(parent);
	parent->setVisible(true);

#if 0
	QFont smFont = settings->smFont;
	QFont medFont = settings->medFont;
	QFont lgFont = settings->lgFont;

	optionUi->bandwidthBox->setFont(medFont);
	optionUi->bootLabel->setFont(medFont);
	optionUi->firmwareLabel->setFont(medFont);
	optionUi->ifGainBox->setFont(medFont);
	optionUi->interfaceVersionLabel->setFont(medFont);
	optionUi->label->setFont(medFont);
	optionUi->label_2->setFont(medFont);
	optionUi->label_3->setFont(medFont);
	optionUi->nameLabel->setFont(medFont);
	optionUi->rfGainBox->setFont(medFont);
	optionUi->serialNumberLabel->setFont(medFont);
#endif

	optionUi->rfGainBox->addItem("  0db");
	optionUi->rfGainBox->addItem("-10db");
	optionUi->rfGainBox->addItem("-20db");
	optionUi->rfGainBox->addItem("-30db");
	connect(optionUi->rfGainBox,SIGNAL(currentIndexChanged(int)),this,SLOT(rfGainChanged(int)));

	optionUi->ifGainBox->addItem("  0db");
	optionUi->ifGainBox->addItem(" +6db");
	optionUi->ifGainBox->addItem("+12db");
	optionUi->ifGainBox->addItem("+18db");
	optionUi->ifGainBox->addItem("+24db");
	connect(optionUi->ifGainBox,SIGNAL(currentIndexChanged(int)),this,SLOT(ifGainChanged(int)));

	optionUi->bandwidthBox->addItem(" 5KHz");
	optionUi->bandwidthBox->addItem(" 10KHz");
	optionUi->bandwidthBox->addItem(" 25KHz");
	optionUi->bandwidthBox->addItem(" 50KHz");
	optionUi->bandwidthBox->addItem("100KHz");
	optionUi->bandwidthBox->addItem("150KHz");
	optionUi->bandwidthBox->addItem("190KHz");
	connect(optionUi->bandwidthBox,SIGNAL(currentIndexChanged(int)),this,SLOT(bandwidthChanged(int)));


	if (connected) {
		optionUi->nameLabel->setText("Name: " + targetName);
		optionUi->serialNumberLabel->setText("Serial #: " + serialNumber);
		optionUi->firmwareLabel->setText("Firmware Version: " + QString::number(firmwareVersion));
		optionUi->bootLabel->setText("Bootcode Version: " + QString::number(bootcodeVersion));
		optionUi->interfaceVersionLabel->setText("Interface Version: " + QString::number(interfaceVersion));

		if (rfGain != 0)
			optionUi->rfGainBox->setCurrentIndex(rfGain / -10);
		if (ifGain != 0)
			optionUi->ifGainBox->setCurrentIndex(ifGain / 6);

		optionUi->bandwidthBox->setCurrentIndex(sBandwidth);
	}


}

void RFSpaceDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

//Device specific
//0,-10,-20,-30
bool RFSpaceDevice::SetRFGain(qint8 gain)
{
	BYTE writeBuf[6] = { 0x06, 0x00, 0x38, 0x00, 0xff, 0xff};
	writeBuf[4] = 0x00;
	writeBuf[5] = gain ;

	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf))) {
		return false;
	}
	return true;
}
//This is not documented in Interface spec, but used in activeX examples
bool RFSpaceDevice::SetIFGain(qint8 gain)
{
	BYTE writeBuf[6] = { 0x06, 0x00, 0x40, 0x00, 0xff, 0xff};
	//Bits 7,6,5 are Factory test bits and are masked
	writeBuf[4] = 0; //gain & 0xE0;
	writeBuf[5] = gain;

	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return false;
	else
		return true;
}
unsigned RFSpaceDevice::StartCapture()
{
	BYTE writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x00, 0x00};
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return -1;
	return 0;
}

unsigned RFSpaceDevice::StopCapture()
{
	BYTE writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x01, 0x00, 0x00};
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return -1;
	return 0;
}
void RFSpaceDevice::FlushDataBlocks()
{
	bool result;
	do
		result = usbUtil->Read(readBuf, 1);
	while (result);
}
//Control Item Code 0x0001
bool RFSpaceDevice::RequestTargetName()
{
	targetName = "Pending";
	//0x04, 0x20 is the request command
	//0x01, 0x00 is the Control Item Code (command)
	BYTE writeBuf[4] = { 0x04, 0x20, 0x01, 0x00 };
	//Ask for data
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return false;
	return true;
}
//Same pattern as TargetName
bool RFSpaceDevice::RequestTargetSerialNumber()
{
	serialNumber = "Pending";
	BYTE writeBuf[4] = { 0x04, 0x20, 0x02, 0x00 };
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return false;
	return true;
}
bool RFSpaceDevice::RequestInterfaceVersion()
{
	interfaceVersion = 0;
	BYTE writeBuf[4] = { 0x04, 0x20, 0x03, 0x00 };
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return false;
	else
		return true;
}
bool RFSpaceDevice::RequestFirmwareVersion()
{
	firmwareVersion = 0;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x04, 0x00, 0x01 };
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return false;
	return true;
}
bool RFSpaceDevice::RequestBootcodeVersion()
{
	bootcodeVersion = 0;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x04, 0x00, 0x00 };
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return false;
	return true;
}

unsigned RFSpaceDevice::StatusCode()
{
	BYTE writeBuf[4] = { 0x04, 0x20, 0x05, 0x00};
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return -1;
	return 0;
}
//Call may not be working right in SDR-IQ
QString RFSpaceDevice::StatusString(BYTE code)
{
	BYTE writeBuf[5] = { 0x05, 0x20, 0x06, 0x00, code};
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return "";
	return "TBD";
}
double RFSpaceDevice::GetFrequency()
{
	BYTE writeBuf[5] = { 0x05, 0x20, 0x20, 0x00, 0x00};
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return -1;
	return 0;
}
double RFSpaceDevice::SetFrequency(double freq)
{
	if (freq==0)
		return freq; //ignore

	ULONG f = (ULONG)freq;
	BYTE writeBuf[0x0a] = { 0x0a, 0x00, 0x20, 0x00, 0x00,0xff,0xff,0xff,0xff,0x01};
	//Byte order is LSB/MSB
	//Ex: 14010000 -> 0x00D5C690 -> 0x90C6D500
	//We just reverse the byte order by masking and shifting
	writeBuf[5] = (BYTE)((f) & 0xff);
	writeBuf[6] = (BYTE)((f >> 8) & 0xff);
	writeBuf[7] = (BYTE)((f >> 16) & 0xff);
	writeBuf[8] = (BYTE)((f >> 24) & 0xff);;

	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return 0;
	return freq;
}
//Receiver needs to get sample rate early in setup process, before we've actually started SDR
int RFSpaceDevice::GetSampleRate(AD6620::BANDWIDTH bw)
{
	switch (bw)
	{
	case AD6620::BW5K: return 8138;
	case AD6620::BW10K: return 16276;
	case AD6620::BW25K: return 37792;
	case AD6620::BW50K: return 55555;
	case AD6620::BW100K: return 111111;
	case AD6620::BW150K: return 158730;
	case AD6620::BW190K: return 196078;
	}
	return 48000;
}

//OneShot - Requests SDR to send numBlocks of data
//Note: Blocks are fixed size, 8192 bytes
unsigned RFSpaceDevice::CaptureBlocks(unsigned numBlocks)
{
	//C++11 doesn't allow variable in constant initializer, so we set writeBuf[7] separately
	BYTE writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x02, 0x00};
	writeBuf[7] = numBlocks;
	if (!usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf)))
		return -1;

	return 0;
}
//Called from Producer thread
//Returns bytesReturned or -1 if error
int RFSpaceDevice::ReadResponse(int expectedType)
{
	Q_UNUSED(expectedType);

	//mutex.lock();
	//FT_Read blocks until bytes are ready or timeout, which wastes time
	//Check to make sure we've got bytes ready before proceeding
	if (!usbUtil->GetQueueStatus())
		//We get a lot of this initially as there's no data to return until rcv gets set up
		return 0;

	//Note, this assumes that we're in sync with stream
	//Read 16 bit Header which contains message length and type
	if (!usbUtil->Read(readBufProducer,2))
		//mutex.unlock();
		return -1;

	//Get 13bit message length, mask 3 high order bits of 2nd byte
	//Example from problem response rb = 0x01 0x03
	//rb[0] = 0000 0001 rb[1] = 000 0011
	//type is 0 and length is 769
	int type = readBufProducer[1]>>5; //Get 3 high order bits
	int length = readBufProducer[0] + (readBufProducer[1] & 0x1f) * 0x100;

	if (type == 0x04 && length==0) {

		//Special case to allow 2048 samples per block
		//When length==0 it means to read 8192 bytes
		//Largest value in a 13bit field is 8191 and we need length of 8192
		length = 8192;
		char *producerFreeBufPtr;
		if ((producerFreeBufPtr = (char *)producerConsumer.AcquireFreeBuffer()) == NULL)
			return -1;

		if (!usbUtil->Read(producerFreeBufPtr, length)) {
			//Lost data
			producerConsumer.ReleaseFreeBuffer(); //Put back what we acquired
			return -1;
		}

		//Increment the number of data buffers that are filled so consumer thread can access
		producerConsumer.ReleaseFilledBuffer();
		return 0;
	}

	if (length <= 2 || length > 8192)
		//Receive a 2 byte NAK nothing else to read
		return -1;
	else
		//Length can never be zero
		length -= 2; //length includes 2 byte header which we've already read

	//BUG: We always get a type 0 with length 769(-2) early in the startup sequence
	//readBuf is filled with 0x01 0x03 (repeated)
	//FT_Read fails in this case because there are actually no more bytes to read
	//See if we this is ack, nak, or something else that needs special casing
	//Trace back and see what last FT_Write was?
	if (!usbUtil->Read(readBufProducer,length)) {
		//mutex.unlock();
		return -1;
	}

	int itemCode = readBufProducer[0] | readBufProducer[1]<<8;
	//qDebug()<<"Type ="<<type<<" ItemCode = "<<itemCode;
	switch (type)
	{
	//Response to Set or Request Current Control Item
	case 0x00:
		switch (itemCode)
		{
		case 0x0001: //Target Name
			targetName = QString((char*)&readBufProducer[2]);
			break;
		case 0x0002: //Serial Number
			serialNumber = QString((char*)&readBufProducer[2]);
			break;
		case 0x0003: //Interface Version
			interfaceVersion = (unsigned)(readBufProducer[2] + readBufProducer[3] * 256);
			break;
		case 0x0004: //Hardware/Firmware Version
			if (readBufProducer[2] == 0x00)
				bootcodeVersion = (unsigned)(readBufProducer[3] + readBufProducer[4] * 256);
			else if (readBufProducer[2] == 0x01)
				firmwareVersion = (unsigned)(readBufProducer[3] + readBufProducer[4] * 256);
			break;
		case 0x0005: //Status/Error Code
			statusCode = (unsigned)(readBufProducer[2]);
			break;
		case 0x0006: //Status String
			statusString = QString((char*)&readBufProducer[4]);
			break;
		case 0x0018: //Receiver Control
			lastReceiverCommand = readBufProducer[3];
			//More to get if we really need it
			break;
		case 0x0020: //Receiver Frequency
			receiverFrequency = (double) readBufProducer[3] + readBufProducer[4] * 0x100 + readBufProducer[5] * 0x10000 + readBufProducer[6] * 0x1000000;
			break;
		case 0x0038: //RF Gain - values are 0 to -30
			rfGain = (qint8)readBufProducer[3];
			break;
		case 0x0040: //IF Gain - values are all 8bit
			ifGain = (qint8)readBufProducer[3];
			break;
		case 0x00B0: //A/D Input Sample Rate
			break;
		default:
			//bool brk = true;
			break;
		}
		break;
	//Unsolicited Control Item
	case 0x01:
		break;
	//Response to Request Control Item Range
	case 0x02:
		break;
	case 0x03:
		//ACK: ReadBuf[0] has data item being ack'e (0-3)
		break;
	//Target Data Item 0
	case 0x04:
		//Should never get here, data blocks handled in separate thread
		qDebug()<<"Should never get to case 0x04 in ReadResponse()";
		break;
	//Target Data Item 1 (not used in SDR-IQ)
	case 0x05:
		break;
	//Target Data Item 2 (not used in SDR-IQ)
	case 0x06:
		break;
	//Target Data Item 3 (not used in SDR-IQ)
	case 0x07:
		break;
	default:
		break;
	}
	//mutex.unlock();
	return -1; //Fix

}
//This is called from Consumer thread
void RFSpaceDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);

	//BYTE ackBuf[] = {0x03,0x60,0x00}; //Ack data block 0
	//DWORD bytesWritten;
	//FT_STATUS ftStatus;
	float I,Q;
	//Wait for data to be available from producer
	short *consumerFilledBufPtr; //Treat producerConsumer buffer as an array of short
	if ((consumerFilledBufPtr = (short *)producerConsumer.AcquireFilledBuffer()) == NULL)
		return;

	for (int i=0,j=0; i<inBufferSize; i++, j+=2)
	{
		/*
		After a couple of days of banging my head trying to figure why I couldn't get anything but static,
		I figured out how to interpret the data coming back from the SDR-IQ.
		I tried converting LSB/MSB to an integer (0-65535) and normalizing to -32767 to +32767
			I = (SHORT)(readBuf[j] + (readBuf[j + 1] * 0x100));
			I -= 32767.0;
		I tried bit shifting and cast to signed
			I = (short)(readBuf[j] | (readBuf[j + 1] <<8))
		And several other crazy (with hindsight) interpretations
		Finally this worked: csst address in buffer as (short *)
		Update: Reading the AD6620 data sheet, I see that the data is a 16bit integer in 2's compliment form
		Better late than never!
		*/
		//producerBuffer is an array of bytes that we need to treat as array of shorts
		I = consumerFilledBufPtr[j];
		//Convert to float: div by 32767 for -1 to 1,
		I /= (32767.0);
		//SoundCard levels approx 0.02 at 50% mic

		//maxI = I>maxI ? I : maxI;
		//minI = I<minI ? I : minI;


		Q = consumerFilledBufPtr[j+1];
		Q /= (32767.0);

		//IQ appear to be reversed
		inBuffer[i].re =  Q;
		inBuffer[i].im =  I;
		//qDebug() << QString::number(I) << ":" << QString::number(Q);
		//qDebug() << minI << "-" << maxI;

	}

	//Not sure if this is required for SDR-IQ, but it is for SDR-14
	//Send Ack
	//FT_Write(sdr_iq->ftHandle,ackBuf,sizeof(ackBuf),&bytesWritten);

	//We're done with databuf, so we can release before we call ProcessBlock
	//Update lastDataBuf & release dataBuf
	producerConsumer.ReleaseFreeBuffer();

	ProcessIQData(inBuffer,inBufferSize);
}



//Dialog Stuff
void RFSpaceDevice::rfGainChanged(int i)
{
	rfGain = i * -10;
	sRFGain = rfGain;
	SetRFGain(rfGain);
}
void RFSpaceDevice::ifGainChanged(int i)
{
	ifGain = i * 6;
	sIFGain = ifGain;
	SetIFGain(ifGain);
}
void RFSpaceDevice::bandwidthChanged(int i)
{
	switch (i)
	{
	case 0: //5k
		bandwidth = AD6620::BW5K;
		break;
	case 1: //10k
		bandwidth = AD6620::BW10K;
		break;
	case 2: //25k
		bandwidth = AD6620::BW25K;
		break;
	case 3: //50k
		bandwidth = AD6620::BW50K;
		break;
	case 4: //100k
		bandwidth = AD6620::BW100K;
		break;
	case 5: //150k
		bandwidth = AD6620::BW150K;
		break;
	case 6: //190k
		bandwidth = AD6620::BW190K;
		break;
	}
	sBandwidth = bandwidth; //If settings are saved, this is saved
}
