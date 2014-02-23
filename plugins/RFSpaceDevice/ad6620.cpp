#include "ad6620.h"
#include "rfsfilters.h"

AD6620::AD6620(USBUtil * _usbUtil)
{
	usbUtil = _usbUtil;
}

//AD6620 registers
bool AD6620::SetAD6620Raw(unsigned address, short d0, short d1, short d2, short d3, short d4)
{
	qint8 writeBuf[9];
	writeBuf[0] = 0x09;
	writeBuf[1] = 0xa0;
	writeBuf[2] = address & 0x00ff;
	writeBuf[3] = (address >> 8) & 0x00ff;
	writeBuf[4] = d0 & 0x00ff;
	writeBuf[5] = d1 & 0x00ff;
	writeBuf[6] = d2 & 0x00ff;
	writeBuf[7] = d3 & 0x00ff;
	writeBuf[8] = d4 & 0x00ff;
	return usbUtil->Write((LPVOID)&writeBuf,9);
}
//2byte register followed by 5 data bytes
bool AD6620::SetRegisters(unsigned adrs, qint32 data)
{
	AD6620_Register writeBuf;

	writeBuf.header[0] = 9;
	writeBuf.header[1] = 0xa0;
	writeBuf.address = adrs;
	writeBuf.data1 = data;
	writeBuf.data2 = 0;

	return usbUtil->Write((LPVOID)&writeBuf,9);

}

/*
Bandwidth options, these also change the effective sampleRate we use throughout the program
From AD6620 DataSheet "Programming AD6620"
Some registers can only be set in 'reset' mode, others can be changed dynamically
BW (IF Filter) can not be changed on the fly, SDR-IQ has to be stopped, then restarted
*/

int AD6620::SetBandwidth(int bw)
{
	qint8 CIC2Rate=0;
	qint8 CIC5Rate=0;
	qint8 RCFRate=0;
	qint16 RCFTaps=0;
	qint32 *RCFCoef=NULL;
	int totalDecimationRate;
	int usableBandwidth;

	int newSampleRate=0;

	//Rate, Taps, and SampleRate are one less than Spectrum shows in UI.  Do redundant math here so it's obvious
	switch (bw)
	{
	case BW5K:
		CIC2Rate = FIL5KHZ_CIC2RATE;
		CIC5Rate = FIL5KHZ_CIC5RATE;
		RCFRate = FIL5KHZ_RCFRATE;
		RCFTaps = FIL5KHZ_TAPS;
		RCFCoef = (qint32 *)FIL5KHZ_COEF;
		usableBandwidth = FIL5KHZ_USEABLEBW;
		newSampleRate = 8138;
		break;
	case BW10K:
		CIC2Rate = FIL10KHZ_CIC2RATE;
		CIC5Rate = FIL10KHZ_CIC5RATE;
		RCFRate = FIL10KHZ_RCFRATE;
		RCFTaps = FIL10KHZ_TAPS;
		RCFCoef = (qint32 *)FIL10KHZ_COEF;
		usableBandwidth = FIL10KHZ_USEABLEBW;
		newSampleRate = 16276;
		break;
	case BW25K:
		CIC2Rate = FIL25KHZ_CIC2RATE;
		CIC5Rate = FIL25KHZ_CIC5RATE;
		RCFRate = FIL25KHZ_RCFRATE;
		RCFTaps = FIL25KHZ_TAPS;
		RCFCoef = (qint32 *)FIL25KHZ_COEF;
		usableBandwidth = FIL25KHZ_USEABLEBW;
		newSampleRate = 37792-1;
		break;
	case BW50K:
		CIC2Rate = FIL50KHZ_CIC2RATE;
		CIC5Rate = FIL50KHZ_CIC5RATE;
		RCFRate = FIL50KHZ_RCFRATE;
		RCFTaps = FIL50KHZ_TAPS;
		RCFCoef = (qint32 *)FIL50KHZ_COEF;
		usableBandwidth = FIL50KHZ_USEABLEBW;
		newSampleRate = 55555-1;
		break;
	case BW100K:
		CIC2Rate = FIL100KHZ_CIC2RATE;
		CIC5Rate = FIL100KHZ_CIC5RATE;
		RCFRate = FIL100KHZ_RCFRATE;
		RCFTaps = FIL100KHZ_TAPS;
		RCFCoef = (qint32 *)FIL100KHZ_COEF;
		usableBandwidth = FIL100KHZ_USEABLEBW;
		newSampleRate = 111111-1;
		break;
	case BW150K:
		CIC2Rate = FIL150KHZ_CIC2RATE;
		CIC5Rate = FIL150KHZ_CIC5RATE;
		RCFRate = FIL150KHZ_RCFRATE;
		RCFTaps = FIL150KHZ_TAPS;
		RCFCoef = (qint32 *)FIL150KHZ_COEF;
		usableBandwidth = FIL150KHZ_USEABLEBW;
		newSampleRate = 158730-1;
		break;
	case BW190K:
		CIC2Rate = FIL190KHZ_CIC2RATE;
		CIC5Rate = FIL190KHZ_CIC5RATE;
		RCFRate = FIL190KHZ_RCFRATE;
		RCFTaps = FIL190KHZ_TAPS;
		RCFCoef = (qint32 *)FIL190KHZ_COEF;
		usableBandwidth = FIL190KHZ_USEABLEBW;
		newSampleRate = 196078-1;
		break;
	}
	bool res;
	//SDR-IQ has to be off to change rates and filters
	//Scales can be changed at any time
	//You would think mode should be SCOMPLEX (single 2input Complex) but SREAL is the only thing that works
	res = SetRegisters(MODECTRL, MODECTRL_SREAL | MODECTRL_SYNCMASTER | MODECTRL_RESET);
	//Not related to Bandwidth, but set here while we're in reset mode
	res = SetRegisters(ADR_NCOCTRL,NCOCTRL_PHZDITHER | NCOCTRL_AMPDITHER); //Dither options

	//-1 values indicate decimation registers which are defined as value-1 in AD6620 data sheet
	res = SetRegisters(CIC2SCALE,CIC2_SCALE_TBL[CIC2Rate]);
	res = SetRegisters(CIC2RATE, CIC2Rate-1);
	res = SetRegisters(CIC5SCALE, CIC5_SCALE_TBL[CIC5Rate]);
	res = SetRegisters(CIC5RATE, CIC5Rate-1);
	res = SetRegisters(RCFRATE, RCFRate-1);
	//res = SetRegisters(ADR_RCFOFFSET,0); //Don't know if we need this
	res = SetRegisters(RCFTAPS, RCFTaps-1);
	//We need to load filter coeffients for each range, they are not in SDR-IQ by default
	for (int i=0; i<RCFTaps; i++) {
		res = SetRegisters(i,RCFCoef[i]);
	}
	//Turn off RESET mode
	res = SetRegisters(MODECTRL, MODECTRL_SREAL | MODECTRL_SYNCMASTER);

	totalDecimationRate = CIC2Rate * CIC5Rate * RCFRate;

	//Todo: compare this to fixed rates above and determine which to use
	//int calculatedSampleRate = 66666667 / totalDecimationRate;

	return newSampleRate;
}


