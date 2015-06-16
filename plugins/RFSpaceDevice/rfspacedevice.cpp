#include "rfspacedevice.h"
#include "rfsfilters.h"
#include <QMessageBox>

//Mac native socket API, needed for UDP buffer
#ifndef Q_OS_WIN
#include <sys/socket.h>
#endif

RFSpaceDevice::RFSpaceDevice():DeviceInterfaceBase()
{
	InitSettings("");
	ReadSettings();

	optionUi = NULL;
	inBuffer = NULL;
	readBuf = usbReadBuf = NULL;
	usbUtil = NULL;
	ad6620 = NULL;
	afedri = NULL;
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
	deviceDiscoveredAddress.clear();
	deviceDiscoveredPort = 0;
}

RFSpaceDevice::~RFSpaceDevice()
{
	WriteSettings();
	if (usbUtil != NULL)
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
	if (afedri != NULL)
		delete afedri;
}

void RFSpaceDevice::InitSettings(QString fname)
{
	Q_UNUSED(fname);

	DeviceInterfaceBase::InitSettings("SDR_IQ");
	sdriqSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SDR_IP");
	sdripSettings = qSettings;
	DeviceInterfaceBase::InitSettings("AFEDRI_USB");
	afedri_usbSettings = qSettings;
	qSettings = NULL; //Catch errors
}

bool RFSpaceDevice::Initialize(cbProcessIQData _callback,
							   cbProcessBandscopeData _callbackBandscope,
							   cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	numProducerBuffers = 50;
	producerConsumer.Initialize(std::bind(&RFSpaceDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&RFSpaceDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, framesPerBuffer * sizeof(CPX));

	usbUtil = new USBUtil(USBUtil::FTDI_D2XX);
	ad6620 = new AD6620(usbUtil);
	afedri = new AFEDRI();



	if (deviceNumber == SDR_IQ) {
		//SR * 2 bytes for I * 2 bytes for Q .  dataBlockSize is 8192
		producerConsumer.SetProducerInterval(sampleRate,framesPerBuffer);
		producerConsumer.SetConsumerInterval(sampleRate,framesPerBuffer);
	} else if(deviceNumber == SDR_IP) {
		//Get get UDP datagrams of 1024 bytes, 4bytes per CPX or 256 CPX samples
		producerConsumer.SetProducerInterval(sampleRate,udpBlockSize / 4);
		//Consumer only has to run once every 2048 CPX samples
		producerConsumer.SetConsumerInterval(sampleRate,framesPerBuffer);
	} else if (deviceNumber == AFEDRI_USB) {
		DeviceInterfaceBase::Initialize(_callback, NULL, NULL, _framesPerBuffer); //Handle audio input
		afedri->Initialize(); //HID
	}

	readBufferIndex = 0;

	if (deviceNumber == SDR_IP && tcpSocket == NULL) {
		tcpSocket = new QTcpSocket();

		//We use the signals emitted by QTcpSocket to get new data, instead of polling in a separate thread
		//As a result, there is no producer thread in the TCP case.
		//Set up state change connections
		connect(tcpSocket,&QTcpSocket::connected, this, &RFSpaceDevice::TCPSocketConnected);
		connect(tcpSocket,&QTcpSocket::disconnected, this, &RFSpaceDevice::TCPSocketDisconnected);
		connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(TCPSocketError(QAbstractSocket::SocketError)));
		connect(tcpSocket,&QTcpSocket::readyRead, this, &RFSpaceDevice::TCPSocketNewData);

	}
	if (deviceNumber == SDR_IP && autoDiscover) {
		if (!SendUDPDiscovery()) {
			QMessageBox::information(NULL,"RFSpace Discovery",
				"No SDR-IP compatible device found.\nPlease try again or set a fixed IP/Port");
			return false;
		} else {
			//Got something
			deviceAddress = deviceDiscoveredAddress;
			devicePort = deviceDiscoveredPort;
			WriteSettings(); //Save
			return true;
		}

		//Device address/port from settings will be used
		return true;
	}
	return true;
}

