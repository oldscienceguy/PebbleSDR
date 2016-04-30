#ifndef ELEKTORDEVICE_H
#define ELEKTORDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "usbutil.h"
#include "ui_elektoroptions.h"

class ElektorDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	ElektorDevice();
	~ElektorDevice();

	//Required
	bool initialize(CB_ProcessIQData _callback,
					CB_ProcessBandscopeData _callbackBandscope,
					CB_ProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	QVariant get(StandardKeys _key, QVariant _option = 0);
	bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

private:
	bool connectDevice();
	bool disconnectDevice();
	void startDevice();
	void stopDevice();
	void readSettings();
	void writeSettings();
	//Display device option widget in settings dialog
	void setupOptionUi(QWidget *parent);

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	USBUtil *usbUtil;

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
	void I2C_Byte(unsigned char byte);
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

#define BUFFER_SIZE 4096
#define I2C_ADDRESS 0xD2
#define XTAL_FREQ 10000000 //10mhz crystal used by CY27EE16ZE programmable clock generator

	unsigned char ftPortValue;
	unsigned char *ftWriteBuffer;
	quint32 ftWriteBufferCount;
	int P, Q, N;
	double LOfreq;
	int bandStep; //Tuning increment for selected band
	//Currently selected options
	int attenuator;
	int inputMux; //-1 = automatic

	Ui::ElektorOptions *optionUi;

	double ELEKTOR_Low;
	double ELEKTOR_High;


	double SetFrequency(double fRequested);
};
#endif // ELEKTORDEVICE_H
