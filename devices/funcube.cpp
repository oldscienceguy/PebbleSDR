//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
//Uses HIDAPI cross platform source files from http://www.signal11.us/oss/hidapi/

#include "global.h"
#include "qmessagebox.h"
#include "qcoreapplication.h"
#include "funcube.h"
#include "demod.h"
#include "settings.h"

// * = FCD PLUS
#define FCD_HID_CMD_QUERY              1 // * Returns string with "FCDAPP version"

#define FCD_HID_CMD_SET_FREQUENCY    100 // * Send with 3 byte unsigned little endian frequency in kHz.
//Not sure if this still works, returns error in firmware 18i
#define FCD_HID_CMD_SET_FREQUENCY_HZ 101 // * Send with 4 byte unsigned little endian frequency in Hz, returns wit actual frequency set in Hz
#define FCD_HID_CMD_GET_FREQUENCY_HZ 102 // * Returns 4 byte unsigned little endian frequency in Hz.

#define FCD_HID_CMD_GET_IF_RSSI      104 // Returns 1 byte unsigned IF RSSI, -35dBm ~=0, -10dBm ~=70.
#define FCD_HID_CMD_GET_PLL_LOCK     105 // Returns 1 bit, true if locked
#define FCD_HID_CMD_SET_DC_CORR      106 // Send with 2 byte signed I DC correction followed by 2 byte signed Q DC correction. 0 is the default centre value.
#define FCD_HID_CMD_GET_DC_CORR      107 // Returns 2 byte signed I DC correction followed by 2 byte signed Q DC correction. 0 is the default centre value.
#define FCD_HID_CMD_SET_IQ_CORR      108 // Send with 2 byte signed phase correction followed by 2 byte unsigned gain correction. 0 is the default centre value for phase correction, 32768 is the default centre value for gain.
#define FCD_HID_CMD_GET_IQ_CORR      109 // Returns 2 byte signed phase correction followed by 2 byte unsigned gain correction. 0 is the default centre value for phase correction, 32768 is the default centre value for gain.

#define FCD_HID_CMD_SET_LNA_GAIN     110 // * Send a 1 byte value, see enums for reference
#define FCD_HID_CMD_SET_LNA_ENHANCE  111
#define FCD_HID_CMD_SET_BAND         112
#define FCD_HID_CMD_SET_RF_FILTER    113 // * Send one byte enum, see TUNERRFFILTERENUM
#define FCD_HID_CMD_SET_MIXER_GAIN   114 // * Send one byte, 1 on, 0 of
#define FCD_HID_CMD_SET_BIAS_CURRENT 115
#define FCD_HID_CMD_SET_MIXER_FILTER 116
//Same as SET_IF_GAIN for FCD+
#define FCD_HID_CMD_SET_IF_GAIN1     117 // * Send one byte value, valid value 0 to 59 (dB)
#define FCD_HID_CMD_SET_IF_GAIN_MODE 118
#define FCD_HID_CMD_SET_IF_RC_FILTER 119
#define FCD_HID_CMD_SET_IF_GAIN2     120
#define FCD_HID_CMD_SET_IF_GAIN3     121
#define FCD_HID_CMD_SET_IF_FILTER    122 // * Send one byte enum, see TUNERIFFILTERENUM
#define FCD_HID_CMD_SET_IF_GAIN4     123
#define FCD_HID_CMD_SET_IF_GAIN5     124
#define FCD_HID_CMD_SET_IF_GAIN6     125
#define FCD_HID_CMD_SET_BIAS_TEE     126 // * PLUS ONLY Send one byte, 1 on, 0 off


#define FCD_HID_CMD_GET_LNA_GAIN     150 // * Retrieve a 1 byte value, 1=ON, 0=OFF
#define FCD_HID_CMD_GET_LNA_ENHANCE  151
#define FCD_HID_CMD_GET_BAND         152
#define FCD_HID_CMD_GET_RF_FILTER    153 // * Returns one byte enum, see TUNERRFFILTERENUM
#define FCD_HID_CMD_GET_MIXER_GAIN   154 // * Returns one byte, 1 on, 0 off
#define FCD_HID_CMD_GET_BIAS_CURRENT 155
#define FCD_HID_CMD_GET_MIXER_FILTER 156
#define FCD_HID_CMD_GET_IF_GAIN1     157 // * Returns one byte value, valid value 0 to 59 (dB)
#define FCD_HID_CMD_GET_IF_GAIN_MODE 158
#define FCD_HID_CMD_GET_IF_RC_FILTER 159
#define FCD_HID_CMD_GET_IF_GAIN2     160
#define FCD_HID_CMD_GET_IF_GAIN3     161
#define FCD_HID_CMD_GET_IF_FILTER    162 // * Returns one byte enum, see TUNERIFFILTERENUM
#define FCD_HID_CMD_GET_IF_GAIN4     163
#define FCD_HID_CMD_GET_IF_GAIN5     164
#define FCD_HID_CMD_GET_IF_GAIN6     165
#define FCD_HID_CMD_GET_BIAS_TEE     166 // * PLUS ONLY Returns one byte, 1 on, 0 off


#define FCD_HID_CMD_I2C_SEND_BYTE    200
#define FCD_HID_CMD_I2C_RECEIVE_BYTE 201

#define FCD_RESET                    255 // * Reset to bootloader

typedef enum
{
	TLGE_N5_0DB=0,
	TLGE_N2_5DB=1,
	TLGE_P0_0DB=4,
	TLGE_P2_5DB=5,
	TLGE_P5_0DB=6,
	TLGE_P7_5DB=7,
	TLGE_P10_0DB=8,
	TLGE_P12_5DB=9,
	TLGE_P15_0DB=10,
	TLGE_P17_5DB=11,
	TLGE_P20_0DB=12,
	TLGE_P25_0DB=13,
	TLGE_P30_0DB=14
} TUNERLNAGAINENUM;

typedef enum
{
  TLEE_OFF=0,
  TLEE_0=1,
  TLEE_1=3,
  TLEE_2=5,
  TLEE_3=7
} TUNERLNAENHANCEENUM;

typedef enum
{
  TBE_VHF2,
  TBE_VHF3,
  TBE_UHF,
  TBE_LBAND
} TUNERBANDENUM;

typedef enum
{
    //FCD
    // Band 0, VHF II
    TRFE_LPF268MHZ=0,
    TRFE_LPF299MHZ=8,
    // Band 1, VHF III
    TRFE_LPF509MHZ=0,
    TRFE_LPF656MHZ=8,
    // Band 2, UHF
    TRFE_BPF360MHZ=0,
    TRFE_BPF380MHZ=1,
    TRFE_BPF405MHZ=2,
    TRFE_BPF425MHZ=3,
    TRFE_BPF450MHZ=4,
    TRFE_BPF475MHZ=5,
    TRFE_BPF505MHZ=6,
    TRFE_BPF540MHZ=7,
    TRFE_BPF575MHZ=8,
    TRFE_BPF615MHZ=9,
    TRFE_BPF670MHZ=10,
    TRFE_BPF720MHZ=11,
    TRFE_BPF760MHZ=12,
    TRFE_BPF840MHZ=13,
    TRFE_BPF890MHZ=14,
    TRFE_BPF970MHZ=15,
    // Band 2, L band
    TRFE_BPF1300MHZ=0,
    TRFE_BPF1320MHZ=1,
    TRFE_BPF1360MHZ=2,
    TRFE_BPF1410MHZ=3,
    TRFE_BPF1445MHZ=4,
    TRFE_BPF1460MHZ=5,
    TRFE_BPF1490MHZ=6,
    TRFE_BPF1530MHZ=7,
    TRFE_BPF1560MHZ=8,
    TRFE_BPF1590MHZ=9,
    TRFE_BPF1640MHZ=10,
    TRFE_BPF1660MHZ=11,
    TRFE_BPF1680MHZ=12,
    TRFE_BPF1700MHZ=13,
    TRFE_BPF1720MHZ=14,
    TRFE_BPF1750MHZ=15,

    //FCD PLUS
    TRFE_0_4=0,
    TRFE_4_8=1,
    TRFE_8_16=2,
    TRFE_16_32=3,
    TRFE_32_75=4,
    TRFE_75_125=5,
    TRFE_125_250=6,
    TRFE_145=7,
    TRFE_410_875=8,
    TRFE_435=9,
    TRFE_875_2000=10

} TUNERRFFILTERENUM;

