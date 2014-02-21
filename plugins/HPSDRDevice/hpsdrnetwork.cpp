#include "hpsdrnetwork.h"

HPSDRNetwork::HPSDRNetwork()
{
	waitingForDiscoveryResponse = false;

	udpSocket = new QUdpSocket();
}

HPSDRNetwork::~HPSDRNetwork()
{
}

#if 0

The Discovery packet is a UDP/IP frame sent to Ethernet address 255.255.255.255 (i.e. a Broadcast) and port 1024
with the following payload: <0xEFFE><0x02><60 bytes of 0x00>
#endif
bool HPSDRNetwork::Discovery()
{
	DiscoveryPayload payload;
	bool result;
	//Bind establishes the port we are listening to
	result = udpSocket->bind();
	connect(udpSocket, &QUdpSocket::readyRead, this, &HPSDRNetwork::NewUDPData);

	qint64 actual = udpSocket->writeDatagram((char*)&payload,sizeof(payload),QHostAddress::Broadcast,1024);
	if (actual == sizeof(payload)) {
		//Set flag waiting for response
		waitingForDiscoveryResponse = true;
	}
	return true;
}

void HPSDRNetwork::NewUDPData()
{
	if (waitingForDiscoveryResponse) {
		//qDebug()<<"New UDP data";
		DiscoveryResponsePayload payload;
		QHostAddress sender;
		quint16 senderPort;
		qint16 actual = udpSocket->readDatagram((char*)&payload,sizeof(payload),&sender, &senderPort);
		if (actual == sizeof(payload)) {
			metisHostAddress = sender;
			metisPort = senderPort;
			metisBoardId = payload.boardId;
			metisFWVersion = payload.fwVersion;
			memcpy(metisMACAddress,payload.macAddress,6);
			waitingForDiscoveryResponse = false;
		}
	}

}
