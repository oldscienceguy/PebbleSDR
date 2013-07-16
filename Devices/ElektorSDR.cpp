//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "ElektorSDR.h"
#include <QString>
#include <QTextStream>
#include "qmessagebox.h"
#include "settings.h"
#include "Demod.h"

/*
Derived from:
    Copyright (c) 2007 by G.J. Kruizinga (gjk4all@gmail.com)
    
    communications.c defines the functions for setting the functions
    of the Elektor Software Defined Radio (Elektor may 2007) using
    the FT-232R USB chip and the CY27EE16-8 clock generator.
 
*/
ElektorSDR::ElektorSDR(Receiver *_receiver, SDRDEVICE dev,Settings *_settings):SDR(_receiver, dev,_settings)
{
    //If settings is NULL we're getting called just to get defaults, check in destructor
    settings = _settings;
    if (!settings)
        return;

	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif

    qSettings = new QSettings(path+"/PebbleData/elektor.ini",QSettings::IniFormat);
	ReadSettings();

	ftHandle = NULL;
	ftWriteBuffer = NULL;
	elektorOptions = NULL;
	if (!isFtdiLoaded) {
        if (!USBUtil::LoadFtdi()) {
			QMessageBox::information(NULL,"Pebble","ftd2xx.dll could not be loaded.  Elektor communication is disabled.");
			return;
		}
        isFtdiLoaded = true;

	}

}

ElektorSDR::~ElektorSDR(void)
{
    if (!settings)
        return;
	WriteSettings();
}
void ElektorSDR::Start()
{
    audioInput->StartInput(inputDeviceName, GetSampleRate());
}

void ElektorSDR::Stop()
{
	if (audioInput != NULL)
		audioInput->Stop();

}

void ElektorSDR::ReadSettings()
{
    SDR::ReadSettings();
	ELEKTOR_Startup = qSettings->value("Startup",10000000).toDouble();
	ELEKTOR_Low = qSettings->value("Low",150000).toDouble();
	ELEKTOR_High = qSettings->value("High",30000000).toDouble();
	ELEKTOR_StartupMode = qSettings->value("StartupMode",dmAM).toInt();
	ELEKTOR_Gain = qSettings->value("Gain",0.25).toDouble();

}
void ElektorSDR::WriteSettings()
{
    SDR::WriteSettings();
	qSettings->setValue("Startup",ELEKTOR_Startup);
	qSettings->setValue("Low",ELEKTOR_Low);
	qSettings->setValue("High",ELEKTOR_High);
	qSettings->setValue("StartupMode",ELEKTOR_StartupMode);
	qSettings->setValue("Gain",ELEKTOR_Gain);

	qSettings->sync();
}

bool ElektorSDR::Connect()
{
	if (!isFtdiLoaded)
		return false;

	FT_STATUS ftStatus;
	UCHAR Mask = 0xff;
	UCHAR Mode = 1; // Set asynchronous bit-bang mode
	DWORD BaudRate = 38400;

	// Initialize send buffer
	if (ftWriteBuffer == NULL) {
		ftWriteBuffer = (UCHAR *)malloc(BUFFER_SIZE * sizeof(UCHAR));
		if (ftWriteBuffer == NULL) {
			//MessageDlg("Elektor SDR: Unable to allocate command buffer",
			//	mtError, TMsgDlgButtons() << mbOK, 0);
			return false;
		}
	}

	//We may have more than 1 FTDI device plugged in, check for our serial number
	//SerialNumber	0x03694cb8 "JS001590"	char [16]
	//Description	0x03694cc8 "SDR-IQ"	char [64]

	//SerialNumber: 0x03694d1c "A6009xER"
	//Description: 0x03694d2c "FT232R USB UART"
    int deviceNumber = USBUtil::FTDI_FindDeviceBySerialNumber("A6009xER");
	if (deviceNumber == -1)
		return false;  //Didn't find Elektor

	// FT-232R initialisation
	if (ftHandle == NULL) {
		ftStatus = FT_Open(deviceNumber,&ftHandle);
		if (ftStatus != FT_OK) {
			//commError(ftStatus);
			return false; // FT_Open failed
		}
	}

	// Reset the device
	ftStatus = FT_ResetDevice(ftHandle);
	if (ftStatus != FT_OK) {
		//commError(ftStatus);
		FT_Close(ftHandle);
		return false; // Reset of device failed
	}

	// Set for Bit-Bang mode
	ftStatus = FT_SetBitMode(ftHandle, Mask, Mode);
	if (ftStatus != FT_OK) {
		//commError(ftStatus);
		FT_Close(ftHandle);
		return false; // FT_SetBitMode FAILED!
	}

	// Set the baudrate
	ftStatus = FT_SetBaudRate(ftHandle, BaudRate);
	if (ftStatus != FT_OK) {
		//commError(ftStatus);
		FT_Close(ftHandle);
		return false; // Set Baudrate failed
	}

	// Set initial portvalue to 0
	ftPortValue = 0;
	ftWriteBufferCount = 0;

	return true;
}
bool ElektorSDR::Disconnect()
{

	// Close the USB link
	if (ftHandle)
		FT_Close(ftHandle);

	// Free the ftWriteBuffer
	if (ftWriteBuffer)
		free(ftWriteBuffer);

	// Set both to NULL
	ftHandle = NULL;
	ftWriteBuffer = NULL;

	return true;
}

