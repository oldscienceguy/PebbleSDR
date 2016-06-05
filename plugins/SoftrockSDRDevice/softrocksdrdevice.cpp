#include "softrocksdrdevice.h"
#include <QMessageBox>
#include "db.h"

SoftrockSDRDevice::SoftrockSDRDevice():DeviceInterfaceBase()
{
	initSettings("");
	optionUi = NULL;
}

SoftrockSDRDevice::~SoftrockSDRDevice()
{
}

bool SoftrockSDRDevice::initialize(CB_ProcessIQData _callback,
								   CB_ProcessBandscopeData _callbackBandscope,
								   CB_ProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callbackBandscope);
	Q_UNUSED(_callbackAudio);
	return DeviceInterfaceBase::initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
}

bool SoftrockSDRDevice::connectDevice()
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
	if (m_deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded()) {
		m_connected = true;
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
			m_connected = true;
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

bool SoftrockSDRDevice::disconnectDevice()
{
	writeSettings();
	if (m_deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded()) {
		m_connected = false;
		return true;
	}

	//usb_reset(hDev); //Same as unplugging and plugging back in
	usbUtil.ReleaseInterface(0);
	usbUtil.CloseDevice();
	//usbUtil.Exit();
	m_connected = false;
	return true;
}

void SoftrockSDRDevice::startDevice()
{
	return DeviceInterfaceBase::startDevice();
}

void SoftrockSDRDevice::stopDevice()
{
	return DeviceInterfaceBase::stopDevice();
}

void SoftrockSDRDevice::initSettings(QString fname)
{
	Q_UNUSED(fname);

	DeviceInterfaceBase::initSettings("SREnsemble");
	srEnsembleSettings = m_settings;
	DeviceInterfaceBase::initSettings("SREnsemble2M");
	srEnsemble2MSettings = m_settings;
	DeviceInterfaceBase::initSettings("SREnsemble4M");
	srEnsemble4MSettings = m_settings;
	DeviceInterfaceBase::initSettings("SREnsemble6M");
	srEnsemble6MSettings = m_settings;
	DeviceInterfaceBase::initSettings("SREnsembleLF");
	srEnsembleLFSettings = m_settings;
	DeviceInterfaceBase::initSettings("SRLite");
	srLiteSettings = m_settings;
	DeviceInterfaceBase::initSettings("SRV9");
	srV9Settings = m_settings;
	DeviceInterfaceBase::initSettings("SRFifi");
	srFifiSettings = m_settings;
}

void SoftrockSDRDevice::readSettings()
{
	int currentDeviceNumber = m_deviceNumber; //Save until we remove it from settings
	switch (m_deviceNumber)
	{
		case FiFi: m_settings = srFifiSettings; break;
		case SR_V9: m_settings = srV9Settings; break;
		case SR_LITE: m_settings = srLiteSettings; break;
		case SR_ENSEMBLE: m_settings = srEnsembleSettings; break;
		case SR_ENSEMBLE_LF: m_settings = srEnsembleLFSettings; break;
		case SR_ENSEMBLE_2M: m_settings = srEnsemble2MSettings; break;
		case SR_ENSEMBLE_4M: m_settings = srEnsemble4MSettings; break;
		case SR_ENSEMBLE_6M: m_settings = srEnsemble6MSettings; break;
		default:
			m_settings = NULL; break;
	}

	//Device Settings

	//Keep si570 frequency within 'reasonable' range, not exactly to spec
	//Standard si570 support 4000000 to 160000000
	//Limits will be based on divider settings for each radio

	//Different defaults for different devices
	switch(m_deviceNumber) {
		case SR_LITE: //Need separate settings
		case SR_V9: //Same as ensemble
		case SR_ENSEMBLE:
			//si570 / 4.0
			m_startupFrequency = 10000000;
			m_lowFrequency = 1000000;
			m_highFrequency = 40000000;
			m_startupDemodMode = dmAM;
			break;
		case SR_ENSEMBLE_2M:
			//si570 / 0.8 = 5000000 to 200000000
			m_startupFrequency = 146000000;
			m_lowFrequency = 100000000;
			m_highFrequency = 175000000;
			m_startupDemodMode = dmFMN;
			break;
		case SR_ENSEMBLE_4M:
			//si570 / 1.33 = 3000000 to 120000000
			m_startupFrequency = 70000000;
			m_lowFrequency = 60000000;
			m_highFrequency = 100000000;
			m_startupDemodMode = dmFMN;
			break;
		case SR_ENSEMBLE_6M:
			//si570 / 1.33
			m_startupFrequency = 52000000;
			m_lowFrequency = 40000000;
			m_highFrequency = 60000000;
			m_startupDemodMode = dmFMN;
			break;
		case SR_ENSEMBLE_LF:
			//Extra div/4 stage, so 1/4 normal SR
			m_startupFrequency = 1000000;
			m_lowFrequency = 150000;
			m_highFrequency = 4000000;
			m_startupDemodMode = dmAM;
			break;
		case FiFi:
			//si570 / 4.0
			m_startupFrequency = 10000000;
			m_lowFrequency = 200000;
			//highFrequency = 30000000;
			m_highFrequency = 150000000; //Test
			m_startupDemodMode = dmAM;

			//Default input device
#ifdef USE_PORT_AUDIO
			inputDeviceName = "CoreAudio:UDA1361 Input";
#endif
#ifdef USE_QT_AUDIO
			m_inputDeviceName = "UDA1361 Input";
#endif
			m_iqOrder = IQO_QI; //Fifi is normal inverted
			break;
		default:
			break;
	}

	//Set defaults, then read common settings
	DeviceInterfaceBase::readSettings();

	m_deviceNumber = currentDeviceNumber; //Restore

	sdrNumber = m_settings->value("SDRNumber",-1).toInt();
	//useABPF = qSettings->value("UseABPF",true).toInt();

}

void SoftrockSDRDevice::writeSettings()
{
	switch (m_deviceNumber)
	{
		case FiFi: m_settings = srFifiSettings; break;
		case SR_V9: m_settings = srV9Settings; break;
		case SR_LITE: m_settings = srLiteSettings; break;
		case SR_ENSEMBLE: m_settings = srEnsembleSettings; break;
		case SR_ENSEMBLE_LF: m_settings = srEnsembleLFSettings; break;
		case SR_ENSEMBLE_2M: m_settings = srEnsemble2MSettings; break;
		case SR_ENSEMBLE_4M: m_settings = srEnsemble4MSettings; break;
		case SR_ENSEMBLE_6M: m_settings = srEnsemble6MSettings; break;
		default:
			m_settings = NULL; break;
	}

	DeviceInterfaceBase::writeSettings();
	m_settings->setValue("SDRNumber",sdrNumber);

	m_settings->sync();

}

QVariant SoftrockSDRDevice::get(DeviceInterface::StandardKeys _key, QVariant _option)
{
	switch (_key) {
		//Used to select device
		case Key_PluginName:
			return "SoftRock Family";
		case Key_PluginDescription:
			return "Softrock devices";
		case Key_PluginNumDevices:
			return 8;
		//Used in titles
		case Key_DeviceName:
			switch (_option.toInt())
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
			return DeviceInterfaceBase::get(_key, _option);
	}
}

bool SoftrockSDRDevice::set(DeviceInterface::StandardKeys _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);
	double fRequested = _value.toDouble();
	switch (_key) {
		case Key_DeviceFrequency: {
			//What type of SoftRock are we using?  Determines how we calc LO
			double mult = 4;
			switch (m_deviceNumber)
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
				m_deviceFrequency = fRequested;
				return fRequested; //SoftRock sets exact freq
			} else {
				return m_deviceFrequency; //Freq didn't change, return previous value
			}
			break;
		}
		default:
		return DeviceInterfaceBase::set(_key, _value, _option);
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

// Case 0 Return the SVN build number
// http://o28.sischa.net/fifisdr/trac/browser/trunk/Software/LPC/fifisdr/fifisdr/src/softrock.c use translate.google.com

bool SoftrockSDRDevice::FiFiGetSVNNumber(quint32 *fifiSVN)
{
	quint32 svn;

	int value = 0;
	int index = 0;
	int result = usbCtrlMsgIn(0xAB, value, index, (unsigned char*)&svn, sizeof(svn));
	if (result != 0 ) {
		*fifiSVN = svn;
		return true;
	} else {
		*fifiSVN = 0;
		return false;
	}

	return true;
}

//Case 1 return firmware version string
//Value 0, Index 1 = version
bool SoftrockSDRDevice::FiFiVersion(QString *fifiVersion)
{
	unsigned char version[256];
	int value = 0;
	int index = 1;
	int length = usbCtrlMsgIn(0xAB, value, index, version, sizeof(version));
	if (length > 0 ) {
		*fifiVersion = QString((char *)version);
		return true;
	} else {
		*fifiVersion = QString();
		return false;
	}

}

/* Receiving at harmonics of the LO frequency.
* If the LO should be set to a frequency> = one of the thresholds,
* He will statdessen to one-third or one FÃŒnftel the required
* Set frequency. For application, but towards the hÃ¶here virtual frequency
* Reported.
*/
//Case 3 return 3rd harmonic
bool SoftrockSDRDevice::FiFi3rdHarmonic(quint32 * fifi3rdHarmonic)
{
	*fifi3rdHarmonic = 0;
	return true;
}

//Case 4 not used

//Case 5 return 5th harmonic
bool SoftrockSDRDevice::Fifi5thHarmonic(quint32 *fifi5thHarmonic)
{
	*fifi5thHarmonic = 0;
	return true;
}

//Case 6 return preselector mode
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

//Case 7 return preselector values
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

//Case 10 return real SI570 registers
//Case 11 return Virtual VCO factor
//Case 12 return factory starup ??
//Case 13 not used

//Case 14 return demodulator volume

//Case 15 return demodulator mode

//Case 16 return demodulator bandwidth

//Case 17 return S-Meter

//Case 18 return FM Center

//Case 19 Return preamp (ADC 0/-6 dB) volume

//Case 20 return squelch control

//Case 21 return AGC mode

//Case 0xFFFF return debug info


//OxAC Fifi Write Request



/*
Batch file for setting Fifi default values.  http://o28.sischa.net/fifisdr/trac/wiki/Frequenzabgleich

ECHO BATCH-Programmierung mittels ROCKPROG0

REM Preselektor-Betriebsart: 4 Ausg‰nge
REM Preselector mode: 4 outputs ‰ length
rockprog0 -w --presel --mode=1

REM Umschaltgrenzen der Tiefp‰sse auf FiFi-Preselektor
REM Switching threshold of ??? on FiFi preselector
rockprog0 -w --presel --index=0 --freq-from=0     --freq-to=0.123 --pattern=1
rockprog0 -w --presel --index=1 --freq-from=0.123 --freq-to=0.307 --pattern=7
rockprog0 -w --presel --index=2 --freq-from=0.307 --freq-to=0.768 --pattern=0
rockprog0 -w --presel --index=3 --freq-from=0.768 --freq-to=1.92  --pattern=2
rockprog0 -w --presel --index=4 --freq-from=1.92  --freq-to=4.8   --pattern=6
rockprog0 -w --presel --index=5 --freq-from=4.8   --freq-to=12    --pattern=5
rockprog0 -w --presel --index=6 --freq-from=12    --freq-to=30    --pattern=3
rockprog0 -w --presel --index=7 --freq-from=30    --freq-to=75    --pattern=4
rockprog0 -w --presel --index=8 --freq-from=75    --freq-to=150   --pattern=12

REM ‹brige Tabellen-Eintr‰ge lˆschen
REM Rest of table entries are not used
rockprog0 -w --presel --index=9  --freq-from=0 --freq-to=0 --pattern=0
rockprog0 -w --presel --index=10 --freq-from=0 --freq-to=0 --pattern=0
rockprog0 -w --presel --index=11 --freq-from=0 --freq-to=0 --pattern=0
rockprog0 -w --presel --index=12 --freq-from=0 --freq-to=0 --pattern=0
rockprog0 -w --presel --index=13 --freq-from=0 --freq-to=0 --pattern=0
rockprog0 -w --presel --index=14 --freq-from=0 --freq-to=0 --pattern=0

REM Standardwert f¸r Preselektor-Ausg‰nge
REM PRESELEKTOR-TABELLE WIRD NUR GESCHRIEBEN, WENN AM SCHLUSS INDEX 15 BENUTZT WIRD!
REM Default value for preselector Ed ‰ length
REM Preselector CHART IS ONLY WRITTEN WHEN AT INDEX 15 USED AT THE END!

rockprog0 -w --presel --index=15 --freq-from=0.0 --freq-to=500 --pattern=0

REM Schaltgrenzen f¸r 3. und 5. Oberwelle des LO
REM Switching limits for 3rd and 5th harmonic of the LO
rockprog0 -w --3rd --freq=35.0
rockprog0 -w --5th --freq=85.0

REM Offset zwischen LO und RF wird in Standard-Firmware nicht gebraucht
REM Offset between LO and RF is not used in standard firmware
rockprog0 -w --offset --subtract=0

REM Automatische Frequenzkorrektur
REM Automatic frequency correction
rockprog0 -w --autotune

ECHO BATCH-Programmierung beendet

*/

// Mode 0 = Use SoftRock ABPF compatibility 4 outputs
// Mode 1 = Use FiFi Preselector.  16 bands 8 outputs
// Mode 2 = 16 bands, 3 outputs, 1 UART serial output frequency
// Mode 3 =

//See params.h in Fifi LPC source and use translate.google.com to get the following
/* Preselector.
* There are different modes possible:
* 0 = ABPF, adjustable About PE0FKO or compatible software.
* Four high-active digital AusgÃ € length.
*
* 1 = 16 separate BÃ € Direction with start and stop frequencies. For each band one
* Be set * combination of eight control pins (FiFi SDR 0.1 / 0.2 only four).
* AuÃ ?? OUTSIDE the BÃ € Direction are all AusgÃ € nge low.
*
* 2 = Same as (1) but only three digital AusgÃ € length (still 16 BÃ € Direction!), And ZuSa € USEFUL
* Serial output frequency About UART.
*
* 3 = Same as (1), but there are only three AusgÃ € nge FOR the eight BÃ € Direction of
* FiFi preselector used.
* The pin X6.8 is used as PTT (PTT active -> X6.8 = low).
* Pin X6.8 of the FiFi SDR corresponds Pin SV1.12 at the FiFi preselector.
*/
bool SoftrockSDRDevice::FiFiWritePreselctorMode (quint32 mode)
{
	int result = usbCtrlMsgOut(0xAC,0, 6, (unsigned char*) &mode, sizeof(mode));
	//if (mode == 0)
	//    SetAutoBPF(true);
	return (result != 0);
}

//See softrock.c in LPCUSB directory of FiFi source for all commands


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
	if (m_deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded())
		return size; //No USB, pretend everything is working

	return usbUtil.ControlMsg(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
						request,value,index,bytes,size,500);
}

