#include "ghpsdr3device.h"

#if 0
http://qtradio.napan.ca/qtradio/qtradio.pl
IP	Call	Location	Band	Rig	Ant	Status	UTC
98.16.87.162	N8MDP	Cleveland, Ohio, EN91jj	160M - 10M	Softrock Ensemble II	G5RV	1 client(s)	2014-04-01 21:27:21
77.89.62.35	IU4BLX	Sparvo, BO - JN54oe	unknown	Funcube Dongle Pro+	vertical	0 client(s)	2014-04-01 21:27:35
83.211.100.157	IK1JNS	Rivoli, TO JN35sb	160m - 10m	SoftRock v9 EMU202 EeePC	Dipoles	1 client(s)	2014-04-01 21:29:16
67.164.154.63	AD5DZ	Santa Fe, NM (DM75ap)	160-10M	RXII/SYBA-Dongle	30/20/17M indoor cobwebb	1 client(s)	2014-04-01 21:30:15
79.255.247.138	DK0NR	JO30RK	80-10m	HiQSDR	FD4	2 client(s)	2014-04-01 21:30:36
23.243.237.150	N6EV	Los Angeles, CA DM03tv	160-10M	Softrock Ensemble RX II	AM:HexBeam, PM:R7000 Vert	1 client(s)	2014-04-01 21:30:57
74.85.89.174	KL7NA	Walla Walla, WA, DN06sa	20-15	Softrock RXTX 30-15 meters	Tribander	1 client(s)	2014-04-01 21:31:19
87.219.89.103	MYCALL	My Location	20	ANAN100D	My Antenna is	0 client(s)	2014-04-01 21:31:59

#endif
Ghpsdr3Device::Ghpsdr3Device():DeviceInterfaceBase()
{
	InitSettings("Ghpsdr3");
	tcpSocket = NULL;


	//Testing
	//deviceAddress = QHostAddress("74.85.89.174");
	//deviceAddress = QHostAddress("71.47.206.230");
	//deviceAddress = QHostAddress("79.255.247.138");
	deviceAddress = QHostAddress("84.175.15.210");
	devicePort = 8000; //8000 + Receiver #

}

Ghpsdr3Device::~Ghpsdr3Device()
{
	if (tcpSocket != NULL)
		delete tcpSocket;
}

bool Ghpsdr3Device::Initialize(cbProcessIQData _callback,
							   cbProcessBandscopeData _callbackBandscope,
							   cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	numProducerBuffers = 50;
	producerConsumer.Initialize(std::bind(&Ghpsdr3Device::producerWorker, this, std::placeholders::_1),
		std::bind(&Ghpsdr3Device::consumerWorker, this, std::placeholders::_1),numProducerBuffers, framesPerBuffer*sizeof(CPX));
	//Review interval since we're gettin audio rates not IQ sample rates
	producerConsumer.SetConsumerInterval(8000,AUDIO_OUTPUT_SIZE);
	producerConsumer.SetProducerInterval(8000,AUDIO_OUTPUT_SIZE);
	producerBufIndex = 0;

	tcpReadState = READ_HEADER_TYPE; //Start out looking for header type

	if (tcpSocket == NULL) {
		tcpSocket = new QTcpSocket();
		connect(tcpSocket,&QTcpSocket::connected, this, &Ghpsdr3Device::TCPSocketConnected);
		connect(tcpSocket,&QTcpSocket::disconnected, this, &Ghpsdr3Device::TCPSocketDisconnected);
		connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(TCPSocketError(QAbstractSocket::SocketError)));
		connect(tcpSocket,&QTcpSocket::readyRead, this, &Ghpsdr3Device::TCPSocketNewData);
		connect(this,SIGNAL(CheckNewData()), this, SLOT(TCPSocketNewData()));
	}

	serverInfo.isValid = false;
	return true;
}

bool Ghpsdr3Device::Connect()
{
	//Smaller buffers generally work better, shorter processing time per buffer
	tcpSocket->setReadBufferSize(AUDIO_OUTPUT_SIZE);

	tcpSocket->connectToHost(deviceAddress,devicePort,QTcpSocket::ReadWrite);
	if (!tcpSocket->waitForConnected(1000)) {
		//Socket was closed or error
		qDebug()<<"Server error "<<tcpSocket->errorString();
		return false;
	}
	connected = true;
	running = false;
	//Sometimes server responds with data, before we call startAudio...
	tcpSocket->readAll(); //Clear it
	return true;
}

