#ifndef FILEDEVICE_H
#define FILEDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "../pebblelib/device_interfaces.h"
#include "wavfile.h"
#include <QThread>

class SDRProducerThread;
class SDRConsumerThread;

class FileSDRDevice : public QObject, public DeviceInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DeviceInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DeviceInterface)

public:
    FileSDRDevice();
    ~FileSDRDevice();
    QString GetPluginName();
    QString GetPluginDescription();

    bool Initialize(cbProcessIQData _callback);
    bool Connect();
    bool Disconnect();
    void Start();
    void Stop();

    double SetFrequency(double fRequested,double fCurrent);
    void ShowOptions();
    void ReadSettings();
    void WriteSettings();

    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();
    int GetSampleRate();
    int* GetSampleRates(int &len); //Returns array of allowable rates and length of array as ref
    bool UsesAudioInput();

    //Display device option widget in settings dialog
    void SetupOptionUi(QWidget *parent);

    //Should be somewhere else
    //Proucer-Consumer buffer management
    void InitProducerConsumer(int _numDataBufs, int _producerBufferSize);

    bool IsFreeBufferAvailable();
    void AcquireFreeBuffer();
    void ReleaseFreeBuffer() {semNumFreeBuffers->release();}
    void IncrementProducerBuffer() {nextProducerDataBuf = (nextProducerDataBuf +1 ) % numDataBufs;}

    void AcquireFilledBuffer();
    void ReleaseFilledBuffer() {semNumFilledBuffers->release();}
    void IncrementConsumerBuffer() {nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;}

    void InitSettings(QString fname);
protected:
    void StopProducerThread();
    void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();

private:
    //Things every device needs, move to interface
    int framesPerBuffer;

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


    QString fileName;

    WavFile wavFileRead;
    WavFile wavFileWrite;
    bool copyTest; //True if we're reading from one file and writing to another file for testing

};

//Generic thread that can be used in producer/consumer models for devices that don't use soundcard
class SDRProducerThread:public QThread
{
    Q_OBJECT
public:
    SDRProducerThread(DeviceInterface * s);
    void run();
    void stop();
    void setRefresh(int ms);

private:
    DeviceInterface *sdr;
    bool doRun;
    int msSleep;
};
class SDRConsumerThread:public QThread
{
    Q_OBJECT
public:
    SDRConsumerThread(DeviceInterface * s);
    void run();
    void stop();
    void setRefresh(int ms);

private:
    DeviceInterface *sdr;
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


#endif // FILEDEVICE_H
