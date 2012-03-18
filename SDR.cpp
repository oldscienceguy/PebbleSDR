//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QLibrary>

#include "sdr.h"
#include "settings.h"
#include "SoftRock.h"
#include "ElektorSDR.h"
#include "sdr_iq.h"
#include "hpsdr.h"
#include "funcube.h"
#include "sdrfile.h"
#include "SoundCard.h"
#include "Receiver.h"

SDR::SDR(Receiver *_receiver, SDRDEVICE dev,Settings *_settings)
{
	sdrDevice = dev;
	settings = _settings;
	receiver = _receiver;
	startupFrequency = 0;
	//DLL's are loaded explicitly when we connect to SDR.  Not everyone will have DLLs for every SDR installed
	isLibUsbLoaded = false;
	isFtdiLoaded = false;
	isThreadRunning = false;
	audioInput = new SoundCard(receiver,GetSampleRate(),settings->framesPerBuffer,settings);
	producerThread = NULL;
	consumerThread = NULL;

	//Will be set when we read device.ini file, here for safety
	sIQBalanceGain=1.0;
	sIQBalancePhase=0;
	sIQBalanceEnable=false;
	sIQOrder = SDR::IQ;
	iqBalanceOptions = NULL;
	iqDialog = NULL;
}

SDR::~SDR(void)
{
	if (audioInput != NULL) {
		delete audioInput;
	}
	if (iqDialog != NULL && iqDialog->isVisible())
		iqDialog->hide();

}

//Settings common to all devices
void SDR::ReadSettings(QSettings *settings)
{
	sIQBalanceGain = settings->value("IQBalanceGain",1.0).toDouble();
	sIQBalancePhase = settings->value("IQBalancePhase",0.0).toDouble();
	sIQBalanceEnable = settings->value("IQBalanceEnable",false).toBool();
	sIQOrder = (IQORDER)settings->value("IQOrder", SDR::IQ).toInt();


}
void SDR::WriteSettings(QSettings *settings)
{
	settings->setValue("IQBalanceGain",sIQBalanceGain);
	settings->setValue("IQBalancePhase",sIQBalancePhase);
	settings->setValue("IQBalanceEnable",sIQBalanceEnable);
	settings->setValue("IQOrder", sIQOrder);

}

//Static
SDR *SDR::Factory(Receiver *receiver, Settings *settings)
{
	SDR *sdr=NULL;

	switch (settings->sdrDevice)
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
	case SDR::SDR_IQ:
		//Todo: Enum is conflicting with class, rename one or the other
		sdr = new SDR_IQ::SDR_IQ(receiver, SDR::SDR_IQ,settings);
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

    case SDR::FILE:
        sdr = new SDRFile(receiver,SDR::FILE, settings);
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

void SDR::phaseChanged(int v)
{
	//v is an integer, convert to fraction -.500 to +.500
	double newValue = v/1000.0;
	iqBalanceOptions->phaseLabel->setText("Phase: " + QString::number(newValue));
	receiver->GetIQBalance()->setPhaseFactor(newValue);
}
void SDR::gainChanged(int v)
{
	//v is an integer, convert to fraction -.750 to +1.250
	double newValue = v/1000.0;
	iqBalanceOptions->gainLabel->setText("Gain: " + QString::number(newValue));
	receiver->GetIQBalance()->setGainFactor(newValue);
}
void SDR::enabledChanged(bool b)
{
	receiver->GetIQBalance()->setEnabled(b);
}
void SDR::automaticChanged(bool b)
{
	receiver->GetIQBalance()->setAutomatic(b);
}
void SDR::resetClicked()
{
	receiver->GetIQBalance()->setGainFactor(1);
	receiver->GetIQBalance()->setPhaseFactor(0);
	iqBalanceOptions->phaseSlider->setValue(0);
	iqBalanceOptions->gainSlider->setValue(1000);
}
void SDR::saveClicked()
{
	sIQBalanceGain=receiver->GetIQBalance()->getGainFactor();
	sIQBalancePhase=receiver->GetIQBalance()->getPhaseFactor();
	sIQBalanceEnable=receiver->GetIQBalance()->getEnabled();
}
//IQ order can be changed in real time, without saving
void SDR::IQOrderChanged(int i)
{
	sIQOrder = (IQORDER)iqBalanceOptions->IQSettings->itemData(i).toInt();
	//Settings are read in real time by soundcard loop, no need to notify
}

void SDR::ShowIQOptions()
{

	if (iqDialog == NULL) {
		iqBalanceOptions = new Ui::IQBalanceOptions();
		iqDialog = new QDialog();
		iqBalanceOptions->setupUi(iqDialog);

		iqBalanceOptions->IQSettings->addItem("I/Q (normal)",IQ);
		iqBalanceOptions->IQSettings->addItem("Q/I (swap)",QI);
		iqBalanceOptions->IQSettings->addItem("I Only",IONLY);
		iqBalanceOptions->IQSettings->addItem("Q Only",QONLY);
		connect(iqBalanceOptions->IQSettings,SIGNAL(currentIndexChanged(int)),this,SLOT(IQOrderChanged(int)));

		iqBalanceOptions->phaseSlider->setMaximum(500);
		iqBalanceOptions->phaseSlider->setMinimum(-500);

		connect(iqBalanceOptions->phaseSlider,SIGNAL(valueChanged(int)),this,SLOT(phaseChanged(int)));

		iqBalanceOptions->gainSlider->setMaximum(1250);
		iqBalanceOptions->gainSlider->setMinimum(750);

		connect(iqBalanceOptions->gainSlider,SIGNAL(valueChanged(int)),this,SLOT(gainChanged(int)));

		connect(iqBalanceOptions->enableBalanceBox,SIGNAL(toggled(bool)),this,SLOT(enabledChanged(bool)));
		iqBalanceOptions->autoBalanceBox->setEnabled(false); //Not ready yet
		connect(iqBalanceOptions->autoBalanceBox,SIGNAL(toggled(bool)),this,SLOT(automaticChanged(bool)));
		connect(iqBalanceOptions->resetButton,SIGNAL(clicked()),this,SLOT(resetClicked()));

		connect(iqBalanceOptions->saveButton,SIGNAL(clicked()),this,SLOT(saveClicked()));
	}

	iqBalanceOptions->IQSettings->setCurrentIndex(sIQOrder);
	iqBalanceOptions->phaseSlider->setValue(sIQBalancePhase*1000);
	iqBalanceOptions->phaseLabel->setText("Phase: " + QString::number(sIQBalancePhase));
	iqBalanceOptions->gainSlider->setValue(sIQBalanceGain*1000);
	iqBalanceOptions->gainLabel->setText("Gain: " + QString::number(sIQBalanceGain));
	iqBalanceOptions->enableBalanceBox->setChecked(sIQBalanceEnable);

	iqDialog->show();

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