bool Ghpsdr3Device::Disconnect()
{
	if (connected) {
		tcpSocket->disconnectFromHost();
		if (tcpSocket->state() == QTcpSocket::ConnectedState  && !tcpSocket->waitForDisconnected(1000)) {
			qDebug()<<"tcpSocket disconnect timed out";
		}
	}
	connected = false;
	return true;
}

void Ghpsdr3Device::Start()
{
	//Protocol docs are useless, from QT Radio UI.cpp
	//Search for sendCommand
	//"setClient QtRadio"
	//"q-server"
	//"setFrequency "
	//"setMode "
	//"setFilter "
	//"setEncoding 0|1|2"
	//"startAudioStream "
	//"SetPan 0.5"
	//"SetAGC "
	//"setpwsmode "
	//"SetANFVals "
	//"SetNRVals "
	//"SetNBVals "
	//"SetSquelchVal "
	//"SetSquelchState "
	//"SetANF "
	//"SetNR "
	//"SetNB "
	//dspversion >= 20130609 Advanced commands
	//"setrxdcblock "
	//....
	//We start getting data from server, before we start.  Handle it or we fill up ProdCons buffer
	producerConsumer.Start();
	running = true;
	//Testing initial setup requirements
	//SendTcpCmd("setClient QtRadio");
	SendModeCmd(gDemodMode::AM);
	SendGainCmd(50); //0-100, 100 overloads
	SendFilterCmd(-2000, 2000);
	SendTcpCmd("setencoding 0"); //ALAW
	//Get all the status data from server with q- commands
	RequestInfo();

	//If slave mode, we get audio stream even when we don't call this
	//But safe to call either way
	//Don't rely on default buffer size, set explicitly
	//Buffer size(seems to be ignored), sample, channels
	QString buf = "startaudiostream 512 8000 1 0";
	SendTcpCmd(buf);

}

void Ghpsdr3Device::Stop()
{
	if (!running)
		return;
	running = false;
	producerConsumer.Stop();
	SendTcpCmd("stopaudiostream");
	//Drain any buffered data
	tcpSocket->flush();
	tcpSocket->readAll(); //Throw away anything left in buffers
}

void Ghpsdr3Device::ReadSettings()
{
	DeviceInterfaceBase::ReadSettings();
	//Device specific settings follow
}

void Ghpsdr3Device::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	//Device specific settings follow
}

QVariant Ghpsdr3Device::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "Ghpsdr3Server";
			break;
		case PluginDescription:
			return "Ghpsdr3Server";
			break;
		case DeviceName:
			if (serverInfo.isValid)
				return "Ghpsdr3Server " + serverInfo.serverName + (serverInfo.isSlave?"(Slave)":"(Master)");
			else
				return "Ghpsdr3Server (Unknown status)";
		case DeviceFrequency:
			if (serverInfo.isValid && serverInfo.isSlave)
				return serverInfo.frequency;
			else
				return 0;

		case DeviceType:
			//!!Todo: Update all DeviceType usage to handle Audio_only
			return DeviceInterface::IQ_DEVICE; //AUDIO_ONLY;
		case AudioOutputSampleRate:
			return 8000; //Fixed?
			break;
		case DeviceSlave:
			if (!serverInfo.isValid || !serverInfo.isSlave)
				return false;
			else
				return serverInfo.isSlave;
		case DeviceDemodMode:
			if (!serverInfo.isValid || !serverInfo.isSlave)
				return DEMODMODE::dmAM;
			else
				return serverInfo.demodMode;
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool Ghpsdr3Device::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case DeviceFrequency:
			if (serverInfo.isValid && !serverInfo.isSlave)
				return SendFrequencyCmd(_value.toFloat());
			return false;
		case DeviceDemodMode:		//RW quint16 enum DEMODMODE
			if (serverInfo.isValid && !serverInfo.isSlave)
				return true;
			return false;
		case DeviceOutputGain:		//RW quint16
			if (serverInfo.isValid && !serverInfo.isSlave)
				return true;
			return false;
		case DeviceFilter:			//RW QString "lowFilter:highFilter"
			if (serverInfo.isValid && !serverInfo.isSlave)
				return true;
			return false;
		case DeviceAGC:				//RW quint16
			if (serverInfo.isValid && !serverInfo.isSlave)
				return true;
			return false;
		case DeviceANF:				//RW quint16
			if (serverInfo.isValid && !serverInfo.isSlave)
				return true;
			return false;
		case DeviceNB:				//RW quint16
			if (serverInfo.isValid && !serverInfo.isSlave)
				return true;
			return false;

		case DeviceSlave:
			//Fetch new data
			RequestInfo();
			return true;

		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void Ghpsdr3Device::RequestInfo()
{
	if (!running)
		return;

	mutex.lock();
	SendTcpCmd("q-version");
	SendTcpCmd("q-protocol3");
	SendTcpCmd("q-master");
	SendTcpCmd("q-loffset");
	SendTcpCmd("q-cantx#None");
	SendTcpCmd("q-rtpport");
	SendTcpCmd("q-server");
	//SendTcpCmd("*hardware?"); //Doesn't return any answer
	//Send q-info last.  We use response to determine if serverInfo has been set.  Not perfect, but ok
	SendTcpCmd("q-info");
	mutex.unlock();
}

