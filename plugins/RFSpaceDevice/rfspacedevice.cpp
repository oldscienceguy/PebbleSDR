#include "rfspacedevice.h"
#include "rfsfilters.h"
#include <QMessageBox>

RFSpaceDevice::RFSpaceDevice():DeviceInterfaceBase()
{
	InitSettings("RFSpace");
	ReadSettings();

	optionUi = NULL;
	inBuffer = NULL;
	readBuf = usbReadBuf = NULL;
	usbUtil = new USBUtil(USBUtil::FTDI_D2XX);
	ad6620 = new AD6620(usbUtil);

	//Todo: SDR-IQ has fixed block size 2048, are we going to support variable size or just force
	inBufferSize = 2048; //settings->framesPerBuffer;
	inBuffer = CPXBuf::malloc(inBufferSize);
	//Max data block we will ever read is dataBlockSize
	readBuf = new unsigned char[dataBlockSize];
	usbReadBuf = new unsigned char[dataBlockSize];
	//Separate buffers for tcp/udp
	udpReadBuf = new unsigned char[dataBlockSize];
	tcpReadBuf = new unsigned char[dataBlockSize];
	tcpSocket = NULL;
	udpSocket = NULL;

	connectionType = CONNECTION_TYPE::TCP;
}

RFSpaceDevice::~RFSpaceDevice()
{
	WriteSettings();
	usbUtil->CloseDevice();

	if (inBuffer != NULL)
		CPXBuf::free(inBuffer);
	if (readBuf != NULL)
		delete [] readBuf;
	if (usbReadBuf != NULL)
		delete [] usbReadBuf;
	if (udpReadBuf != NULL)
		delete [] udpReadBuf;
	if (tcpReadBuf != NULL)
		delete [] tcpReadBuf;
	if (tcpSocket != NULL)
		delete tcpSocket;
	if (udpSocket != NULL)
		delete udpSocket;
}

bool RFSpaceDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	ProcessIQData = _callback;
	framesPerBuffer = _framesPerBuffer;
	connected = false;
	numProducerBuffers = 50;
	producerConsumer.Initialize(std::bind(&RFSpaceDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&RFSpaceDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, dataBlockSize);
	producerConsumer.SetPollingInterval(sampleRate,dataBlockSize);

	readBufferIndex = 0;

	if (connectionType == TCP && tcpSocket == NULL) {
		tcpSocket = new QTcpSocket();

		//We use the signals emitted by QTcpSocket to get new data, instead of polling in a separate thread
		//As a result, there is no producer thread in the TCP case.
		//Set up state change connections
		connect(tcpSocket,&QTcpSocket::connected, this, &RFSpaceDevice::TCPSocketConnected);
		connect(tcpSocket,&QTcpSocket::disconnected, this, &RFSpaceDevice::TCPSocketDisconnected);
		connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(TCPSocketError(QAbstractSocket::SocketError)));
		connect(tcpSocket,&QTcpSocket::readyRead, this, &RFSpaceDevice::TCPSocketNewData);
	}
	if (connectionType == TCP && udpSocket == NULL) {
		udpSocket = new QUdpSocket();

		//Set up UDP Socket to listen for datagrams addressed to whatever IP/Port we set
		//bool result = udpSocket->bind(); //Binds to QHostAddress:Any on port which is randomly chosen
		//Alternative if we need to set a specific HostAddress and port
		//result = udpSocket->bind(QHostAddress(), SomePort);


		//SDR-IP sends UPD datagrams to same port it uses for TCP
		//So this binds our socket to accept datagrams from any IP, as long as port matches
		//We could add an explicit setup where we tell SDR-IP what IP/Port to use for datagrams, but not needed I think
		// ie SetUDPAddressAndPort(QHostAddress("192.168.0.255"),1234);
		bool result = udpSocket->bind(devicePort);
		if (result) {
			connect(udpSocket, &QUdpSocket::readyRead, this, &RFSpaceDevice::UDPSocketNewData);
		} else {
			return false;
		}

	}
	return true;
}