double ElektorSDR::GetStartupFrequency()
{
	if (sdrDevice==ELEKTOR)
		return ELEKTOR_Startup;
	else if(sdrDevice == ELEKTOR_PA)
		return ELEKTOR_Startup;
	else
		return 0;
}

int ElektorSDR::GetStartupMode()
{
	if (sdrDevice==ELEKTOR)
		return ELEKTOR_StartupMode;
	else if(sdrDevice == ELEKTOR_PA)
		return ELEKTOR_StartupMode;
	else
		return 0;
}

double ElektorSDR::GetHighLimit()
{
	if (sdrDevice==ELEKTOR)
		return ELEKTOR_High;
	else if(sdrDevice == ELEKTOR_PA)
		return ELEKTOR_High;
	else
		return 0;
}

double ElektorSDR::GetLowLimit()
{
	if (sdrDevice==ELEKTOR)
		return ELEKTOR_Low;
	else if(sdrDevice == ELEKTOR_PA)
		return ELEKTOR_Low;
	else
		return 0;
}

double ElektorSDR::GetGain()
{
	if (sdrDevice==ELEKTOR)
		return ELEKTOR_Gain;
	else if(sdrDevice == ELEKTOR_PA)
		return ELEKTOR_Gain;
	else
		return 0;
}

QString ElektorSDR::GetDeviceName()
{
	if (sdrDevice==ELEKTOR)
		return "Elektor 2007";
	else if(sdrDevice == ELEKTOR_PA)
		return "Elektor 2007 - PA";
	else
		return "";
}

int ElektorSDR::WriteBuffer()
{
	if (!isFtdiLoaded)
		return -1;

	FT_STATUS ftStatus;
	DWORD ftBytesWritten;

	// Write buffer to device
	ftStatus = FT_Write(ftHandle, ftWriteBuffer, ftWriteBufferCount, &ftBytesWritten);
	if (ftStatus != FT_OK) {
		//commError(ftStatus);
		FT_Close(ftHandle);
		return -1;
	}

	// Reset buffer counter to zero
	ftWriteBufferCount = 0;

	return (int)ftBytesWritten;
}
/*
No Firmware, FTDI 232R chip is controlled directly via USB
FTDI 232R chip is used in bit-bang mode and controls 8 data lines directly
2 lines are used to control I2C bus (SDA & SCL)
	Pin 1 TXD -> SDA
	Pin 5 RXD ->SCL
2 lines aer use to control IF amplification of receiver
	Pin 9 DSR
	Pin 10 DCD
3 lines control the input mux (8 input choices)
	Pin 3 RTS -> Mux A
	Pin 11 CTS -> Mux B
	Pin 2 DTR -> Mux C

*/

// Push SDA to buffer
void ElektorSDR::SDA(int val)
{
	if (!isFtdiLoaded)
		return;

	if (val == 0)
		ftPortValue &= 0xFE; // Clear SDA bit on port value
	else
		ftPortValue |= 0x01; // Set SDA bit on port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;
}
// Push SCL to buffer
void ElektorSDR::SCL(int val)
{
	if (!isFtdiLoaded)
		return;

	if (val == 0)
		ftPortValue &= 0xFD; // Clear SCL bit on portvalue
	else
		ftPortValue |= 0x02; // Set SCL bit on port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;
}
// Set I2C idle state
void ElektorSDR::I2C_Init()
{
  SCL(1);
  SDA(1);
}