bool Ghpsdr3Device::SendTcpCmd(QString buf)
{
	//Looks like commands are always 64 bytes with unused filled with 0x00
	quint8 cmdBuf[SEND_BUFFER_SIZE];
	memset(cmdBuf,0x00,SEND_BUFFER_SIZE);
	memcpy(cmdBuf,buf.toUtf8(),buf.length());
	qint64 actual = tcpSocket->write((char*)cmdBuf, SEND_BUFFER_SIZE);
	tcpSocket->flush(); //Force immediate blocking write
	//qDebug()<<(char*)cmdBuf;
	if (actual != SEND_BUFFER_SIZE) {
		qDebug()<<"SendTcpCmd did not write 64 bytes";
		return false;
	}
	return true;
}

bool Ghpsdr3Device::SendFrequencyCmd(double f)
{
	QString buf = "setfrequency "+ QString::number(f,'f',0);
	return SendTcpCmd(buf);
}

bool Ghpsdr3Device::SendModeCmd(gDemodMode m)
{
	QString buf = "setmode "+ QString::number(m);
	return SendTcpCmd(buf);
}

bool Ghpsdr3Device::SendGainCmd(quint8 gain)
{
	QString buf = "setrxoutputgain " + QString::number(gain);
	return SendTcpCmd(buf);
}

bool Ghpsdr3Device::SendFilterCmd(qint16 low, qint16 high)
{
	QString buf = "setfilter " + QString::number(low) + " " + QString::number(high);
	return SendTcpCmd(buf);
}

void Ghpsdr3Device::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
}