bool RFSpaceDevice::Connect()
{
	if (connectionType == TCP) {
		tcpSocket->connectToHost(deviceAddress,devicePort,QTcpSocket::ReadWrite);
		if (!tcpSocket->waitForConnected(1000)) {
			//Socket was closed or error
			qDebug()<<"Server error "<<tcpSocket->errorString();
			return false;
		}
		qDebug()<<"Server connected";
		connected = true;

		return true;
	} else if (connectionType == USB) {
		if (!usbUtil->IsUSBLoaded()) {
			if (!usbUtil->LoadUsb()) {
				QMessageBox::information(NULL,"Pebble","USB not loaded.  Elektor communication is disabled.");
				return false;
			}
		}

		if (!usbUtil->FindDevice("SDR-IQ",false))
			return false;

		//Open device
		if (!usbUtil->OpenDevice())
			return false; // FT_Open failed

		//Any errors after here need to call CloseDevice since we succeeded in opening
		if (!usbUtil->ResetDevice()) {
			usbUtil->CloseDevice();
			return false;
		}

		//Make sure driver buffers are empty
		if (!usbUtil->Purge()) {
			usbUtil->CloseDevice();
			return false;
		}

		if (!usbUtil->SetTimeouts(500,500)) {
			usbUtil->CloseDevice();
			return false;
		}

		//Testing: Increase size of internal buffers from default 4096
		//ftStatus = FT_SetUSBParameters(ftHandle,8192,8192);


		//SDR sends a 0 on startup, clear it out of buffer
		//Ignore error if we don't get 1 byte
		usbUtil->Read(readBuf,1);

		connected = true;
		return true;

	}
	return false; //Invalid connection type
}

bool RFSpaceDevice::Disconnect()
{
	if (connectionType == TCP) {
		tcpSocket->disconnectFromHost();
		connected = false;
		return true;
	} else if (connectionType == USB) {
		usbUtil->CloseDevice();
		connected = false;
		return true;
	}
	return false;
}


void RFSpaceDevice::Start()
{
	if (connectionType == TCP) {
		producerConsumer.Start(false,true);
	} else {
		producerConsumer.Start(true,true);
		//We're going to get back over 256 ACKs, eat em before continuing
		//Otherwise we fill up data buffers while processing acks
		FlushDataBlocks();
	}

	//Get basic Device Information for later use, name, version, etc
	bool result = RequestTargetName();
	result = RequestTargetSerialNumber();
	result = RequestFirmwareVersion();
	result = RequestBootcodeVersion();
	result = RequestInterfaceVersion();

	//From SDR-IP doc.  1-5 can be in any order
	//1-Set DDC Output sample rate to 100000. ( do not change after start command issued) [09] [00] [B8] [00] [00] [A0] [86] [01] [00]
	//2-Set the RF Filter mode to automatic. [06] [00] [44] [00] [00] [00]
	//3-Set the A/D mode to Dither and Gain of 1.5. [06] [00] [8A] [00] [00] [03]
	//4-Set the NCO frequency to 20.0MHz [0A] [00] [20] [00] [00] [00] [2D] [31] [01] [00]
	//5-Set the SDR-IP LCD frequency display to match NCO frequency.(optional command) [0A] [00] [20] [00] [01] [00] [2D] [31] [01] [00]
	//6-Send the Start Capture command, Complex I/Q data, 24 bit contiguous mode [08] [00] [18] [00] [81] [02] [80] [00]

	SetSampleRate();

	//Set IF Gain, 0,+6, +12,+18,+24
	SetIFGain(sIFGain);

	//RFGain MUST be set to something when we power up or we get no data in buffer
	//RFGain is actually an attenuator
	SetRFGain(sRFGain);

	//Finally ready to start getting data samples
	StartCapture();
	running = true;
}

void RFSpaceDevice::Stop()
{
	if (!running)
		return;
	StopCapture();
	producerConsumer.Stop();
	FlushDataBlocks();
	running = false;
}