int SoftrockSDRDevice::usbCtrlMsgOut(int request, int value, int index, unsigned char *bytes, int size)
{
	if (m_deviceNumber == SR_LITE || !usbUtil.IsUSBLoaded())
		return size; //No USB, pretend everything is working

	return usbUtil.ControlMsg(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
						 request, value, index, bytes, size, 500);
}

//Dialog stuff
void SoftrockSDRDevice::selectAutomatic(bool b) {
	Q_UNUSED(b);
	if (!m_connected)
		return;
	SetAutoBPF(true);
}
void SoftrockSDRDevice::selectInput0(bool b) {
	Q_UNUSED(b);
	if (!m_connected)
		return;
	//Turn off ABPF
	SetAutoBPF(false);
	SetInputMux(0);
}
void SoftrockSDRDevice::selectInput1(bool b) {
	Q_UNUSED(b);
	if (!m_connected)
		return;
	SetAutoBPF(false);
	SetInputMux(1);
}
void SoftrockSDRDevice::selectInput2(bool b) {
	Q_UNUSED(b);
	if (!m_connected)
		return;
	SetAutoBPF(false);
	SetInputMux(2);
}
void SoftrockSDRDevice::selectInput3(bool b) {
	Q_UNUSED(b);
	if (!m_connected)
		return;
	SetInputMux(3);
}
void SoftrockSDRDevice::serialNumberChanged(int s)
{
	if (!m_connected)
		return;
	SetSerialNumber(s);
}