typedef enum
{
  TMGE_P4_0DB=0,
  TMGE_P12_0DB=1
} TUNERMIXERGAINENUM;

typedef enum
{
  TBCE_LBAND=0,
  TBCE_1=1,
  TBCE_2=2,
  TBCE_VUBAND=3
} TUNERBIASCURRENTENUM;

typedef enum
{
  TMFE_27_0MHZ=0,
  TMFE_4_6MHZ=8,
  TMFE_4_2MHZ=9,
  TMFE_3_8MHZ=10,
  TMFE_3_4MHZ=11,
  TMFE_3_0MHZ=12,
  TMFE_2_7MHZ=13,
  TMFE_2_3MHZ=14,
  TMFE_1_9MHZ=15
} TUNERMIXERFILTERENUM;

typedef enum
{
  TIG1E_N3_0DB=0,
  TIG1E_P6_0DB=1
} TUNERIFGAIN1ENUM;

typedef enum
{
  TIGME_LINEARITY=0,
  TIGME_SENSITIVITY=1
} TUNERIFGAINMODEENUM;

typedef enum
{
  TIRFE_21_4MHZ=0,
  TIRFE_21_0MHZ=1,
  TIRFE_17_6MHZ=2,
  TIRFE_14_7MHZ=3,
  TIRFE_12_4MHZ=4,
  TIRFE_10_6MHZ=5,
  TIRFE_9_0MHZ=6,
  TIRFE_7_7MHZ=7,
  TIRFE_6_4MHZ=8,
  TIRFE_5_3MHZ=9,
  TIRFE_4_4MHZ=10,
  TIRFE_3_4MHZ=11,
  TIRFE_2_6MHZ=12,
  TIRFE_1_8MHZ=13,
  TIRFE_1_2MHZ=14,
  TIRFE_1_0MHZ=15
} TUNERIFRCFILTERENUM;

typedef enum
{
  TIG2E_P0_0DB=0,
  TIG2E_P3_0DB=1,
  TIG2E_P6_0DB=2,
  TIG2E_P9_0DB=3
} TUNERIFGAIN2ENUM;

typedef enum
{
  TIG3E_P0_0DB=0,
  TIG3E_P3_0DB=1,
  TIG3E_P6_0DB=2,
  TIG3E_P9_0DB=3
} TUNERIFGAIN3ENUM;

typedef enum
{
  TIG4E_P0_0DB=0,
  TIG4E_P1_0DB=1,
  TIG4E_P2_0DB=2
} TUNERIFGAIN4ENUM;

typedef enum
{
    TIFE_5_50MHZ=0,
    TIFE_5_30MHZ=1,
    TIFE_5_00MHZ=2,
    TIFE_4_80MHZ=3,
    TIFE_4_60MHZ=4,
    TIFE_4_40MHZ=5,
    TIFE_4_30MHZ=6,
    TIFE_4_10MHZ=7,
    TIFE_3_90MHZ=8,
    TIFE_3_80MHZ=9,
    TIFE_3_70MHZ=10,
    TIFE_3_60MHZ=11,
    TIFE_3_40MHZ=12,
    TIFE_3_30MHZ=13,
    TIFE_3_20MHZ=14,
    TIFE_3_10MHZ=15,
    TIFE_3_00MHZ=16,
    TIFE_2_95MHZ=17,
    TIFE_2_90MHZ=18,
    TIFE_2_80MHZ=19,
    TIFE_2_75MHZ=20,
    TIFE_2_70MHZ=21,
    TIFE_2_60MHZ=22,
    TIFE_2_55MHZ=23,
    TIFE_2_50MHZ=24,
    TIFE_2_45MHZ=25,
    TIFE_2_40MHZ=26,
    TIFE_2_30MHZ=27,
    TIFE_2_28MHZ=28,
    TIFE_2_24MHZ=29,
    TIFE_2_20MHZ=30,
    TIFE_2_15MHZ=31,

    //PLUS
    TIFE_200KHZ=0,
    TIFE_300KHZ=1,
    TIFE_600KHZ=2,
    TIFE_1536KHZ=3,
    TIFE_5MHZ=4,
    TIFE_6MHZ=5,
    TIFE_7MHZ=6,
    TIFE_8MHZ=7

} TUNERIFFILTERENUM;

typedef enum
{
  TIG5E_P3_0DB=0,
  TIG5E_P6_0DB=1,
  TIG5E_P9_0DB=2,
  TIG5E_P12_0DB=3,
  TIG5E_P15_0DB=4
} TUNERIFGAIN5ENUM;

typedef enum
{
  TIG6E_P3_0DB=0,
  TIG6E_P6_0DB=1,
  TIG6E_P9_0DB=2,
  TIG6E_P12_0DB=3,
  TIG6E_P15_0DB=4
} TUNERIFGAIN6ENUM;


FunCube::FunCube(Receiver *_receiver, SDRDEVICE dev,Settings *_settings):
		SDR(_receiver, dev,_settings)
{
    //If settings is NULL we're getting called just to get defaults, check in destructor
    settings = _settings;
    if (!settings)
        return;

    optionUi = NULL;

    if (sdrDevice == SDR::FUNCUBE)
        InitSettings("funcube");
    else
        InitSettings("funcube_plus");
	ReadSettings();

    //Test, This causes FCD to crash in HID on exit
    fcdSetFreqHz = true; //Seems to be the way new FCD+ code is working
    hidDev = NULL;
    hidDevInfo = NULL;
    hid_init();

}

FunCube::~FunCube(void)
{
    if (!settings)
        return;
	WriteSettings();
	Disconnect();
    hid_exit();
}
bool FunCube::Connect()
{

	//Don't know if we can get this from DDK calls
	maxPacketSize = 64;

    if (sdrDevice == SDR::FUNCUBE)
        hidDevInfo = hid_enumerate(sVID, sFCD_PID);
    else
        hidDevInfo = hid_enumerate(sVID, sFCD_PLUS_PID);

    if (hidDevInfo == NULL)
        return false; //Not found

    char *hidPath = strdup(hidDevInfo->path); //Get local copy
    if (hidPath == NULL)
        return false;
    hid_free_enumeration(hidDevInfo);
    hidDevInfo = NULL;

    if ((hidDev = hid_open_path(hidPath)) == NULL){
        free(hidPath);
        hidPath = NULL;
        return false;
    }

    free(hidPath);
    hidPath = NULL;
    connected = true;
    return true;
}



bool FunCube::Disconnect()
{
    //Mac sometimes crashes when hid_close is called, but we need it
    if (hidDev != NULL)
        hid_close(hidDev);
    hidDev = NULL;
    connected = false;
	return true;
}

void FunCube::Start()
{
	//char data[maxPacketSize];
	//bool res = HIDGet(FCD_HID_CMD_QUERY,data,sizeof(data));

	//Throw away any pending reads
	//while(HIDRead(data,sizeof(data))>0)
	//	;

	//First thing HID does is read all values
	//ReadFCDOptions();

    if (sdrDevice == SDR::FUNCUBE) {
        SetDCCorrection(fcdDCICorrection,fcdDCQCorrection);
        SetIQCorrection(fcdIQPhaseCorrection,fcdIQGainCorrection);

        //Set device with current options
        SetLNAGain(fcdLNAGain);
        SetLNAEnhance(fcdLNAEnhance);
        SetBand(fcdBand);
        SetRFFilter(fcdRFFilter);
        SetMixerGain(fcdMixerGain);
        SetBiasCurrent(fcdBiasCurrent);
        SetMixerFilter(fcdMixerFilter);
        SetIFGain1(fcdIFGain1);
        SetIFGain2(fcdIFGain2);
        SetIFGain3(fcdIFGain3);
        SetIFGain4(fcdIFGain4);
        SetIFGain5(fcdIFGain5);
        SetIFGain6(fcdIFGain6);
        SetIFGainMode(fcdIFGainMode);
        SetIFRCFilter(fcdIFRCFilter);
        SetIFFilter(fcdIFFilter);
    } else {
        //FCD+
        SetLNAGain(fcdLNAGain);
        SetRFFilter(fcdRFFilter);
        SetMixerGain(fcdMixerGain);
        SetIFGain1(fcdIFGain1);
        SetIFFilter(fcdIFFilter);
        SetBiasTee(fcdBiasTee);

    }

	if (audioInput != NULL)
        audioInput->StartInput(inputDeviceName, GetSampleRate());

}

