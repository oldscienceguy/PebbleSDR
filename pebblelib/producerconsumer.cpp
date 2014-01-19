//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "producerconsumer.h"

/*
 * This uses the QT producer/consumer model to avoid more complex and deadlock prone mutex's
 * QSemaphores protects the producer thread's buffer pointer  and the consumer threads' buffer pointer in a circular buffer
 *
 *
 */

ProducerConsumer::ProducerConsumer()
{
    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;
    isThreadRunning = false;

    producerWorker = NULL;
    producerWorkerThread = NULL;
    consumerWorker = NULL;
    consumerWorkerThread = NULL;
}

//This can get called on an existing ProducerConsumer object
//Make sure we reset everything
void ProducerConsumer::Initialize(DeviceInterface *_device, int _numDataBufs, int _producerBufferSize)
{
    //Defensive
    if (_producerBufferSize == 0)
        return; //Can't create empty buffer

    device = _device;
    numDataBufs = _numDataBufs;

    isThreadRunning = false;

    //2 bytes per sample, framesPerBuffer samples after decimate
    producerBufferSize = _producerBufferSize;
    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            delete (producerBuffer[i]);
        delete (producerBuffer);
    }
    producerBuffer = new unsigned char *[numDataBufs];
    for (int i=0; i<numDataBufs; i++)
        producerBuffer[i] = new unsigned char [producerBufferSize];

    //Start out with all producer buffers available
    if (semNumFreeBuffers != NULL)
        delete semNumFreeBuffers;
    semNumFreeBuffers = new QSemaphore(numDataBufs);

    if (semNumFilledBuffers != NULL)
        delete semNumFilledBuffers;
    //Init with zero available
    semNumFilledBuffers = new QSemaphore(0);

    nextProducerDataBuf = nextConsumerDataBuf = 0;
    /*
        Thread                      Worker
        producerWorkerThread->start();
        Signal started() ---------->Slot Start()        Thread tells works to start
        producerWorkerThread->requestInterruption();
        Slot quit() <-------------- Signal finished()   Worker tells thread it's finished after detecting interupt request
        Slot finished() ----------->Slot Stop()         Thread tells worker everything is done

    */


    //New worker pattern that replaces subclassed QThread.  Recommended new QT 5 pattern
    if (producerWorkerThread == NULL)
        producerWorkerThread = new QThread(this);
    if (producerWorker == NULL) {
        producerWorker = new SDRProducerWorker(device);
        producerWorker->moveToThread(producerWorkerThread);
        connect(producerWorkerThread,&QThread::started,producerWorker,&SDRProducerWorker::Start);
        //connect(producerWorkerThread,&QThread::finished, this,&ProducerConsumer::ProducerWorkerThreadStopped);
        connect(producerWorkerThread,&QThread::finished, producerWorker,&SDRProducerWorker::Stopped);
        connect(producerWorker,&SDRProducerWorker::finished,producerWorkerThread,&QThread::quit);
    }
    if (consumerWorkerThread == NULL)
        consumerWorkerThread = new QThread(this);
    if (consumerWorker == NULL) {
        consumerWorker = new SDRConsumerWorker(device);
        consumerWorker->moveToThread(consumerWorkerThread);
        connect(consumerWorkerThread,&QThread::started,consumerWorker,&SDRConsumerWorker::Start);
        connect(consumerWorkerThread,&QThread::finished, consumerWorker,&SDRConsumerWorker::Stopped);
        connect(consumerWorker,&SDRConsumerWorker::finished,consumerWorkerThread,&QThread::quit);
    }

}


bool ProducerConsumer::Start(bool _producer, bool _consumer)
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    isThreadRunning = true;
    producerThreadIsRunning = false;
    consumerThreadIsRunning = false;
    //Note: QT only allows us to set thread priority on a running thread or in start()
    if (_consumer) {
        consumerWorkerThread->start();
        //Process event loop so thread signals and slots can all execute
        //If we don't do this, then consumer thread may or may not start before producer
        QCoreApplication::processEvents();
        consumerThreadIsRunning = true;
    }
    if (_producer) {
        producerWorkerThread->start();

        QCoreApplication::processEvents();
        producerThreadIsRunning = true;
    }
    return true;
}

bool ProducerConsumer::Stop()
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    if (producerThreadIsRunning) {
        producerWorkerThread->requestInterruption();
        //Give requestInteruption signals and slots a chance to trigger in event loop
        QCoreApplication::processEvents();
        if (!producerWorkerThread->wait(500))
            qDebug()<<"ProducerWorkerThread didn't respond to interupt request";
        producerThreadIsRunning = false;
    }

    if (consumerThreadIsRunning) {
        consumerWorkerThread->requestInterruption();
        //Give requestInteruption signals and slots a chance to trigger in event loop
        QCoreApplication::processEvents();

        if (!consumerWorkerThread->wait(500))
            qDebug()<<"ProducerWorkerThread didn't respond to interupt request";
        consumerThreadIsRunning = false;
    }
    isThreadRunning = false;
    return true;
}

bool ProducerConsumer::IsRunning()
{
    return isThreadRunning;
}

