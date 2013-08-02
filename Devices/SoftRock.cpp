//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SoftRock.h"
#include <qmessagebox.h>
#include "Settings.h"
#include "Demod.h"
#define _USE_MATH_DEFINES
#include "math.h" //just for modf()!  replace

SoftRock::SoftRock(Receiver *_receiver,SDRDEVICE dev,Settings *_settings) : SDR(_receiver, dev,_settings)
{
    //If settings is NULL we're getting called just to get defaults, check in destructor
    settings = _settings;
    if (!settings)
        return;

	hDev = NULL;

    InitSettings("softrock");
    ReadSettings();

    optionUi = NULL;

	//Note: This uses libusb 0.1 not 1.0
	// See http://www.libusb.org/ and http://libusb.sourceforge.net/doc/ for details of API
	// Look for libusb0.dll, libusb0.sys, lib0_x86.dll, libusb0_x64.dll, libusb0_x64.sys if you have problems
	//Todo: Is there a crash bug if no libusb drivers are found on system?  Lib should handle and return error code

	//Set up libusb
	if (!isLibUsbLoaded) {
		//Explicit load.  DLL may not exist on users system, in which case we can only suppoprt non-USB devices like SoftRock Lite
        if (!USBUtil::LoadLibUsb()) {
			QMessageBox::information(NULL,"Pebble","libusb0 could not be loaded.  SoftRock communication is disabled.");
		}

	}
    if (!isLibUsbLoaded) {
        isLibUsbLoaded = USBUtil::InitLibUsb();
	}

}
SoftRock::~SoftRock()
{
    if (!settings)
        return;
	WriteSettings();
	if (hDev && isLibUsbLoaded) {
#ifdef LIBUSB_VERSION1
        libusb_release_interface(hDev,0);
        USBUtil::CloseDevice(hDev);
        libusb_exit(NULL);
#else
#endif
	}
}
void SoftRock::Start()
{
    audioInput->StartInput(inputDeviceName, GetSampleRate());
}

void SoftRock::Stop()
{
	if (audioInput != NULL)
		audioInput->Stop();

}

