#ifndef HPSDRDEVICE_H
#define HPSDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_hpsdroptions.h"
#include "usbutil.h"
#include "cpx.h"
#include "hpsdrnetwork.h"

class HPSDRDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	friend class HPSDRNetwork;

	const int MAX_PACKET_SIZE = 64;

	const int IN_ENDPOINT2 = 0x82;
	const int OUT_ENDPOINT2 = 0x02; //Commands
	const int IN_ENDPOINT4 = 0x84; //Received bandscope data
	const int OUT_ENDPOINT4 = 0x04;
	const int IN_ENDPOINT6 = 0x86; //Received IQ data
	const int OUT_ENDPOINT6 = 0x06;

	const int OZY_FRAME_SIZE = 512; //Size of an Ozy frame

	//USB_TYPE_VENDOR | USB_ENDPOINT_IN | USB_RECIP_DEVICE
	const int VENDOR_REQ_TYPE_IN = 0xc0;
	//USB_TYPE_VENDOR | USB_ENDPOINT_OUT | USB_RECIP_DEVICE
	const int VENDOR_REQ_TYPE_OUT = 0x40;


	//Ozy Vendor req commands
	const int OZY_SET_LED = 0x01;
	const int OZY_LOAD_FPGA = 0x02;
	//Index options for LOAD_FPGA command, Value=0
	const int FL_BEGIN = 0;
	const int FL_XFER = 1;
	const int FL_END = 2;

	const int OZY_I2C_WRITE = 0x08;
	const int OZY_SDR1K_CTL = 0x0d;
	//Value options for SDR1K_CTL, Index = 0
	const int SDR1K_CTL_READ_VERSION = 0x7;

	const int OZY_WRITE_RAM = 0xa0;

	const int OZY_TIMEOUT = 500; //Default timeout in ms

	const int OZY_SYNC = 0x7f;

	//Commands
	const int C0_MOX_INACTIVE = 0x00;	//00000000
	const int C0_MOX_ACTIVE = 0x01;		//00000001

	const int C0_TX_FREQ  = 0x02;		//0000001x
	const int C0_RX1_FREQ = 0x04;		//0000010x
	const int C0_RX2_FREQ = 0x06;		//0000011x
	const int C0_RX3_FREQ = 0x08;		//0000100x
	const int C0_RX4_FREQ = 0x0a;		//0000101x
	const int C0_RX5_FREQ = 0x0c;		//0000110x
	const int C0_RX6_FREQ = 0x0e;		//0000111x
	const int C0_RX7_FREQ = 0x10;		//0001000x

	const int C0_EXTRA1 = 0x12;			//0001001x
	const int C0_EXTRA2 = 0x14;			//0001010x

	const int C1_SPEED_48KHZ = 0x00;	//00000000
	const int C1_SPEED_96KHZ = 0x01;	//00000001
	const int C1_SPEED_192KHZ = 0x02;	//00000010
	const int C1_SPEED_384KHZ = 0x03;	//00000011

	//What 10mhz clock do we use
	const int C1_10MHZ_ATLAS = 0x00;	//000000xx
	const int C1_10MHZ_PENELOPE = 0x04;	//000001xx
	const int C1_10MHZ_MERCURY = 0x08;	//000010xx

	//What 122.8 hhz clock do we use
	const int C1_122MHZ_PENELOPE = 0x00;//0000xxxx
	const int C1_122MHZ_MERCURY = 0x10;	//0001xxxx

	//Atlas Bus Config
	const int C1_CONFIG_NONE = 0x00;	//000xxxxx
	const int C1_CONFIG_PENELOPE = 0x20;//001xxxxx
	const int C1_CONFIG_MERCURY = 0x40;	//010xxxxx
	const int C1_CONFIG_BOTH = 0x60;	//011xxxxx

	//Mic source
	const int C1_MIC_JANUS = 0x00;		//0xxxxxxx
	const int C1_MIC_PENELOPE = 0x80;	//1xxxxxxx

	const int C2_MODE_OTHERS = 0x00;
	const int C2_MODE_CLASSE = 0x01;

	//Alex attenuator
	const int C3_ALEX_0DB = 0x00;			//00000000
	const int C3_ALEX_10DB = 0x01;			//00000001
	const int C3_ALEX_20DB = 0x02;			//00000010
	const int C3_ALEX_30DB = 0x03;			//00000011
	//LT2208 ADC
	const int C3_LT2208_PREAMP_OFF = 0x00;	//000000xx
	const int C3_LT2208_PREAMP_ON = 0x04;	//000001xx
	const int C3_LT2208_DITHER_OFF = 0x0;	//00000xxx
	const int C3_LT2208_DITHER_ON = 0x08;	//00001xxx
	const int C3_LT2208_RANDOM_OFF = 0x00;	//0000xxxx
	const int C3_LT2208_RANDOM_ON = 0x10;	//0001xxxx
	//Alex antenna
	const int C3_ALEX_RXANT_NONE = 0x00;	//000xxxxx
	const int C3_ALEX_RXANT_RX1 = 0x20;		//001xxxxx
	const int C3_ALEX_RXANT_RX2 = 0x40;		//010xxxxx
	const int C3_ALEX_RXANT_XV = 0x60;		//011xxxxx

	const int C3_ALEX_RXOUT_OFF = 0x00;		//0xxxxxxx
	const int C3_ALEX_RXOUT_ON = 0x80;		//1xxxxxxx

	const int C4_ALEX_TXRELAY_TX1 = 0x00;
	const int C4_ALEX_TXRELAY_TX2 = 0x01;
	const int C4_ALEX_TXRELAY_TX3 = 0x02;

	const int C4_DUPLEX_OFF = 0x00;
	const int C4_DUPLEX_ON = 0x04;

	const int C4_1RECEIVER = 0x00; //000xxx
	const int C4_2RECEIVER = 0x08; //001xxx
	const int C4_3RECEIVER = 0x10; //010xxx
	const int C4_4RECEIVER = 0x18; //011xxx
	const int C4_5RECEIVER = 0x20; //100xxx
	const int C4_6RECEIVER = 0x28; //101xxx
	const int C4_7RECEIVER = 0x30; //110xxx
	const int C4_8RECEIVER = 0x38; //111xxx

	const int C4_CommonFreq = 0x80; //All receivers get same frequency

	HPSDRDevice();
	~HPSDRDevice();

	//Required
	bool initialize(CB_ProcessIQData _callback,
					CB_ProcessBandscopeData _callbackBandscope,
					CB_ProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	QVariant get(StandardKeys _key, QVariant _option = 0);
	bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

private slots:
	//Options dialog
	void preampChanged(bool b);
	void ditherChanged(bool b);
	void randomChanged(bool b);
	void optionsAccepted();
	void useDiscoveryChanged(bool b);
	void useOzyChanged(bool b);
	void useMetisChanged(bool b);
	void metisAddressChanged();
	void metisPortChanged();


private:
	bool connectDevice();
	bool disconnectDevice();
	void startDevice();
	void stopDevice();
	void readSettings();
	void writeSettings();
	//Display device option widget in settings dialog
	void setupOptionUi(QWidget *parent);

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	USBUtil usbUtil;

	bool Open();
	bool Close();

	bool SendConfig();

	int WriteRam(int fx2StartAddr, unsigned char *buf, int count);
	int WriteI2C(int i2c_addr, unsigned char byte[], int length);

	bool ResetCPU(bool reset);
	bool LoadFpga(QString filename);

	int HexByteToUInt(char c);
	int HexBytesToUInt(char * buf, int len);
	bool LoadFirmware(QString filename);

	bool SetLED(int which, bool on);

	QString GetFirmwareInfo();

	bool WriteOutputBuffer(char * commandBuf);
	bool ProcessInputFrame(unsigned char *buf, int len);

	int sampleCount;

	//Buffer for outbound Ozy  traffic
	unsigned char *outputBuffer;
	quint16 inputBufferSize;
	unsigned char *inputBuffer;
	CPX *producerFreeBufPtr;

	//Settings
	//GetGain() options used by processBlock
	double sGainPreampOn; //If preamp on, use lower gain
	double sGainPreampOff;
	//LibUSB PID VID to look for.  Change in hpsdr.ini if necessary
	int sPID;
	int sVID;
	QString sFpga; //Ozy FPGA .rbf file
	QString sFW; //Ozy Firmware .hex file
	bool sNoInit;

	//Hardware config settings
	int sPTT;
	int sSpeed;
	int s10Mhz;
	int s122Mhz;
	int sConfig;
	int sMic;

	int sMode;
	int sPreamp;
	int sDither;
	int sRandom;

	//HPSDR status
	QString ozyFX2FW; //Ozy firmware string
	int ozyFW;
	int mercuryFW;
	int penelopeFW;
	int metisFW;
	int janusFW;

	Ui::HPSDROptions *optionUi;

	bool forceReload; //true will force firmware to be uploaded every time

	//HPSDR Network
	enum CONNECTION_TYPE {UNKNOWN, OZY, METIS};
	CONNECTION_TYPE connectionType;
	enum DISCOVERY {USE_AUTO_DISCOVERY, USE_OZY, USE_METIS};
	DISCOVERY discovery;
	HPSDRNetwork hpsdrNetwork;

	QString metisAddress;
	quint16 metisPort;

	bool ConnectUsb();
	bool ConnectTcp();
};
#endif // HPSDRDEVICE_H