bool RFSpaceDevice::Connect()
{
	if (deviceNumber == SDR_IP) {
		tcpSocket->connectToHost(deviceAddress,devicePort,QTcpSocket::ReadWrite);
		if (!tcpSocket->waitForConnected(1000)) {
			//Socket was closed or error
			qDebug()<<"Server error "<<tcpSocket->errorString();
			return false;
		}
		qDebug()<<"Server connected";
		connected = true;

		return true;
	} else if (deviceNumber == SDR_IQ) {
		header.gotHeader = false;

		if (!usbUtil->IsUSBLoaded()) {
			if (!usbUtil->LoadUsb()) {
				QMessageBox::information(NULL,"Pebble","USB not loaded.  Device communication is disabled.");
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

		//This will cause Read or Write to return after 500ms with whatever data was read or written
		//independent of requested size
		//It can really slow things down if data too high and data is not available
		if (!usbUtil->SetTimeouts(100,100)) {
			usbUtil->CloseDevice();
			return false;
		}

		//Increase size of internal buffers from default 4096 reduces CPU load
		if (!usbUtil->SetUSBParameters(8192,4096)) {
			usbUtil->CloseDevice();
			return false;
		}

		connected = true;
		return true;

	} else if (deviceNumber == AFEDRI_USB) {
		if (afedri->Connect()) {
			connected = true;
			return true;
		} else {
			return false;
		}
	}
	return false; //Invalid connection type
}

bool RFSpaceDevice::Disconnect()
{
	if (!connected)
		return false;

	if (deviceNumber == SDR_IP) {
		tcpSocket->disconnectFromHost();
		connected = false;
		return true;
	} else if (deviceNumber == SDR_IQ) {
		usbUtil->CloseDevice();
		connected = false;
		return true;
	} else if (deviceNumber == AFEDRI_USB) {
		return afedri->Disconnect();
	}
	return false;
}


void RFSpaceDevice::Start()
{
	if (deviceNumber == SDR_IP) {
		//Fastest is no threads, just signals from NewUDPData to ConsumerWorker via ProducerConsumer
		producerConsumer.Start(true,true);
	} else if (deviceNumber == SDR_IQ){
		producerConsumer.Start(true,true);
	} else if (deviceNumber == AFEDRI_USB) {
		DeviceInterfaceBase::Start(); //Audio Input
	} else {
		return;
	}
	SetSampleRate();
	SetADSampleRate();

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
	if (deviceNumber == SDR_IP || deviceNumber == SDR_IQ) {
		StopCapture();
		producerConsumer.Stop();
	} else if (deviceNumber == AFEDRI_USB) {
		DeviceInterfaceBase::Stop(); //Audio input
	}
	running = false;
}

void RFSpaceDevice::ReadSettings()
{
	if (deviceNumber == SDR_IQ)
		qSettings = sdriqSettings;
	else if (deviceNumber == SDR_IP)
		qSettings = sdripSettings;
	else if (deviceNumber == AFEDRI_USB)
		qSettings = afedri_usbSettings;
	else
		return;

	lowFrequency = 150000;
	highFrequency = 33000000;
	iqGain = 0.5;
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
	sRFGain = qSettings->value("RFGain",0).toInt();
	sIFGain = qSettings->value("IFGain",18).toInt();
	deviceAddress = QHostAddress(qSettings->value("DeviceAddress","10.0.1.100").toString());
	devicePort = qSettings->value("DevicePort",50000).toInt();
	autoDiscover = qSettings->value("AutoDiscover",true).toBool();

	//Instead of getting BW from SDR_IQ options dialog, we now get it directly from settings dialog
	//because we're passing GetSampleRates() during dialog setup
	//So sampleRate is actually bandwidth
	if (sampleRate==190000)
		sBandwidth = AD6620::BW190K;
	else if (sampleRate==150000)
		sBandwidth = AD6620::BW150K;
	else if (sampleRate==100000)
		sBandwidth = AD6620::BW100K;
	else if (sampleRate==50000)
		sBandwidth = AD6620::BW50K;
	else
		sBandwidth = AD6620::BW50K;
}

void RFSpaceDevice::WriteSettings()
{
	if (deviceNumber == SDR_IQ)
		qSettings = sdriqSettings;
	else if (deviceNumber == SDR_IP)
		qSettings = sdripSettings;
	else if (deviceNumber == AFEDRI_USB)
		qSettings = afedri_usbSettings;
	else
		return;

	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
	qSettings->setValue("RFGain",sRFGain);
	qSettings->setValue("IFGain",sIFGain);
	qSettings->setValue("DeviceAddress",deviceAddress.toString());
	qSettings->setValue("DevicePort",devicePort);
	qSettings->setValue("AutoDiscover",autoDiscover);

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
			return "SDR-IQ, SDR-IP, AFEDRI_USB";
		case PluginNumDevices:
			return 3;
		case DeviceName:
			switch (_option) {
				case SDR_IQ:
					return "SDR-IQ";
				case SDR_IP:
					return "SDR-IP";
				case AFEDRI_USB:
					return "AFEDRI_USB";
				default:
					return "Unknown";
			}
		case DeviceDescription:
			switch (deviceNumber) {
				case SDR_IQ:
					return "SDR-IQ";
				case SDR_IP:
					return "SDR-IP";
				case AFEDRI_USB:
					return "AFEDRI_USB";
				default:
					return "Unknown";
			}
		case DeviceSampleRates:
			if (deviceNumber == SDR_IP)
				return QStringList()<<"62500"<<"250000"<<"500000"<<"2000000";
			else if (deviceNumber == SDR_IQ)
				//return QStringList()<<"55555"<<"111111"<<"158730"<<"196078";
				return QStringList()<<"50000"<<"100000"<<"150000"<<"190000";
			else if (deviceNumber == AFEDRI_USB)
				return QStringList()<<"50000"<<"100000"<<"150000"<<"190000"<<"250000";
			else
				return QStringList();
		case DeviceType:
			if (deviceNumber == SDR_IP || deviceNumber == SDR_IQ)
				return DeviceInterfaceBase::IQ_DEVICE;
			else if(deviceNumber == AFEDRI_USB)
				return DeviceInterfaceBase::AUDIO_IQ_DEVICE;
			else
				return DeviceInterfaceBase::AUDIO_IQ_DEVICE;
		case DeviceSampleRate: {
			return sampleRate;
			//Note, actual sample rate for SDR_IQ is tied to bandwidth
#if 0
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
#endif
		}
		case DeviceFrequency:
			return GetFrequency();
		case DeviceHealthValue:
			return producerConsumer.GetPercentageFree();
		case DeviceHealthString:
			if (producerConsumer.GetPercentageFree() > 50)
				return "Device running normally";
			else
				return "Device under stress";

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
	if (optionUi != NULL) {
		//This will delete everything from the previous option page, then we re-create
		foreach (QObject *o,parent->children()) {
			delete o;
		}
		delete optionUi;
	}
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

	if (deviceNumber == SDR_IQ || deviceNumber == AFEDRI_USB) {
		optionUi->ipEdit->setVisible(false);
		optionUi->portEdit->setVisible(false);
		optionUi->discoverBox->setVisible(false);
		optionUi->ipDiscovered->setVisible(false);
	} else if (deviceNumber == SDR_IP) {
		optionUi->ipEdit->setVisible(true);
		optionUi->portEdit->setVisible(true);
		optionUi->discoverBox->setVisible(true);
		optionUi->ipDiscovered->setVisible(true);
		if (autoDiscover) {
			optionUi->discoverBox->setChecked(true);
			optionUi->ipEdit->setEnabled(false);
			optionUi->portEdit->setEnabled(false);
		}
		optionUi->ipEdit->setText(deviceAddress.toString());
		optionUi->portEdit->setText(QString::number(devicePort));
		connect(optionUi->ipEdit,&QLineEdit::editingFinished,this,&RFSpaceDevice::IPAddressChanged);
		connect(optionUi->portEdit,&QLineEdit::editingFinished,this,&RFSpaceDevice::IPPortChanged);
		connect(optionUi->discoverBox,SIGNAL(clicked(bool)), this, SLOT(discoverBoxChanged(bool)));
	}

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

void RFSpaceDevice::processControlItem(ControlHeader header, unsigned char *buf)
{
	switch (header.type)
	{
	//Response to Set or Request Current Control Item
	case TargetHeaderTypes::ResponseControlItem: {
		//qDebug()<<"Control - "<<buf[0]<<":"<<buf[1];
		quint16 itemCode = buf[0] | buf[1]<<8;
		switch (itemCode)
		{
		case 0x0001: //Target Name
			targetName = QString((char*)&buf[2]);
			break;
		case 0x0002: //Serial Number
			serialNumber = QString((char*)&buf[2]);
			break;
		case 0x0003: //Interface Version
			interfaceVersion = *(quint16 *)&buf[2];
			break;
		case 0x0004: //Hardware/Firmware Version
			if (buf[2] == 0x00)
				bootcodeVersion = *(quint16 *)&buf[3];
			else if (buf[2] == 0x01)
				firmwareVersion = *(quint16 *)&buf[3];
			else if (buf[2] == 0x02)
				hardwareVersion = *(quint16 *)&buf[3];
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
			qDebug()<<"A/D Sample Rate: "<<*(quint32 *)&buf[3];
			break;
		case 0x00b8: //IQ Sample output rate
			qDebug()<<"Output Sample Rate: "<<*(quint32 *)&buf[3];
			break;
		default:
			//bool brk = true;
			break;
		}
		break;
	}
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
		break;
	case TargetDataItem1:
		break;
	case TargetDataItem2:
		break;
	case TargetDataItem3:
		break;
	default:
		break;
	}

}

void RFSpaceDevice::producerWorker(cbProducerConsumerEvents _event)
{
	switch (_event) {
		//Thread local constuction.  All tcp and udp port access must be kept within thread
		case cbProducerConsumerEvents::Start: {
			if (deviceNumber == SDR_IQ || deviceNumber == AFEDRI_USB)
				return;
			//Construct UDP sockets in thread local
			if (udpSocket == NULL) {
				udpSocket = new QUdpSocket();
			}
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
				//Note: QUdpSocket uses native buffering per Qt documentation and ignores this.  Use native mac api
				//udpSocket->setReadBufferSize(1028000);
				//CuteSDR sets buffer size using native Mac UI after bind
				//Datagrams are 1028 bytes, so set buffer to a large enough mutiple
				//Works on dev box from 102800, make larger just to be sure
				int v = 102800; //CuteSDR sets to 2,000,000
				//socket descriptor, level, option, option value, length
				//SOL_SOCKET, SO_RCVBUF defined in socket.h
				::setsockopt(udpSocket->socketDescriptor(), SOL_SOCKET, SO_RCVBUF, (char *)&v, sizeof(v));
				//Remember we're being called from thread and signal needs to go to something in our thread
				//In this case, we translate the udpSocket signal to a producerConsumer signal
				connect(udpSocket, SIGNAL(readyRead()), &producerConsumer, SIGNAL(ProcessNewData()));

			}
		}
			break;
		case cbProducerConsumerEvents::Stop:
			if (deviceNumber == SDR_IQ || deviceNumber == AFEDRI_USB)
				return;
			//Destruct UDP sockets in thread local
			udpSocket->disconnectFromHost();
			disconnect(udpSocket,0,0,0);
			break;
		case cbProducerConsumerEvents::Run:
			if (deviceNumber == SDR_IQ) {
				DoUSBProducer();
			} else if (deviceNumber == SDR_IP) {
				DoUDPProducer();
			} else if (deviceNumber == AFEDRI_USB) {
				//Audio producer
			}
			break;
	}
}
void RFSpaceDevice::DoUDPProducer()
{
	UDPSocketNewData();
}

void RFSpaceDevice::DoUSBProducer()
{
	//USB Reads block until requested #bytes are availble.  We never want this to happen, since it burns CPU
	quint32 bytesAvailable = usbUtil->GetQueueStatus();

	if (!header.gotHeader) {
		//We need 2 bytes for header
		if (bytesAvailable < 2)
			return;
		if (!usbUtil->Read(usbReadBuf,2))
			return;
		UnpackHeader(usbReadBuf[0], usbReadBuf[1], &header);
		header.gotHeader = true;
		bytesAvailable -= 2;
	}

	//Special case to allow 2048 samples per block
	//When length==0 it means to read 8192 bytes
	//Largest value in a 13bit field is 8191 and we need length of 8192
	if (header.type == TargetHeaderTypes::TargetDataItem0 && header.length==0) {
		if (bytesAvailable < dataBlockSize)
			return;
		header.gotHeader = false; //ready for new header
		if ((producerFreeBufPtr = (CPX *)producerConsumer.AcquireFreeBuffer()) == NULL) {
			//We have to read data and throw away or we'll get out of sync
			usbUtil->Read(usbReadBuf, dataBlockSize);
			return;
		}
		if (!usbUtil->Read(usbReadBuf, dataBlockSize)) {
			//Lost data
			producerConsumer.ReleaseFreeBuffer(); //Put back what we acquired
			return;
		}
		for (int i=0, j=0; i<framesPerBuffer; i++, j+=4) {
			//IQ appear to be reversed
			normalizeIQ(&producerFreeBufPtr[i], (qint16)usbReadBuf[j+2], (qint16)usbReadBuf[j]);
		}
		//Increment the number of data buffers that are filled so consumer thread can access
		producerConsumer.ReleaseFilledBuffer();
		return;
	} else if (header.length < 2 || header.length > 100) {
		qDebug()<<"SDR-IQ header error";
		header.gotHeader = false;
		return;
	} else {
		//Adj for 2 bytes already read
		if (usbUtil->Read(usbReadBuf,header.length-2)) {
			header.gotHeader = false;
			//qDebug()<<"Type ="<<type<<" ItemCode = "<<itemCode;
			processControlItem(header, usbReadBuf);
			return;
		}
		return;
	}
	return;
}

//This is called from Consumer thread
void RFSpaceDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Stop:
			break;
		case cbProducerConsumerEvents::Run:
			//Wait for data to be available from producer
			while (producerConsumer.GetNumFilledBufs() > 0) {
				//qDebug()<<producerConsumer.GetNumFilledBufs()<<" "<<producerConsumer.GetNumFreeBufs();
				if ((consumerFilledBufPtr = (CPX *)producerConsumer.AcquireFilledBuffer()) == NULL)
					return;
				//Not sure if this is required for SDR-IQ, but it is for SDR-14
				//SendAck();

				//ProcessIQ has a max of about 2800us with 2m sample rate from sdr-ip
				//But ramps to 200,000us with sdr-iq!
				//Why?  Wierd samples in sdr-iq causing some error loop?
				//Lower time/slice
				//perform.StartPerformance("ProcessIQ");
				ProcessIQData(consumerFilledBufPtr,inBufferSize);
				//perform.StopPerformance(100);

				//Update lastDataBuf & release dataBuf
				producerConsumer.ReleaseFreeBuffer();
			}

			break;
	}
}

