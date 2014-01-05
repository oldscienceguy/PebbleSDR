#ifndef PRODUCERCONSUMER_H
#define PRODUCERCONSUMER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QtCore>
#include <QThread>
#include "../pebblelib/device_interfaces.h"

class SDRProducerThread;
class SDRConsumerThread;

class ProducerConsumer
{
public:
    ProducerConsumer();

    void Initialize(DeviceInterface *_device, int _numDataBufs, int _producerBufferSize, int _refresh);

    bool Start();
    bool Stop();

    bool IsRunning();

    quint16 GetBufferSize() {return producerBufferSize;}

    unsigned char* GetProducerBuffer();
    unsigned char* GetConsumerBuffer();
    CPX* GetProducerBufferAsCPX();
    CPX* GetConsumerBufferAsCPX();
    double GetConsumerBufferDataAsDouble(quint16 index);

    bool IsFreeBufferAvailable();
    void AcquireFreeBuffer();
    void ReleaseFreeBuffer() {semNumFreeBuffers->release();}
    void AcquireFilledBuffer();
    void ReleaseFilledBuffer() {semNumFilledBuffers->release();}

    void SupplyProducerBuffer() {nextProducerDataBuf = (nextProducerDataBuf +1 ) % numDataBufs;}

    void SupplyConsumerBuffer() {nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;}

private:
    DeviceInterface *device;
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


    int threadRefresh;

    bool isThreadRunning;
    SDRProducerThread *producerThread;
    SDRConsumerThread *consumerThread;

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


#endif // PRODUCERCONSUMER_H
