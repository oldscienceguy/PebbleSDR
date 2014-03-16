#include "ghpsdr3device.h"

Ghpsdr3Device::Ghpsdr3Device():DeviceInterfaceBase()
{
	InitSettings("Ghpsdr3");
	tcpSocket = NULL;

	//Testing
	//deviceAddress = QHostAddress("74.85.89.174"); //Hermes
	deviceAddress = QHostAddress("71.47.206.230"); //SDR-IQ
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
		std::bind(&Ghpsdr3Device::consumerWorker, this, std::placeholders::_1),numProducerBuffers, framesPerBuffer);

	tcpSocket = new QTcpSocket();

	connect(tcpSocket,&QTcpSocket::connected, this, &Ghpsdr3Device::TCPSocketConnected);
	connect(tcpSocket,&QTcpSocket::disconnected, this, &Ghpsdr3Device::TCPSocketDisconnected);
	connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(TCPSocketError(QAbstractSocket::SocketError)));
	connect(tcpSocket,&QTcpSocket::readyRead, this, &Ghpsdr3Device::TCPSocketNewData);

	return true;
}

bool Ghpsdr3Device::Connect()
{
	tcpSocket->setReadBufferSize(tcpReadBufSize);

	tcpSocket->connectToHost(deviceAddress,devicePort,QTcpSocket::ReadWrite);
	if (!tcpSocket->waitForConnected(1000)) {
		//Socket was closed or error
		qDebug()<<"Server error "<<tcpSocket->errorString();
		return false;
	}
	connected = true;
	return true;
}

bool Ghpsdr3Device::Disconnect()
{
	if (connected) {
		tcpSocket->disconnectFromHost();
		if (!tcpSocket->waitForDisconnected(1000)) {
			qDebug()<<"tcpSocket disconnect timed out";
		}
	}
	connected = false;
	return true;
}

void Ghpsdr3Device::Start()
{
	//Don't rely on default buffer size, set explicitly
	QString buf = "startAudioStream "+ QString::number(tcpDataBufSize);
	//qDebug()<<buf;
	qint64 len = sizeof(buf);
	qint64 actual = tcpSocket->write(buf.toUtf8(), len);
	Q_UNUSED(actual);

}

void Ghpsdr3Device::Stop()
{
	char buf[] = "stopAudioStream";
	qint64 len = sizeof(buf);
	qint64 actual = tcpSocket->write((char *)buf, len);
	Q_UNUSED(actual);
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
			return "Ghpsdr3Server";
		case DeviceType:
			return DeviceInterface::AUDIO_ONLY;
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool Ghpsdr3Device::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case DeviceFrequency: {
			QString buf = "setFrequency "+ _value.toString();
			qint64 len = sizeof(buf);
			qint64 actual = tcpSocket->write(buf.toUtf8(), len);
			return actual == len;
		}
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void Ghpsdr3Device::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
}

void Ghpsdr3Device::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void Ghpsdr3Device::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
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
	qDebug()<<"Disconnected";
}

//
void Ghpsdr3Device::TCPSocketNewData()
{
	//mutex.lock();
	qint64 bytesAvailable = tcpSocket->bytesAvailable();
	qDebug()<<bytesAvailable;
	dspServerHeader *header;
	void *samples;
	qint64 bytesRead;
	while (bytesAvailable >= tcpReadBufSize) {
		bytesRead = tcpSocket->read((char*)tcpReadBuf,tcpReadBufSize);

		if (bytesRead != tcpReadBufSize) {
			qDebug()<<"sync error";
			//mutex.unlock();
			return;
		}
		//1st 48 bytes are header
		header = (dspServerHeader *)tcpReadBuf;
		//Rest is audio or IQ data
		samples = &tcpReadBuf[tcpHeaderSize];

		//qDebug()<<header->packetType<<" "<<header->asciiSampleRate<<" "<<header->asciiMeter;
		qDebug()<<tcpReadBuf[0];
		bytesAvailable -= bytesRead;
	}
	//mutex.unlock();
}
