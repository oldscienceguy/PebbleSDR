#include "hpsdrnetwork.h"
#include "hpsdrdevice.h"
#include "arpa/inet.h" //For ntohl()

HPSDRNetwork::HPSDRNetwork()
{
	waitingForDiscoveryResponse = false;
	isBound = false;
	isSendingData = false;
	udpSocket = NULL;
	metisHostAddress.clear();
	udpSequenceNumberIn = 0;
	udpSequenceNumberOut = 0;

}

HPSDRNetwork::~HPSDRNetwork()
{
}

bool HPSDRNetwork::Init(HPSDRDevice *_hpsdrDevice)
{
	bool result;
	hpsdrDevice = _hpsdrDevice;
	if (udpSocket == NULL)
		udpSocket = new QUdpSocket();

	//if (udpSocket->state != QUdpSocket::BoundState) {
	//Lets manage our own state instead of relying on udpSocket
	if (!isBound) {
		//Bind on first use
		//Bind establishes an IP address and port that determines which inbound datagrams we accept
		//It has nothing to do with outboud datagrams
		//Once set, any inbound datagrams addressed to this IP/Port will trigger the readyRead signal
		result = udpSocket->bind(); //Binds to QHostAddress:Any on port which is randomly chosen
		//Alternative if we need to set a specific HostAddress and port
		//result = udpSocket->bind(QHostAddress::Any, port=0);
		if (result) {
			isBound = true;
			connect(udpSocket, &QUdpSocket::readyRead, this, &HPSDRNetwork::NewUDPData);
		} else {
			return false;
		}
	}
	//If we don't have fixed IP/Port for Metis, trigger discovery
	if (metisHostAddress.isNull()) {
		//If we don't have fixed IP address, then trigger automatic Metis discovery
		waitingForDiscoveryResponse = false;
		if (!SendDiscovery())
			return false;
		//Note we still don't have IP address here, have to wait for UDP response
		for (int i=0; i<50; i++) {
			if (waitingForDiscoveryResponse) {
				QThread::msleep(100);
				//We need to call this to give signals a chance to process and trigger readyRead
				qApp->processEvents();
			} else {
				return true; //Got it
			}
		}
		return false; //No response
	}
	return true;
}

#if 0

The Discovery packet is a UDP/IP frame sent to Ethernet address 255.255.255.255 (i.e. a Broadcast) and port 1024
with the following payload: <0xEFFE><0x02><60 bytes of 0x00>
#endif
bool HPSDRNetwork::SendDiscovery()
{
	PcToMetisDiscovery payload;

	qint64 actual = udpSocket->writeDatagram((char*)&payload,sizeof(payload),QHostAddress::Broadcast,1024);
	if (actual == sizeof(payload)) {
		//Set flag waiting for response
		waitingForDiscoveryResponse = true;
		return true;
	} else {
		return false;
	}
}

bool HPSDRNetwork::SendStart()
{
	if (metisHostAddress.isNull())
		return false;

	PcToMetisStart payload;
	payload.command = 0x01; //0x01 for IQ data only, 0x02 for Bandscope only, 0x03 for both
	qint64 actual = udpSocket->writeDatagram((char*)&payload,sizeof(payload),metisHostAddress, metisPort);
	if (actual != sizeof(payload))
		return false;

	udpSequenceNumberOut = 0;
	return true;
}

bool HPSDRNetwork::SendStop()
{
	if (metisHostAddress.isNull())
		return false;

	PcToMetisStart payload;
	payload.command = 0x00; //0x02 to stop IQ only, 0x01 to stop Bandscope only, 0x00 to stop both
	qint64 actual = udpSocket->writeDatagram((char*)&payload,sizeof(payload),metisHostAddress, metisPort);
	if (actual != sizeof(payload))
		return false;
	else
		return true;
}

//cmd is 5 byte array, same as USB
bool HPSDRNetwork::SendCommand(char *cmd1, char *cmd2)
{
	if (metisHostAddress.isNull())
		return false;

	PcToMetisData payload;
	memset(&payload.frame1,0x00,512);
	memset(&payload.frame2,0x00,512);

	//Do we need to use htonl() for big endian?
	payload.sequenceNumber = udpSequenceNumberOut++;
	if (cmd1 != NULL) {
		payload.frame1[0] = 0x7f;
		payload.frame1[1] = 0x7f;
		payload.frame1[2] = 0x7f;
		payload.frame1[3] = cmd1[0];
		payload.frame1[4] = cmd1[1];
		payload.frame1[5] = cmd1[2];
		payload.frame1[6] = cmd1[3];
		payload.frame1[7] = cmd1[4];
	}
	if (cmd2 != NULL) {
		payload.frame2[0] = 0x7f;
		payload.frame2[1] = 0x7f;
		payload.frame2[2] = 0x7f;
		payload.frame2[3] = cmd2[0];
		payload.frame2[4] = cmd2[1];
		payload.frame2[5] = cmd2[2];
		payload.frame2[6] = cmd2[3];
		payload.frame2[7] = cmd2[4];
	}

	qint64 actual = udpSocket->writeDatagram((char*)&payload,sizeof(payload),metisHostAddress, metisPort);
	if (actual != sizeof(payload)) {
		qDebug()<<"Error sending command datagram: "<<udpSocket->errorString();
		return false;
	} else {
		return true;
	}
}

void HPSDRNetwork::NewUDPData()
{
	QHostAddress sender;
	quint16 senderPort;
	qint16 actual;
	qint64 dataGramSize = udpSocket->pendingDatagramSize();

	//We have to read entire datagram, then we can disect it
	actual = udpSocket->readDatagram((char *)dataGramBuffer,dataGramSize, &sender, &senderPort);
	if (actual != dataGramSize) {
		qDebug()<<"ReadDatagram size error";
		return;
	}
	//Make sure it's a valid Metis datagram
	if (dataGramBuffer[0] != 0xEF && dataGramBuffer[1] != 0xFE) {
		qDebug()<<"Received a non-metis datagram";
		return;
	}

	//Now check the datagram info byte to see what it is
	switch (dataGramBuffer[2]) {
		case 0x01: {
			//IQ or Bandscope data
			MetisToPcIQData *payload = (MetisToPcIQData *)dataGramBuffer;
			if (payload->endPoint == 0x04) {
				//Bandscope data (not used yet)
			} else if (payload->endPoint == 0x06) {
				//IQ data
				hpsdrDevice->ProcessInputFrame(payload->iqData.radio.IQframe1, 512);
				hpsdrDevice->ProcessInputFrame(payload->iqData.radio.IQframe2, 512);

			} else {
				qDebug()<<"Received invalid IQ datagram";
				return;
			}
			break;
		}

		//Response to SetIP has same info byte as discovery response
		case 0x02:
		case 0x03: {
			//Response to Discovery
			if (!waitingForDiscoveryResponse) {
				qDebug()<<"Received duplicate discovery datagram";
				return;
			}
			MetisToPcDiscoveryResponse *payload = (MetisToPcDiscoveryResponse *)dataGramBuffer;
			if (payload->info == 0x03)
				isSendingData = true;
			metisHostAddress = sender;
			metisPort = senderPort;
			metisBoardId = payload->boardId;
			metisFWVersion = payload->fwVersion;
			memcpy(metisMACAddress,payload->macAddress,6);
			waitingForDiscoveryResponse = false;
			break;
		}
		default: {
			qDebug()<<"Unkown Metis datagram";
			break;
		}
	}
}