void FunCube::Stop()
{
	if (audioInput != NULL)
		audioInput->Stop();
}
/*
 HID devices only communicate with control or interupt pipes
 Data exchange is via 'reports', input, output, feature
 */
#define HID_SET_REPORT 0x09 //HID request number to set report
#define HID_GET_REPORT 0x01
#define HID_TIMEOUT 1000 //ms

int FunCube::HIDWrite(unsigned char *data, int length)
{
    if (!connected)
        return HID_TIMEOUT;

    return hid_write(hidDev,data,length);
}

/*
 usb error codes
 -116	Timeout
 -22	Invalid arg
 -5
 */
int FunCube::HIDRead(unsigned char *data, int length)
{
    if (!connected)
        return HID_TIMEOUT;

	memset(data,0x00,length); // Clear out the response buffer
    int bytesRead = hid_read(hidDev,data,length);
    //Caller does not expect or handle report# returned from windows
    if (bytesRead > 0) {
        bytesRead--;
        memcpy(data,&data[1],bytesRead);
    }
    return bytesRead;
}

//Adapted from fcd.c in FuncubeDongle download
double FunCube::SetFrequency(double fRequested, double fCurrent)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS) {
        //Reject tuning in gap
        if (fRequested > 260000000 && fRequested < 410000000)
            return fCurrent;
    }
    unsigned char buf[maxPacketSize+1];

	//Apply correction factor
    double fCorrected;
	int nFreq;
	if (fcdSetFreqHz) {
		//hz resolution
        fCorrected = fRequested * (sOffset/1000000.0);
        nFreq = fCorrected;
		buf[0]=0; // Report ID, ignored
		buf[1]=FCD_HID_CMD_SET_FREQUENCY_HZ;
		buf[2]=(unsigned char)nFreq;
		buf[3]=(unsigned char)(nFreq>>8);
		buf[4]=(unsigned char)(nFreq>>16);
		buf[5]=(unsigned char)(nFreq>>24);
	} else {
		//khz resolution, round up to nearest khz and truncate
		fRequested = (int)((fRequested+500)/1000) * 1000; //No resolution below 1khz
        fCorrected = fRequested * (sOffset/1000000.0);
        nFreq = fCorrected/1000;

		buf[0]=0; // Report ID, ignored
		buf[1]=FCD_HID_CMD_SET_FREQUENCY;
		buf[2]=(unsigned char)nFreq;
		buf[3]=(unsigned char)(nFreq>>8);
		buf[4]=(unsigned char)(nFreq>>16);
		buf[5]=0x00;
	}
	int result = HIDWrite(buf,sizeof(buf));
	if (result < 0) {
		logfile<<"FCD: Frequency write error\n";
		return fCurrent; //Write error
	}

	//Test, read it back to confirm
	quint8 inBuf[maxPacketSize];
    result = HIDRead((unsigned char*)inBuf,sizeof(inBuf));
	//We sometimes get timeout errors
	if (result < 0) {
		logfile<<"FCD: Frequency read after write error\n";
		return fCurrent; //Read error
	}
	double fActual;
    //Remember HIDRead throws away buf[0] when comparing to QTHID
    if (fcdSetFreqHz && inBuf[0]==1) {
		//Return actual freq set, which may not be exactly what was requested
		//Note: Results are too strange to be usable, ie user requests a freq and sees something completely different
		//Just return fRequested for now
        fActual = 0;
        fActual += (quint64)inBuf[1];
        fActual += (quint64)inBuf[2]<<8;
        fActual += (quint64)inBuf[3]<<16;
        fActual += (quint64)inBuf[4]<<24;
		fActual = fActual / (sOffset/1000000.0);
        fActual = fRequested;
	} else {
		//Does't return actual freq, just return req
		fActual = fRequested;
	}
	return fActual;
}

bool FunCube::HIDSet(char cmd, unsigned char value)
{
    if (!connected)
        return false;

    unsigned char outBuf[maxPacketSize+1];
    unsigned char inBuf[maxPacketSize];

	outBuf[0] = 0; // Report ID, ignored
	outBuf[1] = cmd;
	outBuf[2] = value;

	int numBytes = HIDWrite(outBuf,sizeof(outBuf));
	if (numBytes < 0) {
		logfile<<"FCD: HIDSet-HIDWrite error\n";
		return false; //Write error
	}
	numBytes = HIDRead(inBuf,sizeof(inBuf));
	if (numBytes < 0) {
		logfile<<"FCD: HIDSet-HIDRead after HIDWrite error \n";
		return false; //Read error
	}

	return true;
}

bool FunCube::HIDGet(char cmd, void *data, int len)
{
    if (!connected)
        return false;

    unsigned char outBuf[maxPacketSize+1]; //HID Write skips report id 0 and adj buffer, so we need +1 to send 64
    unsigned char inBuf[maxPacketSize];

	outBuf[0] = 0; // Report ID, ignored
	outBuf[1] = cmd;

	int numBytes = HIDWrite(outBuf,sizeof(outBuf));
	if (numBytes < 0) {
		logfile<<"FCD: HIDGet-Write error\n";
		return false; //Write error
	}
	numBytes = HIDRead(inBuf,sizeof(inBuf));
	if (numBytes < 0) {
		logfile<<"FCD: HIDGet-Read after Write error\n";
		return false; //Read error
	}
#if 0
    // FCD Plus fails this test
	//inBuf[0] = cmd we're getting data for
    //inBuf[1] = First data byte
	if (inBuf[0] != cmd){
		logfile<<"FCD: Invalid read response for cmd: "<<cmd<<"\n";
		return false;
	}
#endif
    //Don't return cmd, strip from buffer
    int retLen = numBytes-1 < len ? numBytes-1 : len;
    memcpy(data,&inBuf[1],retLen);
	return true;
}
void FunCube::SetDCCorrection(qint16 cI, qint16 cQ)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

    unsigned char outBuf[maxPacketSize+1];
	outBuf[0] = 0; // Report ID, ignored
	outBuf[1] = FCD_HID_CMD_SET_DC_CORR;
	outBuf[2] = cI;
	outBuf[3] = cI>>8;
	outBuf[4] = cQ;
	outBuf[5] = cQ>>8;

	int numBytes = HIDWrite(outBuf,sizeof(outBuf));
	if (numBytes > 0) {
		fcdDCICorrection = cI;
		fcdDCQCorrection = cQ;
		//Every write must be matched by read or we get out of seq
		HIDRead(outBuf,sizeof(outBuf));
	}

}
void FunCube::SetIQCorrection(qint16 phase, quint16 gain)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

    unsigned char outBuf[maxPacketSize+1];
	outBuf[0] = 0; // Report ID, ignored
	outBuf[1] = FCD_HID_CMD_SET_IQ_CORR;
	outBuf[2] = phase;
	outBuf[3] = phase>>8;
	outBuf[4] = gain;
	outBuf[5] = gain>>8;

	int numBytes = HIDWrite(outBuf,sizeof(outBuf));
	if (numBytes > 0) {
		fcdIQPhaseCorrection = phase;
		fcdIQGainCorrection = gain;
		//Every write must be matched with a read
		HIDRead(outBuf,sizeof(outBuf));
	}
}

void FunCube::LNAGainChanged(int s)
{
    int v = optionUi->LNAGain->itemData(s).toInt();
	SetLNAGain(v);
}
void FunCube::SetLNAGain(int v)
{
	if (v<0)
		v = TLGE_P20_0DB;
    //Looks like FCD+ expects enabled/disabled only
	HIDSet(FCD_HID_CMD_SET_LNA_GAIN,v);
	fcdLNAGain = v;
}

