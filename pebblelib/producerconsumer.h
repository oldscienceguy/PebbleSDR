#ifndef PRODUCERCONSUMER_H
#define PRODUCERCONSUMER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "perform.h"
#include <QtCore>
#include <QThread>


//Callback to class that instantiated us
enum cbProducerConsumerEvents{Start, Run, Stop};
typedef std::function<void(cbProducerConsumerEvents _event)> cbProducerConsumer;

//Alternative implemenation using QThread and pcWorker.moveToThread(...)
class PCWorker;

class PCThread : public QThread
{
public:
    PCThread(QObject * parent = 0):QThread(parent) {
        msSleep = 0;
    }

    void SetWorker(cbProducerConsumer _worker) {
        worker = _worker;
    }
    void SetMsSleep(quint16 _msSleep) {
        msSleep = _msSleep;
    }

    //QThread override
    //This is the ONLY method that actually runs in thread
    //All thread local construction should be done here (Start)
    //All thread local descruction should be done here (Stop)
    void run() {
        worker(cbProducerConsumerEvents::Start);
        doRun = true;
        while (doRun) {
            worker(cbProducerConsumerEvents::Run);
            msleep(msSleep);
        }
        worker(cbProducerConsumerEvents::Stop);
    }

    //QThread override
    void quit() {
        doRun = false;
        QThread::quit();
    }

private:
    cbProducerConsumer worker;
    volatile bool doRun;
    quint16 msSleep;
};

class ProducerConsumer : public QObject
{
    Q_OBJECT
public:
    ProducerConsumer();

    void Initialize(cbProducerConsumer _producerWorker, cbProducerConsumer _consumerWorker, int _numDataBufs, int _producerBufferSize);

    bool Start(bool _producer = true, bool _consumer = true);
    bool Stop();

    bool IsRunning();

    quint16 GetBufferSize() {return producerBufferSize;}

    bool IsFreeBufferAvailable();
    unsigned char *AcquireFreeBuffer(quint16 _timeout = 0);
    void ReleaseFreeBuffer();
    void PutbackFreeBuffer(); //Release a buffer without incrementing semaphors, used in error conditions

    unsigned char *AcquireFilledBuffer(quint16 _timeout = 0);
    void ReleaseFilledBuffer();
    void PutbackFilledBuffer();

    //Informational
    bool IsFreeBufferOverflow() {return freeBufferOverflow;}
    bool IsFilledBufferOverflow() {return filledBufferOverflow;}
    quint16 GetNumFreeBufs() {return semNumFreeBuffers->available();}
    quint16 GetNumFilledBufs() {return semNumFilledBuffers->available();}

private:
    //Experiment using worker thread vs QThread and moveToThread
    bool useWorkerThread;
    //Experiment using blocking acquire() vs checking for available first
    bool useBlockingAcquire;

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
    QThread *producerWorkerThread;
    QThread *consumerWorkerThread;
    PCWorker *producerWorker;
    PCWorker *consumerWorker;

    bool producerThreadIsRunning;
    bool consumerThreadIsRunning;

    cbProducerConsumer cbProducerWorker;
    cbProducerConsumer cbConsumerWorker;

};

//Alternative thread model using Worker objects and no QThread subclass
class PCWorker: public QObject
{
    Q_OBJECT
public:
    PCWorker(cbProducerConsumer _producerWorker);
    public slots:
    void start();
    void stop();
signals:
    void finished();
private:
    cbProducerConsumer worker;
    Perform perform;
    volatile bool doRun; //volatile bool is thread safe, but not other volatile data types
    quint16 msSleep;

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