void RFSpaceDevice::ReadSettings()
{
	lowFrequency = 150000;
	highFrequency = 33000000;
	iqGain = 0.5;
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
	sRFGain = qSettings->value("RFGain",0).toInt();
	sIFGain = qSettings->value("IFGain",18).toInt();
	deviceAddress = QHostAddress(qSettings->value("DeviceAddress","10.0.1.100").toString());
	devicePort = qSettings->value("DevicePort",50000).toInt();

	//Instead of getting BW from SDR_IQ options dialog, we now get it directly from settings dialog
	//because we're passing GetSampleRates() during dialog setup
	//So sampleRate is actually bandwidth
	if (sampleRate==196078)
		sBandwidth = AD6620::BW190K;
	else if (sampleRate==158730)
		sBandwidth = AD6620::BW150K;
	else if (sampleRate==111111)
		sBandwidth = AD6620::BW100K;
	else if (sampleRate==55555)
		sBandwidth = AD6620::BW50K;
	else
		sBandwidth = AD6620::BW50K;
}

void RFSpaceDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
	qSettings->setValue("RFGain",sRFGain);
	qSettings->setValue("IFGain",sIFGain);
	qSettings->setValue("DeviceAddress",deviceAddress.toString());
	qSettings->setValue("DevicePort",devicePort);

	qSettings->sync();
}

QVariant RFSpaceDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "RFSpace Family";
			break;
		case PluginDescription:
			return "RFSpace SDR-IQ and SDR-IP";
		case PluginNumDevices:
			return 2;
		case DeviceName:
			switch (_option) {
				case 0:
					return "SDR-IQ";
				case 1:
					return "SDR-IP";
				default:
					return "Unknown";
			}

			if (connectionType == TCP)
				return "SDR-IP";
			else
				return "SDR-IQ";
		case DeviceSampleRates:
			if (connectionType == TCP)
				return QStringList()<<"62500"<<"250000"<<"500000"<<"2000000";
			else if (connectionType == USB)
				return QStringList()<<"55555"<<"111111"<<"158730"<<"196078";
			else
				return QStringList();
		case DeviceType:
			return DeviceInterfaceBase::INTERNAL_IQ;
		case DeviceSampleRate: {
			if (connectionType == TCP) {
				return sampleRate;
			} else if (connectionType == USB) {
				//Sample rate is derived from bandwidth
				switch (sBandwidth)
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
			} else
				return 0;
		}
		case DeviceFrequency:
			return GetFrequency();
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool RFSpaceDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case DeviceFrequency:
			return SetFrequency(_value.toDouble());
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void RFSpaceDevice::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
	if (optionUi != NULL)
		delete optionUi;
	optionUi = new Ui::SdrIqOptions;
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

	}
}

void RFSpaceDevice::processControlItem(quint16 headerType, char *buf)
{
	int itemCode = buf[0] | buf[1]<<8;

	switch (headerType)
	{
	//Response to Set or Request Current Control Item
	case TargetHeaderTypes::ResponseControlItem:
		switch (itemCode)
		{
		case 0x0001: //Target Name
			targetName = QString((char*)&buf[2]);
			break;
		case 0x0002: //Serial Number
			serialNumber = QString((char*)&buf[2]);
			break;
		case 0x0003: //Interface Version
			interfaceVersion = (unsigned)(buf[2] + buf[3] * 256);
			break;
		case 0x0004: //Hardware/Firmware Version
			if (buf[2] == 0x00)
				bootcodeVersion = (unsigned)(buf[3] + buf[4] * 256);
			else if (buf[2] == 0x01)
				firmwareVersion = (unsigned)(buf[3] + buf[4] * 256);
			break;
		case 0x0005: //Status/Error Code
			statusCode = (unsigned)(buf[2]);
			break;
		case 0x0006: //Status String
			statusString = QString((char*)&buf[4]);
			break;
		case 0x0018: //Receiver Control
			lastReceiverCommand = buf[3];
			//More to get if we really need it
			break;
		case 0x0020: //Receiver Frequency
			receiverFrequency = (double) buf[3] + buf[4] * 0x100 + buf[5] * 0x10000 + buf[6] * 0x1000000;
			break;
		case 0x0038: //RF Gain - values are 0 to -30
			rfGain = (qint8)buf[3];
			break;
		case 0x0040: //IF Gain - values are all 8bit
			ifGain = (qint8)buf[3];
			break;
		case 0x00B0: //A/D Input Sample Rate
			break;
		default:
			//bool brk = true;
			break;
		}
		break;
	//Unsolicited Control Item
	case TargetHeaderTypes::UnsolicitedControlItem:
		break;
	//Response to Request Control Item Range
	case TargetHeaderTypes::ResponseControlItemRange:
		break;
	case TargetHeaderTypes::DataItemACKTargetToHost:
		//ACK: ReadBuf[0] has data item being ack'e (0-3)
		break;
	case TargetDataItem0:
		//Should never get here, data blocks handled elsewhere
		Q_UNREACHABLE();
		break;
	case TargetDataItem1:
		Q_UNREACHABLE();
		break;
	case TargetDataItem2:
		Q_UNREACHABLE();
		break;
	case TargetDataItem3:
		Q_UNREACHABLE();
		break;
	default:
		break;
	}

}

void RFSpaceDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
	//mutex.lock();
	//FT_Read blocks until bytes are ready or timeout, which wastes time
	//Check to make sure we've got bytes ready before proceeding
	if (!usbUtil->GetQueueStatus())
		//We get a lot of this initially as there's no data to return until rcv gets set up
		return;

	//Note, this assumes that we're in sync with stream
	//Read 16 bit Header which contains message length and type
	if (!usbUtil->Read(usbReadBuf,2))
		//mutex.unlock();
		return;

	ControlHeader header;
	UnpackHeader(usbReadBuf[0], usbReadBuf[1], &header);

	if (header.type == TargetHeaderTypes::TargetDataItem0 && header.length==0) {

		//Special case to allow 2048 samples per block
		//When length==0 it means to read 8192 bytes
		//Largest value in a 13bit field is 8191 and we need length of 8192
		header.length = 8192;
		char *producerFreeBufPtr;
		if ((producerFreeBufPtr = (char *)producerConsumer.AcquireFreeBuffer()) == NULL)
			return;

		if (!usbUtil->Read(producerFreeBufPtr, header.length)) {
			//Lost data
			producerConsumer.ReleaseFreeBuffer(); //Put back what we acquired
			return;
		}

		//Increment the number of data buffers that are filled so consumer thread can access
		producerConsumer.ReleaseFilledBuffer();
		return;
	}

	if (header.length <= 2 || header.length > 8192)
		//Receive a 2 byte NAK nothing else to read
		return;
	else
		//Length can never be zero
		header.length -= 2; //length includes 2 byte header which we've already read

	//BUG: We always get a type 0 with length 769(-2) early in the startup sequence
	//readBuf is filled with 0x01 0x03 (repeated)
	//FT_Read fails in this case because there are actually no more bytes to read
	//See if we this is ack, nak, or something else that needs special casing
	//Trace back and see what last FT_Write was?
	if (!usbUtil->Read(usbReadBuf,header.length)) {
		//mutex.unlock();
		return;
	}

	//qDebug()<<"Type ="<<type<<" ItemCode = "<<itemCode;
	processControlItem(header.type, (char *)usbReadBuf);

	//mutex.unlock();
	return;


}

//This is called from Consumer thread
void RFSpaceDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);

	//unsigned char ackBuf[] = {0x03,0x60,0x00}; //Ack data block 0
	//DWORD bytesWritten;
	//FT_STATUS ftStatus;
	float I,Q;
	//Wait for data to be available from producer
	short *consumerFilledBufPtr; //Treat producerConsumer buffer as an array of short
	while (producerConsumer.GetNumFilledBufs() > 0) {

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

		ProcessIQData(inBuffer,inBufferSize);
		//Update lastDataBuf & release dataBuf
		producerConsumer.ReleaseFreeBuffer();

	}
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

void RFSpaceDevice::TCPSocketError(QAbstractSocket::SocketError socketError)
{
	Q_UNUSED(socketError);
	qDebug()<<"Socket Error";
}