void FunCube::LNAEnhanceChanged(int s)
{
    int v = optionUi->LNAEnhance->itemData(s).toInt();
	SetLNAEnhance(v);
}
void FunCube::SetLNAEnhance(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TLEE_OFF;
	HIDSet(FCD_HID_CMD_SET_LNA_ENHANCE,v);
	fcdLNAEnhance = v;
}

void FunCube::SetBand(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TBE_VHF2;
	HIDSet(FCD_HID_CMD_SET_BAND,v);
	fcdBand = v;
}

void FunCube::BandChanged(int s)
{
    //If FCD+ there are no bands, just populate combo
    if (sdrDevice == FUNCUBE_PLUS) {
        optionUi->RFFilter->clear();
        optionUi->RFFilter->addItem("0 to 4 LPF",TRFE_0_4);
        optionUi->RFFilter->addItem("4 to 8 LPF",TRFE_4_8);
        optionUi->RFFilter->addItem("8 to 16 LPF",TRFE_8_16);
        optionUi->RFFilter->addItem("16 to 32 LPF",TRFE_16_32);
        optionUi->RFFilter->addItem("32 to 75 LPF",TRFE_32_75);
        optionUi->RFFilter->addItem("75 to 125 LPF",TRFE_75_125);
        optionUi->RFFilter->addItem("125 to 250 LPF",TRFE_125_250);
        optionUi->RFFilter->addItem("145 LPF",TRFE_145);
        optionUi->RFFilter->addItem("410 to 875 LPF",TRFE_410_875);
        optionUi->RFFilter->addItem("435 LPF",TRFE_435);
        optionUi->RFFilter->addItem("875 to 2000 LPF",TRFE_875_2000);

        return;
    }
    int v = optionUi->Band->itemData(s).toInt();
	SetBand(v);
	//Change RFFilter options to match band
    optionUi->RFFilter->clear();
	switch (v) {
	case 0:
        optionUi->RFFilter->addItem("268mhz LPF",TRFE_LPF268MHZ);
        optionUi->RFFilter->addItem("299mhz LPF",TRFE_LPF299MHZ);
		break;
	case 1:
        optionUi->RFFilter->addItem("509mhz LPF",TRFE_LPF509MHZ);
        optionUi->RFFilter->addItem("656mhz LPF",TRFE_LPF656MHZ);
		break;
	case 2:
        optionUi->RFFilter->addItem("360mhz LPF",TRFE_BPF360MHZ);
        optionUi->RFFilter->addItem("380mhz LPF",TRFE_BPF380MHZ);
        optionUi->RFFilter->addItem("405mhz LPF",TRFE_BPF405MHZ);
        optionUi->RFFilter->addItem("425mhz LPF",TRFE_BPF425MHZ);
        optionUi->RFFilter->addItem("450mhz LPF",TRFE_BPF450MHZ);
        optionUi->RFFilter->addItem("475mhz LPF",TRFE_BPF475MHZ);
        optionUi->RFFilter->addItem("505mhz LPF",TRFE_BPF505MHZ);
        optionUi->RFFilter->addItem("540mhz LPF",TRFE_BPF540MHZ);
        optionUi->RFFilter->addItem("575mhz LPF",TRFE_BPF575MHZ);
        optionUi->RFFilter->addItem("615mhz LPF",TRFE_BPF615MHZ);
        optionUi->RFFilter->addItem("670mhz LPF",TRFE_BPF670MHZ);
        optionUi->RFFilter->addItem("720mhz LPF",TRFE_BPF720MHZ);
        optionUi->RFFilter->addItem("760mhz LPF",TRFE_BPF760MHZ);
        optionUi->RFFilter->addItem("840mhz LPF",TRFE_BPF840MHZ);
        optionUi->RFFilter->addItem("890mhz LPF",TRFE_BPF890MHZ);
        optionUi->RFFilter->addItem("970mhz LPF",TRFE_BPF970MHZ);
		break;
	case 3:
        optionUi->RFFilter->addItem("1300mhz LPF",TRFE_BPF1300MHZ);
        optionUi->RFFilter->addItem("1320mhz LPF",TRFE_BPF1320MHZ);
        optionUi->RFFilter->addItem("1360mhz LPF",TRFE_BPF1360MHZ);
        optionUi->RFFilter->addItem("1410mhz LPF",TRFE_BPF1410MHZ);
        optionUi->RFFilter->addItem("1445mhz LPF",TRFE_BPF1445MHZ);
        optionUi->RFFilter->addItem("1460mhz LPF",TRFE_BPF1460MHZ);
        optionUi->RFFilter->addItem("1490mhz LPF",TRFE_BPF1490MHZ);
        optionUi->RFFilter->addItem("1530mhz LPF",TRFE_BPF1530MHZ);
        optionUi->RFFilter->addItem("1560mhz LPF",TRFE_BPF1560MHZ);
        optionUi->RFFilter->addItem("1590mhz LPF",TRFE_BPF1590MHZ);
        optionUi->RFFilter->addItem("1640mhz LPF",TRFE_BPF1640MHZ);
        optionUi->RFFilter->addItem("1660mhz LPF",TRFE_BPF1660MHZ);
        optionUi->RFFilter->addItem("1680mhz LPF",TRFE_BPF1680MHZ);
        optionUi->RFFilter->addItem("1700mhz LPF",TRFE_BPF1700MHZ);
        optionUi->RFFilter->addItem("1720mhz LPF",TRFE_BPF1720MHZ);
        optionUi->RFFilter->addItem("1750mhz LPF",TRFE_BPF1750MHZ);
		break;
	}
	//We're not keeping filter values per band yet, so just set filter to first value
	RFFilterChanged(0);
}
void FunCube::RFFilterChanged(int s)
{
    int v = optionUi->RFFilter->itemData(s).toInt();
	SetRFFilter(v);
}
void FunCube::SetRFFilter(int v)
{
	if (v<0)
		v = 0;
    if (sdrDevice == FUNCUBE_PLUS && v > TRFE_875_2000)
        return; //Invalid for FCD+

	HIDSet(FCD_HID_CMD_SET_RF_FILTER,v);
	fcdRFFilter = v;
}

void FunCube::MixerGainChanged(int s)
{
    int v = optionUi->MixerGain->itemData(s).toInt();
	SetMixerGain(v);
}
void FunCube::SetMixerGain(int v)
{
	if (v<0)
		v = TMGE_P12_0DB;
	HIDSet(FCD_HID_CMD_SET_MIXER_GAIN,v);
	fcdMixerGain = v;
}

void FunCube::BiasCurrentChanged(int s)
{
    int v = optionUi->BiasCurrent->itemData(s).toInt();
	SetBiasCurrent(v);
}
void FunCube::SetBiasCurrent(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TBCE_LBAND;
	HIDSet(FCD_HID_CMD_SET_BIAS_CURRENT,v);
	fcdBiasCurrent = v;
}

//FCD+ only
void FunCube::SetBiasTee(int v)
{
    if (sdrDevice == SDR::FUNCUBE)
        return; //Not supported

    if (v<0)
        v = TBCE_LBAND;
    HIDSet(FCD_HID_CMD_SET_BIAS_TEE,v);
    fcdBiasTee = v;
}

void FunCube::BiasTeeChanged(int s)
{
    if (sdrDevice == SDR::FUNCUBE)
        return; //Not supported
    int v = optionUi->BiasTee->itemData(s).toInt();

    SetBiasTee(v);
}

void FunCube::MixerFilterChanged(int s)
{
    int v = optionUi->MixerFilter->itemData(s).toInt();
	SetMixerFilter(v);
}
void FunCube::SetMixerFilter(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TMFE_1_9MHZ;
	HIDSet(FCD_HID_CMD_SET_MIXER_FILTER,v);
	fcdMixerFilter = v;
}

void FunCube::IFGain1Changed(int s)
{
    int v = optionUi->IFGain1->itemData(s).toInt();
	SetIFGain1(v);
}
void FunCube::SetIFGain1(int v)
{
	if (v<0)
		v = TIG1E_P6_0DB;
    else if (v > 59)
        return;

	HIDSet(FCD_HID_CMD_SET_IF_GAIN1,v);
	fcdIFGain1 = v;
}

