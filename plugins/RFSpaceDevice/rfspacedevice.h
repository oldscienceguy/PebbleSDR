#ifndef RFSPACEDEVICE_H
#define RFSPACEDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "producerconsumer.h"
#include "usbutil.h"
#include "cpx.h"
#include "ad6620.h"
#include "ui_sdriqoptions.h"
#include <QTcpSocket>
#include <QUdpSocket>

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
	quint8 type;
	quint16 length;
};


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
	bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer);
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
private slots:
	void rfGainChanged(int i);
	void ifGainChanged(int i);

private:
	enum CONNECTION_TYPE {UNKNOWN, USB, TCP};
	CONNECTION_TYPE connectionType;

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	ProducerConsumer producerConsumer;
	USBUtil *usbUtil;
	AD6620 *ad6620;

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
	unsigned char *readBufProducer; //Exclusively for producer thread to avoid possible buffer contention with main thread

	//Returned data values from SDR-IQ
	QString targetName;
	QString serialNumber;
	unsigned interfaceVersion;
	unsigned firmwareVersion;
	unsigned bootcodeVersion;
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

	bool SendTcpCommand(void *buf, qint64 len);
};


#endif // EXAMPLESDRDEVICE_H