void SoftRock::ReadSettings()
{
    SDR::ReadSettings();

	//Device Settings
    sdrNumber = qSettings->value("sdrNumber",-1).toInt();

	//Keep si570 frequency within 'reasonable' range, not exactly to spec
	//Standard si570 support 4000000 to 160000000
	//Limits will be based on divider settings for each radio

	//si570 / 4.0
	SR_ENSEMBLE_Startup = qSettings->value("SR_ENSEMBLE/Startup",10000000).toDouble();
	SR_ENSEMBLE_Low = qSettings->value("SR_ENSEMBLE/Low",1000000).toDouble();
	SR_ENSEMBLE_High = qSettings->value("SR_ENSEMBLE/High",40000000).toDouble();
	SR_ENSEMBLE_StartupMode = qSettings->value("SR_ENSEMBLE/StartupMode",dmAM).toInt();
	SR_ENSEMBLE_Gain = qSettings->value("SR_ENSEMBLE/Gain",1.0).toDouble();

	//si570 / 0.8 = 5000000 to 200000000
	SR_ENSEMBLE_2M_Startup = qSettings->value("SR_ENSEMBLE_2M/Startup",146000000).toDouble();
	SR_ENSEMBLE_2M_Low = qSettings->value("SR_ENSEMBLE_2M/Low",100000000).toDouble();
	SR_ENSEMBLE_2M_High = qSettings->value("SR_ENSEMBLE_2M/High",175000000).toDouble();
	SR_ENSEMBLE_2M_StartupMode = qSettings->value("SR_ENSEMBLE_2M/StartupMode",dmFMN).toInt();
	SR_ENSEMBLE_2M_Gain = qSettings->value("SR_ENSEMBLE_2M/Gain",1.0).toDouble();

	//si570 / 1.33 = 3000000 to 120000000
	SR_ENSEMBLE_4M_Startup = qSettings->value("SR_ENSEMBLE_4M/Startup",70000000).toDouble();
	SR_ENSEMBLE_4M_Low = qSettings->value("SR_ENSEMBLE_4M/Low",60000000).toDouble();
	SR_ENSEMBLE_4M_High = qSettings->value("SR_ENSEMBLE_4M/High",100000000).toDouble();
		SR_ENSEMBLE_4M_StartupMode = qSettings->value("SR_ENSEMBLE_4M/StartupMode",dmFMN).toInt();
	SR_ENSEMBLE_4M_Gain = qSettings->value("SR_ENSEMBLE_4M/Gain",1.0).toDouble();

	//si570 / 1.33
	SR_ENSEMBLE_6M_Startup = qSettings->value("SR_ENSEMBLE_6M/Startup",52000000).toDouble();
	SR_ENSEMBLE_6M_Low = qSettings->value("SR_ENSEMBLE_6M/Low",40000000).toDouble();
	SR_ENSEMBLE_6M_High = qSettings->value("SR_ENSEMBLE_6M/High",60000000).toDouble();
		SR_ENSEMBLE_6M_StartupMode = qSettings->value("SR_ENSEMBLE_6M/StartupMode",dmFMN).toInt();
	SR_ENSEMBLE_6M_Gain = qSettings->value("SR_ENSEMBLE_6M/Gain",1.0).toDouble();

	//Extra div/4 stage, so 1/4 normal SR
	SR_ENSEMBLE_LF_Startup = qSettings->value("SR_ENSEMBLE_LF/Startup",1000000).toDouble();
	SR_ENSEMBLE_LF_Low = qSettings->value("SR_ENSEMBLE_LF/Low",150000).toDouble();
	SR_ENSEMBLE_LF_High = qSettings->value("SR_ENSEMBLE_LF/High",4000000).toDouble();
	SR_ENSEMBLE_LF_StartupMode = qSettings->value("SR_ENSEMBLE_LF/StartupMode",dmAM).toInt();
	SR_ENSEMBLE_LF_Gain = qSettings->value("SR_ENSEMBLE_LF/Gain",1.0).toDouble();

}
void SoftRock::WriteSettings()
{
    SDR::WriteSettings();

    qSettings->setValue("sdrNumber",sdrNumber);

	qSettings->setValue("SR_ENSEMBLE/Startup",SR_ENSEMBLE_Startup);
	qSettings->setValue("SR_ENSEMBLE/Low",SR_ENSEMBLE_Low);
	qSettings->setValue("SR_ENSEMBLE/High",SR_ENSEMBLE_High);
	qSettings->setValue("SR_ENSEMBLE/StartupMode",SR_ENSEMBLE_StartupMode);
	qSettings->setValue("SR_ENSEMBLE/Gain",SR_ENSEMBLE_Gain);

	qSettings->setValue("SR_ENSEMBLE_2M/Startup",SR_ENSEMBLE_2M_Startup);
	qSettings->setValue("SR_ENSEMBLE_2M/Low",SR_ENSEMBLE_2M_Low);
	qSettings->setValue("SR_ENSEMBLE_2M/High",SR_ENSEMBLE_2M_High);
	qSettings->setValue("SR_ENSEMBLE_2M/StartupMode",SR_ENSEMBLE_2M_StartupMode);
	qSettings->setValue("SR_ENSEMBLE_2M/Gain",SR_ENSEMBLE_2M_Gain);

	qSettings->setValue("SR_ENSEMBLE_4M/Startup",SR_ENSEMBLE_4M_Startup);
	qSettings->setValue("SR_ENSEMBLE_4M/Low",SR_ENSEMBLE_4M_Low);
	qSettings->setValue("SR_ENSEMBLE_4M/High",SR_ENSEMBLE_4M_High);
	qSettings->setValue("SR_ENSEMBLE_4M/StartupMode",SR_ENSEMBLE_4M_StartupMode);
	qSettings->setValue("SR_ENSEMBLE_4M/Gain",SR_ENSEMBLE_4M_Gain);

	qSettings->setValue("SR_ENSEMBLE_6M/Startup",SR_ENSEMBLE_6M_Startup);
	qSettings->setValue("SR_ENSEMBLE_6M/Low",SR_ENSEMBLE_6M_Low);
	qSettings->setValue("SR_ENSEMBLE_6M/High",SR_ENSEMBLE_6M_High);
	qSettings->setValue("SR_ENSEMBLE_6M/StartupMode",SR_ENSEMBLE_6M_StartupMode);
	qSettings->setValue("SR_ENSEMBLE_6M/Gain",SR_ENSEMBLE_6M_Gain);

	qSettings->setValue("SR_ENSEMBLE_LF/Startup",SR_ENSEMBLE_LF_Startup);
	qSettings->setValue("SR_ENSEMBLE_LF/Low",SR_ENSEMBLE_LF_Low);
	qSettings->setValue("SR_ENSEMBLE_LF/High",SR_ENSEMBLE_LF_High);
	qSettings->setValue("SR_ENSEMBLE_LF/StartupMode",SR_ENSEMBLE_LF_StartupMode);
	qSettings->setValue("SR_ENSEMBLE_LF/Gain",SR_ENSEMBLE_LF_Gain);

	qSettings->sync();
}