void FunCube::IFGain2Changed(int s)
{
    int v = optionUi->IFGain2->itemData(s).toInt();
	SetIFGain2(v);
}
void FunCube::SetIFGain2(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIG2E_P0_0DB;
	HIDSet(FCD_HID_CMD_SET_IF_GAIN2,v);
	fcdIFGain2 = v;
}

void FunCube::IFGain3Changed(int s)
{
    int v = optionUi->IFGain3->itemData(s).toInt();
	SetIFGain3(v);
}
void FunCube::SetIFGain3(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIG3E_P0_0DB;
	HIDSet(FCD_HID_CMD_SET_IF_GAIN3,v);
	fcdIFGain3 = v;
}

void FunCube::IFGain4Changed(int s)
{
    int v = optionUi->IFGain4->itemData(s).toInt();
	SetIFGain4(v);
}
void FunCube::SetIFGain4(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIG4E_P0_0DB;
	HIDSet(FCD_HID_CMD_SET_IF_GAIN4,v);
	fcdIFGain4 = v;
}

void FunCube::IFGain5Changed(int s)
{
    int v = optionUi->IFGain5->itemData(s).toInt();
	SetIFGain5(v);
}
void FunCube::SetIFGain5(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIG5E_P3_0DB;
	HIDSet(FCD_HID_CMD_SET_IF_GAIN5,v);
	fcdIFGain5 = v;
}

void FunCube::IFGain6Changed(int s)
{
    int v = optionUi->IFGain6->itemData(s).toInt();
	SetIFGain6(v);
}
void FunCube::SetIFGain6(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIG6E_P3_0DB;
	HIDSet(FCD_HID_CMD_SET_IF_GAIN6,v);
	fcdIFGain6 = v;
}

void FunCube::IFGainModeChanged(int s)
{
    int v = optionUi->IFGainMode->itemData(s).toInt();
	SetIFGainMode(v);
}
void FunCube::SetIFGainMode(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIGME_LINEARITY;
	HIDSet(FCD_HID_CMD_SET_IF_GAIN_MODE,v);
	fcdIFGainMode = v;
}

void FunCube::IFRCFilterChanged(int s)
{
    int v = optionUi->IFRCFilter->itemData(s).toInt();
	SetIFRCFilter(v);
}
void FunCube::SetIFRCFilter(int v)
{
    if (sdrDevice == SDR::FUNCUBE_PLUS)
        return; //Not supported yet

	if (v<0)
		v = TIRFE_1_0MHZ;
	HIDSet(FCD_HID_CMD_SET_IF_RC_FILTER,v);
	fcdIFRCFilter = v;
}

void FunCube::IFFilterChanged(int s)
{
    int v = optionUi->IFFilter->itemData(s).toInt();
	SetIFFilter(v);
}
void FunCube::SetIFFilter(int v)
{
	if (v<0)
		v = TIFE_2_15MHZ; //Default
	HIDSet(FCD_HID_CMD_SET_IF_FILTER,v);
	fcdIFFilter = v;
}


void FunCube::ReadFCDOptions()
{
	quint8 data[maxPacketSize];
	bool res = HIDGet(FCD_HID_CMD_QUERY,data,sizeof(data));
	if (res)
		fcdVersion = (char*)data;

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IQ_CORR,data,sizeof(data));
        if (res) {
            fcdIQPhaseCorrection = (qint16)data[0];
            fcdIQPhaseCorrection += (qint16)data[1]<<8;
            fcdIQGainCorrection = (quint16)data[2];
            fcdIQGainCorrection += (quint16)data[3]<<8;
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_DC_CORR,data,sizeof(data));
        if (res) {
            fcdDCICorrection = (quint16)data[0];
            fcdDCICorrection += (quint16)data[1]<<8;
            fcdDCQCorrection = (quint16)data[2];
            fcdDCQCorrection += (quint16)data[3]<<8;
        }
    }

    res = HIDGet(FCD_HID_CMD_GET_FREQUENCY_HZ,data,sizeof(data));
    if (res) {
		fcdFreq = (quint64)data[0]+(((quint64)data[1])<<8)+(((quint64)data[2])<<16)+(((quint64)data[3])<<24);
		fcdFreq = fcdFreq / (sOffset/1000000.0);
	}

    //FCD+ return 0 or 1
	res = HIDGet(FCD_HID_CMD_GET_LNA_GAIN,data,sizeof(data));
	if (res) {
		fcdLNAGain=data[0];
	}

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_BAND,data,sizeof(data));
        if (res) {
            fcdBand=data[0];
        }
    }

	res = HIDGet(FCD_HID_CMD_GET_RF_FILTER,data,sizeof(data));
	if (res) {
		fcdRFFilter=data[0];
	}

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_LNA_ENHANCE,data,sizeof(data));
        if (res) {
            fcdLNAEnhance=data[0];
        }
    }
	res = HIDGet(FCD_HID_CMD_GET_MIXER_GAIN,data,sizeof(data));
	if (res) {
		fcdMixerGain=data[0];
	}

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_BIAS_CURRENT,data,sizeof(data));
        if (res) {
            fcdBiasCurrent=data[0];
        }
    }
    if (sdrDevice == SDR::FUNCUBE_PLUS) {
        res = HIDGet(FCD_HID_CMD_GET_BIAS_TEE,data,sizeof(data));
        if (res) {
            fcdBiasTee=data[0];
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_MIXER_FILTER,data,sizeof(data));
        if (res) {
            fcdMixerFilter=data[0];
        }
    }

	res = HIDGet(FCD_HID_CMD_GET_IF_GAIN1,data,sizeof(data));
	if (res) {
		fcdIFGain1=data[0];
	}

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_GAIN_MODE,data,sizeof(data));
        if (res) {
            fcdIFGainMode=data[0];
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_RC_FILTER,data,sizeof(data));
        if (res) {
            fcdIFRCFilter=data[0];
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_GAIN2,data,sizeof(data));
        if (res) {
            fcdIFGain2=data[0];
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_GAIN3,data,sizeof(data));
        if (res) {
            fcdIFGain3=data[0];
        }
    }

	res = HIDGet(FCD_HID_CMD_GET_IF_FILTER,data,sizeof(data));
	if (res) {
		fcdIFFilter=data[0];
	}

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_GAIN4,data,sizeof(data));
        if (res) {
            fcdIFGain4=data[0];
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_GAIN5,data,sizeof(data));
        if (res) {
            fcdIFGain5=data[0];
        }
    }

    if (sdrDevice == SDR::FUNCUBE) {
        res = HIDGet(FCD_HID_CMD_GET_IF_GAIN6,data,sizeof(data));
        if (res) {
            fcdIFGain6=data[0];
        }
    }

}