void RFSpaceDevice::TCPSocketConnected()
{
	qDebug()<<"Connected";
}

void RFSpaceDevice::TCPSocketDisconnected()
{
	qDebug()<<"Disconnected";
}

//TCP is all command and control responses
void RFSpaceDevice::TCPSocketNewData()
{
	qint64 bytesAvailable = tcpSocket->bytesAvailable();

	qint64 bytesRead = tcpSocket->read((char*)tcpReadBuf,bytesAvailable);
	if (bytesRead != bytesAvailable) {
		return;
	}

	//Assume we always get complete responses, ie responses don't cross NewData signals
	ControlHeader header;
	int index = 0;
	while ( index < bytesRead ) {
		UnpackHeader(tcpReadBuf[index], tcpReadBuf[index+1], &header);
		processControlItem(header.type,(char*)&tcpReadBuf[index + 2]);
		index += header.length;
	}
}

//UDP is all IQ data
void RFSpaceDevice::UDPSocketNewData()
{
	QHostAddress sender;
	quint16 senderPort;
	qint16 actual;
	qint64 dataGramSize = udpSocket->pendingDatagramSize();
	if (dataGramSize != 1028) {
		qDebug()<<"Invalid IQ datagram size: "<< dataGramSize;
		return;
	}
	actual = udpSocket->readDatagram((char*)udpReadBuf,dataGramSize, &sender, &senderPort);
	if (actual != dataGramSize) {
		qDebug()<<"ReadDatagram size error";
		return;
	}
	//Double check to make sure datagram is right format
	if (udpReadBuf[0] != TargetDataItem0 ||
			udpReadBuf[1] != 0x84) {
		return; //Invalid
	}
	//Ignore sequence nubmer for now and assume we get in correct order
	//Eventually we can get fancy and buffer datagrams that arrive out of sequence and re-assemble in right order
	quint16 sequenceNumer = udpReadBuf[2];
	Q_UNUSED(sequenceNumer);
	//1024 data bytes follow or 512 2 byte samples

	if (readBufferIndex == 0) {
		//Starting a new producer buffer
		if ((producerFreeBufPtr = (char *)producerConsumer.AcquireFreeBuffer()) == NULL)
			return;
	}

	//USB gets 2048 I and 2048 Q samples at a time, or 8192 bytes
	//We need to build over 8 datagrams
	memcpy(&producerFreeBufPtr[readBufferIndex], &udpReadBuf[4], 1024); //256 I/Q samples
	readBufferIndex += 1024;

	if (readBufferIndex == 8192) {
		readBufferIndex = 0;
		//Increment the number of data buffers that are filled so consumer thread can access
		producerConsumer.ReleaseFilledBuffer();
	}

	return;

}

bool RFSpaceDevice::SendTcpCommand(void *buf, qint64 len)
{
	qint64 actual = tcpSocket->write((char *)buf, len);
	return actual == len;

}