// Set I2C start condition
void ElektorSDR::I2C_Start()
{
  SDA(0);
  SCL(0);
}

// Set I2C stop condition
void ElektorSDR::I2C_Stop()
{
  SCL(0);
  SDA(0);
  SCL(1);
  SDA(1);
}

// Set I2C acknowledge
void ElektorSDR::I2C_Ack()
{
  SCL(0);
  SDA(0);
  SCL(1);
  SCL(0);
}

// Set I2C not-acknowledge
void ElektorSDR::I2C_Nack()
{
  SCL(0);
  SDA(1);
  SCL(1);
  SCL(0);
}

// Send a whole byte to I2C
void ElektorSDR::I2C_Byte(UCHAR byte)
{
  UCHAR mask = 0x80;

  while (mask > 0) {
    SDA(byte & mask);
    SCL(1);
    SCL(0);
    mask >>= 1;
  }

  // Read ack from slave
  SDA(1);
  SCL(1);
  SCL(0);
}
// Select an input on the receiver (0-7)
/*
1: Wideband:  Audio freq shunted to ground
2: Medium Wave:  1.6mhz low pass filter
3. Short Wave: 1.6mhz high pass filter
4. Ext: PC1 header
5. Ext: no header
6. Ext: no header
7. Calibration:  5 mhz signal
*/
void ElektorSDR::setInput(int input)
{
	if (!isFtdiLoaded)
		return;

	//double newFreq = LOfreq;

	ftPortValue &= 0xE3; // clear input select bits on port value
	ftPortValue |= ((UCHAR)(input << 2) & 0x1C); // Set input select bits on port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;

	// Set calibration frequency
	//if (input == 7)
	//	newFreq = calcNewFreq(5012500); // Set for the test frequency on calibration input

	//Sends command and new freq if cal
	WriteBuffer();
	//SetFrequency(newFreq); // Update frequency

	//if ((input == 7) && CallBack)
	//	(* CallBack)(-1, 101, 0, NULL); // Call callback on frequency update
}

