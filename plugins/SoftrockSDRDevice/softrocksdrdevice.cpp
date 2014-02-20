#include "softrocksdrdevice.h"
#include <QMessageBox>

SoftrockSDRDevice::SoftrockSDRDevice():DeviceInterfaceBase()
{
	InitSettings("");
	optionUi = NULL;
}

SoftrockSDRDevice::~SoftrockSDRDevice()
{
}

bool SoftrockSDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callback);
	Q_UNUSED(_framesPerBuffer);
	return true;
}

bool SoftrockSDRDevice::Connect()
{
	//Was in constructor for internal implemenation, but should be in Initialize or connect
	//Note: This uses libusb 0.1 not 1.0
	// See http://www.libusb.org/ and http://libusb.sourceforge.net/doc/ for details of API
	// Look for libusb0.dll, libusb0.sys, lib0_x86.dll, libusb0_x64.dll, libusb0_x64.sys if you have problems
	//Todo: Is there a crash bug if no libusb drivers are found on system?  Lib should handle and return error code

	//Set up libusb
	if (!usbUtil.IsUSBLoaded()) {
		//Explicit load.  DLL may not exist on users system, in which case we can only suppoprt non-USB devices like SoftRock Lite
		if (!usbUtil.LoadUsb()) {
			QMessageBox::information(NULL,"Pebble","libusb0 could not be loaded.");
			return false;
		} else {
			usbUtil.InitUSB();
		}

	}


	//No USB to connect to for SR Lite
	if (deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded()) {
		connected = true;
		return true;
	}
	int numFound = 0;
	int ret;
	while(true){
		if (!usbUtil.FindAndOpenDevice(SR_PID,SR_VID,numFound))
			return false; //No devices match and/or serial number not found

		// Default is config #0, Select config #1 for SoftRock, -1 is flag to reset
		//This stops Fifi from working because it block the Fifi audio device
		//I don't think it's needed for SoftRocks either, test
		//ret = libusb_set_configuration(hDev,1);//Not sure what config 1 is yet

		// Claim interface #0.
		ret = usbUtil.Claim_interface(0);

		//Is it the right serial number?
		//unsigned serial = dev->descriptor.iSerialNumber; //This is NOT the SoftRock serial number suffix
		int serial = GetSerialNumber();
		if (sdrNumber == -1 || serial == sdrNumber) {
			connected = true;
			//FifiGetSvn(); //We may want to add specific support for FiFi in the future

			return true; //We've got it
		}
		//Not ours, close and keep looking
		ret = usbUtil.ReleaseInterface(0);
		usbUtil.CloseDevice();
		numFound++;
	}

	return false;

}

bool SoftrockSDRDevice::Disconnect()
{
	WriteSettings();
	if (deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded()) {
		connected = false;
		return true;
	}

	//usb_reset(hDev); //Same as unplugging and plugging back in
	usbUtil.ReleaseInterface(0);
	usbUtil.CloseDevice();
	//usbUtil.Exit();
	connected = false;
	return true;
}

void SoftrockSDRDevice::Start()
{
	//Receiver handles Start() for audio input
}

void SoftrockSDRDevice::Stop()
{
}

void SoftrockSDRDevice::InitSettings(QString fname)
{
	Q_UNUSED(fname);

	DeviceInterfaceBase::InitSettings("SREnsemble");
	srEnsembleSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SREnsemble2M");
	srEnsemble2MSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SREnsemble4M");
	srEnsemble4MSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SREnsemble6M");
	srEnsemble6MSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SREnsembleLF");
	srEnsembleLFSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SRLite");
	srLiteSettings = qSettings;
	DeviceInterfaceBase::InitSettings("SRV9");
	srV9Settings = qSettings;
	DeviceInterfaceBase::InitSettings("SRFifi");
	srFifiSettings = qSettings;
}