bool SoftRock::Connect()
{
	//No USB to connect to for SR Lite
    if (sdrDevice == SR_LITE || !isLibUsbLoaded) {
        connected = true;
		return true;
    }
	int numFound = 0;
    int ret;
	while(true){
        hDev = USBUtil::LibUSB_FindAndOpenDevice(SR_PID,SR_VID,numFound);
		if (hDev == NULL)
			return false; //No devices match and/or serial number not found

        // Default is config #0, Select config #1 for SoftRock, -1 is flag to reset
        //This stops Fifi from working because it block the Fifi audio device
        //I don't think it's needed for SoftRocks either, test
        //ret = libusb_set_configuration(hDev,1);//Not sure what config 1 is yet

		// Claim interface #0.
        ret = libusb_claim_interface(hDev,0);

		//Is it the right serial number?
		//unsigned serial = dev->descriptor.iSerialNumber; //This is NOT the SoftRock serial number suffix
		int serial = GetSerialNumber();
        if (sdrNumber == -1 || serial == sdrNumber) {
            connected = true;
            //FifiGetSvn(); //We may want to add specific support for FiFi in the future

			return true; //We've got it
        }
		//Not ours, close and keep looking
        ret = libusb_release_interface(hDev,0);
        USBUtil::CloseDevice(hDev);
		numFound++;
	}
	
	return false;
}
bool SoftRock::Disconnect()
{
    if (sdrDevice == SR_LITE || !isLibUsbLoaded) {
        connected = false;
		return true;
    }

	//usb_reset(hDev); //Same as unplugging and plugging back in
	if (hDev) {
        libusb_release_interface(hDev,0);
        USBUtil::CloseDevice(hDev);
		hDev = NULL;
	}
    connected = false;
	return true;
}

double SoftRock::GetStartupFrequency()
{
	switch (sdrDevice)
	{
	case SR_V9: return SR_ENSEMBLE_Startup;
	case SR_ENSEMBLE: return SR_ENSEMBLE_Startup;
	case SR_ENSEMBLE_LF: return SR_ENSEMBLE_LF_Startup;
	case SR_ENSEMBLE_2M: return SR_ENSEMBLE_2M_Startup;
	case SR_ENSEMBLE_4M: return SR_ENSEMBLE_4M_Startup;
	case SR_ENSEMBLE_6M: return SR_ENSEMBLE_6M_Startup;
	default:
		return 0;
	}
}

int SoftRock::GetStartupMode()
{
	switch (sdrDevice)
	{
	case SR_V9: return SR_ENSEMBLE_StartupMode;
	case SR_ENSEMBLE: return SR_ENSEMBLE_StartupMode;
	case SR_ENSEMBLE_LF: return SR_ENSEMBLE_LF_StartupMode;
	case SR_ENSEMBLE_2M: return SR_ENSEMBLE_2M_StartupMode;
	case SR_ENSEMBLE_4M: return SR_ENSEMBLE_4M_StartupMode;
	case SR_ENSEMBLE_6M: return SR_ENSEMBLE_6M_StartupMode;
	default:
		return 0;
	}
}

double SoftRock::GetHighLimit()
{
	switch (sdrDevice)
	{
	case SR_V9: return SR_ENSEMBLE_High;
	case SR_ENSEMBLE: return SR_ENSEMBLE_High;
	case SR_ENSEMBLE_LF: return SR_ENSEMBLE_LF_High;
	case SR_ENSEMBLE_2M: return SR_ENSEMBLE_2M_High;
	case SR_ENSEMBLE_4M: return SR_ENSEMBLE_4M_High;
	case SR_ENSEMBLE_6M: return SR_ENSEMBLE_6M_High;
	default:
		return 0;
	}
}