// Write the frequency to te device
/*
Note: This will only set frequency in increments as set by calcFactors, not exact frequency
To get exact frequency, it seems we need to apply mixer value
*/
double ElektorSDR::SetFrequency(double fRequested, double fCurrent)
{
	if (!isFtdiLoaded)
		return fCurrent;

	double newFreq;
	//Sanity check, make sure factors are correct
	//double chkFreq = calcNewFreq(freq);
	//if (chkFreq != freq)
	//	return;
	FindBand(fRequested);
	if (bandStep == 0)
		return fCurrent; //Reached band limits

	//Find the next higher or lower frequency that's an even multiple of bandStep
	newFreq = (int)(fRequested / bandStep) * bandStep;
	if (fRequested != newFreq && fRequested > fCurrent)
	{
		newFreq += bandStep;
	} 

	int PB;
	UCHAR pump = 0;
	UCHAR DIV1N = (UCHAR)(N & 0x7F);//bits 0-7 have value, mask high order
	UCHAR P0, R09, R40, R41, R42, R44, R45, R46;

	//I think CAPLOAD is what we can change in settings to calibrate
	UCHAR CAPLOAD = 0x90; // 12 pF extra load.

	// Set crosspoint switch matrix registers
	R44 = 0x02; R45 = 0x8E; R46 = 0x47; // Select the N=1 divider bank
	if (N == 2) {
		R44 = 0x02; R45 = 0x8E; R46 = 0x87; // Select the N=2 static divider bank
		DIV1N = 8;
	}

	// Set clock source enabled
	R09 =  0x20;  // Clock 5 (LO) enabled
	if (getInput() == 7)
		R09 = 0x24; // Clock 5 (LO) and clock 3 (5MHz ref) enabled

	// Set actual PB and P0 registers
	P0 = P % 2;
	PB = (int)((P - P0) / 2 - 4);

	//Charge pump settings for max stability.  From Cypress CY27EE16ZE data sheet
	if (P < 45) pump = 0;
	else if (P <480) pump = 1;
	else if (P < 640) pump = 2;
	else if (P <800) pump = 3;
	else pump = 4;

	// fill divider registers
	//R40 bits 1..0 hold PB bits 9..8, shift right by 8 to set 
	//R40 bits 4..2 hold pump, shift pump left by 2 to set
	//R40 bits 7..6 are always 1, 0xc0 to set
	R40 = (UCHAR)((PB >> 8) | (pump << 2) | 0xC0);
	//R41 bits 7..0 hold PB bits 7..0
	R41 = (UCHAR)((PB & 0xFF));
	//R42 bit 7 holds P0
	//R42 bits 6..0 holds Q
	R42 = (UCHAR)((P0 << 7) | (UCHAR)(Q & 0x7F));

	// Set I2C to idle mode
	I2C_Init();

	// Send Xtal capacitance correction
	I2C_Start();
	I2C_Byte(I2C_ADDRESS);
	I2C_Byte(0x13);
	I2C_Byte(CAPLOAD);
	I2C_Stop();

	// Send P and Q dividers
	I2C_Start();
	I2C_Byte(I2C_ADDRESS);
	I2C_Byte(0x40);
	I2C_Byte(R40);
	I2C_Byte(R41);
	I2C_Byte(R42);
	I2C_Stop();

	// Send clock source
	I2C_Start();
	I2C_Byte(I2C_ADDRESS);
	I2C_Byte(0x09);
	I2C_Byte(R09);
	I2C_Stop();

	// Send output divider N1
	I2C_Start();
	I2C_Byte(I2C_ADDRESS);
	I2C_Byte(0x0C);
	I2C_Byte(DIV1N);
	I2C_Stop();

	// Send clock select matrix
	I2C_Start();
	I2C_Byte(I2C_ADDRESS);
	I2C_Byte(0x44);
	I2C_Byte(R44);
	I2C_Byte(R45);
	I2C_Byte(R46);
	I2C_Stop();

	// Write to actual device
	WriteBuffer();

	// Set actual LO frequency
	LOfreq = newFreq; //calcFrequency();
	return newFreq;
}
//Todo: standardize bands across receivers.
//Broadcast, SW, Ham, etc
void ElektorSDR::SetBand(double low, double high)
{	//N & Q values taken from SDRElektor.pas which shipped with receiver
	if (low==130000 && high == 800000)
		{N = 100; Q = 48; bandStep=1000;}
	else if (low==400000 && high == 1600000)
		{N = 50; Q = 48; bandStep=9000;}
	else if (low==800000 && high == 3200000)
		{N = 25; Q = 48; bandStep=2000;}
	else if (low==2000000 && high == 8000000)
		{N = 10; Q = 48; bandStep=5000;}
	else if (low==4000000 && high == 16000000)
		{N = 5; Q = 48; bandStep=10000;}
	else if (low==10000000 && high == 30000000)
		{N = 4; Q = 23; bandStep=25000;}
}
//Finds an appropriate band for freq and use
//Todo: add useage param, ie SW, Ham, etc
//Todo: add FindBPF to select input filter mux
/*
	// Automatic input select
	if ((freq < 1500000) && (getInput() == 2))
		setInput(1); // Select MW input if SW was selected
	else if ((freq >= 1500000) && (getInput() == 1))
		setInput(2); // Select SW input if MW was selected
*/
void ElektorSDR::FindBand(double freq)
{
	//Find values for N & Q, Set input, Set Step
	if (freq < 130000) {
		bandStep = 0; //Outside range
		return;
	}
	else if (freq>=130000 && freq < 800000)
		{N = 100; Q = 48; bandStep=500;}
	//else if (low==400000 && high == 1600000)
	//	{N = 50; Q = 48; BandStep=1000;}
	else if (freq>=800000 && freq < 3200000)
		{N = 25; Q = 48; bandStep=2000;}
	else if (freq>=3200000 && freq < 8000000)
		{N = 10; Q = 48; bandStep=5000;}
	else if (freq >=8000000 && freq< 16000000)
		{N = 5; Q = 48; bandStep=10000;}
	else if (freq>=16000000 && freq< 30000000)
		{N = 4; Q = 23; bandStep=25000;}
	else {
		bandStep=0; //Outside range
		return;
	}

	// Calculate P based on the Q and N values
	P = (int)((freq * 4 * N)/(XTAL_FREQ / (Q+2)));
	//Make sure P is valid
	if (P < 8 || P > 2055)
	{
		//int err=0;//Todo: Error handling
	}

}
/*
Q and N values are chosen to set the desired step, then P is calculated to give an LO freq of 4X displayed freq
*/
// Calculate the divider values for a frequency
void ElektorSDR::calcFactors(double freq)
{
	//Step size is 1/N * 10^5
	// Search values for Q and N
	if (freq < 800000) {
		Q = 23; N = 125; 			    // Step size = 800Hz
	}
	else if (freq < 3000000) {
		Q = 23; N = 32; 			    // Step size = 3125Hz
	}
	else if (freq < 12500000) {
		Q = 23; N = 8; 			    // Step size = 12500Hz
	}
	else {
		Q = 78; N = 2; 			    // Step size = 15625Hz
	}

	// Calculate P based on the Q and N values
	P = (int)((freq * 4 * N)/(XTAL_FREQ / (Q+2)));
}