void SoftrockSDRDevice::ReadSettings()
{
	switch (deviceNumber)
	{
		case FiFi: qSettings = srFifiSettings; break;
		case SR_V9: qSettings = srV9Settings; break;
		case SR_LITE: qSettings = srLiteSettings; break;
		case SR_ENSEMBLE: qSettings = srEnsembleSettings; break;
		case SR_ENSEMBLE_LF: qSettings = srEnsembleLFSettings; break;
		case SR_ENSEMBLE_2M: qSettings = srEnsemble2MSettings; break;
		case SR_ENSEMBLE_4M: qSettings = srEnsemble4MSettings; break;
		case SR_ENSEMBLE_6M: qSettings = srEnsemble6MSettings; break;
		default:
			qSettings = NULL; break;
	}

	//Device Settings

	//Keep si570 frequency within 'reasonable' range, not exactly to spec
	//Standard si570 support 4000000 to 160000000
	//Limits will be based on divider settings for each radio

	//Different defaults for different devices
	switch(deviceNumber) {
		case SR_LITE: //Need separate settings
		case SR_V9: //Same as ensemble
		case SR_ENSEMBLE:
			//si570 / 4.0
			startupFrequency = 10000000;
			lowFrequency = 1000000;
			highFrequency = 40000000;
			startupDemodMode = dmAM;
			iqGain = 1.0;
			break;
		case SR_ENSEMBLE_2M:
			//si570 / 0.8 = 5000000 to 200000000
			startupFrequency = 146000000;
			lowFrequency = 100000000;
			highFrequency = 175000000;
			startupDemodMode = dmFMN;
			iqGain = 1.0;
			break;
		case SR_ENSEMBLE_4M:
			//si570 / 1.33 = 3000000 to 120000000
			startupFrequency = 70000000;
			lowFrequency = 60000000;
			highFrequency = 100000000;
			startupDemodMode = dmFMN;
			iqGain = 1.0;
			break;
		case SR_ENSEMBLE_6M:
			//si570 / 1.33
			startupFrequency = 52000000;
			lowFrequency = 40000000;
			highFrequency = 60000000;
			startupDemodMode = dmFMN;
			iqGain = 1.0;
			break;
		case SR_ENSEMBLE_LF:
			//Extra div/4 stage, so 1/4 normal SR
			startupFrequency = 1000000;
			lowFrequency = 150000;
			highFrequency = 4000000;
			startupDemodMode = dmAM;
			iqGain = 1.0;
			break;
		case FiFi:
			//si570 / 4.0
			startupFrequency = 10000000;
			lowFrequency = 200000;
			highFrequency = 30000000;
			startupDemodMode = dmAM;
			//FiFi runs hot, even at lowest device setting, reduce gain
			iqGain = 0.25;
			break;
		default:
			break;
	}

	//Set defaults, then read common settings
	DeviceInterfaceBase::ReadSettings();
	sdrNumber = qSettings->value("SDRNumber",-1).toInt();
	//useABPF = qSettings->value("UseABPF",true).toInt();

}

void SoftrockSDRDevice::WriteSettings()
{
	switch (deviceNumber)
	{
		case FiFi: qSettings = srFifiSettings; break;
		case SR_V9: qSettings = srV9Settings; break;
		case SR_LITE: qSettings = srLiteSettings; break;
		case SR_ENSEMBLE: qSettings = srEnsembleSettings; break;
		case SR_ENSEMBLE_LF: qSettings = srEnsembleLFSettings; break;
		case SR_ENSEMBLE_2M: qSettings = srEnsemble2MSettings; break;
		case SR_ENSEMBLE_4M: qSettings = srEnsemble4MSettings; break;
		case SR_ENSEMBLE_6M: qSettings = srEnsemble6MSettings; break;
		default:
			qSettings = NULL; break;
	}

	DeviceInterfaceBase::WriteSettings();
	qSettings->setValue("SDRNumber",sdrNumber);

	qSettings->sync();

}

