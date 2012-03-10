#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "sdr.h"
#include <QSettings>
#include "ui_ElektorOptions.h"

class ElektorSDR:public SDR
{
	Q_OBJECT

public:
	ElektorSDR(Receiver *_receiver,SDRDEVICE dev,Settings *settings);
	~ElektorSDR(void);
	bool Connect();
	bool Disconnect();
	void Start();
	void Stop();

	//Sets requested freq, returns actual which may be different due to step
	double SetFrequency(double fRequested,double fCurrent); 
	void ShowOptions(); //SDR specific options page
	double GetStartupFrequency();
	int GetStartupMode();
	double GetHighLimit();
	double GetLowLimit();
	double GetGain();
	QString GetDeviceName();

	void ReadSettings();
	void WriteSettings();


	void FindBand(double freq);
	void SetBand(double low, double high);

	int WriteBuffer();
	void SDA(int val);
	void SCL(int val);
	void I2C_Init();
	void I2C_Start();
	void I2C_Stop();
	void I2C_Ack();
	void I2C_Nack();
	void I2C_Byte(UCHAR byte);
	void setInput(int input);
	double calcNewFreq(double freq);
	double calcFrequency(void);
	void calcFactors(double freq);
	int getInput();
	void setAttenuator(int attenuator);
	int getAttenuator();

	private slots:
		void setAtten0(bool b);
		void setAtten1(bool b);
		void setAtten2(bool b);
		void setInput0(bool b);
		void setInput1(bool b);
		void setInput2(bool b);
		void setInput3(bool b);
		void setInput4(bool b);
		void setInput5(bool b);
		void setInput6(bool b);
		void setInput7(bool b);

private:
	QSettings *qSettings;

	FT_HANDLE ftHandle;
#define BUFFER_SIZE 4096
#define I2C_ADDRESS 0xD2
#define XTAL_FREQ 10000000 //10mhz crystal used by CY27EE16ZE programmable clock generator

	UCHAR ftPortValue;
	UCHAR *ftWriteBuffer;
	DWORD ftWriteBufferCount;
	int P, Q, N;
	double LOfreq;
	int bandStep; //Tuning increment for selected band
	//Currently selected options
	int attenuator;
	int inputMux; //-1 = automatic

	//Dialog stuff
	QDialog *elektorOptions;
	Ui::ElektorOptions *eo;

	double ELEKTOR_Startup;
	double ELEKTOR_Low;
	double ELEKTOR_High;
	int ELEKTOR_StartupMode;
	double ELEKTOR_Gain;

};