void SoftrockSDRDevice::setupOptionUi(QWidget *parent)
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

	//Enable/disable for different softrock-ish devices
	if (m_deviceNumber == FiFi) {
		optionUi->fifiVersionLabel->setVisible(true);
		optionUi->fifiHelp->setVisible(true);

		optionUi->serialLabel->setVisible(false);
		optionUi->automaticButton->setVisible(false);
		optionUi->filter0Button->setVisible(false);
		optionUi->filter1Button->setVisible(false);
		optionUi->filter2Button->setVisible(false);
		optionUi->filter3Button->setVisible(false);
	} else {
		optionUi->fifiVersionLabel->setVisible(false);
		optionUi->fifiHelp->setVisible(false);

		optionUi->serialLabel->setVisible(true);
		optionUi->filter0Button->setVisible(true);
		optionUi->filter1Button->setVisible(true);
		optionUi->filter2Button->setVisible(true);
		optionUi->filter3Button->setVisible(true);
	}

	//This can only be displayed when power is on
	if (m_connected) {
		if (m_deviceNumber == FiFi) {
			quint32 fifiSVN;
			QString version;
			QString label;
			if (FiFiGetSVNNumber(&fifiSVN) ) {
				if (FiFiVersion(&version)) {
					//Version 2 (192k) will return "fifisdr2" which we can check
					QTextStream(&label) << "Fifi SW Version: "<<version<<" SVN: "<<fifiSVN;
					optionUi->fifiVersionLabel->setText(label);
				} else {
					QTextStream(&label) << "Fifi SVN: "<<fifiSVN;
					optionUi->fifiVersionLabel->setText(label);
				}
			} else {
				optionUi->fifiVersionLabel->setText("Error connecting with FiFi");
			}

			quint32 mode;
			if (FiFiReadPreselectorMode(&mode)) {
				qDebug()<<"FiFi preselector mode "<<mode;
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