QVariant SoftrockSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	switch (_key) {
		//Used to select device
		case PluginName:
			switch (_option)
			{
				case SR_LITE: return "SR Lite - Fixed"; break;
				case FiFi: return "FiFi"; break;
				case SR_V9: return "SR V9 - ABPF"; break;
				case SR_ENSEMBLE: return "SoftRock Ensemble"; break;
				case SR_ENSEMBLE_2M: return "SR Ensemble 2M"; break;
				case SR_ENSEMBLE_4M: return "SR Ensemble 4M"; break;
				case SR_ENSEMBLE_6M: return "SR Ensemble 6M"; break;
				case SR_ENSEMBLE_LF: return "SR Ensemble LF"; break;
				default:
					return "";
			}
			break;
		case PluginDescription:
			return "Softrock devices";
			break;
		case PluginNumDevices:
			return 8;
		//Used in titles
		case DeviceName:
			switch (deviceNumber)
			{
				case SR_LITE: return "SR Lite - Fixed"; break;
				case FiFi: return "FiFi"; break;
				case SR_V9: return "SR V9 - ABPF"; break;
				case SR_ENSEMBLE: return "SoftRock Ensemble"; break;
				case SR_ENSEMBLE_2M: return "SR Ensemble 2M"; break;
				case SR_ENSEMBLE_4M: return "SR Ensemble 4M"; break;
				case SR_ENSEMBLE_6M: return "SR Ensemble 6M"; break;
				case SR_ENSEMBLE_LF: return "SR Ensemble LF"; break;
				default:
					return "";
			}

		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool SoftrockSDRDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
{
	Q_UNUSED(_option);
	double fRequested = _value.toDouble();
	switch (_key) {
		case DeviceFrequency: {
			//What type of SoftRock are we using?  Determines how we calc LO
			double mult = 4;
			switch (deviceNumber)
			{
				//SR_LITE has no USB control, just return what was requested so app can set fixed freq
				case SR_LITE:
					return fRequested;

				case FiFi:
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
			if (result) {
				deviceFrequency = fRequested;
				return fRequested; //SoftRock sets exact freq
			} else {
				return deviceFrequency; //Freq didn't change, return previous value
			}
			break;
		}
		default:
		return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void SoftrockSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void SoftrockSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

bool SoftrockSDRDevice::Version(short *major, short *minor)
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
bool SoftrockSDRDevice::RestartSoftRock()
{
	return usbCtrlMsgIn(0x0f, 0x0000, 0, NULL, 0);
}
//0x15
//sets active input in inpNum (0..3)
//Ignored If autoBPF enabled
bool SoftrockSDRDevice::SetInputMux(qint16 inpNum)
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
bool SoftrockSDRDevice::GetInputMux(qint16 &inpNum)
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
bool SoftrockSDRDevice::FilterCrossOver()
{
	return false;
}
//Helper function for Cmd 0x17 that just access filter bank 1 auto set status
bool SoftrockSDRDevice::GetAutoBPF(bool &b)
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
bool SoftrockSDRDevice::SetAutoBPF(bool b)
{
	//We are not reading back, so pass NULL for data ptr
	//We are enabling or disabling filter bank 1, index 3 specifies which
	usbCtrlMsgIn(0x17,b ? 1:0 ,3,NULL,0);
	return true;
}
//0x18
bool SoftrockSDRDevice::SetBandPass()
{
	return false;
}
//0x19
bool SoftrockSDRDevice::GetBandPass()
{
	return false;
}
//0x1A
bool SoftrockSDRDevice::SetTXLowPass()
{
	return false;
}
//0x1B
bool SoftrockSDRDevice::GetTXLowPass()
{
	return false; //Not implemented in firmware
}
//0x20
bool SoftrockSDRDevice::WriteRegister()
{
	return false;
}
//0x30
bool SoftrockSDRDevice::SetFrequencyByRegister()
{
	return false;
}
//0x31
bool SoftrockSDRDevice::SetFrequencyAdjust()
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
bool SoftrockSDRDevice::SetFrequencyByValue(double dFreq)
{
	//Todo: Do we need to protect Si570 from invalid frequency range
	//CMOS is typically 4mhz to 260mhz
	//Removed to allow FiFi to go to 200khz or 800k Si570 (x4)
//	if (dFreq < SI570_MIN || dFreq > SI570_MAX)
//		return false;
	qint32 iFreq = Freq2SRFreq(dFreq);
	int result = usbCtrlMsgOut(0x32, 0, 0, (unsigned char*)&iFreq, 4);
	return result >= 0;
}
//0x33
bool SoftrockSDRDevice::SetCrystalFrequency()
{
	return false;
}
//0x34
bool SoftrockSDRDevice::SetStartupFrequency()
{
	return false;
}
//0x35
bool SoftrockSDRDevice::SetSmoothTune()
{
	return false;
}
//0x39
bool SoftrockSDRDevice::GetFrequencyAdjust()
{
	return false;
}
//Use this for UI, it handles 4x sampling rate of LO vs actual frequency
double SoftrockSDRDevice::GetFrequency()
{
	return GetLOFrequency() / 4;
}
//0x3A
//Return LO frequency in Hz
double SoftrockSDRDevice::GetLOFrequency()
{
	qint32 iFreq = 0;
	bool res = usbCtrlMsgIn(0x3A, 0, 0, (unsigned char*)&iFreq, 4);
	if (res)
		return SRFreq2Freq(iFreq);
	else
		return 0;
}
//0x3B
bool SoftrockSDRDevice::GetSmoothTune()
{
	return false;
}
//0x3C
bool SoftrockSDRDevice::GetSRStartupFrequency()
{
	return false;
}
//0x3D
bool SoftrockSDRDevice::GetCrystalFrequency()
{
	return false;
}
//0x3F
//Returns si570 frequency control registers (reg 7 .. 12)
bool SoftrockSDRDevice::GetRegisters(short &r7,short &r8, short &r9, short &r10, short &r11, short &r12)
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
bool SoftrockSDRDevice::SetI2CAddress()
{
	return false;
}
//0x42
bool SoftrockSDRDevice::GetCPUTemperature()
{
	return false;
}
//0x43
//Serial Number
int SoftrockSDRDevice::GetSerialNumber()
{
	char serialNumber = 0; //Zero to just return
	int result = usbCtrlMsgIn(0x43, serialNumber, 0, (unsigned char *) &serialNumber, sizeof(serialNumber));
	if (result == 0)
		return -1;
	else
		return serialNumber-'0'; //Return 0 - N

}
int SoftrockSDRDevice::SetSerialNumber(int _serialNumber)
{
	char serialNumber = '0' + _serialNumber;
	int result = usbCtrlMsgIn(0x43, serialNumber, 0, (unsigned char *) &serialNumber, sizeof(serialNumber));
	if (result == 0)
		return -1;
	else
		return serialNumber-'0'; //Return 0 - N

}
//0x50
bool SoftrockSDRDevice::PTT()
{
	return false;
}
//0x51
bool SoftrockSDRDevice::GetCWLevel()
{
	return false;
}
//FiFi Specific
//0xAB Fifi Read Request
//Value 0, Index 0 = version
bool SoftrockSDRDevice::FiFiVersion(quint32 *fifiVersion)
{
	quint32 version;
	int value = 0;
	int index = 0;
	int result = usbCtrlMsgIn(0xAB, value, index, (unsigned char*)&version, sizeof(version));
	if (result != 0 ) {
		*fifiVersion = version;
		return true;
	} else {
		*fifiVersion = 0;
		return false;
	}

}

//OxAC Fifi Write Request

//index == preselector number to read (0-15)
bool SoftrockSDRDevice::FiFiReadPreselector(int preselNum, double* freq1, double *freq2, quint32 *pattern)
{
	quint8 buf[9];
	int value = 7;
	int result = usbCtrlMsgIn(0xAB, preselNum, value  , (unsigned char*)&buf, sizeof(buf));
	if (result != 0) {
		*freq1 = *((uint32_t *)&buf[0]) / (4.0 * 32.0 * 256.0 * 256.0);
		*freq2 = *((uint32_t *)&buf[4]) / (4.0 * 32.0 * 256.0 * 256.0);
		*pattern = buf[8];
		return true;
	}
	return false;
}

bool SoftrockSDRDevice::FiFiReadPreselectorMode (quint32 *mode)
{
	quint32 preselMode;
	int result = usbCtrlMsgIn(0xAB, 0, 6  , (unsigned char*)&preselMode, sizeof(preselMode));
	if (result != 0) {
		*mode = preselMode;
		return true;
	} else {
		return false;
	}
}

//See params.h in Fifi LPC source
// Mode 0 = Use SoftRock ABPF compatibility 4 outputs
// Mode 1 = Use FiFi Preselector.  16 bands 8 outputs
// Mode 2 = 16 bands, 3 outputs, 1 UART serial output frequency
// Mode 3 =
bool SoftrockSDRDevice::FiFiWritePreselctorMode (quint32 mode)
{
	int result = usbCtrlMsgOut(0xAC,0, 6, (unsigned char*) &mode, sizeof(mode));
	//if (mode == 0)
	//    SetAutoBPF(true);
	return (result != 0);
}

//See softrock.c in LPCUSB directory of FiFi source for all commands
/*
case 3: 3rd harmonic
case 5: 5th harmnonic
case 6: Preselector mode (0 - 3)
case 7: Preselector freq range and pattern
case 11: Virtual VCO factor
case 13: Set audio level (volume control)
case 19: Preamp (ADC 0/-6 dB)

*/

//Converts 11.21 Mhz.Khz freq we get from Softrock to Hz.
double SoftrockSDRDevice::SRFreq2Freq(qint32 iFreq)
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
qint32 SoftrockSDRDevice::Freq2SRFreq(double iFreq)
{
	iFreq = iFreq / 1000000;  //Convert to Mhz
	return (qint32)(iFreq * 0x200000ul);
}


//Utility functions that each SoftRock command used to send/receive data.  See Firmware.txt
int SoftrockSDRDevice::usbCtrlMsgIn(int request, int value, int index, unsigned char *bytes, int size)
{
	if (deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded())
		return size; //No USB, pretend everything is working

	return usbUtil.ControlMsg(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
						request,value,index,bytes,size,500);
}

int SoftrockSDRDevice::usbCtrlMsgOut(int request, int value, int index, unsigned char *bytes, int size)
{
	if (deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded())
		return size; //No USB, pretend everything is working

	return usbUtil.ControlMsg(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
						 request, value, index, bytes, size, 500);
}

//Dialog stuff
void SoftrockSDRDevice::selectAutomatic(bool b) {
	Q_UNUSED(b);
	if (!connected)
		return;
	SetAutoBPF(true);
}
void SoftrockSDRDevice::selectInput0(bool b) {
	Q_UNUSED(b);
	if (!connected)
		return;
	//Turn off ABPF
	SetAutoBPF(false);
	SetInputMux(0);
}
void SoftrockSDRDevice::selectInput1(bool b) {
	Q_UNUSED(b);
	if (!connected)
		return;
	SetAutoBPF(false);
	SetInputMux(1);
}
void SoftrockSDRDevice::selectInput2(bool b) {
	Q_UNUSED(b);
	if (!connected)
		return;
	SetAutoBPF(false);
	SetInputMux(2);
}
void SoftrockSDRDevice::selectInput3(bool b) {
	Q_UNUSED(b);
	if (!connected)
		return;
	SetInputMux(3);
}
void SoftrockSDRDevice::serialNumberChanged(int s)
{
	if (!connected)
		return;
	SetSerialNumber(s);
}
void SoftrockSDRDevice::fifiUseABPFChanged(bool b)
{
	useABPF = b;
	if (b)
		FiFiWritePreselctorMode(0);
	else
		FiFiWritePreselctorMode(1);
}

void SoftrockSDRDevice::SetupOptionUi(QWidget *parent)
{
	if (optionUi != NULL)
		delete optionUi;
	optionUi = new Ui::SoftRockOptions();
	optionUi->setupUi(parent);
	parent->setVisible(true);

#if 0
	//How to get global settings for each plugin?
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
#endif
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
	connect(optionUi->fifiUseABPF,SIGNAL(toggled(bool)),this,SLOT(fifiUseABPFChanged(bool)));

	//Enable/disable for different softrock-ish devices
	if (deviceNumber == FiFi) {
		optionUi->fifiVersionLabel->setVisible(true);
		optionUi->fifiHelp->setVisible(true);
		optionUi->fifiUseABPF->setVisible(true);

		optionUi->serialLabel->setVisible(false);
		optionUi->automaticButton->setVisible(false);
		optionUi->filter0Button->setVisible(false);
		optionUi->filter1Button->setVisible(false);
		optionUi->filter2Button->setVisible(false);
		optionUi->filter3Button->setVisible(false);
	} else {
		optionUi->fifiVersionLabel->setVisible(false);
		optionUi->fifiHelp->setVisible(false);
		optionUi->fifiUseABPF->setVisible(false);

		optionUi->serialLabel->setVisible(true);
		optionUi->filter0Button->setVisible(true);
		optionUi->filter1Button->setVisible(true);
		optionUi->filter2Button->setVisible(true);
		optionUi->filter3Button->setVisible(true);
	}

	//This can only be displayed when power is on
	if (connected) {
		if (deviceNumber == FiFi) {
			quint32 fifiVersion;
			if (FiFiVersion(&fifiVersion) )
				optionUi->fifiVersionLabel->setText(QString().sprintf("FiFi version: %d",fifiVersion));
			else
				optionUi->fifiVersionLabel->setText("Error connecting with FiFi");

			quint32 mode;
			if (FiFiReadPreselectorMode(&mode)) {
				optionUi->fifiUseABPF->setChecked(mode == 0);
				//qDebug()<<"FiFi preselector mode "<<mode;
			}

			double freq1,freq2;
			quint32 pattern;
			for (int i = 0; i<16; i++) {
				FiFiReadPreselector(i,&freq1, &freq2, &pattern);
				qDebug()<<"Fifi preselector "<<i<<" "<<freq1<<" to "<<freq2<<"pattern "<<pattern;
			}
		} else {
			QString serial = "";
			serial = serial.sprintf("Serial #: PE0FKO-%d",GetSerialNumber());
			optionUi->serialLabel->setText(serial);

		}
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
