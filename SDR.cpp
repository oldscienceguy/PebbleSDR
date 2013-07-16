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
    sdrOptions = NULL;
    sd = NULL;
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

void SDR::ShowSdrOptions(bool b)
{
    if (sdrOptions == NULL) {
        int cur;

        sdrOptions = new QDialog();
        sd = new Ui::SdrOptions();
        sd->setupUi(sdrOptions);
        sdrOptions->setWindowTitle("Pebble Settings");

        //Audio devices may have been plugged or unplugged, refresh list on each show
        //This will use PortAudio or QTAudio depending on configuration
        inputDevices = Audio::InputDeviceList();
        outputDevices = Audio::OutputDeviceList();
        //Adding items triggers selection, block signals until list is complete
        sd->sourceBox->blockSignals(true);
        sd->sourceBox->clear();
        int id;
        QString dn;
        //Input devices may be restricted form some SDRs
        for (int i=0; i<inputDevices.count(); i++)
        {
            //This is portAudio specific format ID:Name
            //AudioQt will emulate
            id = inputDevices[i].left(2).toInt();
            dn = inputDevices[i].mid(3);
            sd->sourceBox->addItem(dn, id);
            if (dn == inputDeviceName)
                sd->sourceBox->setCurrentIndex(i);
        }
        sd->sourceBox->blockSignals(false);
        connect(sd->sourceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(InputChanged(int)));

        sd->outputBox->blockSignals(true);
        sd->outputBox->clear();
        for (int i=0; i<outputDevices.count(); i++)
        {
            id = outputDevices[i].left(2).toInt();
            dn = outputDevices[i].mid(3);
            sd->outputBox->addItem(dn, id);
            if (dn == outputDeviceName)
                sd->outputBox->setCurrentIndex(i);
        }
        sd->outputBox->blockSignals(false);
        connect(sd->outputBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OutputChanged(int)));

        sd->startupBox->addItem("Last Frequency",SDR::LASTFREQ);
        sd->startupBox->addItem("Set Frequency", SDR::SETFREQ);
        sd->startupBox->addItem("Device Default", SDR::DEFAULTFREQ);
        cur = sd->startupBox->findData(startup);
        sd->startupBox->setCurrentIndex(cur);
        connect(sd->startupBox,SIGNAL(currentIndexChanged(int)),this,SLOT(StartupChanged(int)));

        sd->startupEdit->setText(QString::number(this->startupFreq,'f',0));
        connect(sd->startupEdit,SIGNAL(editingFinished()),this,SLOT(StartupFrequencyChanged()));

        sd->iqGain->setValue(iqGain);
        sd->IQSettings->setCurrentIndex(iqOrder);

        sd->iqBalanceGain->setValue(iqBalanceGain * 1000);
        sd->iqBalancePhase->setValue(iqBalancePhase * 1000);
        sd->iqEnableBalance->setChecked(iqBalanceEnable);

        sd->IQSettings->addItem("I/Q (normal)",IQ);
        sd->IQSettings->addItem("Q/I (swap)",QI);
        sd->IQSettings->addItem("I Only",IONLY);
        sd->IQSettings->addItem("Q Only",QONLY);
        connect(sd->IQSettings,SIGNAL(currentIndexChanged(int)),this,SLOT(IQOrderChanged(int)));

        sd->iqBalancePhase->setMaximum(500);
        sd->iqBalancePhase->setMinimum(-500);
        connect(sd->iqBalancePhase,SIGNAL(valueChanged(int)),this,SLOT(BalancePhaseChanged(int)));

        sd->iqBalanceGain->setMaximum(1250);
        sd->iqBalanceGain->setMinimum(750);
        connect(sd->iqBalanceGain,SIGNAL(valueChanged(int)),this,SLOT(BalanceGainChanged(int)));

        connect(sd->iqEnableBalance,SIGNAL(toggled(bool)),this,SLOT(BalanceEnabledChanged(bool)));

        connect(sd->iqBalanceReset,SIGNAL(clicked()),this,SLOT(BalanceReset()));

        //Serial box only applies to SoftRocks for now
        sd->serialBox->addItem("Any",-1);
        sd->serialBox->addItem("0",0);
        sd->serialBox->addItem("1",1);
        sd->serialBox->addItem("2",2);
        sd->serialBox->addItem("3",3);
        sd->serialBox->addItem("4",4);
        sd->serialBox->addItem("5",5);
        sd->serialBox->addItem("6",6);
        sd->serialBox->addItem("7",7);
        sd->serialBox->addItem("8",8);
        sd->serialBox->addItem("9",9);
        if (sdrDevice == SDR::SR_ENSEMBLE||sdrDevice==SDR::SR_ENSEMBLE_2M||sdrDevice==SDR::SR_ENSEMBLE_4M||
            sdrDevice==SDR::SR_ENSEMBLE_6M || sdrDevice==SDR::SR_ENSEMBLE_LF || sdrDevice==SDR::SR_V9)
            sd->serialBox->setEnabled(true);
        else
            sd->serialBox->setEnabled(false);
        sd->serialBox->setCurrentIndex(sdrNumber+1);

        //Set up options and get allowable sampleRates from device

        int numSr;
        int *sr = this->GetSampleRates(numSr);
        sd->sampleRateBox->blockSignals(true);
        sd->sampleRateBox->clear();
        for (int i=0; i<numSr; i++) {
            sd->sampleRateBox->addItem(QString::number(sr[i]),sr[i]);
            if (sampleRate == sr[i])
                sd->sampleRateBox->setCurrentIndex(i);
        }
        sd->sampleRateBox->blockSignals(false);
        connect(sd->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

        this->SetupOptionUi(sd->sdrSpecific);

    }

    if (b)
        sdrOptions->show();
    else
        sdrOptions->hide();

}

