#ifndef HPSDR_H
#define HPSDR_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QSettings>
#include <QSemaphore>
#include "SDR.h"
#include "ui/ui_hpsdroptions.h"


class HPSDR:public SDR
{
	Q_OBJECT
public:
	static const int MAX_PACKET_SIZE = 64;

	static const int IN_ENDPOINT2 = 0x82;
	static const int OUT_ENDPOINT2 = 0x02; //Commands
	static const int IN_ENDPOINT4 = 0x84; //Received bandscope data
	static const int OUT_ENDPOINT4 = 0x04; //Received bandscope data
	static const int IN_ENDPOINT6 = 0x86; //Received IQ data
	static const int OUT_ENDPOINT6 = 0x06; //Received bandscope data

	static const int BUFSIZE = 512; //Size of an Ozy frame
	static const int NUMFRAMES = 24; //Number of frames to fetch at once to get 2048 samples

	//USB_TYPE_VENDOR | USB_ENDPOINT_IN | USB_RECIP_DEVICE
	static const int VENDOR_REQ_TYPE_IN = 0xc0;
	//USB_TYPE_VENDOR | USB_ENDPOINT_OUT | USB_RECIP_DEVICE
	static const int VENDOR_REQ_TYPE_OUT = 0x40;


	//Ozy Vendor req commands
	static const int OZY_SET_LED = 0x01;
	static const int OZY_LOAD_FPGA = 0x02;
	//Index options for LOAD_FPGA command, Value=0
	static const int FL_BEGIN = 0;
	static const int FL_XFER = 1;
	static const int FL_END = 2;

	static const int OZY_I2C_WRITE = 0x08;
	static const int OZY_SDR1K_CTL = 0x0d;
	//Value options for SDR1K_CTL, Index = 0
	static const int SDR1K_CTL_READ_VERSION = 0x7;

	static const int OZY_WRITE_RAM = 0xa0;

	static const int OZY_TIMEOUT = 500; //Default timeout in ms

	static const int OZY_SYNC = 0x7f;

	//Commands
	static const int C0_MOX_INACTIVE = 0;
	static const int C0_MOX_ACTIVE = 1;
	static const int C0_FREQUENCY = 2; //If this is set, C1-C4 has frequency to set

	static const int C1_SPEED_48KHZ = 0x00;
	static const int C1_SPEED_96KHZ = 0x01;
	static const int C1_SPEED_192KHZ = 0x02;
	//What 10mhz clock do we use
	static const int C1_10MHZ_ATLAS = 0x00; //Excalibur
	static const int C1_10MHZ_PENELOPE = 0x04;
	static const int C1_10MHZ_MERCURY = 0x08;
	//What 122.8 hhz clock do we use
	static const int C1_122MHZ_PENELOPE = 0x00;
	static const int C1_122MHZ_MERCURY = 0x10;
	//Atlas Bus Config
	static const int C1_CONFIG_NONE = 0x00;
	static const int C1_CONFIG_PENELOPE = 0x20;
	static const int C1_CONFIG_MERCURY = 0x40;
	static const int C1_CONFIG_BOTH = 0x60;
	//Mic source
	static const int C1_MIC_JANUS = 0x00;
	static const int C1_MIC_PENELOPE = 0x80;

	static const int C2_MODE_OTHERS = 0x00;
	static const int C2_MODE_CLASSE = 0x01;

	//Alex attenuator
	static const int C3_ALEX_0DB = 0x00;
	static const int C3_ALEX_10DB = 0x01;
	static const int C3_ALEX_20DB = 0x02;
	static const int C3_ALEX_30DB = 0x03;
	//LT2208 ADC
	static const int C3_LT2208_PREAMP_OFF = 0x00;
	static const int C3_LT2208_PREAMP_ON = 0x04;
	static const int C3_LT2208_DITHER_OFF = 0x0;
	static const int C3_LT2208_DITHER_ON = 0x08;
	static const int C3_LT2208_RANDOM_OFF = 0x00;
	static const int C3_LT2208_RANDOM_ON = 0x10;
	//Alex antenna
	static const int C3_ALEX_RXANT_NONE = 0x00;
	static const int C3_ALEX_RXANT_RX1 = 0x20;
	static const int C3_ALEX_RXANT_RX2 = 0x40;
	static const int C3_ALEX_RXANT_XV = 0x60;