double SoftRock::GetLowLimit()
{
	switch (sdrDevice)
	{
	case SR_V9: return SR_ENSEMBLE_Low;
	case SR_ENSEMBLE: return SR_ENSEMBLE_Low;
	case SR_ENSEMBLE_LF: return SR_ENSEMBLE_LF_Low;
	case SR_ENSEMBLE_2M: return SR_ENSEMBLE_2M_Low;
	case SR_ENSEMBLE_4M: return SR_ENSEMBLE_4M_Low;
	case SR_ENSEMBLE_6M: return SR_ENSEMBLE_6M_Low;
	default:
		return 0;
	}
}

double SoftRock::GetGain()
{
	switch (sdrDevice)
	{
	case SR_V9: return SR_ENSEMBLE_Gain;
	case SR_ENSEMBLE: return SR_ENSEMBLE_Gain;
	case SR_ENSEMBLE_LF: return SR_ENSEMBLE_LF_Gain;
	case SR_ENSEMBLE_2M: return SR_ENSEMBLE_2M_Gain;
	case SR_ENSEMBLE_4M: return SR_ENSEMBLE_4M_Gain;
	case SR_ENSEMBLE_6M: return SR_ENSEMBLE_6M_Gain;
	default:
		return 1.0;
	}
}

QString SoftRock::GetDeviceName()
{
	switch (sdrDevice)
	{
	case SR_LITE: return "SR Lite - Fixed"; break;
	case SR_V9: return "SR V9 - ABPF"; break;
	case SR_ENSEMBLE: return "SoftRock Ensemble"; break;
	case SR_ENSEMBLE_2M: return "SR Ensemble 2M"; break;
	case SR_ENSEMBLE_4M: return "SR Ensemble 4M"; break;
	case SR_ENSEMBLE_6M: return "SR Ensemble 6M"; break;
	case SR_ENSEMBLE_LF: return "SR Ensemble LF"; break;
	default:
		return "";
	}
}



//Utility functions that each SoftRock command used to send/receive data.  See Firmware.txt
int SoftRock::usbCtrlMsgIn(int request, int value, int index, unsigned char *bytes, int size)
{
	if (sdrDevice == SR_LITE || !isLibUsbLoaded)
		return size; //No USB, pretend everything is working
#ifdef LIBUSB_VERSION1
    int ret = USBUtil::ControlMsg(hDev,LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
                        request,value,index,bytes,size,500);
    return ret;
#else
    return USBUtil::ControlMsg(hDev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                         request, value, index, bytes, size, 500);
#endif
}

