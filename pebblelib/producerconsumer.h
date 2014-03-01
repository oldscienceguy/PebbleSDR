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
class ProducerWorker;
class ConsumerWorker;

class ProducerConsumer : public QObject
{
    Q_OBJECT
public:
    ProducerConsumer();

    void Initialize(cbProducerConsumer _producerWorker, cbProducerConsumer _consumerWorker, int _numDataBufs, int _producerBufferSize);

	//When producer is in polling mode, we need to sleep in between polls in order to avoid excessive CPU usage
	//This method calculates how many buffers per second we need to handle and a 'safe' sleep interval
	//If we sleep too long, we'll get producer data loss and hear choppy audio
	//If we sleep too short, we'll see excessive CPU
	void SetProducerInterval(quint32 _sampleRate, quint16 _samplesPerBuffer);
	void SetConsumerInterval(quint32 _sampleRate, quint16 _samplesPerBuffer);

    bool Start(bool _producer = true, bool _consumer = true);
    bool Stop();
	bool Reset(); //Resets all semaphores and circular buffer pointers

    quint16 GetBufferSize() {return producerBufferSize;}

    bool IsFreeBufferAvailable();
    unsigned char *AcquireFreeBuffer(quint16 _timeout = 0);
    void ReleaseFreeBuffer();
    void PutbackFreeBuffer(); //Release a buffer without incrementing semaphors, used in error conditions

    unsigned char *AcquireFilledBuffer(quint16 _timeout = 0);
    void ReleaseFilledBuffer();
    void PutbackFilledBuffer();

    //Informational
	//Returns 0 to 100
	quint16 GetPercentageFree();
    quint16 GetNumFreeBufs() {return semNumFreeBuffers->available();}
    quint16 GetNumFilledBufs() {return semNumFilledBuffers->available();}

signals:
	void CheckNewData();
	void ProcessNewData(); //Producer released a filled buffer

private:
    //Experiment using blocking acquire() vs checking for available first
    bool useBlockingAcquire;

    //Producer/Consumer buffer management
    int numDataBufs; //Producer/Consumer buffers
    unsigned char **producerBuffer; //Array of buffers
    int producerBufferSize;
    int nextProducerDataBuf;
    int nextConsumerDataBuf;
    /*
      NumFreeBuffers starts at NUMDATABUFS and is decremented (acquire()) everytime the producer thread has new data.
      If it ever gets to zero, it will block forever and program will hang until consumer thread catches up.
      NumFreeBuffers is incremented (release()) in consumer thread when a buffer has been processed and can be reused.
    */
    QSemaphore *semNumFreeBuffers; //Init to NUMDATABUFS
    QSemaphore *semNumFilledBuffers;

    QThread *producerWorkerThread;
    QThread *consumerWorkerThread;
	ProducerWorker *producerWorker;
	ConsumerWorker *consumerWorker;

    bool producerThreadIsRunning;
    bool consumerThreadIsRunning;

    cbProducerConsumer cbProducerWorker;
    cbProducerConsumer cbConsumerWorker;

	quint16 msProducerInterval;
	quint16 msConsumerInterval;

};

//Alternative thread model using Worker objects and no QThread subclass
class ProducerWorker: public QObject
{
    Q_OBJECT
public:
	ProducerWorker(cbProducerConsumer _worker);
	void SetPollingInterval(quint16 _msInterval) {msInterval = _msInterval;}

public slots:
    void start();
    void stop();
	void checkNewData();

private:
    cbProducerConsumer worker;
    Perform perform;
	quint16 msInterval;
	QTimer *timer;
};
class ConsumerWorker: public QObject
{
	Q_OBJECT
public:
	ConsumerWorker(cbProducerConsumer _worker);
	void SetPollingInterval(quint16 _msInterval) {msInterval = _msInterval;}

public slots:
	void start();
	void stop();
	void processNewData();

private:
	cbProducerConsumer worker;
	Perform perform;
	quint16 msInterval;
	QTimer *timer;
};

#endif // PRODUCERCONSUMER_H