//Dialog Stuff
void RFSpaceDevice::rfGainChanged(int i)
{
	rfGain = i * -10;
	sRFGain = rfGain;
	SetRFGain(rfGain);
	WriteSettings();
}
void RFSpaceDevice::ifGainChanged(int i)
{
	ifGain = i * 6;
	sIFGain = ifGain;
	SetIFGain(ifGain);
	WriteSettings();
}
void RFSpaceDevice::discoverBoxChanged(bool b)
{
	autoDiscover = b;
	//Update options dialog if displayed
	WriteSettings();
}

void RFSpaceDevice::IPAddressChanged()
{
	//qDebug()<<optionUi->ipAddress->text();
	deviceAddress = QHostAddress(optionUi->ipEdit->text());
	WriteSettings();
}

void RFSpaceDevice::IPPortChanged()
{
	devicePort = optionUi->portEdit->text().toInt();
	WriteSettings();
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
		processControlItem(header,&tcpReadBuf[index + 2]);
		index += header.length;
	}
}

//UDP is all IQ data
void RFSpaceDevice::UDPSocketNewData()
{
	QHostAddress sender;
	quint16 senderPort;
	qint16 actual;
	qint64 dataGramSize;
	quint16 sequenceNumer;
	//We have to process all the data we have, won't get a second signal for same data
	while (udpSocket->hasPendingDatagrams()) {
		dataGramSize = udpSocket->pendingDatagramSize();
		//qDebug()<<"UDP datagram count: "<<++dataGramCount<<" size: "<<dataGramSize;
		actual = udpSocket->readDatagram((char*)udpReadBuf,dataGramSize, &sender, &senderPort);
		if (actual != dataGramSize) {
			qDebug()<<"ReadDatagram size error";
			return;
		}
		//The only datagrams we handle are IQ
		if (dataGramSize != 1028 || udpReadBuf[0] != TargetDataItem0 || udpReadBuf[1] != 0x84) {
			return; //Invalid
		}


		//Ignore sequence nubmer for now and assume we get in correct order
		//Eventually we can get fancy and buffer datagrams that arrive out of sequence and re-assemble in right order
		//Or we can determine how many packets we missed by comparing with last sequence number
		sequenceNumer = udpReadBuf[2];
		Q_UNUSED(sequenceNumer);
		//1024 data bytes follow or 512 2 byte samples

		if (readBufferIndex == 0) {
			//qDebug()<<producerConsumer.GetNumFreeBufs();
			//Starting a new producer buffer
			if ((producerFreeBufPtr = (CPX *)producerConsumer.AcquireFreeBuffer()) == NULL)
				return;
		}

		//USB gets 2048 I and 2048 Q samples at a time, or 8192 bytes
		//We need to build over 8 datagrams
		memcpy(&readBuf[readBufferIndex], &udpReadBuf[4], udpBlockSize); //256 I/Q samples
		readBufferIndex += udpBlockSize;

		if (readBufferIndex == dataBlockSize) {
			readBufferIndex = 0;
			for (int i=0, j=0; i<framesPerBuffer; i++, j+=4) {
				//Reverse IQ order
				normalizeIQ(&producerFreeBufPtr[i],readBuf[j+2],readBuf[j]);
			}
			//Increment the number of data buffers that are filled so consumer thread can access
			producerConsumer.ReleaseFilledBuffer();
		}
	}
	return;
}

