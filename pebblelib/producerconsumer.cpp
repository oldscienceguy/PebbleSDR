//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "producerconsumer.h"

ProducerConsumer::ProducerConsumer()
{
    producerThread = NULL;
    consumerThread = NULL;
    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;
}

void ProducerConsumer::Initialize(DeviceInterface *_device, int _numDataBufs, int _producerBufferSize, int _refresh)
{
    //Defensive
    if (_producerBufferSize == 0)
        return; //Can't create empty buffer

    device = _device;
    threadRefresh = _refresh;
    numDataBufs = _numDataBufs;
    //2 bytes per sample, framesPerBuffer samples after decimate
    producerBufferSize = _producerBufferSize;
    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            free (producerBuffer[i]);
        free (producerBuffer);
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

    producerThread = new SDRProducerThread(device);
    producerThread->setRefresh(threadRefresh);
    consumerThread = new SDRConsumerThread(device);
    consumerThread->setRefresh(threadRefresh);

}

bool ProducerConsumer::Start()
{
    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    consumerThread->start();
    producerThread->start();
    isThreadRunning = true;
    return false;
}

bool ProducerConsumer::Stop()
{
    if (isThreadRunning) {
        producerThread->stop();
        consumerThread->stop();
        isThreadRunning = false;
    }
    return true;
}

bool ProducerConsumer::IsRunning()
{
    return isThreadRunning;
}

CPX *ProducerConsumer::GetProducerBuffer_CPX()
{
    return ((CPX **)producerBuffer)[nextProducerDataBuf];
}

CPX *ProducerConsumer::GetConsumerBuffer_CPX()
{
    return ((CPX **)producerBuffer)[nextConsumerDataBuf];
}

double *ProducerConsumer::GetProducerBuffer_double()
{
    return ((double **)producerBuffer)[nextProducerDataBuf];
}

double *ProducerConsumer::GetConsumerBuffer_double()
{
    return ((double **)producerBuffer)[nextConsumerDataBuf];
}

unsigned char *ProducerConsumer::GetProducerBuffer_char()
{
    return (producerBuffer)[nextProducerDataBuf];
}

unsigned char *ProducerConsumer::GetConsumerBuffer_char()
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

void ProducerConsumer::AcquireFreeBuffer()
{
    if (semNumFreeBuffers == NULL)
        return; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFreeBuffers->available();
    if ( available < 5) { //Ouput when we get within 5 of overflow
        //qDebug("Limited Free buffers available %d",available);
        freeBufferOverflow = true;
    } else {
        freeBufferOverflow = false;
    }

    semNumFreeBuffers->acquire(); //Will not return until we get a free buffer, but will yield
}

void ProducerConsumer::AcquireFilledBuffer()
{
    if (semNumFilledBuffers == NULL)
        return; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFilledBuffers->available();
    if ( available > (numDataBufs - 5)) { //Ouput when we get within 5 of overflow
        //qDebug("Filled buffers available %d",available);
        filledBufferOverflow = true;
    } else {
        filledBufferOverflow = false;
    }

    semNumFilledBuffers->acquire(); //Will not return until we get a filled buffer, but will yield

}

//SDRThreads
SDRProducerThread::SDRProducerThread(DeviceInterface *_sdr)
{
    sdr = _sdr;
    msSleep=5;
    doRun = false;
}
void SDRProducerThread::stop()
{
    sdr->StopProducerThread();
    doRun=false;
}
//Refresh rate in me
void SDRProducerThread::setRefresh(int ms)
{
    msSleep = ms;
}
void SDRProducerThread::run()
{
    doRun = true;
    while(doRun) {
        sdr->RunProducerThread();
        msleep(msSleep);
    }
}
SDRConsumerThread::SDRConsumerThread(DeviceInterface *_sdr)
{
    sdr = _sdr;
    msSleep=5;
    doRun = false;
}
void SDRConsumerThread::stop()
{
    sdr->StopConsumerThread();
    doRun=false;
}
//Refresh rate in me
void SDRConsumerThread::setRefresh(int ms)
{
    msSleep = ms;
}
void SDRConsumerThread::run()
{
    doRun = true;
    while(doRun) {
        sdr->RunConsumerThread();
        msleep(msSleep);
    }
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
