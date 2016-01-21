#ifndef HPSDRNETWORK_H
#define HPSDRNETWORK_H
#include <QThread>
#include <QUdpSocket>

class HPSDRDevice;

//<0xEFFE><0x02><60 bytes of 0x00>
struct PcToMetisDiscovery {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 info = 0x02;
	quint8 padding[60];
};
//<0xEFFE><0x04><Command>< 60 bytes of 0x00>
struct PcToMetisStart {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 info = 0x04;
	//bit [0] set starts I&Q + Mic data and bit [1] set starts the wide bandscope data
	//0x01 for IQ data only, 0x02 for Bandscope only, 0x03 for both
	quint8 command;
	quint8 padding[60];
};
//<0xEFFE><0x04><Command>< 60 bytes of 0x00>
struct PcToMetisStop {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 info = 0x04;
	//bit [0] clear stops I&Q + Mic data and bit [1] clear stops the wide bandscope data
	//0x02 to stop IQ only, 0x01 to stop Bandscope only, 0x00 to stop both
	quint8 command;
	quint8 padding[60];
};
//<0xEFFE>< 0x03>< MAC Address><IP Address to set><60 bytes of 0x00>
struct PcToMetisSetIP {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 info = 0x03;
	quint8 macAddress[6];
	quint8 ipAddress[4];
	quint8 padding[60];
};

//PC to Metis
//<0xEFFE><0x01><End Point><Sequence Number>< 2 x HPSDR frames>
struct PcToMetisData {
	const quint8 header[2] = {0xEF,0xFE};
	const quint8 info = 0x01;
	const quint8 endPoint = 0x02; //0x02 – representing USB EP2
	//32 bit unsigned integer starting at zero and incremented each frame – bytes are in network order [big-endian]
	quint32 sequenceNumber; //Must be sequentially set by PC
	quint8 frame1[512];
	quint8 frame2[512];
};

//<0xEFFE><Status>< Metis MAC Address><Code Version><Board_ID><49 bytes of 0x00>
struct MetisToPcDiscoveryResponse {
	quint8 header[2];
	quint8 info;	//0x02 Metis not sending data, 0x03 Metis is sending data
	quint8 macAddress[6];
	quint8 fwVersion;
	quint8 boardId;
	quint8 padding[49];
};

//<0xEFFE>< 0x02>< MAC Address>< 60 bytes of 0x00>
struct MetisToPcSetIPResponse {
	quint8 header[2];
	quint8 info;	//
	quint8 macAddress[6];
	quint8 padding[60];
};
//Metis to PC
//<0xEFFE><0x01><End Point 6><Sequence Number><2 x HPSDR frames>
struct MetisToPcIQData {
	quint8 header[2];
	quint8 info;
	quint8 endPoint; //0x06 for IQ data, 0x04 for Bandscope data
	//32 bit unsigned integer starting at zero and incremented each frame – bytes are in network order [big-endian]
	quint32 sequenceNumber;
	union {
		struct  {
			quint8 IQframe1[512];
			quint8 IQframe2[512];
		}radio;
		quint16 bandscopeIQ[512];

	}iqData;
};

class HPSDRNetwork: public QObject
{
	Q_OBJECT
public:
	HPSDRNetwork();
	~HPSDRNetwork();

	bool SendDiscovery();

	bool Init(HPSDRDevice *_hpsdrDevice, QString _metisAddress, quint16 _metisPort);
	bool SendStart();
	bool SendStop();
	bool SendCommand(char *cmd1, char *cmd2 = NULL);
public slots:
	void NewUDPData();
private:
	QUdpSocket *udpSocket;
	quint32 udpSequenceNumberIn;
	quint32 udpSequenceNumberOut;

	//This is based on datagram size not fft size.  Ok to hard code
	quint8 dataGramBuffer[2048]; //2x bigger than largest we expect, used to read raw datagram

	QHostAddress metisHostAddress;
	quint16 metisPort;
	quint8 metisMACAddress[6];
	quint16 metisFWVersion;
	quint16 metisBoardId;

	bool waitingForDiscoveryResponse;
	bool isBound; //Socket is listening for specific IP/Port address in datagrams
	bool isSendingData;
	bool isRunning;

	HPSDRDevice *hpsdrDevice;
};

#endif // HPSDRNETWORK_H
