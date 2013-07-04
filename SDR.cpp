//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QLibrary>

#include "sdr.h"
#include "settings.h"
#include "Devices/SoftRock.h"
#include "Devices/ElektorSDR.h"
#include "Devices/sdr_iq.h"
#include "Devices/hpsdr.h"
#include "Devices/funcube.h"
#include "Devices/sdrfile.h"
#include "Devices/rtl2832.h"
#include "Devices/sdr_ip.h"
#include "SoundCard.h"
#include "Receiver.h"

SDR::SDR(Receiver *_receiver, SDRDEVICE dev,Settings *_settings)
{
    settings = _settings;
    if (!settings)
        return; //Init only

	sdrDevice = dev;
	receiver = _receiver;
	startupFrequency = 0;
	//DLL's are loaded explicitly when we connect to SDR.  Not everyone will have DLLs for every SDR installed
	isLibUsbLoaded = false;
	isFtdiLoaded = false;
	isThreadRunning = false;
    audioInput = Audio::Factory(receiver,settings->framesPerBuffer,settings);
	producerThread = NULL;
	consumerThread = NULL;
    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;

}

SDR::~SDR(void)
{
    if (!settings)
        return;

	if (audioInput != NULL) {
		delete audioInput;
	}

    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            free (producerBuffer[i]);
        free (producerBuffer);
    }

}

//Settings common to all devices
void SDR::ReadSettings(QSettings *settings)
{
}
void SDR::WriteSettings(QSettings *settings)
{
}

//Static
SDR *SDR::Factory(Receiver *receiver, SDR::SDRDEVICE dev, Settings *settings)
{
	SDR *sdr=NULL;

    switch (dev)
	{
	case SDR::SR_LITE:
        sdr = new SoftRock(receiver, SDR::SR_LITE,settings);
		break;
	case SDR::SR_V9:
		sdr = new SoftRock(receiver, SDR::SR_V9,settings);
		break;
	case SDR::SR_ENSEMBLE:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE,settings);
		break;
	case SDR::SR_ENSEMBLE_2M:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_2M,settings);
		break;
	case SDR::SR_ENSEMBLE_4M:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_4M,settings);
		break;
	case SDR::SR_ENSEMBLE_6M:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_6M,settings);
		break;
	case SDR::SR_ENSEMBLE_LF:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_LF,settings);
		break;
	case SDR::ELEKTOR:
		sdr = new ElektorSDR(receiver, SDR::ELEKTOR,settings);
		break;
	case SDR::ELEKTOR_PA:
		sdr = new ElektorSDR(receiver, SDR::ELEKTOR_PA,settings);
		break;
    case SDR::SDR_IQ_USB:
        sdr = new SDR_IQ(receiver, SDR::SDR_IQ_USB,settings);
		break;
    case SDR::SDR_IP_TCP:
        sdr = new SDR_IP(receiver, SDR::SDR_IP_TCP,settings);
        break;
    case SDR::HPSDR_USB:
		sdr = new HPSDR(receiver, SDR::HPSDR_USB,settings);
		break;
	case SDR::SDRWIDGET:
		sdr = new HPSDR(receiver, SDR::SDRWIDGET,settings);
		break;

	case SDR::FUNCUBE:
		sdr = new FunCube(receiver, SDR::FUNCUBE,settings);
		break;
    case SDR::FUNCUBE_PLUS:
        sdr = new FunCube(receiver, SDR::FUNCUBE_PLUS,settings);
        break;

    case SDR::FILE:
        sdr = new SDRFile(receiver,SDR::FILE, settings);
        break;

    case SDR::DVB_T:
        sdr = new RTL2832(receiver,SDR::DVB_T, settings);
        break;

    case SDR::HPSDR_TCP:
	case SDR::NOSDR:
		sdr = NULL;
		break;
	}
	return sdr;
}


//Devices may override this and return a rate based on other settings
int SDR::GetSampleRate()
{
	return settings->sampleRate;
}

int *SDR::GetSampleRates(int &len)
{
    len = 3;
    //Ugly, but couldn't find easy way to init with {1,2,3} array initializer
    sampleRates[0] = 48000;
    sampleRates[1] = 96000;
    sampleRates[2] = 192000;
    return sampleRates;
}

SDR::SDRDEVICE SDR::GetDevice()
{
	return sdrDevice;
}
void SDR::SetDevice(SDRDEVICE m)
{
    sdrDevice = m;
}

void SDR::InitProducerConsumer(int _numDataBufs, int _producerBufferSize)
{
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

}

bool SDR::IsFreeBufferAvailable()
{
    if (semNumFreeBuffers == NULL)
        return false; //Not initialized yet

    //Make sure we have at least 1 data buffer available without blocking
    int freeBuf = semNumFreeBuffers->available();
    if (freeBuf == 0) {
        qDebug()<<"No free buffer available, ignoring block.";
        return false;
    }
    return true;
}

void SDR::AcquireFreeBuffer()
{
    if (semNumFreeBuffers == NULL)
        return; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFreeBuffers->available();
    if ( available < 5) { //Ouput when we get within 5 of overflow
        qDebug("Limited Free buffers available %d",available);
        freeBufferOverflow = true;
    } else {
        freeBufferOverflow = false;
    }

    semNumFreeBuffers->acquire(); //Will not return until we get a free buffer, but will yield
}

void SDR::AcquireFilledBuffer()
{
    if (semNumFilledBuffers == NULL)
        return; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFilledBuffers->available();
    if ( available > (numDataBufs - 5)) { //Ouput when we get within 5 of overflow
        qDebug("Filled buffers available %d",available);
        filledBufferOverflow = true;
    } else {
        filledBufferOverflow = false;
    }

    semNumFilledBuffers->acquire(); //Will not return until we get a filled buffer, but will yield

}

void SDR::StopProducerThread(){}
void SDR::RunProducerThread(){}
void SDR::StopConsumerThread(){}
void SDR::RunConsumerThread(){}

//SDRThreads
SDRProducerThread::SDRProducerThread(SDR *_sdr)
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
SDRConsumerThread::SDRConsumerThread(SDR *_sdr)
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