int SoftRock::usbCtrlMsgOut(int request, int value, int index, unsigned char *bytes, int size)
{
	if (sdrDevice == SR_LITE || !isLibUsbLoaded)
		return size; //No USB, pretend everything is working
#ifdef LIBUSB_VERSION1
    return USBUtil::ControlMsg(hDev, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
                         request, value, index, bytes, size, 500);
#else
    return USBUtil::ControlMsg(hDev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                         request, value, index, bytes, size, 500);
#endif
}
bool SoftRock::Version(short *major, short *minor)
{
	qint16 version;
    int result = usbCtrlMsgIn(0x00, 0x0E00, 0, (unsigned char*)&version, sizeof(version));
	// if the return value is 2, the variable version will give the major and minor
	// version number in the high and low byte.
	if (result == 2)
	{
		*major = version >> 8; //high order byte
		*minor = version & 0xff; //low order byte, mask high
	}
	return result == 2;
}
bool SoftRock::Restart()
{
	return usbCtrlMsgIn(0x0f, 0x0000, 0, NULL, 0);
}
//0x15
//sets active input in inpNum (0..3)
//Ignored If autoBPF enabled
bool SoftRock::SetInputMux(qint16 inpNum)
{
	//DDR byte has 2 bits set which indicate input or output for each I/O port
	//We want I/O to be output so we can change input mux.  Confusing huh?
	short ddr = 3;  //set bit 0 and bit 1 to output
	//INP byte has 2 bits set.  If we're output mode, which we are, these are the values to set each bit to
	qint16 inp;
    int result = usbCtrlMsgIn(0x15, ddr, inpNum, (unsigned char*)&inp, sizeof(inp));
	return result==sizeof(inp);
}
//0x16
//returns active input in inp, 0..3
bool SoftRock::GetInputMux(qint16 &inpNum)
{
	qint16 inp;
    int result = usbCtrlMsgIn(0x16,0,0,(unsigned char*)&inp,sizeof(inp));
	if (result == sizeof(inp))
	{
		inpNum = inp;
		return true;
	}
	return false;
}
//0x17
bool SoftRock::FilterCrossOver()
{
	return false;
}
//Helper function for Cmd 0x17 that just access filter bank 1 auto set status
bool SoftRock::GetAutoBPF(bool &b)
{
	//Get the filter data for the 1st (RX) bank and check enabled index
	quint16 filterCrossOver[16];
    int result = usbCtrlMsgIn(0x17,0,255,(unsigned char*)filterCrossOver,sizeof(filterCrossOver));
	if (result == 8) //4 RX filters = 3 cross over points (6 bytes) + 2 bytes for boolean flag
	{
		b = filterCrossOver[3] == 1;
		return true;
	}
	return false;
}
bool SoftRock::SetAutoBPF(bool b)
{
	//We are not reading back, so pass NULL for data ptr
	//We are enabling or disabling filter bank 1, index 3 specifies which
	usbCtrlMsgIn(0x17,b ? 1:0 ,3,NULL,0);
	return true;
}
//0x18
bool SoftRock::SetBandPass()
{
	return false;
}
//0x19
bool SoftRock::GetBandPass()
{
	return false;
}
//0x1A
bool SoftRock::SetTXLowPass()
{
	return false;
}
//0x1B
bool SoftRock::GetTXLowPass()
{
	return false; //Not implemented in firmware
}
//0x20
bool SoftRock::WriteRegister()
{
	return false;
}
//0x30
bool SoftRock::SetFrequencyByRegister()
{
	return false;
}
//0x31
bool SoftRock::SetFrequencyAdjust()
{
	return false;
}
/*
Frequency calculation can be done in firmware, PE0FKO DLL, or direct
We're not using DLL, so we do our own calc and leave firmware in standard configuration
Goal is to have dial read actual freq and hide all this from user

Frequency calculations.  D=Dial Frequency, L = LO Frequency
SoftRock V9, Ensemble, 
	L = D * 4
Ensemble VHF
	2 mtr: L = D * 0.8			//144mhz - 148mhz tunes 28.6 - 29.6 without special handling
	4 mtr: L = D * 1.33333333	//70 - 74 tunes 23.333 - 24.667
	6 mtr: L = D * 1.33333333	//50 - 54 tunes 16.6 - 18
Ensemble LF
	TBD
*/
//0x32
//Handles ABPF, offsets, etc.
//dFreq is LO  Mhz, ie 40 
bool SoftRock::SetFrequencyByValue(double dFreq)
{
	//Todo: Do we need to protect Si570 from invalid frequency range
	//CMOS is typically 4mhz to 260mhz
	if (dFreq < SI570_MIN || dFreq > SI570_MAX)
		return false;
	qint32 iFreq = Freq2SRFreq(dFreq);
    int result = usbCtrlMsgOut(0x32, 0, 0, (unsigned char*)&iFreq, 4);
	return result >= 0;
}
//0x33
bool SoftRock::SetCrystalFrequency()
{
	return false;
}
//0x34
bool SoftRock::SetStartupFrequency()
{
	return false;
}
//0x35
bool SoftRock::SetSmoothTune()
{
	return false;
}
//0x39
bool SoftRock::GetFrequencyAdjust()
{
	return false;
}
//Use this for UI, it handles 4x sampling rate of LO vs actual frequency
double SoftRock::GetFrequency()
{
	return GetLOFrequency() / 4;
} 
double SoftRock::SetFrequency(double fRequested, double fCurrent) //In Hz
{
	if (!isLibUsbLoaded)
		return fRequested;

	//What type of SoftRock are we using?  Determines how we calc LO
	double mult = 4; 
	switch (sdrDevice)
	{
	//SR_LITE has no USB control, just return what was requested so app can set fixed freq
	case SR_LITE:
		return fRequested;
		
	case SR_V9:
	case SR_ENSEMBLE:
	case SR_ENSEMBLE_LF:
		mult = 4;
		break;
	case SR_ENSEMBLE_2M:
		mult = 0.8;
		break;
	case SR_ENSEMBLE_4M:
		mult = 1.33333333;
		break;
	case SR_ENSEMBLE_6M:
		mult = 1.33333333;
		break;
	default:
		mult = 1;
	}
	bool result = SetFrequencyByValue(fRequested * mult);
	if (result)
		return fRequested; //SoftRock sets exact freq
	else
		return fCurrent; //Freq didn't change, return previous value
}