void Ghpsdr3Device::producerWorker(cbProducerConsumerEvents _event)
{
	switch(_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			//Sanity check to see if we have tcp data in case we missed Signal
			emit CheckNewData();
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void Ghpsdr3Device::consumerWorker(cbProducerConsumerEvents _event)
{
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run: {
			//qDebug()<<"Consumer run";
			unsigned char * buf = producerConsumer.AcquireFilledBuffer(100);
			if (buf == NULL) {
				//qDebug()<<"No filled buffer available";
				return;
			}
			ProcessAudioData((CPX*)buf,AUDIO_OUTPUT_SIZE);
			producerConsumer.ReleaseFreeBuffer();
			break;
		}
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void Ghpsdr3Device::TCPSocketError(QAbstractSocket::SocketError socketError)
{
	Q_UNUSED(socketError);
	qDebug()<<"Socket Error";
}

void Ghpsdr3Device::TCPSocketConnected()
{
	qDebug()<<"Connected";
}

void Ghpsdr3Device::TCPSocketDisconnected()
{
	qDebug()<<"Disconnected, attempting re-connect";
	//Todo
}

//
void Ghpsdr3Device::TCPSocketNewData()
{
	//If we haven't called Start() yet, throw away any incoming data
	if (!running) {
		tcpSocket->readAll();
		return;
	}
	//qDebug()<<"New data";
	//mutex.lock();
	quint16 headerCommonSize = sizeof(commonHeader);
	quint16 audioHeaderSize = sizeof(audioHeader);
	QString answer;
	//QStringList answerParts;
	QRegExp rx; //Used for parsing answers, regex from QTRadio source

	qint64 bytesRead;
	quint16 bytesToCompleteAudioBuffer;
	quint64 bytesAvailable = tcpSocket->bytesAvailable();
	if (bytesAvailable <= 0) {
		return; //Nothing to do
	}
	while (bytesAvailable > 0) {
		switch (tcpReadState) {
			case READ_HEADER_TYPE:
				audioBufferIndex = 0;
				//Looking for 3 byte common header
				if (bytesAvailable < headerCommonSize) {
					qDebug()<<"Waiting for enough data for common header";
					return; //Wait for more data
				}
				bytesRead = tcpSocket->read((char*)&commonHeader,headerCommonSize);
				if (bytesRead != headerCommonSize) {
					qDebug()<<"Expected header but didn't get it";
					return;
				}
				bytesAvailable -= bytesRead;
				//qDebug()<<"READ_HEADER_TYPE "<<(commonHeader.packetType & 0x0f);

				//Answer type combines packetType and part of length in byte[0]
				//We need to mask high order bits in first byte
				switch (commonHeader.packetType & 0x0f) {
					case SpectrumData:
						qDebug()<<"Unexpected spectrum data";
						break;
					case AudioData:
						//Get size of data
						tcpReadState = READ_AUDIO_HEADER;
						break;
					case BandscopeData:
						qDebug()<<"Unexpected bandscope data";
						break;
					case RTPReplyData:
						qDebug()<<"Unexpected rtp reply data";
						break;
					case AnswerData:
						//qDebug()<<commonHeader.bytes[0]  << " "<< commonHeader.bytes[1]<< " "<< commonHeader.bytes[2];
						//First bytes are interpreted as ascii
						answerLength = (commonHeader.bytes[1] - 0x30) * 10 + (commonHeader.bytes[2] - 0x30);
						//qDebug()<<"Answer header length"<<answerLength;
						tcpReadState = READ_ANSWER;
						break;
					default:
						//This usually means we missed some data
						qDebug()<<"Invalid packet type "<<(commonHeader.packetType & 0x0f);
						break;
				}

				break;

			case READ_AUDIO_HEADER:
				//qDebug()<<"READ_AUDIO_HEADER";
				if (bytesAvailable < audioHeaderSize) {
					qDebug()<<"Waiting for enough data for audio header";
					return; //Wait for more data
				}
				bytesRead = tcpSocket->read((char*)&audioHeader,audioHeaderSize);
				if (bytesRead != audioHeaderSize) {
					qDebug()<<"Expected audio header but didn't get it";
					tcpReadState = READ_HEADER_TYPE;
					return;
				}
				bytesAvailable -= bytesRead;
				tcpReadState = READ_AUDIO_BUFFER;
				//Use QT's qFromBigEndian aToBitEndian and qFromLittleEndian and qToLittleEndian instead of nthl()
				//Convert to LittleEndian format we can use
				audioHeader.bufLen = qFromBigEndian(audioHeader.bufLen);
				if (audioHeader.bufLen != AUDIO_PACKET_SIZE) {
					//We're out of sync
					qDebug()<<"Invalid audio buffer length";
					tcpReadState = READ_HEADER_TYPE;
					return;
				}
				break;

			case READ_AUDIO_BUFFER:
				//qDebug()<<"READ_AUDIO_BUFFER "<<bytesAvailable<<" "<<audioHeader.bufLen;
				//Process whatever bytes we have, even if not a full buffer
				bytesToCompleteAudioBuffer = audioHeader.bufLen - audioBufferIndex;
				if (bytesAvailable > bytesToCompleteAudioBuffer)
					bytesRead = tcpSocket->read((char*)&audioBuffer,bytesToCompleteAudioBuffer);
				else
					bytesRead = tcpSocket->read((char*)&audioBuffer,bytesAvailable);

				if (bytesRead < 0) {
					qDebug()<<"Error in tcpSocketRead";
					return;
				}
				bytesAvailable -= bytesRead;
				//Process audio data
				qint16 decoded;
				for (int i=0; i<bytesRead; i++) {
					if (producerBufIndex == 0) {
						//Make sure we wait for free buffer long enough to handle short delays
						producerBuf = (CPX*)producerConsumer.AcquireFreeBuffer(1000);
						if (producerBuf == NULL) {
							qDebug()<<"No free buffer available";
							//Todo: We need a reset function to start everything over, reconnect etc
							return;
						}
					}

					decoded = alaw.ALawToLinear(audioBuffer[i]);
					producerBuf[producerBufIndex].re = producerBuf[producerBufIndex].im = decoded / 32767.0;
					producerBufIndex++;
					if (producerBufIndex >= AUDIO_OUTPUT_SIZE) {
						producerConsumer.ReleaseFilledBuffer();
						producerBufIndex = 0;
					}

				}
				//If we've processed AUDIO_OUTPUT_SIZE samples, then change state
				audioBufferIndex += bytesRead;
				if (audioBufferIndex >= audioHeader.bufLen) {
					audioBufferIndex = 0;
					tcpReadState = READ_HEADER_TYPE; //Start over and look for next header
				}

				break;

			case READ_HEADER:
				qDebug()<<"Unexpected read_header";
				break;

			case READ_ANSWER:
				//qDebug()<<"READ_ANSWER";
				if (answerLength > 99) {
					qDebug()<<"Invalid answer length";
					return;
				}
				if (bytesAvailable < answerLength) {
					qDebug()<<"Waiting for enough data for answer";
					return; //Wait for more data
				}
				bytesRead = tcpSocket->read((char*)&answerBuf,answerLength);
				if (bytesRead != answerLength) {
					qDebug()<<"Expected answer but didn't get it";
					tcpReadState = READ_HEADER_TYPE;
					return;
				}
				bytesAvailable -= bytesRead;
				tcpReadState = READ_HEADER_TYPE;
				answerBuf[answerLength] = 0x00;
				answer = (char*)answerBuf;
				qDebug()<<answer;
				//Process answer
				if(answer.contains("q-version")){
					//"q-version:20130609;-master"
				} else if(answer.contains("q-server")){
					//"q-server:KL7NA P"
					rx.setPattern("q-server:(\\S+)");
					rx.indexIn(answer);
					serverInfo.serverName = rx.cap(1);

				} else if(answer.contains("q-master")){
					//"q-master:master"
					if (answer.contains("slave")){
						serverInfo.isSlave = true;
					}else{
						serverInfo.isSlave = false;
					}

				} else if(answer.contains("q-rtpport")){
					//"q-rtpport:5004;"
					rx.setPattern("rtpport:(\\d+);");
					rx.indexIn(answer);
					serverInfo.rtpPort = rx.cap(1);

				} else if(answer.contains("q-cantx:")){
					//"q-cantx:N"
					//Not used, transmit not supported

				} else if(answer.contains("q-loffset:")){
					//"q-loffset:9000.000000;"
					rx.setPattern("q-loffset:(\\d+)\\.");
					rx.indexIn(answer);
					serverInfo.offset = rx.cap(1).toDouble();

				} else if(answer.contains("q-info")){
					//"q-info:s;1;f;10000000;m;6;z;0;l;-2000;r;2000;"
					rx.setPattern("info:s;(\\d+);f;(\\d+);m;(\\d+);z;(\\d+);l;(\\d+|-\\d+);r;(\\d+|-\\d+)");
					rx.indexIn(answer);
					serverInfo.serverNum = rx.cap(1).toInt();
					serverInfo.frequency = rx.cap(2).toLongLong();
					//Map demo mode to Pebble
					gDemodMode gdm = (gDemodMode)rx.cap(3).toInt();
					switch (gdm) {
						case LSB: serverInfo.demodMode = DEMODMODE::dmLSB; break;
						case USB: serverInfo.demodMode = DEMODMODE::dmUSB; break;
						case DSB: serverInfo.demodMode = DEMODMODE::dmDSB; break;
						case CWL: serverInfo.demodMode = DEMODMODE::dmCWL; break;
						case CWH: serverInfo.demodMode = DEMODMODE::dmCWU; break;
						case FM: serverInfo.demodMode = DEMODMODE::dmFMN; break; //Check
						case AM: serverInfo.demodMode = DEMODMODE::dmAM; break;
						case DIGU: serverInfo.demodMode = DEMODMODE::dmDIGU; break;
						case SPEC: serverInfo.demodMode = DEMODMODE::dmNONE; break;
						case DIGL: serverInfo.demodMode = DEMODMODE::dmDIGL; break;
						case SAM: serverInfo.demodMode = DEMODMODE::dmSAM; break;
						case DRM: serverInfo.demodMode = DEMODMODE::dmNONE; break; //Missing
						default: serverInfo.demodMode = DEMODMODE::dmAM; break;

					}

					serverInfo.zoom = rx.cap(4).toInt();
					serverInfo.lowFilter = rx.cap(5).toInt();
					serverInfo.highFilter = rx.cap(6).toInt();
					serverInfo.isValid = true;

				} else if (answer.contains("q-protocol3")){
					//"q-protocol3:Y"
					rx.setPattern("([YN])$");
					rx.indexIn(answer);
					serverInfo.protocol3 = rx.cap(1).compare("Y")==0;

				} else if (answer[0] == '*') {

				}
				break;
			case READ_RTP_REPLY:
				qDebug()<<"Unexpected rtp reply";
				break;
		}

	}
	//mutex.unlock();
}

