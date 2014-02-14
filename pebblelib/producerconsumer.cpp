//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "producerconsumer.h"

/*
 * This uses the QT producer/consumer model to avoid more complex and deadlock prone mutex's
 * QSemaphores protects the producer thread's buffer pointer  and the consumer threads' buffer pointer in a circular buffer
 *
 * 2/12/14: New model
 * Producer ReleaseFilledBuffer emits signal which is connected to Consumer new data slot
 * Consumer should process as many filled buffers as possible before returning
 * This eliminates high CPU caused by consumer thread looping doing nothing
 *
 * Producer thread now has a polling interval which is calculated based on sample rate and samples/buffer
 * This also reduces CPU load because we sleep for a safe amount of time between polling for new data
 */

ProducerConsumer::ProducerConsumer()
{
    //No difference in performance noted, use worker object as per recommendation
	//useWorkerThread uses a subclassed thread with no event loop
	//else we use a normal thread with an event loop and moveToThread
    useWorkerThread = false;
	//Non blocking gives us more control and as long as polling interval is set correctly, same CPU as blocking
	useBlockingAcquire = false;

    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;
    isThreadRunning = false;

    producerWorker = NULL;
    producerWorkerThread = NULL;
    consumerWorker = NULL;
    consumerWorkerThread = NULL;

	usPollingInterval = 0;
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
			producerWorker = new ProducerWorker(cbProducerWorker);
            producerWorker->moveToThread(producerWorkerThread);
			connect(producerWorkerThread,&PCThread::started,producerWorker,&ProducerWorker::start);
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
			consumerWorker = new ConsumerWorker(cbConsumerWorker);
            consumerWorker->moveToThread(consumerWorkerThread);
			connect(this,&ProducerConsumer::NewData, consumerWorker, &ConsumerWorker::processNewData);
        }
    }

}

//Only if producer is polling for data, not needed if producer is triggered via signal
//Call after initialize
void ProducerConsumer::SetPollingInterval(quint32 _sampleRate, quint16 _samplesPerBuffer)
{
	//How many data buffers do we have to process per second
	quint16 buffersPerSec = _sampleRate / _samplesPerBuffer;
	//What's the optimal interval between data buffers (in usec)
	quint16 usBufferInterval = 1000000 / buffersPerSec;
	//Set safe interval (experiment here)
	usPollingInterval = usBufferInterval / 6;
	if (producerWorker != NULL)
		producerWorker->SetPollingInterval(usPollingInterval);
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
			//This will not quit the thread if we are blocking in a semaphore acquire or release
            dynamic_cast<PCThread*>(producerWorkerThread)->quit();
		//If thread doesn't quit by itself, force it
		if (!producerWorkerThread->wait(500)) {
			qDebug()<<"ProducerWorkerThread waiting for free Buffer, force quit";
			semNumFreeBuffers->release();
			producerWorkerThread->quit();
		}
        producerThreadIsRunning = false;
    }

    if (consumerThreadIsRunning) {
		if (useWorkerThread)
			//This will not quit the thread if we are blocking in a semaphore acquire or release
			dynamic_cast<PCThread*>(consumerWorkerThread)->quit();
		if (!consumerWorkerThread->wait(500)) {
			qDebug()<<"ConsumerWorkerThread waiting for filled buffer, force quit";
			semNumFilledBuffers->release();
			consumerWorkerThread->quit();
		}
        consumerThreadIsRunning = false;
    }
    isThreadRunning = false;
	return true;
}

//Not generally needed because we reset everything in Initialize, but here in case we need to restart due to overflow
bool ProducerConsumer::Reset()
{
	nextProducerDataBuf = nextConsumerDataBuf = 0;
	//Reset filled buffers to zero
	semNumFilledBuffers->release(semNumFilledBuffers->available());
	qDebug()<<"ProducerConsumer reset - numFilled = "<<semNumFilledBuffers->available();

	//Reset free buffers to max
	semNumFreeBuffers->acquire(numDataBufs - semNumFreeBuffers->available());
	qDebug()<<"ProducerConsumer reset - numFree = "<<semNumFreeBuffers->available();

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
	emit NewData();
	//Give thread with consumer a chance to pick this up
	QThread::yieldCurrentThread();
}

void ProducerConsumer::PutbackFilledBuffer()
{
    semNumFilledBuffers->release();
}

//SDRThreads
ProducerWorker::ProducerWorker(cbProducerConsumer _worker)
{
	worker = _worker;
	usSleep = 0;
}

//This gets called by producerThread because we connected the started() signal to this method
void ProducerWorker::start()
{
    //Do any worker construction here so it's in the thread
    worker(cbProducerConsumerEvents::Start);
    doRun = true;
    while (doRun) {
        //perform.StartPerformance("Producer Thread");
        //This calls the actual worker function in each SDR device
        worker(cbProducerConsumerEvents::Run);
		//usSleep is calculated based on sample rate and samples per buffer for producer polling
		QThread::currentThread()->usleep(usSleep);
        //perform.StopPerformance(1000);
    }
    //Do any worker destruction here
    worker(cbProducerConsumerEvents::Stop);
    //Quit the thread as if we were returnning from QThread::run() directly
    QThread::currentThread()->quit();
}
void ProducerWorker::stop()
{
    doRun = false;
}

ConsumerWorker::ConsumerWorker(cbProducerConsumer _worker)
{
	worker = _worker;
}

void ConsumerWorker::processNewData()
{
	worker(cbProducerConsumerEvents::Run);
}