//Converts 11.21 Mhz.Khz freq we get from Softrock to Hz. 
double SoftRock::SRFreq2Freq(qint32 iFreq)
{
	//Mhz are in high order 11 bits, Khz in low order 21 bits
	//Assume this is because of some PIC limitation
	//21 bits = 0 to 2097151
	//11 bit = 0 to 2047
	double dFreq;
	dFreq = (double)iFreq / 0x200000; //(1UL << 21) = 0x20000, why calc every time?
	//Convert to Hz
	dFreq *= 1000000;
	//And get rid of decimal value (round up)
	//Firmware examples doesn't do this, not sure why it works
	modf(dFreq+.5,&dFreq);
	return dFreq;
}
//Converts from Freq in Hz to SR 11.21 format
qint32 SoftRock::Freq2SRFreq(double iFreq)
{
	iFreq = iFreq / 1000000;  //Convert to Mhz
	return (qint32)(iFreq * 0x200000ul);
}
//0x3A
//Return LO frequency in Hz
double SoftRock::GetLOFrequency()
{
	qint32 iFreq = 0;
    bool res = usbCtrlMsgIn(0x3A, 0, 0, (unsigned char*)&iFreq, 4);
	if (res)
		return SRFreq2Freq(iFreq);
	else
		return 0;
}
//0x3B
bool SoftRock::GetSmoothTune()
{
	return false;
}
//0x3C
bool SoftRock::GetSRStartupFrequency()
{
	return false;
}
//0x3D
bool SoftRock::GetCrystalFrequency()
{
	return false;
}
//0x3F
//Returns si570 frequency control registers (reg 7 .. 12)
bool SoftRock::GetRegisters(short &r7,short &r8, short &r9, short &r10, short &r11, short &r12)
{
	char buf[6];
    int result = usbCtrlMsgIn(0x3f,0,0,(unsigned char *)buf,6);
	if (result != 0)
	{
		r7=buf[0];
		r8=buf[1];
		r9=buf[2];
		r10=buf[3];
		r11=buf[4];
		r12=buf[5];
	}
	return result != 0;
}
//0x41
bool SoftRock::SetI2CAddress()
{
	return false;
}
//0x42
bool SoftRock::GetCPUTemperature()
{
	return false;
}
//0x43
//Serial Number
int SoftRock::GetSerialNumber()
{
	char serialNumber = 0; //Zero to just return
    int result = usbCtrlMsgIn(0x43, serialNumber, 0, (unsigned char *) &serialNumber, sizeof(serialNumber));
	if (result == 0)
		return -1;
	else
		return serialNumber-'0'; //Return 0 - N

}
int SoftRock::SetSerialNumber(int _serialNumber)
{
	char serialNumber = '0' + _serialNumber; 
    int result = usbCtrlMsgIn(0x43, serialNumber, 0, (unsigned char *) &serialNumber, sizeof(serialNumber));
	if (result == 0)
		return -1;
	else
		return serialNumber-'0'; //Return 0 - N

}
//0x50
bool SoftRock::PTT()
{
	return false;
}
//0x51
bool SoftRock::GetCWLevel()
{
	return false;
}

//FiFi Specific
//0xAB Fifi Read
void SoftRock::FifiGetSvn()
{
    quint32 svn;
    int result = usbCtrlMsgIn(0xAB, 0, 0, (unsigned char *) &svn, sizeof(svn));
    if (result == 0)
        return;
    else
        qDebug()<<svn;

}
//OxAC Fifi Write

