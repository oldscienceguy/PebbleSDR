#ifndef HPSDRNETWORK_H
#define HPSDRNETWORK_H

#include <QUdpSocket>

//<0xEFFE><0x02><60 bytes of 0x00>
struct DiscoveryPayload {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 status = 0x02;
	quint8 padding[60];
};
//<0xEFFE><Status>< Metis MAC Address><Code Version><Board_ID><49 bytes of 0x00>
struct DiscoveryResponsePayload {
	quint8 header[2];
	quint8 status;	//0x02 Metis not sending data, 0x03 Metis is sending data
	quint8 macAddress[6];
	quint8 fwVersion;
	quint8 boardId;
	quint8 padding[49];
};
//<0xEFFE><0x04><Command>< 60 bytes of 0x00>
struct StartPayload {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 status = 0x04;
	//bit [0] set starts I&Q + Mic data and bit [1] set starts the wide bandscope data
	quint8 command;
	quint8 padding[60];
};
//<0xEFFE><0x04><Command>< 60 bytes of 0x00>
struct StopPayload {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 status = 0x04;
	//bit [0] clear stops I&Q + Mic data and bit [1] clear stops the wide bandscope data
	quint8 command;
	quint8 padding[60];
};
//<0xEFFE>< 0x03>< MAC Address><IP Address to set><60 bytes of 0x00>
struct SetIPPayload {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 status = 0x03;
	quint8 macAddress[6];
	quint8 ipAddress[4];
	quint8 padding[60];
};
//<0xEFFE>< 0x02>< MAC Address>< 60 bytes of 0x00>
struct SetIPResponsePayload {
	quint8 header[2];
	quint8 status;	//
	quint8 macAddress[6];
	quint8 padding[60];
};
//PC to Metis
//<0xEFFE><0x01><End Point><Sequence Number>< 2 x HPSDR frames>
struct UDPDataPayload {
	quint8 header[2];
	quint8 status;
	quint8 endPoint; //0x02 – representing USB EP2
	//32 bit unsigned integer starting at zero and incremented each frame – bytes are in network order [big-endian]
	quint8 sequenceNumber[4];
	quint8 frame1[512];
	quint8 frame2[512];
};
//Metis to PC
//<0xEFFE><0x01><End Point><Sequence Number><2 x HPSDR frames>
struct IQDataPayload {
	quint8 header[2];
	quint8 status;
	quint8 endPoint; //0x02 – representing USB EP2
	//32 bit unsigned integer starting at zero and incremented each frame – bytes are in network order [big-endian]
	quint8 sequenceNumber[4];
	quint8 frame1[512];
	quint8 frame2[512];
};
//Metis to PC
//<0xEFFE><0x01><End Point><Sequence Number><512 x 16 bit samples>
struct BandscopePayload {
	quint8 header[2];
	quint8 status;
	quint8 endPoint; //0x02 – representing USB EP2
	//32 bit unsigned integer starting at zero and incremented each frame – bytes are in network order [big-endian]
	quint8 sequenceNumber[4];
	quint16 frame1[512];
};

class HPSDRNetwork: public QObject
{
	Q_OBJECT
public:
	HPSDRNetwork();
	~HPSDRNetwork();

	bool Discovery();

public slots:
	void NewUDPData();
private:
	QUdpSocket *udpSocket;
	QHostAddress metisHostAddress;
	quint16 metisPort;
	quint8 metisMACAddress[6];
	quint16 metisFWVersion;
	quint16 metisBoardId;

	bool waitingForDiscoveryResponse;
};

#endif // HPSDRNETWORK_H