void FunCube::SetupOptionUi(QWidget *parent)
{
    if (optionUi != NULL)
        delete optionUi;
    optionUi = new Ui::FunCubeOptions();
    optionUi->setupUi(parent);
    parent->setVisible(true);

    QFont smFont = settings->smFont;
    QFont medFont = settings->medFont;
    QFont lgFont = settings->lgFont;

    optionUi->Band->setFont(medFont);
    optionUi->BiasCurrent->setFont(medFont);
    optionUi->BiasTee->setFont(medFont);
    optionUi->frequencyLabel->setFont(medFont);
    optionUi->IFFilter->setFont(medFont);
    optionUi->IFGain1->setFont(medFont);
    optionUi->IFGain2->setFont(medFont);
    optionUi->IFGain3->setFont(medFont);
    optionUi->IFGain4->setFont(medFont);
    optionUi->IFGain5->setFont(medFont);
    optionUi->IFGain6->setFont(medFont);
    optionUi->IFGainMode->setFont(medFont);
    optionUi->IFRCFilter->setFont(medFont);
    optionUi->label->setFont(medFont);
    optionUi->label_10->setFont(medFont);
    optionUi->label_11->setFont(medFont);
    optionUi->label_12->setFont(medFont);
    optionUi->label_13->setFont(medFont);
    optionUi->label_14->setFont(medFont);
    optionUi->label_15->setFont(medFont);
    optionUi->label_16->setFont(medFont);
    optionUi->label_17->setFont(medFont);
    optionUi->label_2->setFont(medFont);
    optionUi->label_3->setFont(medFont);
    optionUi->label_4->setFont(medFont);
    optionUi->label_5->setFont(medFont);
    optionUi->label_6->setFont(medFont);
    optionUi->label_7->setFont(medFont);
    optionUi->label_8->setFont(medFont);
    optionUi->label_9->setFont(medFont);
    optionUi->LNAEnhance->setFont(medFont);
    optionUi->LNAGain->setFont(medFont);
    optionUi->MixerFilter->setFont(medFont);
    optionUi->MixerGain->setFont(medFont);
    optionUi->RFFilter->setFont(medFont);
    optionUi->versionLabel->setFont(medFont);

    optionUi->BiasTee->addItem("Off",0);
    optionUi->BiasTee->addItem("On", 1);
    connect(optionUi->BiasTee,SIGNAL(currentIndexChanged(int)),this,SLOT(BiasTeeChanged(int)));

    optionUi->LNAGain->addItem("-5.0db",TLGE_N5_0DB);
    optionUi->LNAGain->addItem("-2.5db",TLGE_N2_5DB);
    optionUi->LNAGain->addItem("+0.0db",TLGE_P0_0DB);
    optionUi->LNAGain->addItem("+2.5db",TLGE_P2_5DB);
    optionUi->LNAGain->addItem("+5.0db",TLGE_P5_0DB);
    optionUi->LNAGain->addItem("+7.5db",TLGE_P7_5DB);
    optionUi->LNAGain->addItem("+10.0db",TLGE_P10_0DB);
    optionUi->LNAGain->addItem("+12.5db",TLGE_P12_5DB);
    optionUi->LNAGain->addItem("+15.0db",TLGE_P15_0DB);
    optionUi->LNAGain->addItem("+17.5db",TLGE_P17_5DB);
    optionUi->LNAGain->addItem("+20.0db",TLGE_P20_0DB);
    optionUi->LNAGain->addItem("+25.0db",TLGE_P25_0DB);
    optionUi->LNAGain->addItem("+30.0db",TLGE_P30_0DB);
    connect(optionUi->LNAGain,SIGNAL(currentIndexChanged(int)),this,SLOT(LNAGainChanged(int)));

    optionUi->MixerGain->addItem("4db",TMGE_P4_0DB);
    optionUi->MixerGain->addItem("12db",TMGE_P12_0DB);
    connect(optionUi->MixerGain,SIGNAL(currentIndexChanged(int)),this,SLOT(MixerGainChanged(int)));

    optionUi->IFGain1->addItem("-3dB",TIG1E_N3_0DB);
    optionUi->IFGain1->addItem("+6dB",TIG1E_P6_0DB);
    connect(optionUi->IFGain1,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGain1Changed(int)));

    optionUi->LNAEnhance->addItem("Off",TLEE_OFF);
    optionUi->LNAEnhance->addItem("0",TLEE_0);
    optionUi->LNAEnhance->addItem("1",TLEE_1);
    optionUi->LNAEnhance->addItem("2",TLEE_2);
    optionUi->LNAEnhance->addItem("3",TLEE_3);
    connect(optionUi->LNAEnhance,SIGNAL(currentIndexChanged(int)),this,SLOT(LNAEnhanceChanged(int)));

    optionUi->Band->addItem("VHF II",TBE_VHF2);
    optionUi->Band->addItem("VHF III",TBE_VHF3);
    optionUi->Band->addItem("UHF",TBE_UHF);
    optionUi->Band->addItem("L band",TBE_LBAND);
    connect(optionUi->Band,SIGNAL(currentIndexChanged(int)),this,SLOT(BandChanged(int)));
    connect(optionUi->RFFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(RFFilterChanged(int)));

    optionUi->BiasCurrent->addItem("00 L band",TBCE_LBAND);
    optionUi->BiasCurrent->addItem("01",TBCE_1);
    optionUi->BiasCurrent->addItem("10",TBCE_2);
    optionUi->BiasCurrent->addItem("11 V/U band",TBCE_VUBAND);
    connect(optionUi->BiasCurrent,SIGNAL(currentIndexChanged(int)),this,SLOT(BiasCurrentChanged(int)));

    optionUi->MixerFilter->addItem("1.9MHz",TMFE_1_9MHZ);
    optionUi->MixerFilter->addItem("2.3MHz",TMFE_2_3MHZ);
    optionUi->MixerFilter->addItem("2.7MHz",TMFE_2_7MHZ);
    optionUi->MixerFilter->addItem("3.0MHz",TMFE_3_0MHZ);
    optionUi->MixerFilter->addItem("3.4MHz",TMFE_3_4MHZ);
    optionUi->MixerFilter->addItem("3.8MHz",TMFE_3_8MHZ);
    optionUi->MixerFilter->addItem("4.2MHz",TMFE_4_2MHZ);
    optionUi->MixerFilter->addItem("4.6MHz",TMFE_4_6MHZ);
    optionUi->MixerFilter->addItem("27MHz",TMFE_27_0MHZ);
    connect(optionUi->MixerFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(MixerFilterChanged(int)));

    optionUi->IFGainMode->addItem("Linearity",TIGME_LINEARITY);
    optionUi->IFGainMode->addItem("Sensitivity",TIGME_SENSITIVITY);
    connect(optionUi->IFGainMode,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGainModeChanged(int)));

    optionUi->IFRCFilter->addItem("1.0MHz",TIRFE_1_0MHZ);
    optionUi->IFRCFilter->addItem("1.2MHz",TIRFE_1_2MHZ);
    optionUi->IFRCFilter->addItem("1.8MHz",TIRFE_1_8MHZ);
    optionUi->IFRCFilter->addItem("2.6MHz",TIRFE_2_6MHZ);
    optionUi->IFRCFilter->addItem("3.4MHz",TIRFE_3_4MHZ);
    optionUi->IFRCFilter->addItem("4.4MHz",TIRFE_4_4MHZ);
    optionUi->IFRCFilter->addItem("5.3MHz",TIRFE_5_3MHZ);
    optionUi->IFRCFilter->addItem("6.4MHz",TIRFE_6_4MHZ);
    optionUi->IFRCFilter->addItem("7.7MHz",TIRFE_7_7MHZ);
    optionUi->IFRCFilter->addItem("9.0MHz",TIRFE_9_0MHZ);
    optionUi->IFRCFilter->addItem("10.6MHz",TIRFE_10_6MHZ);
    optionUi->IFRCFilter->addItem("12.4MHz",TIRFE_12_4MHZ);
    optionUi->IFRCFilter->addItem("14.7MHz",TIRFE_14_7MHZ);
    optionUi->IFRCFilter->addItem("17.6MHz",TIRFE_17_6MHZ);
    optionUi->IFRCFilter->addItem("21.0MHz",TIRFE_21_0MHZ);
    optionUi->IFRCFilter->addItem("21.4MHz",TIRFE_21_4MHZ);
    connect(optionUi->IFRCFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(IFRCFilterChanged(int)));

    optionUi->IFGain2->addItem("0dB",TIG2E_P0_0DB);
    optionUi->IFGain2->addItem("+3dB",TIG2E_P3_0DB);
    optionUi->IFGain2->addItem("+6dB",TIG2E_P6_0DB);
    optionUi->IFGain2->addItem("+9dB",TIG2E_P9_0DB);
    connect(optionUi->IFGain2,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGain2Changed(int)));

    optionUi->IFGain3->addItem("0dB",TIG3E_P0_0DB);
    optionUi->IFGain3->addItem("+3dB",TIG3E_P3_0DB);
    optionUi->IFGain3->addItem("+6dB",TIG3E_P6_0DB);
    optionUi->IFGain3->addItem("+9dB",TIG3E_P9_0DB);
    connect(optionUi->IFGain3,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGain3Changed(int)));

    optionUi->IFGain4->addItem("0dB",TIG4E_P0_0DB);
    optionUi->IFGain4->addItem("+1dB",TIG4E_P1_0DB);
    optionUi->IFGain4->addItem("+2dB",TIG4E_P2_0DB);
    connect(optionUi->IFGain4,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGain4Changed(int)));

    optionUi->IFGain5->addItem("+3dB",TIG5E_P3_0DB);
    optionUi->IFGain5->addItem("+6dB",TIG5E_P6_0DB);
    optionUi->IFGain5->addItem("+9dB",TIG5E_P9_0DB);
    optionUi->IFGain5->addItem("+12dB",TIG5E_P12_0DB);
    optionUi->IFGain5->addItem("+15dB",TIG5E_P15_0DB);
    connect(optionUi->IFGain5,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGain5Changed(int)));

    optionUi->IFGain6->addItem("+3dB",TIG6E_P3_0DB);
    optionUi->IFGain6->addItem("+6dB",TIG6E_P6_0DB);
    optionUi->IFGain6->addItem("+9dB",TIG6E_P9_0DB);
    optionUi->IFGain6->addItem("+12dB",TIG6E_P12_0DB);
    optionUi->IFGain6->addItem("+15dB",TIG6E_P15_0DB);
    connect(optionUi->IFGain6,SIGNAL(currentIndexChanged(int)),this,SLOT(IFGain6Changed(int)));

    connect(optionUi->IFFilter,SIGNAL(currentIndexChanged(int)),this,SLOT(IFFilterChanged(int)));

    if (connected) {
        //Get current options from device
        ReadFCDOptions();
        int index =0;

        //Common to FCD and FCD+
        if (sdrDevice == FUNCUBE)
            optionUi->versionLabel->setText("FunCube: "+fcdVersion);
        else
            optionUi->versionLabel->setText("FunCube+: "+fcdVersion);

        optionUi->frequencyLabel->setText("Freq: "+QString::number(fcdFreq,'f',0));

        index = optionUi->RFFilter->findData(fcdRFFilter);
        optionUi->RFFilter->setCurrentIndex(index);

        index = optionUi->IFFilter->findData(fcdIFFilter);
        optionUi->IFFilter->setCurrentIndex(index);

        index = optionUi->IFGain1->findData(fcdIFGain1);
        optionUi->IFGain1->setCurrentIndex(index);

        index = optionUi->MixerGain->findData(fcdMixerGain);
        optionUi->MixerGain->setCurrentIndex(index);

        index = optionUi->LNAGain->findData(fcdLNAGain);
        optionUi->LNAGain->setCurrentIndex(index);

        if (sdrDevice == FUNCUBE) {
            //FCD only
            optionUi->LNAEnhance->setEnabled(true);
            optionUi->Band->setEnabled(true);
            optionUi->BiasCurrent->setEnabled(true);
            optionUi->MixerFilter->setEnabled(true);
            optionUi->IFGainMode->setEnabled(true);
            optionUi->IFRCFilter->setEnabled(true);
            optionUi->IFGain2->setEnabled(true);
            optionUi->IFGain3->setEnabled(true);
            optionUi->IFGain4->setEnabled(true);
            optionUi->IFGain5->setEnabled(true);
            optionUi->IFGain6->setEnabled(true);
            optionUi->BiasTee->setEnabled(false);

            optionUi->IFFilter->clear();  //Changes for FCD or FCD +
            optionUi->IFFilter->addItem("2.15MHz",TIFE_2_15MHZ);
            optionUi->IFFilter->addItem("2.20MHz",TIFE_2_20MHZ);
            optionUi->IFFilter->addItem("2.24MHz",TIFE_2_24MHZ);
            optionUi->IFFilter->addItem("2.28MHz",TIFE_2_28MHZ);
            optionUi->IFFilter->addItem("2.30MHz",TIFE_2_30MHZ);
            optionUi->IFFilter->addItem("2.40MHz",TIFE_2_40MHZ);
            optionUi->IFFilter->addItem("2.45MHz",TIFE_2_45MHZ);
            optionUi->IFFilter->addItem("2.50MHz",TIFE_2_50MHZ);
            optionUi->IFFilter->addItem("2.55MHz",TIFE_2_55MHZ);
            optionUi->IFFilter->addItem("2.60MHz",TIFE_2_60MHZ);
            optionUi->IFFilter->addItem("2.70MHz",TIFE_2_70MHZ);
            optionUi->IFFilter->addItem("2.75MHz",TIFE_2_75MHZ);
            optionUi->IFFilter->addItem("2.80MHz",TIFE_2_80MHZ);
            optionUi->IFFilter->addItem("2.90MHz",TIFE_2_90MHZ);
            optionUi->IFFilter->addItem("2.95MHz",TIFE_2_95MHZ);
            optionUi->IFFilter->addItem("3.00MHz",TIFE_3_00MHZ);
            optionUi->IFFilter->addItem("3.10MHz",TIFE_3_10MHZ);
            optionUi->IFFilter->addItem("3.20MHz",TIFE_3_20MHZ);
            optionUi->IFFilter->addItem("3.30MHz",TIFE_3_30MHZ);
            optionUi->IFFilter->addItem("3.40MHz",TIFE_3_40MHZ);
            optionUi->IFFilter->addItem("3.60MHz",TIFE_3_60MHZ);
            optionUi->IFFilter->addItem("3.70MHz",TIFE_3_70MHZ);
            optionUi->IFFilter->addItem("3.80MHz",TIFE_3_80MHZ);
            optionUi->IFFilter->addItem("3.90MHz",TIFE_3_90MHZ);
            optionUi->IFFilter->addItem("4.10MHz",TIFE_4_10MHZ);
            optionUi->IFFilter->addItem("4.30MHz",TIFE_4_30MHZ);
            optionUi->IFFilter->addItem("4.40MHz",TIFE_4_40MHZ);
            optionUi->IFFilter->addItem("4.60MHz",TIFE_4_60MHZ);
            optionUi->IFFilter->addItem("4.80MHz",TIFE_4_80MHZ);
            optionUi->IFFilter->addItem("5.00MHz",TIFE_5_00MHZ);
            optionUi->IFFilter->addItem("5.30MHz",TIFE_5_30MHZ);
            optionUi->IFFilter->addItem("5.50MHz",TIFE_5_50MHZ);

            index = optionUi->Band->findData(fcdBand);
            optionUi->Band->setCurrentIndex(index);
            BandChanged(index);

            index = optionUi->LNAEnhance->findData(fcdLNAEnhance);
            optionUi->LNAEnhance->setCurrentIndex(index);

            index = optionUi->BiasCurrent->findData(fcdBiasCurrent);
            optionUi->BiasCurrent->setCurrentIndex(index);

            index = optionUi->MixerFilter->findData(fcdMixerFilter);
            optionUi->MixerFilter->setCurrentIndex(index);

            index = optionUi->IFGainMode->findData(fcdIFGainMode);
            optionUi->IFGainMode->setCurrentIndex(index);

            index = optionUi->IFRCFilter->findData(fcdIFRCFilter);
            optionUi->IFRCFilter->setCurrentIndex(index);

            index = optionUi->IFGain2->findData(fcdIFGain2);
            optionUi->IFGain2->setCurrentIndex(index);

            index = optionUi->IFGain3->findData(fcdIFGain3);
            optionUi->IFGain3->setCurrentIndex(index);

            index = optionUi->IFGain4->findData(fcdIFGain4);
            optionUi->IFGain4->setCurrentIndex(index);

            index = optionUi->IFGain5->findData(fcdIFGain5);
            optionUi->IFGain5->setCurrentIndex(index);

            index = optionUi->IFGain6->findData(fcdIFGain6);
            optionUi->IFGain6->setCurrentIndex(index);

        } else {
            //FCD+ only
            optionUi->LNAEnhance->setEnabled(false);
            optionUi->Band->setEnabled(false);
            optionUi->BiasCurrent->setEnabled(false);
            optionUi->MixerFilter->setEnabled(false);
            optionUi->IFGainMode->setEnabled(false);
            optionUi->IFRCFilter->setEnabled(false);
            optionUi->IFGain2->setEnabled(false);
            optionUi->IFGain3->setEnabled(false);
            optionUi->IFGain4->setEnabled(false);
            optionUi->IFGain5->setEnabled(false);
            optionUi->IFGain6->setEnabled(false);
            optionUi->BiasTee->setEnabled(true);

            optionUi->IFFilter->clear();
            optionUi->IFFilter->addItem("200KHz",TIFE_200KHZ);
            optionUi->IFFilter->addItem("300KHz",TIFE_300KHZ);
            optionUi->IFFilter->addItem("600KHz",TIFE_600KHZ);
            optionUi->IFFilter->addItem("1536KHz",TIFE_1536KHZ);
            optionUi->IFFilter->addItem("5MHz",TIFE_5MHZ);
            optionUi->IFFilter->addItem("6MHz",TIFE_6MHZ);
            optionUi->IFFilter->addItem("7MHz",TIFE_7MHZ);
            optionUi->IFFilter->addItem("8MHz",TIFE_8MHZ);

            index = optionUi->BiasTee->findData(fcdBiasTee);
            optionUi->BiasTee->setCurrentIndex(index);

            //Call BandChanged to set RF Filter options, even though FCD+ doesn have band
            BandChanged(0);
            //Add BiasTee to UI
        }
    }
}