//Device specific
bool RFSpaceDevice::SetUDPAddressAndPort(QHostAddress address, quint16 port) {
	//To set the UDP IP address to 192.168.3.123 and port to 12345: [0A][00] [C5][00] [7B][03][A8]C0] [39][30]
	if (connectionType != TCP)
		return false;
	unsigned char writeBuf[] {0x0a, 0x00, 0xc5, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	//192.168.0.1 is stored as 1, 0, 168, 192 in buffer
	*(quint32 *)&writeBuf[4] = address.toIPv4Address();
	*(quint16 *)&writeBuf[8] = port;
	return SendTcpCommand(writeBuf,sizeof(writeBuf));
}

bool RFSpaceDevice::SetSampleRate()
{
	if (connectionType == TCP) {
		//100k sample rate example: [09] [00] [B8] [00] [00] [A0] [86] [01] [00]
		unsigned char writeBuf[] = {0x09, 0x00, 0xb8, 0x00, 0x00, 0xa0, 0x86, 0x01, 0x00};
		//Verified that this cast to quint32 puts things in right order by setting sampleRate = 100000 and comparing
		*(quint32 *)&writeBuf[5] = sampleRate;
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		//Set bandwidth first, takes a while and returns lots of garbage ACKs
		ad6620->SetBandwidth(sBandwidth);
		return true;
	}
	return false;
}


//0,-10,-20,-30
bool RFSpaceDevice::SetRFGain(qint8 gain)
{
	unsigned char writeBuf[6] = { 0x06, 0x00, 0x38, 0x00, 0xff, 0xff};
	writeBuf[4] = 0x00;
	writeBuf[5] = gain ;
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
//This is not documented in Interface spec, but used in activeX examples
bool RFSpaceDevice::SetIFGain(qint8 gain)
{
	unsigned char writeBuf[6] = { 0x06, 0x00, 0x40, 0x00, 0xff, 0xff};
	//Bits 7,6,5 are Factory test bits and are masked
	writeBuf[4] = 0; //gain & 0xE0;
	writeBuf[5] = gain;
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}

#if 0
SDR-IP This is the main “Start/Stop” command to start or stop data capture by the SDR-IP. Several other control items need to be set first before starting the capture process such as output sample rate, packet size, etc. See the “Examples” section for typical start-up sequences.
	1: The first parameter is a 1 byte data channel/type specifier:
		Bit 7 == 1 specifies complex base band data 0 == real A/D samples
		The remaining 7 bits are for future expansion and should be ignored or set to zero.
		0xxx xxxx = real A/D sample data mode
		1xxx xxxx = complex I/Q base band data mode

	2:The second parameter is a 1 byte run/stop control byte defined as:
		0x01 = Idle(Stop) stops the UDP port and data capture
		0x02 = Run starts the UDP port SDR-IP capturing data
	3: The third parameter is a 1 byte parameter specifying the capture mode.
		Bit 7 == 1 specifies 24 bit data 0 == specifies 16 bit data
		Bit [1:0] Specify the way in which the SDR-IP captures data
		Bit [1:0] == 00 -Contiguously sends data as long as the SDR-IP is running.
		Bit [1:0] == 01 -FIFO mode captures data into FIFO then sends data to the host then repeats. Bit [1:0] == 11 -Hardware triggered mode where start and stop is controlled by HW trigger input
		The following modes are currently defined:
		0x00 = 16 bit Contiguous mode where the data is contiguously sent back to the Host.
		0x80 = 24 bit Contiguous mode where the data is contiguously sent back to the Host.
		0x01 = 16 bit FIFO mode where N samples of data is captured in a FIFO then sent back to the Host. 0x83 = 24 bit Hardware Triggered Pulse mode.(start/stop controlled by HW trigger input)
		0x03 = 16 bit Hardware Triggered Pulse mode.(start/stop controlled by HW trigger input)

	4: The fourth parameter is a 1 byte parameter N specifying the number of 4096 16 bit data samples to capture in the FIFO mode.

USB
		0x08	# bytes in message
		0x00	message type
		0x18	Receiver State Control item
		0x00	Receiver State Control item (Bit 7 specifies complex (1) or Real (0))
		0x81	24 bit data,
		0x02	Run command: 0x02=Start , 0x01 to Stop
		0x00	Continuous samples mode
		0x00	Ignored

TCP
		0x08	# bytes in message
		0x00	message type
		0x18	Receiver State Control item
		0x00
		0x80	1: Complex I/Q data
		0x02	2:Run command: 0x02=Start , 0x01 to Stop
		0x00	3:Capture Mode: 00= 16 bit Continuous
		0x00	4:Num samples in FIFO data buffer



#endif
bool RFSpaceDevice::StartCapture()
{
	if (connectionType == TCP) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x80, 0x02, 0x00, 0x00};
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x00, 0x00};
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}

