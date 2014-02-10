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
    //No difference in performance noted, use worker object as per recommendation
    useWorkerThread = false;
    //useBlockingAcquire = false; //Higher CPU because we run loop faster?
    useBlockingAcquire = true;

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
void ProducerConsumer::Initialize(cbProducerConsumer _producerWorker, cbProducerConsumer _consumerWorker, int _numDataBufs, int _producerBufferSize)
{
    //Defensive
    if (_producerBufferSize == 0)
        return; //Can't create empty buffer

    cbProducerWorker = _producerWorker;
    cbConsumerWorker = _consumerWorker;

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

                                    Stop()              Sets bool stop=true to exit forever loop

        Slot quit() <-------------- Signal finished()   Worker tells thread it's finished after detecting interupt request
        Slot finished() ----------->Slot Stop()         Thread tells worker everything is done

    */


    //New worker pattern that replaces subclassed QThread.  Recommended new QT 5 pattern
    if (producerWorkerThread == NULL) {
        if (useWorkerThread)
            producerWorkerThread = new PCThread(this);
        else
            producerWorkerThread = new QThread(this);
        producerWorkerThread->setObjectName("PebbleProducer");
    }
    if (producerWorker == NULL) {
        if (useWorkerThread) {
            dynamic_cast<PCThread *>(producerWorkerThread)->SetWorker(cbProducerWorker);
        } else {
            producerWorker = new PCWorker(cbProducerWorker);
            producerWorker->moveToThread(producerWorkerThread);
            connect(producerWorkerThread,&PCThread::started,producerWorker,&PCWorker::start);
            //connect(producerWorkerThread,&PCThread::finished, producerWorker,&PCWorker::stop);
            //connect(producerWorker,&PCWorker::finished,producerWorkerThread,&QThread::quit);
        }
    }
    if (consumerWorkerThread == NULL) {
        if (useWorkerThread)
            consumerWorkerThread = new PCThread(this);
        else
            consumerWorkerThread = new QThread(this);
        consumerWorkerThread->setObjectName("PebbleConsumer");
    }
    if (consumerWorker == NULL) {
        if (useWorkerThread) {
            dynamic_cast<PCThread *>(consumerWorkerThread)->SetWorker(cbConsumerWorker);
        } else {
            consumerWorker = new PCWorker(cbConsumerWorker);
            consumerWorker->moveToThread(consumerWorkerThread);
            connect(consumerWorkerThread,&PCThread::started,consumerWorker,&PCWorker::start);
            //connect(consumerWorkerThread,&PCThread::finished, consumerWorker,&PCWorker::stop);
            //connect(consumerWorker,&PCWorker::finished,consumerWorkerThread,&QThread::quit);
        }
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
        if (!useWorkerThread)
            producerWorker->stop(); //Stop the loop then stop the thread
        else
            dynamic_cast<PCThread*>(producerWorkerThread)->quit();
        if (!producerWorkerThread->wait(5000))
            qDebug()<<"ProducerWorkerThread didn't respond to interupt request";
        producerThreadIsRunning = false;
    }

    if (consumerThreadIsRunning) {
        if (!useWorkerThread)
            consumerWorker->stop();
        else
            dynamic_cast<PCThread*>(consumerWorkerThread)->quit();
        if (!consumerWorkerThread->wait(5000))
            qDebug()<<"ConsumerWorkerThread didn't respond to interupt request";
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
    if (!useBlockingAcquire) {
        //We can't block with just acquire() because thread will never finish
        //If we can't get a buffer in N ms, return NULL and caller will exit
        if (semNumFreeBuffers->tryAcquire(1, _timeout)) {
            freeBufferOverflow = false;
            return producerBuffer[nextProducerDataBuf];
        } else {
            freeBufferOverflow = true;
            return NULL;
        }
    } else {
        //Blocks, but yields
        semNumFreeBuffers->acquire();
        return producerBuffer[nextProducerDataBuf];
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

    if (!useBlockingAcquire) {
        //We can't block with just acquire() because thread will never finish
        //If we can't get a buffer in N ms, return NULL and caller will exit
        if (semNumFilledBuffers->tryAcquire(1, _timeout)) {
            filledBufferOverflow = false;
            return producerBuffer[nextConsumerDataBuf];
        } else {
            filledBufferOverflow = true;
            return NULL;
        }
    } else {
        semNumFilledBuffers->acquire();
        return producerBuffer[nextConsumerDataBuf];
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
PCWorker::PCWorker(cbProducerConsumer _producerWorker)
{
    worker = _producerWorker;
    msSleep = 0;
}

//This gets called by producerThread because we connected the started() signal to this method
void PCWorker::start()
{
    //Do any worker construction here so it's in the thread
    worker(cbProducerConsumerEvents::Start);
    doRun = true;
    while (doRun) {
        //perform.StartPerformance("Producer Thread");
        //This calls the actual worker function in each SDR device
        worker(cbProducerConsumerEvents::Run);
        //Without this we consume +100% CPU, why doesn't yield prevent this?
        QThread::currentThread()->msleep(msSleep);
        //perform.StopPerformance(1000);
        //QThread::yieldCurrentThread();
    }
    //Do any worker destruction here
    worker(cbProducerConsumerEvents::Stop);
    //Quit the thread as if we were returnning from QThread::run() directly
    QThread::currentThread()->quit();
}
void PCWorker::stop()
{
    doRun = false;
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