	static const int C3_ALEX_RXOUT_OFF = 0x00;
	static const int C3_ALEX_RXOUT_ON = 0x80;

	static const int C4_ALEX_TXRELAY_TX1 = 0x00;
	static const int C4_ALEX_TXRELAY_TX2 = 0x01;
	static const int C4_ALEX_TXRELAY_TX3 = 0x02;

	static const int C4_DUPLEX_OFF = 0x00;
	static const int C4_DUPLEX_ON = 0x04;

	static const int C4_1RECEIVER = 0x00;
	static const int C4_8RECEIVER = 0x38;

	HPSDR(Receiver *_receiver, SDRDEVICE dev,Settings *settings);
	~HPSDR();
	//SDR class overrides
	bool Connect();
	bool Disconnect();
	void Start();
	void Stop();

	double SetFrequency(double fRequested, double fCurrent);
	void ShowOptions();
	double GetStartupFrequency();
	int GetStartupMode();
	double GetHighLimit();
	double GetLowLimit();
	double GetGain();
	QString GetDeviceName();
	int GetSampleRate(); //Base override so we get match SR to HPSDR BW
    bool UsesAudioInput() {return false;}

	void ReadSettings();
	void WriteSettings();

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

	bool SetSpeed(int speed);


	private slots:
	//Options dialog
	void preampChanged(bool b);
	void ditherChanged(bool b);
	void randomChanged(bool b);
	void optionsAccepted();


private:
	bool WriteOutputBuffer(char * commandBuf);
    bool ProcessInputFrame(unsigned char *buf, int len);

	//Producer acquires (with block) semDataBuf to make sure there is an empty buffer available
	//Consumer releases semDataBuf when done processing to make it available
	QSemaphore semDataBuf; //Init to NUMDATABUFS
	//Producer releases semDataReady to indicate new buffer is ready
	//Consumer acquires (with block) semDataRead to make sure there is data ready
	QSemaphore semDataReady;

	int sampleCount;
	static const int NUMCPXBUFS = 100;
	//At any given time consumer may not be keeping up with producer
	//Circular buffer of data buffers allows producer/consumer rates to vary for a short time
	CPX dataBuf[NUMCPXBUFS][2048];
	//Producer puts data into dataBuf[nextDataBuf] then increments/wraps nextDataBuf
	int nextDataBuf;
	//Consumer pulls data from dataBuf[lastDataBuf] then increments/wraps lastDataBuf
	int lastDataBuf; //Values from 0 to NUMDATABUFS

	CPX outBuffer[2048];

#ifdef LIBUSB_VERSION1
    libusb_device *dev;
    libusb_device_handle* hDev;
    //SDR-Widget testing
    libusb_device_handle* swhDev; //DG8SAQ protocol
    libusb_device *swdev;

#else
	struct usb_device *dev;
	usb_dev_handle* hDev;
	//SDR-Widget testing
	usb_dev_handle* swhDev; //DG8SAQ protocol
	struct usb_device *swdev;    
#endif
	//Buffer for outbound Ozy  traffic
    unsigned char outputBuffer[BUFSIZE]; //
    unsigned char inputBuffer[BUFSIZE * NUMFRAMES]; //2048 samples

	//Settings
	double sStartup;
	double sLow;
	double sHigh;
	int sStartupMode;
	//GetGain() options used by processBlock
	double sGainPreampOn; //If preamp on, use lower gain
	double sGainPreampOff;
	//LibUSB PID VID to look for.  Change in hpsdr.ini if necessary
	int sPID;
	int sVID;
	QString sFpga; //Ozy FPGA .rbf file
	QString sFW; //Ozy Firmware .hex file
	bool sNoInit;
	bool sSDRW_DG8SAQ_SetFreq;

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

	//SDR overrides so we get called
	void StopProducerThread();
	void RunProducerThread();
	void StopConsumerThread();
	void RunConsumerThread();

	//HPSDR status
	QString ozyFX2FW; //Ozy firmware string
	int ozyFW;
	int mercuryFW;
	int penelopeFW;



	//Dialog stuff
	QDialog *hpsdrOptions;
	Ui::HPSDROptions *hpo;


};

#endif // HPSDR_H
