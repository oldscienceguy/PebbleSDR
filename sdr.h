#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
Base class for SDR Receivers
*/
#include "global.h"

#include <QObject>
#include <QString>
#include <QThread>
#include <QSettings>
#include <QDialog>
#include "QSemaphore"

#include "devices/usbutil.h"
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
        HPSDR_USB, HPSDR_TCP, SPARE1, FUNCUBE,
        NOSDR, FILE, DVB_T, FUNCUBE_PLUS, SDR_IP_TCP};

	typedef enum IQORDER {IQ,QI,IONLY,QONLY} IQORDER;
    typedef enum STARTUP {SETFREQ = 0, LASTFREQ, DEFAULTFREQ} STARTUP;

    SDR(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
	~SDR(void);

    static SDR *Factory(Receiver *receiver, SDR::SDRDEVICE dev, Settings *settings);

	virtual bool Connect()=0;
	virtual bool Disconnect()=0;
	virtual double SetFrequency(double fRequested,double fCurrent)=0;
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
    //Assume each device uses audio input.  Devices that don't should over-ride and return false to hide options
    virtual bool UsesAudioInput() {return true;}

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

    void ShowSdrOptions(bool b);
    void InitSettings(QString fname);

    //Hack, these should eventually be access methods
    STARTUP startup;
    double startupFreq;
    QString inputDeviceName;
    QString outputDeviceName;
    int sampleRate;

    double iqGain; //Normalize device so incoming IQ levels are consistent
    IQORDER iqOrder;
    //Image rejection (iqbalance) factors for this device
    double iqBalanceGain;
    double iqBalancePhase;
    bool iqBalanceEnable;

    double lastFreq;
    int lastMode;
    int lastDisplayMode; //Spectrum, waterfall, etc

    bool isTestBenchChecked;


signals:
    //Settings changed, turn off and restart with new settings
    void Restart();

protected:
    //Needed to determine when it's safe to fetch options for display
    bool connected;

    QDialog *sdrOptions;
    Ui::SdrOptions *sd;

    void ReadSettings();
    void WriteSettings();
    QSettings *qSettings;

    QStringList inputDevices;
    QStringList outputDevices;

    SDRDEVICE sdrDevice;
	Audio *audioInput;
	Receiver *receiver;
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

private slots:
    void InputChanged(int i);
    void OutputChanged(int i);
    void StartupChanged(int i);
    void StartupFrequencyChanged();
    void SampleRateChanged(int i);
    void IQGainChanged(double i);
    void IQOrderChanged(int i);
    void BalancePhaseChanged(int v);
    void BalanceGainChanged(int v);
    void BalanceEnabledChanged(bool b);
    void BalanceReset();
    void ResetAllSettings(bool b);
    void CloseOptions(bool b);
    void TestBenchChanged(bool b);
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