double FunCube::GetStartupFrequency()
{
    if (sdrDevice == SDR::FUNCUBE)
        return sFCDStartup;
    else
        return sFCDPlusStartup;
}

int FunCube::GetStartupMode()
{
    if (sdrDevice == SDR::FUNCUBE)
        return sFCDStartupMode;
    else
        return sFCDPlusStartupMode;
}

double FunCube::GetHighLimit()
{
    if (sdrDevice == SDR::FUNCUBE)
        return sFCDHigh;
    else
        return sFCDPlusHigh;
}

double FunCube::GetLowLimit()
{
    if (sdrDevice == SDR::FUNCUBE)
        return sFCDLow;
    else
        return sFCDPlusLow;
}

double FunCube::GetGain()
{
	return sGain;
}

QString FunCube::GetDeviceName()
{
    if (sdrDevice == SDR::FUNCUBE)
        return "FUNCube Dongle";
    else
        return "FunCube Dongle Plus";
}

int FunCube::GetSampleRate()
{
    if (sdrDevice == SDR::FUNCUBE)
        return 96000; //Fixed by device
    else
        return 192000;
}

int *FunCube::GetSampleRates(int &len)
{
    len = 1;
    if (sdrDevice == SDR::FUNCUBE)
        sampleRates[0] = 96000; //Always 96k
    else
        sampleRates[0] = 192000; //Always 192k

    return sampleRates;
}
void FunCube::ReadSettings()
{
    SDR::ReadSettings();
    //We need to over-ride the default sampleRate in SDR with FCD specific default
    sampleRate = GetSampleRate();

    sFCDStartup = qSettings->value("FCDStartup",162450000).toDouble();
    sFCDPlusStartup = qSettings->value("FCDPlusStartup",10000000).toDouble();
    //FCD official range is 64 to 1.7 with no gaps
    sFCDLow = qSettings->value("FCDLow",60000000).toDouble();
    sFCDHigh = qSettings->value("FCDHigh",1700000000).toDouble();
    //FCDPlus 240mhz to 420mhz gap is not handled
    sFCDPlusLow = qSettings->value("FCDPlusLow",150000).toDouble();
    sFCDPlusHigh = qSettings->value("FCDPlusHigh",1900000000).toDouble();
    sFCDStartupMode = qSettings->value("FCDStartupMode",dmFMN).toInt();
    sFCDPlusStartupMode = qSettings->value("FCDPlusStartupMode",dmAM).toInt();
    sGain = qSettings->value("Gain",0.05).toDouble();
    sFCD_PID = qSettings->value("FCD_PID",FCD_PID).toInt();
    sFCD_PLUS_PID = qSettings->value("FCD_PLUS_PID",FCD_PLUS_PID).toInt();
    sVID = qSettings->value("VID",VID).toInt();
	sOffset = qSettings->value("Offset",999885.0).toDouble();
	//All settings default to -1 which will force the device default values to be set
	fcdLNAGain = qSettings->value("LNAGain",-1).toInt();
	fcdLNAEnhance = qSettings->value("LNAEnhance",-1).toInt();
	fcdBand = qSettings->value("Band",-1).toInt();
	fcdRFFilter = qSettings->value("RFFilter",-1).toInt();
	fcdMixerGain = qSettings->value("MixerGain",-1).toInt();
    fcdBiasCurrent = qSettings->value("BiasCurrent",-1).toInt();
    fcdBiasTee = qSettings->value("BiasTee",0).toInt();
    fcdMixerFilter = qSettings->value("MixerFilter",-1).toInt();
	fcdIFGain1 = qSettings->value("IFGain1",-1).toInt();
	fcdIFGain2 = qSettings->value("IFGain2",-1).toInt();
	fcdIFGain3 = qSettings->value("IFGain3",-1).toInt();
	fcdIFGain4 = qSettings->value("IFGain4",-1).toInt();
	fcdIFGain5 = qSettings->value("IFGain5",-1).toInt();
	fcdIFGain6 = qSettings->value("IFGain6",-1).toInt();
	fcdIFGainMode = qSettings->value("IFGainMode",-1).toInt();
	fcdIFRCFilter = qSettings->value("IFRCFilter",-1).toInt();
	fcdIFFilter = qSettings->value("IFFilter",-1).toInt();
	fcdSetFreqHz = qSettings->value("SetFreqHz",false).toBool();
	fcdDCICorrection = qSettings->value("DCICorrection",0).toInt();
	fcdDCQCorrection = qSettings->value("DCQCorrection",0).toInt();
	fcdIQPhaseCorrection = qSettings->value("IQPhaseCorrection",0).toInt();
	fcdIQGainCorrection = qSettings->value("IQGainCorrection",32768).toInt();
}

