//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "producerconsumer.h"

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
void ProducerConsumer::Initialize(DeviceInterface *_device, int _numDataBufs, int _producerBufferSize, int _refresh)
{
    //Defensive
    if (_producerBufferSize == 0)
        return; //Can't create empty buffer

    device = _device;
    threadRefresh = _refresh;
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
        producerWorkerThread = new QThread();
    if (producerWorker == NULL) {
        producerWorker = new SDRProducerWorker(device);
        producerWorker->moveToThread(producerWorkerThread);
        connect(producerWorkerThread,&QThread::started,producerWorker,&SDRProducerWorker::Start);
        //connect(producerWorkerThread,&QThread::finished, this,&ProducerConsumer::ProducerWorkerThreadStopped);
        connect(producerWorkerThread,&QThread::finished, producerWorker,&SDRProducerWorker::Stopped);
        connect(producerWorker,&SDRProducerWorker::finished,producerWorkerThread,&QThread::quit);
    }
    if (consumerWorkerThread == NULL)
        consumerWorkerThread = new QThread();
    if (consumerWorker == NULL) {
        consumerWorker = new SDRConsumerWorker(device);
        consumerWorker->moveToThread(consumerWorkerThread);
        connect(consumerWorkerThread,&QThread::started,consumerWorker,&SDRConsumerWorker::Start);
        connect(consumerWorkerThread,&QThread::finished, consumerWorker,&SDRConsumerWorker::Stopped);
        connect(consumerWorker,&SDRConsumerWorker::finished,consumerWorkerThread,&QThread::quit);
    }

}


bool ProducerConsumer::Start()
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    isThreadRunning = true;

    consumerWorkerThread->start();
    //Process event loop so thread signals and slots can all execute
    //If we don't do this, then consumer thread may or may not start before producer
    QCoreApplication::processEvents();

    producerWorkerThread->start();
    QCoreApplication::processEvents();

    return true;
}

bool ProducerConsumer::Stop()
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    producerWorkerThread->requestInterruption();
    //Give requestInteruption signals and slots a chance to trigger in event loop
    QCoreApplication::processEvents();
    if (!producerWorkerThread->wait(500))
        qDebug()<<"ProducerWorkerThread didn't respond to interupt request";

    consumerWorkerThread->requestInterruption();
    //Give requestInteruption signals and slots a chance to trigger in event loop
    QCoreApplication::processEvents();

    if (!consumerWorkerThread->wait(500))
        qDebug()<<"ProducerWorkerThread didn't respond to interupt request";

    isThreadRunning = false;
    return true;
}

bool ProducerConsumer::IsRunning()
{
    return isThreadRunning;
}

CPX *ProducerConsumer::GetProducerBufferAsCPX()
{
    return ((CPX **)producerBuffer)[nextProducerDataBuf];
}

char *ProducerConsumer::GetProducerBufferAsChar()
{
    return (char*)producerBuffer[nextProducerDataBuf];
}

CPX *ProducerConsumer::GetConsumerBufferAsCPX()
{
    return ((CPX **)producerBuffer)[nextConsumerDataBuf];
}

double ProducerConsumer::GetConsumerBufferDataAsDouble(quint16 index)
{
    return (double)producerBuffer[nextConsumerDataBuf][index];
}

unsigned char *ProducerConsumer::GetProducerBuffer()
{
    return (producerBuffer)[nextProducerDataBuf];
}

unsigned char *ProducerConsumer::GetConsumerBuffer()
{
    return (producerBuffer)[nextConsumerDataBuf];
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

bool ProducerConsumer::AcquireFreeBuffer()
{
    if (semNumFreeBuffers == NULL)
        return false; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFreeBuffers->available();
    if ( available < 5) { //Ouput when we get within 5 of overflow
        //qDebug("Limited Free buffers available %d",available);
        freeBufferOverflow = true;
    } else {
        freeBufferOverflow = false;
    }

    //We can't block with just acquire() because thread will never finish
    //If we can't get a buffer in N ms, return false and caller will exit
    return semNumFreeBuffers->tryAcquire(1);
}

bool ProducerConsumer::AcquireFilledBuffer()
{
    if (semNumFilledBuffers == NULL)
        return false; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFilledBuffers->available();
    if ( available > (numDataBufs - 5)) { //Ouput when we get within 5 of overflow
        //qDebug("Filled buffers available %d",available);
        filledBufferOverflow = true;
    } else {
        filledBufferOverflow = false;
    }

    //We can't block with just acquire() because thread will never finish
    //If we can't get a buffer in N ms, return false and caller will exit
    return semNumFilledBuffers->tryAcquire(1);

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
        sdr->RunProducerThread();  //This calls the actual worker function in each SDR device
        //Give requestInteruption signals and slots a chance to trigger in event loop
        QCoreApplication::processEvents();
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
    while (!QThread::currentThread()->isInterruptionRequested()) {
        sdr->RunConsumerThread();  //This calls the actual worker function in each SDR device
        //Give requestInteruption signals and slots a chance to trigger in event loop
        QCoreApplication::processEvents();
    }
    //Now that we have an interupt request, we signal that we are finished
    //This is connected to the QThread::quit slot
    //qDebug()<<"Handling thread internupt";
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