void SDR::SampleRateChanged(int i)
{
    int cur = sd->sampleRateBox->currentIndex();
    sampleRate = sd->sampleRateBox->itemText(cur).toInt();
    WriteSettings();
}

void SDR::InputChanged(int i)
{
    int cur = sd->sourceBox->currentIndex();
    inputDeviceName = sd->sourceBox->itemText(cur);
    WriteSettings();
}

void SDR::OutputChanged(int i)
{
    int cur = sd->outputBox->currentIndex();
    outputDeviceName = sd->outputBox->itemText(cur);
    WriteSettings();
}

void SDR::StartupChanged(int i)
{

    startup = (STARTUP)sd->startupBox->itemData(i).toInt();
    sd->startupEdit->setText(QString::number(startupFreq,'f',0));
    if (startup == SETFREQ) {
        sd->startupEdit->setEnabled(true);
    }
    else {
        sd->startupEdit->setEnabled(false);
    }
    WriteSettings();
}

void SDR::StartupFrequencyChanged()
{
    startupFreq = sd->startupEdit->text().toDouble();
    WriteSettings();
}

//IQ gain can be changed in real time, without saving
void SDR::IQGainChanged(double i)
{
    iqGain = i;
    WriteSettings();
}

//IQ order can be changed in real time, without saving
void SDR::IQOrderChanged(int i)
{
    iqOrder = (IQORDER)sd->IQSettings->itemData(i).toInt();
    WriteSettings();
}

void SDR::BalancePhaseChanged(int v)
{
    //v is an integer, convert to fraction -.500 to +.500
    double newValue = v/1000.0;
    sd->iqBalancePhaseLabel->setText("Phase: " + QString::number(newValue));
    iqBalancePhase = newValue;

    if (!global->receiver->GetPowerOn())
        return;
    global->receiver->GetIQBalance()->setPhaseFactor(newValue);
    WriteSettings();
}

void SDR::BalanceGainChanged(int v)
{
    //v is an integer, convert to fraction -.750 to +1.250
    double newValue = v/1000.0;
    sd->iqBalanceGainLabel->setText("Gain: " + QString::number(newValue));
    iqBalanceGain = newValue;
    WriteSettings();
    //Update in realtime
    if (!global->receiver->GetPowerOn())
        return;
    global->receiver->GetIQBalance()->setGainFactor(newValue);
}

void SDR::BalanceEnabledChanged(bool b)
{
    iqBalanceEnable = b;
    WriteSettings();
    if (!global->receiver->GetPowerOn())
        return;
    global->receiver->GetIQBalance()->setEnabled(b);
}

void SDR::BalanceReset()
{
    sd->iqBalanceGain->setValue(1000);
    sd->iqBalancePhase->setValue(0);
}


//Settings common to all devices
//Make sure to call SDR::ReadSettings() in any derived class
void SDR::ReadSettings()
{
    startup = (STARTUP)qSettings->value("Startup", DEFAULTFREQ).toInt();
    startupFreq = qSettings->value("StartupFreq", 10000000).toDouble();
    inputDeviceName = qSettings->value("InputDeviceName", "").toString();
    outputDeviceName = qSettings->value("OutputDeviceName", "").toString();
    sampleRate = qSettings->value("SampleRate", 48000).toInt();
    sdrNumber = qSettings->value("sdrNumber",-1).toInt();
    iqGain = qSettings->value("iqGain",1).toDouble();
    iqOrder = (IQORDER)qSettings->value("IQOrder", Settings::IQ).toInt();
    iqBalanceGain = qSettings->value("iqBalanceGain",1).toDouble();
    iqBalancePhase = qSettings->value("iqBalancePhase",0).toDouble();
    iqBalanceEnable = qSettings->value("iqBalanceEnable",false).toBool();

}
//Make sure to call SDR::WriteSettings() in any derived class
void SDR::WriteSettings()
{
    qSettings->setValue("Startup",startup);
    qSettings->setValue("StartupFreq",startupFreq);
    qSettings->setValue("InputDeviceName", inputDeviceName);
    qSettings->setValue("OutputDeviceName", outputDeviceName);
    qSettings->setValue("SampleRate",sampleRate);
    qSettings->setValue("sdrNumber",sdrNumber);
    qSettings->setValue("iqGain",iqGain);
    qSettings->setValue("IQOrder", iqOrder);
    qSettings->setValue("iqBalanceGain", iqBalanceGain);
    qSettings->setValue("iqBalancePhase", iqBalancePhase);
    qSettings->setValue("iqBalanceEnable", iqBalanceEnable);

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
    return sampleRate;
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

void SDR::SetupOptionUi(QWidget *parent)
{
    parent->setVisible(false);
    return;
    //Deleting childres messes things up, just hide
    //Default if not overridden - Remove any previous option widget
#if 0
    QObjectList children = parent->children();
    for (int i=0; i<children.count(); i++) {
        delete children[i]; //Delete controls created by sdr specific option widget
    }
#endif
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