bool RFSpaceDevice::SendTcpCommand(void *buf, qint64 len)
{
	qint64 actual = tcpSocket->write((char *)buf, len);
	return actual == len;

}

bool RFSpaceDevice::SendUsbCommand(void *buf, qint64 len)
{
	return usbUtil->Write((char *)buf, len);
}

bool RFSpaceDevice::SendAck()
{
	unsigned char writeBuf[] = {0x03,0x60,0x00}; //Ack data block 0
	if (deviceNumber == SDR_IQ)
		return SendUsbCommand(writeBuf, sizeof(writeBuf));
	return false;
	//If udpSocket is in ProducerThread and tcpSocket is in main thread, we can't send ack from ProducerThread
	//else
	//	return SendTcpCommand(writeBuf,sizeof(writeBuf));
}

//Device specific
//Self contained method
//Finds 1st device, can be extended to find multiple if needed
bool RFSpaceDevice::SendUDPDiscovery()
{
	QUdpSocket discoverUdpSocket;
	//We listen for a different port than we send so we don't get our own datagram
	quint16 discoverPort = 48322; //Accept UPD packets on this port (bind)
	quint16 discoverServerPort = 48321; //Send UDP broadcast packets to this port
	QHostAddress sender;
	quint16 senderPort;

	DISCOVER_MSG discover;
	DISCOVER_MSG_SDRIP discoverSDR_IP;
	quint16 discoverSize = sizeof(DISCOVER_MSG);

	memset((void*)&discover, 0, discoverSize); //zero out all fields
	discover.length = discoverSize;
	discover.key[0] = 0x5a;
	discover.key[1] = 0xa5;
	discover.op = 0; //PC to Device
	//strcpy((char *)discover.name,"SDR-IP"); //Device we're looking for

	if (!discoverUdpSocket.bind(discoverPort)){
		qDebug()<<"Failed bind";
		return false;
	}
	//Send broadcast (255.255.255.255) with the port we are listening on
	discoverUdpSocket.writeDatagram((char*)&discover, discoverSize, QHostAddress::Broadcast, discoverServerPort);
	//Sometimes this doesn't work and user has to try multiple times, try sending again after short delay to increase changes
	QThread::msleep(500);
	discoverUdpSocket.writeDatagram((char*)&discover, discoverSize, QHostAddress::Broadcast, discoverServerPort);

	//If we don't get response in 5 sec, give up
	QTimer timer;
	timer.start(5000);
	while (timer.remainingTime() > 0) {
		if (!discoverUdpSocket.hasPendingDatagrams()) {
			//Nothing pending, wait a bit
			QThread::msleep(100);
			continue;
		}
		//Read datagram and see if a discovery response
		qint64 dataGramSize = discoverUdpSocket.pendingDatagramSize();
		//qDebug()<<"UDP datagram count: "<<++dataGramCount<<" size: "<<dataGramSize;
		qint64 actual = discoverUdpSocket.readDatagram((char*)udpReadBuf,dataGramSize, &sender, &senderPort);
		if (actual != dataGramSize) {
			qDebug()<<"ReadDatagram size error";
			discoverUdpSocket.close();
			return false;
		}

		//SDR-IP returns 103 bytes
		if (udpReadBuf[2] == 0x5a && udpReadBuf[3] == 0xa5) {
			//Handle disovery data
			DISCOVER_MSG discover;
			//Read standard stuff first
			memcpy(&discover, udpReadBuf,discoverSize);
			deviceDiscoveredAddress = QHostAddress(discover.ipAddr.ip4.addr);
			deviceDiscoveredPort = discover.port;
			qDebug()<<"Found "<<(char*)discover.name<<"("<<(char*)discover.sn<<") at "<<deviceDiscoveredAddress<<" port "<<deviceDiscoveredPort;

			//Now extra
			if (QString::compare((char*)discover.name,"SDR-IP") == 0) {
				memcpy(&discoverSDR_IP, &udpReadBuf[discoverSize], sizeof(DISCOVER_MSG_SDRIP));
				qDebug()<<"SDR-IP fw version "<<discoverSDR_IP.fwver<<
					" boot version "<<discoverSDR_IP.btver<<
					" hw version "<<discoverSDR_IP.hwver;
			}
			discoverUdpSocket.close();
			return true;
		}
	}
	qDebug()<<"Discovery timed out";
	discoverUdpSocket.close();
	return false;
}


