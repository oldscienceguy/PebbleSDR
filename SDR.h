#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
Base class for SDR Receivers
*/
#include <QObject>
#include <QString>
#include <QThread>
#include <QSettings>
#include <QDialog>
#include "QSemaphore"

#include "Devices/usbutil.h"
#include "audio.h"
#include "ui/ui_iqbalanceoptions.h"
#include "ui/ui_sdr.h"

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
        ELEKTOR, ELEKTOR_PA, SDR_IQ_USB,
		HPSDR_USB, HPSDR_TCP, SDRWIDGET, FUNCUBE,
        NOSDR, FILE, DVB_T, FUNCUBE_PLUS, SDR_IP_TCP};

	typedef enum IQORDER {IQ,QI,IONLY,QONLY} IQORDER;
    SDR(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
	~SDR(void);

    static SDR *Factory(Receiver *receiver, SDR::SDRDEVICE dev, Settings *settings);

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
    virtual int* GetSampleRates(int &len); //Returns array of allowable rates and length of array as ref

    //Display device option widget in settings dialog
    virtual void SetupOptionUi(QWidget *parent);
    //Called by settings to write device options to ini file
    virtual void WriteOptionUi() {}

	SDRDEVICE GetDevice();
	void SetDevice(SDRDEVICE m);

    //Proucer-Consumer buffer management
    void InitProducerConsumer(int _numDataBufs, int _producerBufferSize);

    bool IsFreeBufferAvailable();
    void AcquireFreeBuffer();
    void ReleaseFreeBuffer() {semNumFreeBuffers->release();}
    void IncrementProducerBuffer() {nextProducerDataBuf = (nextProducerDataBuf +1 ) % numDataBufs;}

    void AcquireFilledBuffer();
    void ReleaseFilledBuffer() {semNumFilledBuffers->release();}
    void IncrementConsumerBuffer() {nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;}


    typedef enum STARTUP {SETFREQ = 0, LASTFREQ, DEFAULTFREQ} STARTUP;

//Begin settings testing - Initial copy from settings.h
    static void ShowSdrOptions(bool b);

    //Hack, these should eventually be access methods
    STARTUP startup;
    double startupFreq;
    double lastFreq;
    int lastMode;
    int lastDisplayMode; //Spectrum, waterfall, etc
    QString inputDeviceName;
    QString outputDeviceName;
    int sampleRate;
    //If Output Sample Rate is above this, then we try to reduce it by skipping samples when we output
    int decimateLimit;
    bool postMixerDecimate; //If true, then downsample to decimate limit after mixer
    int framesPerBuffer;
    double dbOffset; //DB calibration for spectrum and smeter
    int sdrNumber; //For SoftRocks, selects last digit in serial number
    //Increment for left-right and up-down in spectrum display
    int leftRightIncrement;
    int upDownIncrement;

    double iqGain; //Normalize device so incoming IQ levels are consistent
    IQORDER iqOrder;
    //Image rejection (iqbalance) factors for this device
    double iqBalanceGain;
    double iqBalancePhase;
    bool iqBalanceEnable;
//End settings testing

protected:
    static QDialog *sdrOptions;

    void ReadSettings();
    void WriteSettings();
    QSettings *qSettings;

	Audio *audioInput;
	Receiver *receiver;
	SDRDEVICE sdrDevice;
	double startupFrequency; //0 means auto-set
	Settings *settings;
	bool isLibUsbLoaded;
	bool isFtdiLoaded;
    int sampleRates[10]; //Max 10 for testing

    //Producer/Consumer buffer management
    int numDataBufs; //Producer/Consumer buffers
    unsigned char **producerBuffer; //Array of buffers
    int producerBufferSize;
    int nextProducerDataBuf;
    int nextConsumerDataBuf;
    bool freeBufferOverflow;
    bool filledBufferOverflow;
    /*
      NumFreeBuffers starts at NUMDATABUFS and is decremented (acquire()) everytime the producer thread has new data.
      If it ever gets to zero, it will block forever and program will hang until consumer thread catches up.
      NumFreeBuffers is incremented (release()) in consumer thread when a buffer has been processed and can be reused.


    */
    QSemaphore *semNumFreeBuffers; //Init to NUMDATABUFS
    QSemaphore *semNumFilledBuffers;

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

//Replacement for windows Sleep() function
class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs);
    static void msleep(unsigned long msecs);
    static void sleep(unsigned long secs);
};
