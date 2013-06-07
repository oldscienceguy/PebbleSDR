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
}

SDR::~SDR(void)
{
    if (!settings)
        return;

	if (audioInput != NULL) {
		delete audioInput;
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