bool RFSpaceDevice::StopCapture()
{
	if (connectionType == TCP) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x00};
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x01, 0x00, 0x00};
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
void RFSpaceDevice::FlushDataBlocks()
{
	bool result;
	if (connectionType == TCP) {
		return;
	} else if (connectionType == USB) {
		do
			result = usbUtil->Read(readBuf, 1);
		while (result);
	}
}
//Control Item Code 0x0001
bool RFSpaceDevice::RequestTargetName()
{
	targetName = "Pending";
	//0x04, 0x20 is the request command
	//0x01, 0x00 is the Control Item Code (command)
	unsigned char writeBuf[4] = { 0x04, 0x20, 0x01, 0x00 };
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
//Same pattern as TargetName
bool RFSpaceDevice::RequestTargetSerialNumber()
{
	serialNumber = "Pending";
	unsigned char writeBuf[4] = { 0x04, 0x20, 0x02, 0x00 };
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
bool RFSpaceDevice::RequestInterfaceVersion()
{
	interfaceVersion = 0;
	unsigned char writeBuf[4] = { 0x04, 0x20, 0x03, 0x00 };
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
bool RFSpaceDevice::RequestFirmwareVersion()
{
	firmwareVersion = 0;
	unsigned char writeBuf[5] = { 0x05, 0x20, 0x04, 0x00, 0x01 };
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
bool RFSpaceDevice::RequestBootcodeVersion()
{
	bootcodeVersion = 0;
	unsigned char writeBuf[5] = { 0x05, 0x20, 0x04, 0x00, 0x00 };
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}

bool RFSpaceDevice::RequestStatusCode()
{
	unsigned char writeBuf[4] = { 0x04, 0x20, 0x05, 0x00};
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
//Call may not be working right in SDR-IQ
bool RFSpaceDevice::RequestStatusString(unsigned char code)
{
	unsigned char writeBuf[5] = { 0x05, 0x20, 0x06, 0x00, code};
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
bool RFSpaceDevice::GetFrequency()
{
	unsigned char writeBuf[5] = { 0x05, 0x20, 0x20, 0x00, 0x00};
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}
bool RFSpaceDevice::SetFrequency(double freq)
{
	if (freq==0)
		return freq; //ignore

	unsigned char writeBuf[0x0a] = { 0x0a, 0x00, 0x20, 0x00, 0x00,0xff,0xff,0xff,0xff,0x01};
	DoubleToBuf(&writeBuf[5],freq);
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}

//OneShot - Requests SDR to send numBlocks of data
//Note: Blocks are fixed size, 8192 bytes
bool RFSpaceDevice::CaptureBlocks(unsigned numBlocks)
{
	//C++11 doesn't allow variable in constant initializer, so we set writeBuf[7] separately
	unsigned char writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x02, 0x00};
	writeBuf[7] = numBlocks;
	if (connectionType == TCP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (connectionType == USB) {
		return usbUtil->Write((LPVOID)writeBuf,sizeof(writeBuf));
	}
	return false;
}

//Static utility functions
//Convert double to byte buffer, usually called with buf[5] before sending commands
//Byte order is LSB/MSB
//Ex: 14010000 -> 0x00D5C690 -> 0x90C6D500
void RFSpaceDevice::DoubleToBuf(unsigned char *buf, double value)
{
	//So we can bit-shift
	ULONG v = (ULONG)value;

	//Byte order is LSB/MSB
	//Ex: 14010000 -> 0x00D5C690 -> 0x90C6D500
	//We just reverse the byte order by masking and shifting
	buf[0] = (unsigned char)((v) & 0xff);
	buf[1] = (unsigned char)((v >> 8) & 0xff);
	buf[2] = (unsigned char)((v >> 16) & 0xff);
	buf[3] = (unsigned char)((v >> 24) & 0xff);;

}

//byte0				byte1
//8 bit Length lsb ,3 bit type , 5 bit Length msb
void RFSpaceDevice::UnpackHeader(quint8 byte0, quint8 byte1, ControlHeader *unpacked)
{
	//Get 13bit message length, mask 3 high order bits of 2nd byte
	//Example from problem response rb = 0x01 0x03
	//rb[0] = 0000 0001 rb[1] = 000 0011
	//type is 0 and length is 769
	unpacked->type = byte1>>5; //Get 3 high order bits
	unpacked->length = byte0 + (byte1 & 0x1f) * 0x100;
}