/*
From Cypress CY27EE16ZE Datasheet
PLL Frequency calculations
	REF = Desired frequency
	REF = REF / (Q+2)	//Q Counter step
	REF = REF * P		//P Counter step
	REF = REF / N		//Post divider step
*/
// Calculate the frequency for the current divider values
double ElektorSDR::calcFrequency(void)
{
	return (double)(P * (XTAL_FREQ / (Q+2)) / (N * 4));
}

// Calculate divider values for a frequency and return the actual frequency
double ElektorSDR::calcNewFreq(double freq)
{
	double newFreq;

	calcFactors(freq);
	newFreq = calcFrequency();

	// Make sure we can go up and down
	if ((newFreq == LOfreq) && (freq < LOfreq))
		P--;
	else if ((newFreq == LOfreq) && (freq > LOfreq))
		P++;

	return calcFrequency();
}
// Return the selected input
int ElektorSDR::getInput()
{
	return (int)((ftPortValue & 0x1C) >> 2);
}
// Select the attenuator on the receiver
/*
0db
-10db
-20db
*/
void ElektorSDR::setAttenuator(int attenuator)
{
	if (!isFtdiLoaded)
		return;

	ftPortValue &= 0x9F; // clear attenuator bits on port value
	ftPortValue |= ((UCHAR)(attenuator << 5) & 0x60); // Set attenuator bits on the port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;

	WriteBuffer();
}

// Return the selected attenuator
int ElektorSDR::getAttenuator()
{
	return ((int)(ftPortValue & 0x60) >> 5);
}

void ElektorSDR::setInput0(bool b) {setInput(0);}
void ElektorSDR::setInput1(bool b) {setInput(1);}
void ElektorSDR::setInput2(bool b) {setInput(2);}
void ElektorSDR::setInput3(bool b) {setInput(3);}
void ElektorSDR::setInput4(bool b) {setInput(4);}
void ElektorSDR::setInput5(bool b) {setInput(5);}
void ElektorSDR::setInput6(bool b) {setInput(6);}
void ElektorSDR::setInput7(bool b) {setInput(7);}

void ElektorSDR::setAtten0(bool b) {setAttenuator(0);}
void ElektorSDR::setAtten1(bool b) {setAttenuator(1);}
void ElektorSDR::setAtten2(bool b) {setAttenuator(2);}

