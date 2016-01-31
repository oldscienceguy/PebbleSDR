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
 *
 * Producer and Consumer threads now has a timing interval which is calculated based on sample rate and samples/buffer
 * This also reduces CPU load because we sleep for a safe amount of time between polling for new data
 *
 * 2/26/14: Significant update to use QTimers in threads.  QThread::msSleep() actually blocks, but QTimer() let other threads run
 *
 * 1/31/16: Another major update to handle sample rates above 2048 and reduce CPU usage at the same time
 * QTimer only has ms intervals and poor accuracy, making it impossible to use reliably at higher sample rates
 * We now use QIntervalTimer, which has ns resolution and high accuracy
 * Then we use nanonsleep() to wait for the next sampling interval.
 */

ProducerConsumer::ProducerConsumer()
{
	//Non blocking gives us more control and as long as polling interval is set correctly, same CPU as blocking
	useBlockingAcquire = false;

    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;

    producerWorker = NULL;
    producerWorkerThread = NULL;
    consumerWorker = NULL;
    consumerWorkerThread = NULL;

	//Interval of 0 will run as fast as possible
	nsProducerInterval = 1;
	nsConsumerInterval = 1;
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

    //New worker pattern that replaces subclassed QThread.  Recommended new QT 5 pattern
    if (producerWorkerThread == NULL) {
		producerWorkerThread = new QThread(this);
		producerWorkerThread->setObjectName("PebbleProducer");
    }
    if (producerWorker == NULL) {
		producerWorker = new ProducerWorker(cbProducerWorker);
		connect(producerWorkerThread,&QThread::started,producerWorker,&ProducerWorker::start);
	}
    if (consumerWorkerThread == NULL) {
		consumerWorkerThread = new QThread(this);
		consumerWorkerThread->setObjectName("PebbleConsumer");
    }
    if (consumerWorker == NULL) {
		consumerWorker = new ConsumerWorker(cbConsumerWorker);
		connect(consumerWorkerThread,&QThread::started,consumerWorker,&ConsumerWorker::start);
	}

}

//Make sure sampleRate and samplesPerBuffer are in same units
//Bytes, CPX's, etc
void ProducerConsumer::SetConsumerInterval(quint32 _sampleRate, quint16 _samplesPerBuffer)
{
	//1 sample every N ns X number of samples per buffer = how long it takes device to fill a buffer
	float nsToFillBuffer = (1000000000.0 / _sampleRate) * _samplesPerBuffer;
	//Set safe interval (experiment here)
	//Use something that results in a non-cyclic interval to avoid checking constantly at the wrong time
	nsConsumerInterval = nsToFillBuffer * 0.90;
	if (nsConsumerInterval == 0)
		qDebug()<<"Warning: Consumer running as fast as possible, high CPU";
	else
		qDebug()<<"Consumer checks every "<<nsConsumerInterval<<" ns "<<"SampleRate | SamplesPerBuffer"<<_sampleRate<<_samplesPerBuffer;
	if (consumerWorker != NULL)
		consumerWorker->SetPollingInterval(nsConsumerInterval);

}

//Only if producer is polling for data, not needed if producer is triggered via signal
//Call after initialize
void ProducerConsumer::SetProducerInterval(quint32 _sampleRate, quint16 _samplesPerBuffer)
{
	//1 sample every N ns X number of samples per buffer = how long it takes device to fill a buffer
	float nsToFillBuffer = (1000000000.0 / _sampleRate) * _samplesPerBuffer;
	//Set safe interval (experiment here)
	nsProducerInterval = nsToFillBuffer * 0.90;
	if (nsProducerInterval == 0)
		qDebug()<<"Warning: Producer running as fast as possible, high CPU";
	else
		qDebug()<<"Producer checks every "<<nsProducerInterval<<" ns "<<"SampleRate | SamplesPerBuffer"<<_sampleRate<<_samplesPerBuffer;;
	if (producerWorker != NULL)
		producerWorker->SetPollingInterval(nsProducerInterval);
}


