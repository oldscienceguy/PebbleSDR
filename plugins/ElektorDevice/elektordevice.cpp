#include "elektordevice.h"
#include <QMessageBox>

/*
Derived from:
	Copyright (c) 2007 by G.J. Kruizinga (gjk4all@gmail.com)

	communications.c defines the functions for setting the functions
	of the Elektor Software Defined Radio (Elektor may 2007) using
	the FT-232R USB chip and the CY27EE16-8 clock generator.

*/

ElektorDevice::ElektorDevice():DeviceInterfaceBase()
{
	initSettings("Elektor");
	readSettings();
}

ElektorDevice::~ElektorDevice()
{
}

bool ElektorDevice::initialize(CB_ProcessIQData _callback,
							   CB_ProcessBandscopeData _callbackBandscope,
							   CB_ProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	m_numProducerBuffers = 50;

	usbUtil = new USBUtil(USBUtil::FTDI_D2XX);

	ftWriteBuffer = NULL;
	optionUi = NULL;
	if (!usbUtil->IsUSBLoaded()) {
		if (!usbUtil->LoadUsb()) {
			QMessageBox::information(NULL,"Pebble","USB not loaded.  Elektor communication is disabled.");
			return false;
		}
	}

	return true;
}

bool ElektorDevice::connectDevice()
{
	if (!usbUtil->IsUSBLoaded())
		return false;

	unsigned char Mask = 0xff;
	unsigned char Mode = 1; // Set asynchronous bit-bang mode
	quint32 BaudRate = 38400;

	// Initialize send buffer
	if (ftWriteBuffer == NULL) {
		ftWriteBuffer = (unsigned char *)malloc(BUFFER_SIZE * sizeof(unsigned char));
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

	//Also has a PID of 0x6001 and VID of 0x0403
	if (!usbUtil->FindDevice("A6009xER", true))
		return false;  //Didn't find Elektor

	// FT-232R initialisation
	if (!usbUtil->OpenDevice())
		return false;// FT_Open failed

	// Reset the device
	if (!usbUtil->ResetDevice()) {
		usbUtil->CloseDevice();
		return false; // Reset of device failed
	}

	// Set for Bit-Bang mode
	if (!usbUtil->SetBitMode(Mask,Mode)) {
		usbUtil->CloseDevice();
		return false; // FT_SetBitMode FAILED!
	}

	// Set the baudrate
	if (!usbUtil->SetBaudRate(BaudRate)) {
		usbUtil->CloseDevice();
		return false; // Set Baudrate failed
	}

	// Set initial portvalue to 0
	ftPortValue = 0;
	ftWriteBufferCount = 0;

	m_connected = true;
	return true;
}

bool ElektorDevice::disconnectDevice()
{
	writeSettings();

	// Close the USB link
	usbUtil->CloseDevice();

	// Free the ftWriteBuffer
	if (ftWriteBuffer)
		free(ftWriteBuffer);

	ftWriteBuffer = NULL;

	m_connected = false;
	return true;
}

void ElektorDevice::startDevice()
{
	DeviceInterfaceBase::startDevice();
}

void ElektorDevice::stopDevice()
{
	DeviceInterfaceBase::stopDevice();
}

void ElektorDevice::readSettings()
{
	DeviceInterfaceBase::readSettings();
	//Device specific settings follow
	ELEKTOR_Low = m_settings->value("Low",150000).toDouble();
	ELEKTOR_High = m_settings->value("High",30000000).toDouble();


}

void ElektorDevice::writeSettings()
{
	DeviceInterfaceBase::writeSettings();
	//Device specific settings follow
	m_settings->setValue("Low",ELEKTOR_Low);
	m_settings->setValue("High",ELEKTOR_High);

	m_settings->sync();

}

QVariant ElektorDevice::get(DeviceInterface::StandardKeys _key, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_PluginName:
			return "Elektor";
			break;
		case Key_PluginDescription:
			return "Elektor";
			break;
		case Key_DeviceName:
			return "Elektor 2007";
		case Key_StartupFrequency:
			return 10000000;
		case Key_HighFrequency:
			return ELEKTOR_High;
		case Key_LowFrequency:
			return ELEKTOR_Low;

		default:
			return DeviceInterfaceBase::get(_key, _option);
	}
}

