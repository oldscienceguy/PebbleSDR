#ifndef RFSPACEDEVICE_H
#define RFSPACEDEVICE_H

#include "afedri.h"

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "usbutil.h"
#include "cpx.h"
#include "ad6620.h"
#include "ui_sdriqoptions.h"
#include <QTcpSocket>
#include <QUdpSocket>

//This should go in global.h or somewhere we can use everywhere
#ifdef _WIN32
	//gcc and clang use __attribute, define it for windows as noop
	#define packStruct
	//Use within struct definition
	#define packStart pack(push, 1)
	#define packEnd pragma pack(pop)
#else
	#define packStart
	#define packEnd
	//Use at end of struct definition
	#define packStruct __attribute__((packed))
#endif

//Host to Target(Device)
enum HostHeaderTypes {
	SetControlItem = 0x00,
	RequestCurrentControlItem = 0x01,
	RequestControlItemRange = 0x02,
	DataItemACKHostToTarget = 0x03,
	HostDataItem0 = 0x04,
	HostDataItem1 = 0x05,
	HostDataItem2 = 0x06,
	HostDataItem3 = 0x07,
};
//Targt (Device) to Host
enum TargetHeaderTypes {
	ResponseControlItem = 0x00,
	UnsolicitedControlItem = 0x01,
	ResponseControlItemRange = 0x02,
	DataItemACKTargetToHost = 0x03,
	TargetDataItem0 = 0x04,
	TargetDataItem1 = 0x05,
	TargetDataItem2 = 0x06,
	TargetDataItem3 = 0x07,
};

//Unpacked, not as received
struct ControlHeader {
	bool gotHeader;
	quint8 type;
	quint16 length;
};

//Simple Network Discovery Protocol
//
struct DISCOVER_MSG
{
	packStart
	//Fixed common fields (56 bytes)
	quint16 length; //length of total message in bytes (little endian byte order)
	quint8 key[2]; //fixed key key[0]==0x5A key[1]==0xA5
	quint8 op; //0==Request(to device) 1==Response(from device) 2 ==Set(to device)
	unsigned char name[16]; //Device name string null terminated
	unsigned char sn[16]; //Serial number string null terminated
	//device IP address (little endian byte order)
	union {
		struct {
			quint32 addr;
			quint8 unused[12];
		}ip4;
		quint8 ip6Addr[16];
	}ipAddr;
	quint16 port; //device Port number (little endian byte order)
	quint8 customfield; //Specifies a custom data field for a particular device
	//start of optional variable custom byte fields
	//unsigned char custom1;
	packEnd
}packStruct;
//}__attribute__((packed));

//Optional fields for SDR-IP
struct DISCOVER_MSG_SDRIP
{
	packStart
	unsigned char macaddr[6];	//HW mac address (little endian byte order) (read only)
	quint16 hwver;				//Hardware version*100  (little endian byte order) (read only)
	quint16 fwver;				//Firmware version*100 (little endian byte order)(read only)
	quint16 btver;				//Boot version*100 (little endian byte order) (read only)
	unsigned char fpgaid;		//FPGA ID (read only)
	unsigned char fpgarev;		//FPGA revision (read only)
	unsigned char opts;			//Options (read only)
	unsigned char mode;			//0 == Use DHCP 1==manual  2==manual Alternate data address
	unsigned char subnet[4];	//IP subnet mask (little endian byte order)
	unsigned char gwaddr[4];	//gateway address (little endian byte order)
	unsigned char dataipaddr[4];// Alternate data IP address for UDP data  (little endian byte order)
	unsigned char dataport[2];	// Alternate data Port address for UDP (little endian byte order)
	unsigned char fpga;			//0 == default cfg   1==custom1    2==custom2
	unsigned char status;		//bit 0 == TCP connected   Bit 1 == running  Bit 2-7 not defined
	unsigned char future[15];	//future use
	packEnd
}packStruct;

struct DISCOVER_MSG_SDRIQ
{
	packStart
	quint16 fwver;		//Firmware version*100 (little endian byte order)(read only)
	quint16 btver;		//Boot version*100 (little endian byte order) (read only)
	unsigned char subnet[4];	//IP subnet mask (little endian byte order)
	unsigned char gwaddr[4];	//gateway address (little endian byte order)
	char connection[32];		//interface connection string null terminated(ex: COM3, DEVTTY5, etc)
	unsigned char status;		//bit 0 == TCP connected   Bit 1 == running  Bit 2-7 not defined
	unsigned char future[15];	//future use
	packEnd
}packStruct;

class RFSpaceDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	//Data blocks are fixed at 8192 bytes by SDR-IQ
	//4096 samples at 2 bytes (short) per sample
	static const quint16 samplesPerDataBlock = 4096; //shorts
	static const quint16 dataBlockSize = samplesPerDataBlock * 2;

	RFSpaceDevice();
	~RFSpaceDevice();

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

	static void UnpackHeader(quint8 byte0, quint8 byte1, ControlHeader *unpacked);
	static void DoubleToBuf(unsigned char *buf, double value);
protected:
	void InitSettings(QString fname);
private slots:
	void rfGainChanged(int i);
	void ifGainChanged(int i);
	void discoverBoxChanged(bool b);
	void IPAddressChanged();
	void IPPortChanged();

	void TCPSocketError(QAbstractSocket::SocketError socketError);
	void TCPSocketConnected();
	void TCPSocketDisconnected();
	void TCPSocketNewData();
	void UDPSocketNewData();


private:
	enum SDRDEVICE {SDR_IQ =0, SDR_IP, AFEDRI_USB};

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	USBUtil *usbUtil;
	AD6620 *ad6620;

	ControlHeader header;

	QSettings *sdripSettings;
	QSettings *sdriqSettings;
	QSettings *afedri_usbSettings;

	//Capture N blocks of data
	bool CaptureBlocks(unsigned numBlocks);
	bool StartCapture();
	bool StopCapture();
	bool ReadDataBlock(CPX cpxBuf[], unsigned cpxBufSize);
	void FlushDataBlocks();
	//Write 5 bytes of data
	bool SetRegisters(unsigned adrs, qint32 data);
	bool SetAD6620Raw(unsigned adrs, short d0, short d1, short d2, short d3, short d4);
	bool SetRFGain(qint8 g);
	unsigned GetRFGain();
	bool SetIFGain(qint8 g);
	//SDR-IQ General Control Items
	bool RequestTargetName();
	bool RequestTargetSerialNumber();
	bool RequestInterfaceVersion(); //Version * 100, ie 1.23 returned as 123
	bool RequestFirmwareVersion();
	bool RequestBootcodeVersion();
	bool RequestStatusCode();
	bool RequestStatusString(unsigned char code);


	Ui::SdrIqOptions *optionUi;

	QMutex mutex;
	CPX *inBuffer;
	int inBufferSize;
	unsigned char *readBuf;
	unsigned char *usbReadBuf; //Exclusively for producer thread to avoid possible buffer contention with main thread
	unsigned char *udpReadBuf;
	unsigned char *tcpReadBuf;

	//Returned data values from SDR-IQ
	QString targetName;
	QString serialNumber;
	unsigned interfaceVersion;
	unsigned firmwareVersion;
	unsigned bootcodeVersion;
	unsigned hardwareVersion;
	unsigned statusCode;
	QString statusString;
	unsigned lastReceiverCommand;
	double receiverFrequency;
	int rfGain;
	int ifGain;

	//Settings
	AD6620::BANDWIDTH sBandwidth;
	int sRFGain;
	int sIFGain;

	bool SetFrequency(double freq);
	bool GetFrequency();

	//SDR-IP Specific
	QTcpSocket *tcpSocket;
	QUdpSocket *udpSocket;
	QHostAddress deviceAddress;
	quint16 devicePort;
	bool autoDiscover;
	QHostAddress deviceDiscoveredAddress;
	quint16 deviceDiscoveredPort;

	quint16 readBufferIndex; //Used to track whether we have full buffer or not, 0 to readBufferSize-1
	char *producerFreeBufPtr;

	AFEDRI *afedri; //USB HID protocol and special features

	bool SendTcpCommand(void *buf, qint64 len);
	bool SendUsbCommand(void *buf, qint64 len);
	void processControlItem(ControlHeader header, unsigned char *buf);
	bool SetSampleRate();
	bool SetUDPAddressAndPort(QHostAddress address, quint16 port);
	bool SendAck();
	void DoConsumer();
	void DoUSBProducer();
	void DoUDPProducer();
	bool SetADSampleRate();
	bool SendUDPDiscovery();
};


#endif // EXAMPLESDRDEVICE_H