void FunCube::WriteSettings()
{
    SDR::WriteSettings();
    qSettings->setValue("FCDStartup",sFCDStartup);
    qSettings->setValue("FCDPlusStartup",sFCDPlusStartup);
    qSettings->setValue("FCDLow",sFCDLow);
    qSettings->setValue("FCDHigh",sFCDHigh);
    qSettings->setValue("FCDPlusLow",sFCDPlusLow);
    qSettings->setValue("FCDPlusHigh",sFCDPlusHigh);
    qSettings->setValue("FCDStartupMode",sFCDStartupMode);
    qSettings->setValue("FCDPlusStartupMode",sFCDPlusStartupMode);
    qSettings->setValue("Gain",sGain);
    qSettings->setValue("FCD_PID",sFCD_PID);
    qSettings->setValue("FCD_PLUS_PID",sFCD_PLUS_PID);
    qSettings->setValue("VID",sVID);
	qSettings->setValue("Offset",sOffset);
	qSettings->setValue("LNAGain",fcdLNAGain);
	qSettings->setValue("LNAEnhance",fcdLNAEnhance);
	qSettings->setValue("Band",fcdBand);
	qSettings->setValue("RFFilter",fcdRFFilter);
	qSettings->setValue("MixerGain",fcdMixerGain);
	qSettings->setValue("BiasCurrent",fcdBiasCurrent);
    qSettings->setValue("BiasTee",fcdBiasTee);
    qSettings->setValue("MixerFilter",fcdMixerFilter);
	qSettings->setValue("IFGain1",fcdIFGain1);
	qSettings->setValue("IFGain2",fcdIFGain2);
	qSettings->setValue("IFGain3",fcdIFGain3);
	qSettings->setValue("IFGain4",fcdIFGain4);
	qSettings->setValue("IFGain5",fcdIFGain5);
	qSettings->setValue("IFGain6",fcdIFGain6);
	qSettings->setValue("IFGainMode",fcdIFGainMode);
	qSettings->setValue("IFRCFilter",fcdIFRCFilter);
	qSettings->setValue("IFFilter",fcdIFFilter);
	qSettings->setValue("SetFreqHz",fcdSetFreqHz);
	qSettings->setValue("DCICorrection",fcdDCICorrection);
	qSettings->setValue("DCQCorrection",fcdDCQCorrection);
	qSettings->setValue("IQPhaseCorrection",fcdIQPhaseCorrection);
	qSettings->setValue("IQGainCorrection",fcdIQGainCorrection);

	qSettings->sync();
}