bool ElektorDevice::set(DeviceInterface::StandardKeys _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_DeviceFrequency: {
			if (SetFrequency(_value.toDouble()) != m_deviceFrequency)
				return true;
			else
				return false;
			break;
		}
		default:
		return DeviceInterfaceBase::set(_key, _value, _option);
	}
}

void ElektorDevice::setupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
	if (optionUi != NULL)
		delete optionUi;
	optionUi = new Ui::ElektorOptions();
	optionUi->setupUi(parent);
	parent->setVisible(true);

#if 0
	QFont smFont = settings->smFont;
	QFont medFont = settings->medFont;
	QFont lgFont = settings->lgFont;
#endif

#if 0
	optionUi->automaticButton->setFont(medFont);
	optionUi->calButton->setFont(medFont);
	optionUi->mwButton->setFont(medFont);
	optionUi->pre1Button->setFont(medFont);
	optionUi->pre2Button->setFont(medFont);
	optionUi->pre3Button->setFont(medFont);
	optionUi->pre4Button->setFont(medFont);
	optionUi->swButton->setFont(medFont);
	optionUi->tenDbButton->setFont(medFont);
	optionUi->twentyDbButton->setFont(medFont);
	optionUi->widebandButton->setFont(medFont);
	optionUi->zeroDbButton->setFont(medFont);
#endif

	connect(optionUi->automaticButton,SIGNAL(clicked(bool)),this,SLOT(setInput0(bool)));
	connect(optionUi->widebandButton,SIGNAL(clicked(bool)),this,SLOT(setInput0(bool)));
	connect(optionUi->mwButton,SIGNAL(clicked(bool)),this,SLOT(setInput1(bool)));
	connect(optionUi->swButton,SIGNAL(clicked(bool)),this,SLOT(setInput2(bool)));
	connect(optionUi->calButton,SIGNAL(clicked(bool)),this,SLOT(setInput7(bool)));

	connect(optionUi->pre1Button,SIGNAL(clicked(bool)),this,SLOT(setInput3(bool)));
	connect(optionUi->pre2Button,SIGNAL(clicked(bool)),this,SLOT(setInput4(bool)));
	connect(optionUi->pre3Button,SIGNAL(clicked(bool)),this,SLOT(setInput5(bool)));
	connect(optionUi->pre4Button,SIGNAL(clicked(bool)),this,SLOT(setInput6(bool)));

	connect(optionUi->zeroDbButton,SIGNAL(clicked(bool)),this,SLOT(setAtten0(bool)));
	connect(optionUi->tenDbButton,SIGNAL(clicked(bool)),this,SLOT(setAtten1(bool)));
	connect(optionUi->twentyDbButton,SIGNAL(clicked(bool)),this,SLOT(setAtten2(bool)));
}

void ElektorDevice::producerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}

void ElektorDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	Q_UNUSED(_event);
}


