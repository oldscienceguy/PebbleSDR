#ifndef GHPSDR3DEVICE_H
#define GHPSDR3DEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include <QHostAddress>
#include <QTcpSocket>
//#include <QUdpSocket>


#if 0
On line servers can be found at http://qtradio.napan.ca/qtradio/qtradio.pl
74.85.89.174 is a Hermes transceiver that is good for testing
71.47.206.230 is SDR-IQ

Documentation may not be up to date, look at client.c readcb() in ghpsdr3 source
	'!' Documented

	First byte == '*' signals hardware directed command
	First byte == 'q' signals ???
	"mic" Incoming microphone data
	! "getspectrum"
	! "setfrequency"
	"setpreamp"
	"setmode"
	"setfilter"
	"setagc"
	"setfixedagc"
	"enablenotchfilter"
	"setnotchfilter"
	"setagctlevel"
	"setrxgreqcmd"
	"setrx3bdgreq"
	"setrx10bdgreq"
	"settxgreqcmd"
	"settx3bdgreq"
	"settx10bdgreq"
	"setnr"
	"setnb"
	"setsdrom"
	"setanf"
	"setrxoutputgain"
	"setsubrxoutputgain"
	! "startaudiostream" <bufferSize> <sampleRate> <audioChannel> <micEncoding>
	! "startrtpstream" <rtpPort> <encoding> <rate> <channels>
	"stopaudiostream"
	"setencoding"
	"setsubrx"
	"setsubrxfrequency"
	"setpan"
	"setsubrxpan"
	"record"
	"setanfvals"
	"setnrvals"
	"setrxagcattack"
	"setrxagcdecay"
	"setrxagchang"
	"setrxagchanglevel"
	"setrxagchangthreshold"
	"setrxagcthresh"
	"setrxagcmaxgain"
	"setrxagcslope"
	"setnbvals"
	"setsdromvals"
	"setpwsmode"
	"setbin"
	"settxcompst"
	"settxcompvalue"
	"setoscphase"
	"setPanaMode"
	"setRxMeterMode"
	"setTxMeterMode"
	"setrxdcblockgain"
	"setrxdcblock"
	"settxdcblock"
	"mox"
	"off"
	"settxamcarrierlevel"
	"settxalcstate"
	"settxalcattack"
	"settxalcdecay"
	"settxalcbot"
	"settxalchang"
	"settxlevelerstate"
	"settxlevelerattack"
	"settxlevelerdecay"
	"settxlevelerhang"
	"settxlevelermaxgain"
	"settxagcff"
	"settxagcffcompression"
	"setcorrecttxiqmu"
	"setcorrecttxiqw"
	"setfadelevel"
	"setsquelchval"
	"setsubrxquelchval"
	"setsquelchstate"
	"setsubrxquelchstate"
	"setspectrumpolyphase"
	"setocoutputs"
	"setclient"
	"setiqenable"
	"testbutton"
	"testslider"
	"rxiqmuval"
	"txiqcorrectval"
	"txiqphasecorrectval"
	"txiqgaincorrectval"
	"rxiqphasecorrectval"
	"rxiqgaincorrectval"
	"rxiqcorrectwr"
	"rxiqcorrectwi"
	! "setfps" Spectrum fps
	"zoom"
	"setmaster"



#endif

class Ghpsdr3Device : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	Ghpsdr3Device();
	~Ghpsdr3Device();

	//Required
	bool Initialize(cbProcessIQData _callback,
					cbProcessBandscopeData _callbackBandscope,
					cbProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool Connect();
	bool Disconnect();
	void Start();
	void Stop();
	void ReadSettings();
	void WriteSettings();
	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);
	//Display device option widget in settings dialog
	void SetupOptionUi(QWidget *parent);

private slots:
	void TCPSocketError(QAbstractSocket::SocketError socketError);
	void TCPSocketConnected();
	void TCPSocketDisconnected();
	void TCPSocketNewData();
	//void UDPSocketNewData();

private:
	//48 byte header returned in every tcp response from dspserver
	struct dspServerHeader {
		packStart
		//0 Packet type: 0=spectrum data, 1=audio data 1-31 Reserved
		quint8 packetType;
		quint8 reserved[31];
		//32-39 Sample rate in ASCII characters
		unsigned char asciiSampleRate[8];
		//40-47 Meter reading in ASCII characters
		unsigned char asciiMeter[8];
		//48-... spectrum or audio data
		packEnd
	}packStruct;

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	QTcpSocket *tcpSocket;
	QHostAddress deviceAddress;
	quint16 devicePort;
	static const quint16 tcpHeaderSize = 48; //Must match size of packed structure
	static const quint16 tcpDataBufSize = 480; //Number bytes of data in each packet
	static const quint16 tcpReadBufSize = tcpDataBufSize + tcpHeaderSize; //Data buffer size + prefix
	unsigned char tcpReadBuf[tcpReadBufSize];

	//QUdpSocket *udpSocket;
	QMutex mutex;

};
#endif // GHPSDR3DEVICE_H
