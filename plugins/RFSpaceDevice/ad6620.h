#ifndef AD6620_H
#define AD6620_H
#include "usbutil.h"

// Defines for all the AD6620 registers
#define ADR_MODECTRL 0x300			//8 bits
#define MODECTRL_RESET (1<<0)	//pwrs up in soft reset with this bit set
#define MODECTRL_SREAL (0<<1)
#define MODECTRL_DREAL (1<<1)
#define MODECTRL_SCOMPLEX (1<<2)
#define MODECTRL_SYNCMASTER (1<<3)
#define ADR_NCOCTRL 0x301			//8 bits
#define NCOCTRL_BYPASS (1<<0)
#define NCOCTRL_PHZDITHER (1<<1)
#define NCOCTRL_AMPDITHER (1<<2)
#define ADR_NCOSYNCMASK 0x302	//32 bits (write all 1's)
#define ADR_NCOFREQ 0x303
#define ADR_NCOPHZOFFSET 0x304
#define ADR_CIC2SCALE 0x305	//8 bits, range 0 to 6 each count==6dB atten
#define ADR_CIC2M 0x306		//8 bits, 0 to 15( decimation of 1 to 16 )
#define ADR_CIC5SCALE 0x307	//8 bits, range 0 to 20 each count == 6dB
#define ADR_CIC5M 0x308		//0 to 31(decimation 1 to 32)
#define ADR_RCFCTRL 0x309	// 8 bits
#define ADR_RCFM 0x30A	//8 bits 0 to 31 (decimation 1 to 32)
#define ADR_RCFOFFSET 0x30B	//8 bit filter coef offset for RCF filter
#define ADR_TAPS 0x30C		//8 bits 0 to 255(number of RCF taps 1 to 256)
#define ADR_RESERVED 0x30D	//must be zero

#define ADR_RCF_COEFFICIENTS 0x00 //0x00 to 0xff

//Use Qt types to ensure size on all platforms
//Data block for setting AD6620 Registers (9 bytes)
struct AD6620_Register{
	qint8 header[2];
	qint16 address;
	qint32 data1; //Registers vary in size from 4bits to 32 bits
	qint8 data2;
};
const qint32 CIC5_SCALE_TBL[33] = {
	0,
	0,  0, 3, 5,	//1-32
	7,  8,10,10,
	11,12,13,13,
	14,15,15,15,
	16,16,17,17,
	17,18,18,18,
	19,19,19,20,
	20,20,20,20
};
//These tables are used to calculate the decimation stage gains
// which are a function of the decimation rate of each stage
const qint32 CIC2_SCALE_TBL[17] = {
	0,
	0,0,2,2,	//1-16
	3,4,4,4,
	5,5,5,6,
	6,6,6,6
};
//Not using this yet
const double FILT_GAIN[13] =
{
	0.0,
	6.0,
	6.0,
	9.0,
	9.0,
	11.0,
	14.0,
	12.0,
	10.0,
	9.0,
	6.0,
	8.0,
	6.0
};


class AD6620
{
public:
	enum BANDWIDTH {BW5K,BW10K,BW25K,BW50K,BW100K,BW150K,BW190K};

	AD6620(USBUtil *_usbUtil);
	//AD6620 Registers.  See Analog Devices data sheet for gory details
	static const unsigned CIC2SCALE = 0x305;
	static const unsigned CIC2RATE = 0x306; //Decimation -1,Max value 15 (16-1)
	static const unsigned CIC5SCALE = 0x307;
	static const unsigned CIC5RATE = 0x308; //Decimation -1,Max value 31 (32-1)
	static const unsigned RCFSCALE = 0x309;
	static const unsigned RCFRATE = 0x30a; //Decimation 01, Max value 31
	static const unsigned RCFTAPS = 0x30c; //#Taps - 1
	static const unsigned MODECTRL = 0x300;

	bool SetAD6620Raw(unsigned address, short d0, short d1, short d2, short d3, short d4);
	bool SetRegisters(unsigned adrs, qint32 data);
	int SetBandwidth(int bw);
private:
	USBUtil *usbUtil;
};

#endif // AD6620_H