void ElektorSDR::ShowOptions()
{
    QFont smFont = settings->smFont;
    QFont medFont = settings->medFont;
    QFont lgFont = settings->lgFont;

	if (elektorOptions == NULL)
	{
		elektorOptions = new QDialog();
		eo = new Ui::ElektorOptions();
		eo->setupUi(elektorOptions);

        eo->automaticButton->setFont(medFont);
        eo->calButton->setFont(medFont);
        eo->cancelButton->setFont(medFont);
        eo->mwButton->setFont(medFont);
        eo->okButton->setFont(medFont);
        eo->pre1Button->setFont(medFont);
        eo->pre2Button->setFont(medFont);
        eo->pre3Button->setFont(medFont);
        eo->pre4Button->setFont(medFont);
        eo->swButton->setFont(medFont);
        eo->tenDbButton->setFont(medFont);
        eo->twentyDbButton->setFont(medFont);
        eo->widebandButton->setFont(medFont);
        eo->zeroDbButton->setFont(medFont);
		
		connect(eo->automaticButton,SIGNAL(clicked(bool)),this,SLOT(setInput0(bool)));
		connect(eo->widebandButton,SIGNAL(clicked(bool)),this,SLOT(setInput0(bool)));
		connect(eo->mwButton,SIGNAL(clicked(bool)),this,SLOT(setInput1(bool)));
		connect(eo->swButton,SIGNAL(clicked(bool)),this,SLOT(setInput2(bool)));
		connect(eo->calButton,SIGNAL(clicked(bool)),this,SLOT(setInput7(bool)));
		connect(eo->pre1Button,SIGNAL(clicked(bool)),this,SLOT(setInput3(bool)));
		connect(eo->pre2Button,SIGNAL(clicked(bool)),this,SLOT(setInput4(bool)));
		connect(eo->pre3Button,SIGNAL(clicked(bool)),this,SLOT(setInput5(bool)));
		connect(eo->pre4Button,SIGNAL(clicked(bool)),this,SLOT(setInput6(bool)));
		connect(eo->zeroDbButton,SIGNAL(clicked(bool)),this,SLOT(setAtten0(bool)));
		connect(eo->tenDbButton,SIGNAL(clicked(bool)),this,SLOT(setAtten1(bool)));
		connect(eo->twentyDbButton,SIGNAL(clicked(bool)),this,SLOT(setAtten2(bool)));
	}
	elektorOptions->show();
}
#if (0)
//---------------------------------------------------------------------------
#include <Dialogs.hpp>
#include "main.h"
#include "communications.h"
#include "../D2XX/bin/ftd2xx.h"
//---------------------------------------------------------------------------


// Functions
void commError(FT_STATUS ftStatus)
{
  char Message[64];

  strcpy(Message, "Elektor SDR: ");

  switch (ftStatus) {
    case FT_OK: // Should never occur
      strcat(Message, "OK");
      break;
    case FT_INVALID_HANDLE:
      strcat(Message, "Invalid handle");
      break;
    case FT_DEVICE_NOT_FOUND:
      strcat(Message, "Device not found");
      break;
    case FT_DEVICE_NOT_OPENED:
      strcat(Message, "Device not opened");
      break;
    case FT_IO_ERROR:
      strcat(Message, "I/O error");
      break;
    case FT_INSUFFICIENT_RESOURCES:
      strcat(Message, "Insufficient resources");
      break;
    case FT_INVALID_PARAMETER:
      strcat(Message, "Invalid parameters");
      break;
    case FT_INVALID_BAUD_RATE:
      strcat(Message, "Invalid baud rate");
      break;
    case FT_DEVICE_NOT_OPENED_FOR_ERASE:
      strcat(Message, "Device not opened for erase");
      break;
    case FT_DEVICE_NOT_OPENED_FOR_WRITE:
      strcat(Message, "Device not opened for write");
      break;
    case FT_FAILED_TO_WRITE_DEVICE:
      strcat(Message, "Failed to write to device");
      break;
    case FT_EEPROM_READ_FAILED:
      strcat(Message, "EEprom read failed");
      break;
    case FT_EEPROM_WRITE_FAILED:
      strcat(Message, "EEprom write failed");
      break;
    case FT_EEPROM_ERASE_FAILED:
      strcat(Message, "EEprom erase failed");
      break;
    case FT_EEPROM_NOT_PRESENT:
      strcat(Message, "EEprom not present");
      break;
    case FT_EEPROM_NOT_PROGRAMMED:
      strcat(Message, "EEprom not programmed");
      break;
    case FT_INVALID_ARGS:
      strcat(Message, "Invalid arguments");
      break;
    case FT_NOT_SUPPORTED:
      strcat(Message, "Action not supported");
      break;
    case FT_OTHER_ERROR:
      strcat(Message, "Other error");
      break;
    default: // Should also never occur.
      strcat(Message, "Unknown status");
  }

  MessageDlg(Message, mtError, TMsgDlgButtons() << mbOK, 0);
}

#endif
