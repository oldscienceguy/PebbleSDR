#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
Base class for SDR Receivers
Common support for USB interfaces
FTDI dlls and .h available at http://www.ftdichip.com

*/
#include <QObject>
#include <QString>
#include <QThread>
#include <QSettings>
#include <QDialog>
//Avoid, look for altern to PVOID etc definitions
#include "windows.h"
#include "ftd2xx.h" //Is there a cross plaftorm version that doesn't use WINAPI crap
#include "usb.h" //Libusb
#include "audio.h"
#include "ui_iqbalanceoptions.h"

class Settings; //Can't include settings.h due to circular dependencies
class Receiver;
class SDRProducerThread;
class SDRConsumerThread;

class SDR:public QObject
{
	Q_OBJECT
	friend class SDRProducerThread; //Thread has access to private data
	friend class SDRConsumerThread;

public:
	enum SDRDEVICE {SR_LITE=1, SR_V9, SR_ENSEMBLE, SR_ENSEMBLE_2M,
		SR_ENSEMBLE_4M, SR_ENSEMBLE_6M, SR_ENSEMBLE_LF,
		ELEKTOR, ELEKTOR_PA, SDR_IQ,
		HPSDR_USB, HPSDR_TCP, SDRWIDGET, FUNCUBE,
		NOSDR};	
	typedef enum IQORDER {IQ,QI,IONLY,QONLY} IQORDER;

	SDR(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
	~SDR(void);

	static SDR *Factory(Receiver *receiver, Settings *settings);

	virtual bool Connect()=0;
	virtual bool Disconnect()=0;
	virtual double SetFrequency(double fRequested,double fCurrent)=0;
	virtual void ShowOptions()=0;
	//If SDR device is not using sound card, start/stop thread that returns data
	//Ignored unless overridden
	virtual void Start()=0;
	virtual void Stop()=0;
	virtual double GetStartupFrequency()=0;
	virtual int GetStartupMode()=0;
	virtual double GetHighLimit()=0;
	virtual double GetLowLimit()=0;
	virtual double GetGain()=0;
	virtual QString GetDeviceName()=0;
	//Sample rate for some devices, like SDR-IQ, is dependent on bandwidth
	virtual int GetSampleRate();

	SDRDEVICE GetDevice();
	void SetDevice(SDRDEVICE m);

	//USB Utilities
	bool LoadLibUsb();
	bool LoadFtdi();

	//Returns device # for serialNumber or -1 if not found
	int FTDI_FindDeviceBySerialNumber(QString serialNumber);
	int FTDI_FindDeviceByName(QString deviceName);
	//LibUSB
	struct usb_device * LibUSB_FindDevice(int PID, int VID, int multiple);

	//Settings
	void ShowIQOptions();
	//Image rejection (iqbalance) factors for this device
	double sIQBalanceGain;
	double sIQBalancePhase;
	bool sIQBalanceEnable;
	IQORDER sIQOrder;


public slots:
	//IQBalance
	void phaseChanged(int v);
	void gainChanged(int v);
	void enabledChanged(bool b);
	void automaticChanged(bool b);
	void resetClicked();
	void saveClicked();
	void IQOrderChanged(int);


protected:
	Ui_IQBalanceOptions *iqBalanceOptions;
	QDialog *iqDialog;

	void ReadSettings(QSettings *settings);
	void WriteSettings(QSettings *settings);

	Audio *audioInput;
	Receiver *receiver;
	SDRDEVICE sdrDevice;
	double startupFrequency; //0 means auto-set
	Settings *settings;
	bool isLibUsbLoaded;
	bool isFtdiLoaded;

	bool isThreadRunning;
	SDRProducerThread *producerThread;
	SDRConsumerThread *consumerThread;
	//Derived classes implement device specific functions for producer/consumer thread
	//We could pass method pointers to stop() and run(), but this is simpler for now
	virtual void StopProducerThread();
	virtual void RunProducerThread();
	virtual void StopConsumerThread();
	virtual void RunConsumerThread();

};

//Generic thread that can be used in producer/consumer models for devices that don't use soundcard
class SDRProducerThread:public QThread
{
	Q_OBJECT
public:
	SDRProducerThread(SDR * s);
	void run();
	void stop();
	void setRefresh(int ms);

private:
	SDR *sdr;
	bool doRun;
	int msSleep;
};
class SDRConsumerThread:public QThread
{
	Q_OBJECT
public:
	SDRConsumerThread(SDR * s);
	void run();
	void stop();
	void setRefresh(int ms);

private:
	SDR *sdr;
	bool doRun;
	int msSleep;
};