bool ProducerConsumer::Start(bool _producer, bool _consumer)
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    producerThreadIsRunning = false;
    consumerThreadIsRunning = false;
    //Note: QT only allows us to set thread priority on a running thread or in start()
    if (_consumer) {
		//Don't move worker to thread unless thread is started.  Signals can still be connected to consumerWorker
		consumerWorker->moveToThread(consumerWorkerThread);
        consumerWorkerThread->start();
		//Thread priorities from CuteSdr model
		//Producer get highest, time critical, slices in order to be able to keep up with data
		//Consumer gets next highest, in order to run faster than main UI thread
		consumerWorkerThread->setPriority(QThread::HighestPriority);
        consumerThreadIsRunning = true;
    }
	if (_producer) {
		producerWorker->moveToThread(producerWorkerThread);
		producerWorkerThread->start();
		producerWorkerThread->setPriority(QThread::NormalPriority);
        producerThreadIsRunning = true;
    }
    return true;
}

bool ProducerConsumer::Stop()
{
    if (producerWorkerThread == NULL || consumerWorkerThread == NULL)
        return false; //Init has not been called

    if (producerThreadIsRunning) {
		producerWorker->stop();
		producerWorkerThread->quit();
        producerThreadIsRunning = false;
    }

    if (consumerThreadIsRunning) {
		consumerWorker->stop();
		consumerWorkerThread->quit();
        consumerThreadIsRunning = false;
    }
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
            return producerBuffer[nextProducerDataBuf];
        } else {
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
            return producerBuffer[nextConsumerDataBuf];
        } else {
            return NULL;
        }
    } else {
		//This will block the thread waiting for a filled buffer
		//Internally QSemaphore wraps QWaitCondition, see QWaitCondition for lower level example
		//Does QWaitCondition wait() just block, or yield
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

quint16 ProducerConsumer::GetPercentageFree()
{
	if (semNumFreeBuffers != NULL)
		return (semNumFreeBuffers->available() / (float)numDataBufs) * 100;
	else
		return 100;
}

//SDRThreads
ProducerWorker::ProducerWorker(cbProducerConsumer _worker)
{
	worker = _worker;
	isRunning = false;
}

void ProducerWorker::start()
{
	//Do any construction here so it's in the thread
	worker(cbProducerConsumerEvents::Start);
	isRunning = true;
	timespec req, rem;
	elapsedTimer.start();
	while (isRunning) {
		qint64 nsRemaining = nsInterval - elapsedTimer.nsecsElapsed();
		if (nsRemaining > 0) {
			req.tv_sec = 0;
			//We want to get close to exact time, but not go over
			req.tv_nsec = nsRemaining;
			if (nanosleep(&req,&rem) < 0) {
				qDebug()<<"nanosleep failed";
			}
		}
		elapsedTimer.start(); //Restart elapsed timer
		worker(cbProducerConsumerEvents::Run);
	}
	return;
}
void ProducerWorker::stop()
{
	isRunning = false;
	//Do any worker destruction here
	worker(cbProducerConsumerEvents::Stop);
}

ConsumerWorker::ConsumerWorker(cbProducerConsumer _worker)
{
	worker = _worker;
	isRunning = false;
}

void ConsumerWorker::start()
{
	//Do any construction here so it's in the thread
	worker(cbProducerConsumerEvents::Start);
	isRunning = true;
	timespec req, rem;
	elapsedTimer.start();
	while (isRunning) {
		qint64 nsRemaining = nsInterval - elapsedTimer.nsecsElapsed();
		if (nsRemaining > 0) {
			req.tv_sec = 0;
			//We want to get close to exact time, but not go over
			req.tv_nsec = nsRemaining;
			if (nanosleep(&req,&rem) < 0) {
				qDebug()<<"nanosleep failed";
			}
		}
		elapsedTimer.start(); //Restart elapsed timer
		worker(cbProducerConsumerEvents::Run);
	}
	return;
}
//Called just before thread finishes
void ConsumerWorker::stop()
{
	isRunning = false;
	//Do any worker destruction here
	worker(cbProducerConsumerEvents::Stop);
}