//Dialog stuff
void SoftRock::selectAutomatic(bool b) {
    if (!connected)
        return;
	SetAutoBPF(true); 
}
void SoftRock::selectInput0(bool b) {
    if (!connected)
        return;
    //Turn off ABPF
	SetAutoBPF(false);
	SetInputMux(0);
}
void SoftRock::selectInput1(bool b) {
    if (!connected)
        return;
    SetAutoBPF(false);
	SetInputMux(1);
}
void SoftRock::selectInput2(bool b) {
    if (!connected)
        return;
    SetAutoBPF(false);
	SetInputMux(2);
}
void SoftRock::selectInput3(bool b) {
    if (!connected)
        return;
    SetInputMux(3);
}
void SoftRock::serialNumberChanged(int s)
{
    if (!connected)
        return;
    SetSerialNumber(s);
}
void SoftRock::SetupOptionUi(QWidget *parent)
{
    if (optionUi != NULL)
        delete optionUi;
    optionUi = new Ui::SoftRockOptions();
    optionUi->setupUi(parent);
    parent->setVisible(true);

    QFont smFont = settings->smFont;
    QFont medFont = settings->medFont;
    QFont lgFont = settings->lgFont;

    optionUi->automaticButton->setFont(medFont);
    optionUi->filter0Button->setFont(medFont);
    optionUi->filter1Button->setFont(medFont);
    optionUi->filter2Button->setFont(medFont);
    optionUi->filter3Button->setFont(medFont);
    optionUi->label->setFont(medFont);
    optionUi->serialBox->setFont(medFont);
    optionUi->serialLabel->setFont(medFont);
    optionUi->si570Label->setFont(medFont);
    optionUi->versionLabel->setFont(medFont);

    optionUi->serialBox->addItem("0",0);
    optionUi->serialBox->addItem("1",1);
    optionUi->serialBox->addItem("2",2);
    optionUi->serialBox->addItem("3",3);
    optionUi->serialBox->addItem("4",4);
    optionUi->serialBox->addItem("5",5);
    optionUi->serialBox->addItem("6",6);
    optionUi->serialBox->addItem("7",7);
    optionUi->serialBox->addItem("8",8);
    optionUi->serialBox->addItem("9",9);


    //connects
    connect(optionUi->automaticButton,SIGNAL(clicked(bool)),this,SLOT(selectAutomatic(bool)));
    connect(optionUi->filter0Button,SIGNAL(clicked(bool)),this,SLOT(selectInput0(bool)));
    connect(optionUi->filter1Button,SIGNAL(clicked(bool)),this,SLOT(selectInput1(bool)));
    connect(optionUi->filter2Button,SIGNAL(clicked(bool)),this,SLOT(selectInput2(bool)));
    connect(optionUi->filter3Button,SIGNAL(clicked(bool)),this,SLOT(selectInput3(bool)));
    connect(optionUi->serialBox,SIGNAL(currentIndexChanged(int)),this,SLOT(serialNumberChanged(int)));


    //This can only be displayed when power is on
    if (connected) {
        //Test: If we can see version then comm to SR is ok
        short vMaj = 0,vMin = 0;
        QString ver = "";
        if (Version(&vMaj, &vMin))
            ver = ver.sprintf("Version: %d.%d",vMaj,vMin);
        else
            ver = "Error connecting with SoftRock";
        optionUi->versionLabel->setText(ver);

        QString si570 = "";
        si570 = si570.sprintf("Si570: %.0f Hz",GetLOFrequency());
        optionUi->si570Label->setText(si570);

        QString serial = "";
        serial = serial.sprintf("Serial #: PE0FKO-%d",GetSerialNumber());
        optionUi->serialLabel->setText(serial);

        //Show current BPF option
        qint16 inp;
        bool enabled;
        if (GetAutoBPF(enabled))
        {
            if (enabled)
                optionUi->automaticButton->setChecked(true);
            else if (GetInputMux(inp))
            {
                if (inp == 0) optionUi->filter0Button->setChecked(true);
                else if (inp == 1) optionUi->filter1Button->setChecked(true);
                else if (inp == 2) optionUi->filter2Button->setChecked(true);
                else if (inp == 3) optionUi->filter3Button->setChecked(true);
            }
        }
        //Show current min/max settings (div by 4 to show actual frequency)
        //short r7,r8,r9,r10,r11,r12;
        //GetRegisters(r7,r8,r9,r10,r11,r12); //Don't know where the min/max data is stored

        int sn = GetSerialNumber();
        optionUi->serialBox->setCurrentIndex(sn);
    }

}