bool RFSpaceDevice::SetUDPAddressAndPort(QHostAddress address, quint16 port) {
	//To set the UDP IP address to 192.168.3.123 and port to 12345: [0A][00] [C5][00] [7B][03][A8]C0] [39][30]
	if (deviceNumber != SDR_IP)
		return false;
	unsigned char writeBuf[] {0x0a, 0x00, 0xc5, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	//192.168.0.1 is stored as 1, 0, 168, 192 in buffer
	*(quint32 *)&writeBuf[4] = address.toIPv4Address();
	*(quint16 *)&writeBuf[8] = port;
	return SendTcpCommand(writeBuf,sizeof(writeBuf));
}

//This lets the device tweak its internal clock so we can tweak frequency accuracy if needed
//Not used yet, so just set to default value
bool RFSpaceDevice::SetADSampleRate()
{
	unsigned char writeBuf[] = {0x09, 0x00, 0xB0, 0x00, 0x02, 0xff, 0xff, 0xff, 0xff};
	*(quint32 *)&writeBuf[5] = 66666667;
	if (deviceNumber == SDR_IP)
		return false;
	else if (deviceNumber == SDR_IQ)
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	else if (deviceNumber == AFEDRI_USB)
		return true;
	else
		return false;
}

//This sets the bandwidth, not actual sample rate
bool RFSpaceDevice::SetSampleRate()
{
	//100k sample rate example: [09] [00] [B8] [00] [00] [A0] [86] [01] [00]
	unsigned char writeBuf[] = {0x09, 0x00, 0xb8, 0x00, 0x00, 0xa0, 0x86, 0x01, 0x00};
	//Verified that this cast to quint32 puts things in right order by setting sampleRate = 100000 and comparing
	if (deviceNumber == SDR_IP) {
		*(quint32 *)&writeBuf[5] = sampleRate;
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		//Setting AD6620 registers, takes a while and returns lots of garbage data
		//We used to have to flush usb buffer after doing this
		//Replaced in firmware 1.4 with 0xb8 command
		// ad6620->SetBandwidth(sBandwidth);
		//SDR-IQ has specific values we have to map to
		switch(sBandwidth) {
			case AD6620::BANDWIDTH::BW5K:
				*(quint32 *)&writeBuf[5] = 8138;
				break;
			case AD6620::BANDWIDTH::BW10K:
				*(quint32 *)&writeBuf[5] = 16276;
				break;
			case AD6620::BANDWIDTH::BW25K:
				*(quint32 *)&writeBuf[5] = 37793;
				break;
			case AD6620::BANDWIDTH::BW50K:
				*(quint32 *)&writeBuf[5] = 55556;
				break;
			case AD6620::BANDWIDTH::BW100K:
				*(quint32 *)&writeBuf[5] = 111111;
				break;
			case AD6620::BANDWIDTH::BW150K:
				*(quint32 *)&writeBuf[5] = 158730;
				break;
			case AD6620::BANDWIDTH::BW190K:
				*(quint32 *)&writeBuf[5] = 196078;
				break;
		}
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == AFEDRI_USB) {
		//afedri->Set_New_Sample_Rate(sampleRate);
		afedri->Send_Sample_Rate(sampleRate);
	}
	return false;
}


//0,-10,-20,-30
bool RFSpaceDevice::SetRFGain(qint8 gain)
{
	unsigned char writeBuf[] = { 0x06, 0x00, 0x38, 0x00, 0xff, 0xff};
	writeBuf[4] = 0x00;
	writeBuf[5] = gain ;
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
//This is not documented in Interface spec, but used in activeX examples
bool RFSpaceDevice::SetIFGain(qint8 gain)
{
	unsigned char writeBuf[] = { 0x06, 0x00, 0x40, 0x00, 0xff, 0xff};
	//Bits 7,6,5 are Factory test bits and are masked
	writeBuf[4] = 0; //gain & 0xE0;
	writeBuf[5] = gain;
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
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
	if (deviceNumber == SDR_IP) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x80, 0x02, 0x00, 0x00};
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x00, 0x00};
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}