bool ProducerConsumer::IsFreeBufferAvailable()
{
    if (semNumFreeBuffers == NULL)
        return false; //Not initialized yet

    //Make sure we have at least 1 data buffer available without blocking
    int freeBuf = semNumFreeBuffers->available();
    if (freeBuf == 0) {
        //qDebug()<<"No free buffer available, ignoring block.";
        return false;
    }
    return true;
}

//This call is the only way we can get a producer buffer ptr
//We can use the ptr until ReleaeFreeBuffer is called
//Returns NULL if no free buffer can be acquired
unsigned char* ProducerConsumer::AcquireFreeBuffer(quint16 _timeout)
{
    if (semNumFreeBuffers == NULL)
        return NULL; //Not initialized yet

    //We can't block with just acquire() because thread will never finish
    //If we can't get a buffer in N ms, return NULL and caller will exit
    if (semNumFreeBuffers->tryAcquire(1, _timeout)) {
        freeBufferOverflow = false;
        return producerBuffer[nextProducerDataBuf];
    } else {
        freeBufferOverflow = true;
        return NULL;
    }
}

void ProducerConsumer::ReleaseFreeBuffer()
{
    nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;
    semNumFreeBuffers->release();
}

void ProducerConsumer::PutbackFreeBuffer()
{
    semNumFreeBuffers->release();
}

unsigned char *ProducerConsumer::AcquireFilledBuffer(quint16 _timeout)
{
    if (semNumFilledBuffers == NULL)
        return NULL; //Not initialized yet

    //We can't block with just acquire() because thread will never finish
    //If we can't get a buffer in N ms, return NULL and caller will exit
    if (semNumFilledBuffers->tryAcquire(1, _timeout)) {
        filledBufferOverflow = false;
        return producerBuffer[nextConsumerDataBuf];
    } else {
        filledBufferOverflow = true;
        return NULL;
    }

}

//We have to consider the case where the producer is being called more frequently than the consumer
//This can be the case if we are running without a producer thread and filling the producer buffers
//directly.  For example in a QTcpSocket readyRead() signal handler.
void ProducerConsumer::ReleaseFilledBuffer()
{
    nextProducerDataBuf = (nextProducerDataBuf +1 ) % numDataBufs; //Increment producer pointer
    semNumFilledBuffers->release();
}

void ProducerConsumer::PutbackFilledBuffer()
{
    semNumFilledBuffers->release();
}

//SDRThreads
SDRProducerWorker::SDRProducerWorker(DeviceInterface * s)
{
    sdr = s;
}

//This gets called by producerThread because we connected the started() signal to this method
void SDRProducerWorker::Start()
{
    //Do any worker construction here so it's in the thread
    //We loop continuously until we get an interupt request, this allows us to complete all processing before we exit
    qDebug()<<"Starting Producer Worker";
    isRunning = true;
    while (!QThread::currentThread()->isInterruptionRequested()) {
        //perform.StartPerformance("Producer Thread");

        sdr->RunProducerThread();  //This calls the actual worker function in each SDR device

        //perform.StopPerformance(1000);

        //Give requestInteruption signals and slots a chance to trigger in this thread's event loop
        //Note: This does NOT allow other threads to process events, just our worker thread
        QCoreApplication::processEvents();
        //Test yield to give consumer chance to keep up.  Maybe only if new data is actually available
        //QThread::yieldCurrentThread();
    }
    //Now that we have an interupt request, we signal that we are finished
    //This is connected to the QThread::quit slot
    isRunning = false;
    //Do any worker destruction here
    emit finished();
}
//This gets called with the thread signals finished() via connect we made
//We only get called when thread has actually completed
void SDRProducerWorker::Stopped()
{
    qDebug()<<"Producer thread finished";
    //Any work to do here?
}

SDRConsumerWorker::SDRConsumerWorker(DeviceInterface * s)
{
    sdr = s;
}

//This gets called by producerThread because we connected the started() signal to this method
void SDRConsumerWorker::Start()
{
    //We loop continuously until we get an interupt request, this allows us to complete all processing before we exit
    qDebug()<<"Starting Consumer Worker";
    isRunning = true;
    while (!QThread::currentThread()->isInterruptionRequested()) {
        //perform.StartPerformance("Consumer Thread");

        sdr->RunConsumerThread();  //This calls the actual worker function in each SDR device

        //perform.StopPerformance(1000);

        //Give requestInteruption signals and slots a chance to trigger in event loop
        QCoreApplication::processEvents();
    }
    //Now that we have an interupt request, we signal that we are finished
    //This is connected to the QThread::quit slot
    //qDebug()<<"Handling thread internupt";
    isRunning = false;
    emit finished();
}
void SDRConsumerWorker::Stopped()
{
    qDebug()<<"Consumer thread finished";
}

void Sleeper::usleep(unsigned long usecs)
{
    QThread::usleep(usecs);
}
void Sleeper::msleep(unsigned long msecs)
{
    QThread::msleep(msecs);
}
void Sleeper::sleep(unsigned long secs)
{
    QThread::sleep(secs);
}