int ElektorDevice::WriteBuffer()
{
	if (!usbUtil->IsUSBLoaded())
		return -1;

	quint32 ftBytesWritten;

	// Write buffer to device
	if (!usbUtil->Write(ftWriteBuffer, ftWriteBufferCount)) {
		usbUtil->CloseDevice();
		return -1;
	}
	ftBytesWritten = ftWriteBufferCount;
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
void ElektorDevice::SDA(int val)
{
	if (!usbUtil->IsUSBLoaded())
		return;

	if (val == 0)
		ftPortValue &= 0xFE; // Clear SDA bit on port value
	else
		ftPortValue |= 0x01; // Set SDA bit on port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;
}
// Push SCL to buffer
void ElektorDevice::SCL(int val)
{
	if (!usbUtil->IsUSBLoaded())
		return;

	if (val == 0)
		ftPortValue &= 0xFD; // Clear SCL bit on portvalue
	else
		ftPortValue |= 0x02; // Set SCL bit on port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;
}
// Set I2C idle state
void ElektorDevice::I2C_Init()
{
  SCL(1);
  SDA(1);
}

// Set I2C start condition
void ElektorDevice::I2C_Start()
{
  SDA(0);
  SCL(0);
}

// Set I2C stop condition
void ElektorDevice::I2C_Stop()
{
  SCL(0);
  SDA(0);
  SCL(1);
  SDA(1);
}

// Set I2C acknowledge
void ElektorDevice::I2C_Ack()
{
  SCL(0);
  SDA(0);
  SCL(1);
  SCL(0);
}

// Set I2C not-acknowledge
void ElektorDevice::I2C_Nack()
{
  SCL(0);
  SDA(1);
  SCL(1);
  SCL(0);
}

// Send a whole byte to I2C
void ElektorDevice::I2C_Byte(unsigned char byte)
{
  unsigned char mask = 0x80;

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
void ElektorDevice::setInput(int input)
{
	if (!m_connected)
		return;

	if (!usbUtil->IsUSBLoaded())
		return;

	//double newFreq = LOfreq;

	ftPortValue &= 0xE3; // clear input select bits on port value
	ftPortValue |= ((unsigned char)(input << 2) & 0x1C); // Set input select bits on port value

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
double ElektorDevice::SetFrequency(double fRequested)
{
	if (!usbUtil->IsUSBLoaded())
		return m_deviceFrequency;

	double newFreq;
	//Sanity check, make sure factors are correct
	//double chkFreq = calcNewFreq(freq);
	//if (chkFreq != freq)
	//	return;
	FindBand(fRequested);
	if (bandStep == 0)
		return m_deviceFrequency; //Reached band limits

	//Find the next higher or lower frequency that's an even multiple of bandStep
	newFreq = (int)(fRequested / bandStep) * bandStep;
	if (fRequested != newFreq && fRequested > m_deviceFrequency)
	{
		newFreq += bandStep;
	}

	int PB;
	unsigned char pump = 0;
	unsigned char DIV1N = (unsigned char)(N & 0x7F);//bits 0-7 have value, mask high order
	unsigned char P0, R09, R40, R41, R42, R44, R45, R46;

	//I think CAPLOAD is what we can change in settings to calibrate
	unsigned char CAPLOAD = 0x90; // 12 pF extra load.

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
	R40 = (unsigned char)((PB >> 8) | (pump << 2) | 0xC0);
	//R41 bits 7..0 hold PB bits 7..0
	R41 = (unsigned char)((PB & 0xFF));
	//R42 bit 7 holds P0
	//R42 bits 6..0 holds Q
	R42 = (unsigned char)((P0 << 7) | (unsigned char)(Q & 0x7F));

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
void ElektorDevice::SetBand(double low, double high)
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
void ElektorDevice::FindBand(double freq)
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
void ElektorDevice::calcFactors(double freq)
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
double ElektorDevice::calcFrequency(void)
{
	return (double)(P * (XTAL_FREQ / (Q+2)) / (N * 4));
}

// Calculate divider values for a frequency and return the actual frequency
double ElektorDevice::calcNewFreq(double freq)
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
int ElektorDevice::getInput()
{
	return (int)((ftPortValue & 0x1C) >> 2);
}
// Select the attenuator on the receiver
/*
0db
-10db
-20db
*/
void ElektorDevice::setAttenuator(int attenuator)
{
	if (!m_connected)
		return;
	if (!usbUtil->IsUSBLoaded())
		return;

	ftPortValue &= 0x9F; // clear attenuator bits on port value
	ftPortValue |= ((unsigned char)(attenuator << 5) & 0x60); // Set attenuator bits on the port value

	ftWriteBuffer[ftWriteBufferCount++] = ftPortValue;

	WriteBuffer();
}

// Return the selected attenuator
int ElektorDevice::getAttenuator()
{
	return ((int)(ftPortValue & 0x60) >> 5);
}

void ElektorDevice::setInput0(bool b) {Q_UNUSED(b); setInput(0);}
void ElektorDevice::setInput1(bool b) {Q_UNUSED(b); setInput(1);}
void ElektorDevice::setInput2(bool b) {Q_UNUSED(b); setInput(2);}
void ElektorDevice::setInput3(bool b) {Q_UNUSED(b); setInput(3);}
void ElektorDevice::setInput4(bool b) {Q_UNUSED(b); setInput(4);}
void ElektorDevice::setInput5(bool b) {Q_UNUSED(b); setInput(5);}
void ElektorDevice::setInput6(bool b) {Q_UNUSED(b); setInput(6);}
void ElektorDevice::setInput7(bool b) {Q_UNUSED(b); setInput(7);}

void ElektorDevice::setAtten0(bool b) {Q_UNUSED(b); setAttenuator(0);}
void ElektorDevice::setAtten1(bool b) {Q_UNUSED(b); setAttenuator(1);}
void ElektorDevice::setAtten2(bool b) {Q_UNUSED(b); setAttenuator(2);}

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