bool RFSpaceDevice::StopCapture()
{
	if (deviceNumber == SDR_IP) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x00};
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x01, 0x00, 0x00};
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
void RFSpaceDevice::FlushDataBlocks()
{
	bool result;
	if (deviceNumber == SDR_IP) {
		return;
	} else if (deviceNumber == SDR_IQ) {
		do
			result = usbUtil->Read(readBuf, 1);
		while (result);
	} else  if (deviceNumber == AFEDRI_USB)
		return;
}
//Control Item Code 0x0001
bool RFSpaceDevice::RequestTargetName()
{
	targetName = "Pending";
	//0x04, 0x20 is the request command
	//0x01, 0x00 is the Control Item Code (command)
	unsigned char writeBuf[] = { 0x04, 0x20, 0x01, 0x00 };
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
//Same pattern as TargetName
bool RFSpaceDevice::RequestTargetSerialNumber()
{
	serialNumber = "Pending";
	unsigned char writeBuf[] = { 0x04, 0x20, 0x02, 0x00 };
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB) {
		serialNumber = afedri->Get_Serial_Number();
		return true;
	}
	return false;
}
bool RFSpaceDevice::RequestInterfaceVersion()
{
	interfaceVersion = 0;
	unsigned char writeBuf[] = { 0x04, 0x20, 0x03, 0x00 };
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB) {
		return true;
	}
	return false;
}
bool RFSpaceDevice::RequestFirmwareVersion()
{
	firmwareVersion = 0;
	unsigned char writeBuf[] = { 0x05, 0x20, 0x04, 0x00, 0x01 };
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
bool RFSpaceDevice::RequestBootcodeVersion()
{
	bootcodeVersion = 0;
	unsigned char writeBuf[] = { 0x05, 0x20, 0x04, 0x00, 0x00 };
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}

bool RFSpaceDevice::RequestStatusCode()
{
	unsigned char writeBuf[] = { 0x04, 0x20, 0x05, 0x00};
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
//Call may not be working right in SDR-IQ
bool RFSpaceDevice::RequestStatusString(unsigned char code)
{
	unsigned char writeBuf[] = { 0x05, 0x20, 0x06, 0x00, code};
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
bool RFSpaceDevice::GetFrequency()
{
	unsigned char writeBuf[] = { 0x05, 0x20, 0x20, 0x00, 0x00};
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
	return false;
}
bool RFSpaceDevice::SetFrequency(double freq)
{
	if (freq==0)
		return freq; //ignore

	unsigned char writeBuf[] = { 0x0a, 0x00, 0x20, 0x00, 0x00,0xff,0xff,0xff,0xff,0x01};
	DoubleToBuf(&writeBuf[5],freq);
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		//HID commands look almost like writeBuf[2] to [n] above
		//But HID command to set freq is 0x03, not 0x20 as above.  Not sure why
		return afedri->SetFrequency(freq);
	return false;
}

//OneShot - Requests SDR to send numBlocks of data
//Note: Blocks are fixed size, 8192 bytes
bool RFSpaceDevice::CaptureBlocks(unsigned numBlocks)
{
	//C++11 doesn't allow variable in constant initializer, so we set writeBuf[7] separately
	unsigned char writeBuf[] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x02, 0x00};
	writeBuf[7] = numBlocks;
	if (deviceNumber == SDR_IP) {
		return SendTcpCommand(writeBuf,sizeof(writeBuf));
	} else if (deviceNumber == SDR_IQ) {
		return SendUsbCommand(writeBuf,sizeof(writeBuf));
	} else  if (deviceNumber == AFEDRI_USB)
		return true;
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

