//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include <QLibrary>
#include "sdr.h"
#include "settings.h"
#include "devices/softrock.h"
#include "devices/elektorsdr.h"
#include "devices/sdr_iq.h"
#include "devices/hpsdr.h"
#include "devices/funcube.h"
#include "devices/sdr_ip.h"
#include "soundcard.h"
#include "receiver.h"
#include "QMessageBox"

//Called from plugins.cpp to create an SDR object that delegates to a device plugin
//Necessary while we are in transition from internal to plugins
SDR::SDR(DeviceInterface *_plugin, int _devNum)
{
    plugin = _plugin;
    pluginDeviceNumber = _devNum; //Critical - this is used everywhere to disambiguate multi device plugins

    sdrOptions = NULL;
    sd = NULL;
    //Note: Plugins with multiple device support will all have the same deviceInterface _plugin
    //So each call to this constructor with the same interface ptr will replace all settings
    //This is here just to make sure we get initial values, don't count on data
    ReadSettings();
}

SDR::SDR(Receiver *_receiver, SDRDEVICE dev,Settings *_settings)
{
    settings = _settings;
    if (!settings)
        return; //Init only

    plugin = NULL;
	sdrDevice = dev;
	receiver = _receiver;
	startupFrequency = 0;
	//DLL's are loaded explicitly when we connect to SDR.  Not everyone will have DLLs for every SDR installed
	isLibUsbLoaded = false;
	isFtdiLoaded = false;
	isThreadRunning = false;
    audioInput = NULL;
	producerThread = NULL;
	consumerThread = NULL;
    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;
    producerBuffer = NULL;
    sdrOptions = NULL;
    sd = NULL;
    connected = false;

    //Setup callback for device plugins to use when they have new IQ data
    using namespace std::placeholders;
    //function template must match exactly the method that is used in bind()
    //std::function<void(CPX *, quint16)> cb = std::bind(&Receiver::ProcessIQData, _receiver, std::placeholders::_1, std::placeholders::_2);
    //bind(Method ptr, object, arg1, ... argn)
    ProcessIQData = std::bind(&Receiver::ProcessIQData, _receiver, _1, _2);
}

SDR::~SDR(void)
{
    if (!settings)
        return;

    if (DelegateToPlugin())
        WriteSettings();

    if (sdrOptions != NULL)
        sdrOptions->hide();

	if (audioInput != NULL) {
		delete audioInput;
	}

    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            free (producerBuffer[i]);
        free (producerBuffer);
    }

}

bool SDR::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
    if (DelegateToPlugin())
        return plugin->Initialize(_callback, _framesPerBuffer);
    audioInput = Audio::Factory(receiver,settings->framesPerBuffer,settings);
    return true;
}

bool SDR::Connect()
{
    if (DelegateToPlugin())
        return plugin->Connect();
    else
        return false;
}

bool SDR::Disconnect()
{
    if (DelegateToPlugin())
        return plugin->Disconnect();
    else
        return false;
}

double SDR::SetFrequency(double fRequested, double fCurrent)
{
    if (DelegateToPlugin())
        return plugin->SetFrequency(fRequested,fCurrent);
    else
        return fCurrent;
}

void SDR::Start()
{
    if (DelegateToPlugin())
        return plugin->Start();
}

void SDR::Stop()
{
    if (DelegateToPlugin()) {
        //Things we used to do in ~SDR which is not destructed when we have plugins
        if (sdrOptions != NULL)
            sdrOptions->hide();

        return plugin->Stop();
    }
}

double SDR::GetStartupFrequency()
{
	return 0; //Internal devices override
}

int SDR::GetStartupMode()
{
	return 0;
}

double SDR::GetHighLimit()
{
	return 0;
}

double SDR::GetLowLimit()
{
	return 0;
}

//Redundant with Get(), remove
double SDR::GetGain()
{
	return 1;
}

QString SDR::GetDeviceName()
{
	return "";
}

//Must be called from derived class constructor to work correctly
void SDR::InitSettings(QString fname)
{
    //Use ini files to avoid any registry problems or install/uninstall
    //Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
    //Scope::SystemScope puts file c:\ProgramData\n1ddy

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = new QSettings(path + "/PebbleData/" + fname +".ini",QSettings::IniFormat);

}

//If b is false, close and delete options
void SDR::ShowSdrOptions(bool b)
{
    if (sdrOptions != NULL) {
        //If we're visible, and asking to be shown again, toggle off
        if (b && sdrOptions->isVisible())
            b = false;
        //Close and return
        if (!b) {
            sdrOptions->setVisible(b); //hide
            delete sdrOptions;
            sdrOptions = NULL;
            return;
        }
    }

    if (DelegateToPlugin()) {
        //Do nothing, just call to sync
    }
    DeviceInterface *di = (plugin == NULL) ? this:plugin;

    if (sdrOptions == NULL) {
        int cur;

        sdrOptions = new QDialog();
        sd = new Ui::SdrOptions();
        sd->setupUi(sdrOptions);
        sdrOptions->setWindowTitle("Pebble Settings");

        int id;
        QString dn;
		if (di->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ) {
            //Audio devices may have been plugged or unplugged, refresh list on each show
            //This will use PortAudio or QTAudio depending on configuration
            inputDevices = Audio::InputDeviceList();
            //Adding items triggers selection, block signals until list is complete
            sd->sourceBox->blockSignals(true);
            sd->sourceBox->clear();
            //Input devices may be restricted form some SDRs
            for (int i=0; i<inputDevices.count(); i++)
            {
                //This is portAudio specific format ID:Name
                //AudioQt will emulate
                id = inputDevices[i].left(2).toInt();
                dn = inputDevices[i].mid(3);
                sd->sourceBox->addItem(dn, id);
				if (dn == di->Get(DeviceInterface::InputDeviceName).toString())
                    sd->sourceBox->setCurrentIndex(i);
            }
            sd->sourceBox->blockSignals(false);
            connect(sd->sourceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(InputChanged(int)));
        } else {
            sd->sourceLabel->setVisible(false);
            sd->sourceBox->setVisible(false);
        }
        outputDevices = Audio::OutputDeviceList();
        sd->outputBox->blockSignals(true);
        sd->outputBox->clear();
        for (int i=0; i<outputDevices.count(); i++)
        {
            id = outputDevices[i].left(2).toInt();
            dn = outputDevices[i].mid(3);
            sd->outputBox->addItem(dn, id);
            if (dn == di->outputDeviceName)
                sd->outputBox->setCurrentIndex(i);
        }
        sd->outputBox->blockSignals(false);
        connect(sd->outputBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OutputChanged(int)));

        sd->startupBox->addItem("Last Frequency",SDR::LASTFREQ);
        sd->startupBox->addItem("Set Frequency", SDR::SETFREQ);
        sd->startupBox->addItem("Device Default", SDR::DEFAULTFREQ);
        cur = sd->startupBox->findData(di->startup);
        sd->startupBox->setCurrentIndex(cur);
        connect(sd->startupBox,SIGNAL(currentIndexChanged(int)),this,SLOT(StartupChanged(int)));

        sd->startupEdit->setText(QString::number(di->freqToSet,'f',0));
        connect(sd->startupEdit,SIGNAL(editingFinished()),this,SLOT(StartupFrequencyChanged()));

        sd->iqGain->setValue(di->iqGain);
        connect(sd->iqGain,SIGNAL(valueChanged(double)),this,SLOT(IQGainChanged(double)));

        sd->IQSettings->addItem("I/Q (normal)",IQ);
        sd->IQSettings->addItem("Q/I (swap)",QI);
        sd->IQSettings->addItem("I Only",IONLY);
        sd->IQSettings->addItem("Q Only",QONLY);
        sd->IQSettings->setCurrentIndex(di->iqOrder);
        connect(sd->IQSettings,SIGNAL(currentIndexChanged(int)),this,SLOT(IQOrderChanged(int)));

        sd->iqEnableBalance->setChecked(di->iqBalanceEnable);

        sd->iqBalancePhase->setMaximum(500);
        sd->iqBalancePhase->setMinimum(-500);
        sd->iqBalancePhase->setValue(di->iqBalancePhase * 1000);
        connect(sd->iqBalancePhase,SIGNAL(valueChanged(int)),this,SLOT(BalancePhaseChanged(int)));

        sd->iqBalanceGain->setMaximum(1250);
        sd->iqBalanceGain->setMinimum(750);
        sd->iqBalanceGain->setValue(di->iqBalanceGain * 1000);
        connect(sd->iqBalanceGain,SIGNAL(valueChanged(int)),this,SLOT(BalanceGainChanged(int)));

        connect(sd->iqEnableBalance,SIGNAL(toggled(bool)),this,SLOT(BalanceEnabledChanged(bool)));

        connect(sd->iqBalanceReset,SIGNAL(clicked()),this,SLOT(BalanceReset()));

        //Set up options and get allowable sampleRates from device

		int sr;
		QStringList sampleRates = di->Get(DeviceInterface::DeviceSampleRates).toStringList();
        sd->sampleRateBox->blockSignals(true);
        sd->sampleRateBox->clear();
		for (int i=0; i<sampleRates.count(); i++) {
			sr = sampleRates[i].toInt();
			sd->sampleRateBox->addItem(sampleRates[i],sr);
			if (di->sampleRate == sr)
                sd->sampleRateBox->setCurrentIndex(i);
        }
        sd->sampleRateBox->blockSignals(false);
        connect(sd->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

        connect(sd->closeButton,SIGNAL(clicked(bool)),this,SLOT(CloseOptions(bool)));
        connect(sd->resetAllButton,SIGNAL(clicked(bool)),this,SLOT(ResetAllSettings(bool)));

		sd->testBenchBox->setChecked(global->settings->useTestBench);
        connect(sd->testBenchBox, SIGNAL(toggled(bool)), this, SLOT(TestBenchChanged(bool)));

        //Careful here: Fragile coding practice
        //We're calling a virtual function in a base class method and expect it to call the over-ridden method in derived class
        //This only works because ShowSdrOptions() is being called via a pointer to the derived class
        //Some other method that's called from the base class could not do this
        di->SetupOptionUi(sd->sdrSpecific);

    }

    if (b)
        sdrOptions->show();
    else
        sdrOptions->hide();

}

void SDR::CloseOptions(bool b)
{
    if (sdrOptions != NULL)
        sdrOptions->close();
}

void SDR::TestBenchChanged(bool b)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
	settings->useTestBench = b;
    WriteSettings();
}

void SDR::SampleRateChanged(int i)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    int cur = sd->sampleRateBox->currentIndex();
    di->sampleRate = sd->sampleRateBox->itemText(cur).toInt();
    WriteSettings();
}

void SDR::InputChanged(int i)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    int cur = sd->sourceBox->currentIndex();
    di->inputDeviceName = sd->sourceBox->itemText(cur);
    WriteSettings();
}

void SDR::OutputChanged(int i)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    int cur = sd->outputBox->currentIndex();
    di->outputDeviceName = sd->outputBox->itemText(cur);
    WriteSettings();
}

void SDR::StartupChanged(int i)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;

    di->startup = (STARTUP)sd->startupBox->itemData(i).toInt();
    sd->startupEdit->setText(QString::number(freqToSet,'f',0));
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
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    di->freqToSet = sd->startupEdit->text().toDouble();
    WriteSettings();
}

//IQ gain can be changed in real time, without saving
void SDR::IQGainChanged(double i)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    di->iqGain = i;
    WriteSettings();
}

//IQ order can be changed in real time, without saving
void SDR::IQOrderChanged(int i)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
	di->Set(DeviceInterface::IQOrder, sd->IQSettings->itemData(i).toInt());
    WriteSettings();
}

void SDR::BalancePhaseChanged(int v)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    //v is an integer, convert to fraction -.500 to +.500
    double newValue = v/1000.0;
    sd->iqBalancePhaseLabel->setText("Phase: " + QString::number(newValue));
    di->iqBalancePhase = newValue;

    if (!global->receiver->GetPowerOn())
        return;
    global->receiver->GetIQBalance()->setPhaseFactor(newValue);
    WriteSettings();
}

void SDR::BalanceGainChanged(int v)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    //v is an integer, convert to fraction -.750 to +1.250
    double newValue = v/1000.0;
    sd->iqBalanceGainLabel->setText("Gain: " + QString::number(newValue));
    di->iqBalanceGain = newValue;
    WriteSettings();
    //Update in realtime
    if (!global->receiver->GetPowerOn())
        return;
    global->receiver->GetIQBalance()->setGainFactor(newValue);
}

void SDR::BalanceEnabledChanged(bool b)
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    di->iqBalanceEnable = b;
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

void SDR::ResetAllSettings(bool b)
{
    //Confirm
    QMessageBox::StandardButton bt = QMessageBox::question(NULL,
            tr("Confirm Reset"),
            tr("Are you sure you want to reset all settings for this device")
            );
    if (bt == QMessageBox::Ok || bt == QMessageBox::Yes) {
        //Delete ini files and restart
        if (sdrOptions != NULL)
            sdrOptions->close();
        //Disabled
        //emit Restart();

		QString fName = qSettings->fileName();
        QFile f(fName);
        f.remove();
    }
}

//Settings common to all devices
//Make sure to call SDR::ReadSettings() in any derived class
void SDR::ReadSettings()
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
	qDebug()<<"Reading settings for "<<di->Get(PluginName,pluginDeviceNumber).toString();

    if (DelegateToPlugin()) {
        plugin->ReadSettings();
    } else {
		di->startup = (STARTUP)qSettings->value("Startup", DEFAULTFREQ).toInt();
		di->freqToSet = qSettings->value("StartupFreq", 10000000).toDouble();
		di->inputDeviceName = qSettings->value("InputDeviceName", "").toString();
		di->outputDeviceName = qSettings->value("OutputDeviceName", "").toString();
		di->sampleRate = qSettings->value("SampleRate", 48000).toInt();
		di->iqGain = qSettings->value("iqGain",1).toDouble();
		di->iqOrder = (IQORDER)qSettings->value("IQOrder", SDR::IQ).toInt();
		di->iqBalanceGain = qSettings->value("iqBalanceGain",1).toDouble();
		di->iqBalancePhase = qSettings->value("iqBalancePhase",0).toDouble();
		di->iqBalanceEnable = qSettings->value("iqBalanceEnable",false).toBool();
		di->lastFreq = qSettings->value("LastFreq", 10000000).toDouble();
		di->lastMode = qSettings->value("LastMode",0).toInt();
		di->lastSpectrumMode = qSettings->value("LastDisplayMode",0).toInt();
    }
}
//Make sure to call SDR::WriteSettings() in any derived class
void SDR::WriteSettings()
{
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
	qDebug()<<"Writing settings for "<<di->Get(PluginName,pluginDeviceNumber).toString();

    if (DelegateToPlugin()) {
        plugin->WriteSettings();
    } else {
        //Plugins are responsible for these, but if no plugin, write them here
		qSettings->setValue("Startup",di->startup);
		qSettings->setValue("StartupFreq",di->freqToSet);
		qSettings->setValue("InputDeviceName", di->inputDeviceName);
		qSettings->setValue("OutputDeviceName", di->outputDeviceName);
		qSettings->setValue("SampleRate",di->sampleRate);
		qSettings->setValue("iqGain",di->iqGain);
		qSettings->setValue("IQOrder", di->iqOrder);
		qSettings->setValue("iqBalanceGain", di->iqBalanceGain);
		qSettings->setValue("iqBalancePhase", di->iqBalancePhase);
		qSettings->setValue("iqBalanceEnable", di->iqBalanceEnable);
		qSettings->setValue("LastFreq",di->lastFreq);
		qSettings->setValue("LastMode",di->lastMode);
		qSettings->setValue("LastDisplayMode",di->lastSpectrumMode);
		qSettings->sync();
	}
}

//Devices may override this and return a rate based on other settings
int SDR::GetSampleRate()
{
	return sampleRate;
}

QStringList SDR::GetSampleRates()
{
	return QStringList()<<"48000"<<"96000"<<"192000";
}

void SDR::SetupOptionUi(QWidget *parent)
{
    //Do nothing by default to leave empty frame in dialog
    //parent->setVisible(false);
    if (DelegateToPlugin())
        return plugin->SetupOptionUi(parent);
    else
        return;
}

bool SDR::UsesAudioInput()
{
	return true;
}

void SDR::SetLastDisplayMode(int mode)
{
	lastSpectrumMode = mode;
}

void SDR::SetIQOrder(DeviceInterface::IQORDER o)
{
	iqOrder = o;
}

void SDR::SetLastFreq(double f)
{
	lastFreq = f;
}

void SDR::SetLastMode(int mode)
{
	lastMode = mode;
}

QString SDR::GetInputDeviceName()
{
	return inputDeviceName;
}

QString SDR::GetOutputDeviceName()
{
	return outputDeviceName;
}

void SDR::SetIQGain(double g)
{
	iqGain = g;
}

//Call this in every deviceInterface method to see if we should delegate, and if so make sure deviceNumber is in sync
bool SDR::DelegateToPlugin()
{
    if (plugin == NULL)
        return false;

	plugin->Set(DeviceInterface::DeviceNumber,pluginDeviceNumber);
    return true;
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
        //qDebug()<<"No free buffer available, ignoring block.";
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
        //qDebug("Limited Free buffers available %d",available);
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
        //qDebug("Filled buffers available %d",available);
        filledBufferOverflow = true;
    } else {
        filledBufferOverflow = false;
    }

    semNumFilledBuffers->acquire(); //Will not return until we get a filled buffer, but will yield

}

//This will be removed when we switch over to producerConsumer class
void SDR::StopProducerThread(){}
void SDR::RunProducerThread(){}
void SDR::StopConsumerThread(){}
void SDR::RunConsumerThread(){}

QVariant SDR::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	if (DelegateToPlugin())
		return plugin->Get(_key,_option);

	//Handle transition here
	switch (_key) {
		case PluginName:
			return GetDeviceName(); //Not a plugin, return something meaningful
			break;
		case PluginDescription:
			return "Internal Device (not a plugin)";
			break;
		case PluginNumDevices:
			return 1;
			break;
		case DeviceName:
			return GetDeviceName();
			break;
		case DeviceDescription:
			break;
		case DeviceNumber:
			return deviceNumber;
			break;
		case DeviceType:
			return UsesAudioInput();
			break;
		case DeviceSampleRates:
			return GetSampleRates();
			break;
		case InputDeviceName:
			return inputDeviceName;
			break;
		case OutputDeviceName:
			return outputDeviceName;
			break;
		case HighFrequency:
			return GetHighLimit();
			break;
		case LowFrequency:
			return GetLowLimit();
			break;
		case FrequencyCorrection:
			return 0;
			break;
		case IQGain:
			return iqGain;
			break;
		case SampleRate:
			return GetSampleRate();
			break;
		case StartupType:
			return startup;
			break;
		case StartupMode:
			return GetStartupMode();
			break;
		case StartupFrequency:
			return GetStartupFrequency();
			break;
		case LastMode:
			return lastMode;
			break;
		case LastFrequency:
			//If freq is outside of mode we are in return default
			if (lastFreq > GetHighLimit() || lastFreq < GetLowLimit())
				return GetStartupFrequency();
			else
				return lastFreq;
			break;
		case LastSpectrumMode:
			return lastSpectrumMode;
			break;
		case UserMode:
			break;
		case UserFrequency:
			//If freq is outside of mode we are in return default
			if (freqToSet > GetHighLimit() || freqToSet < GetLowLimit())
				return GetStartupFrequency();
			else
				return freqToSet;
			break;
		case IQOrder:
			return iqOrder;
			break;
		case IQBalanceEnabled:
			return iqBalanceEnable;
			break;
		case IQBalanceGain:
			return iqBalanceGain;
			break;
		case IQBalancePhase:
			return iqBalancePhase;
			break;
		default:
			break;

	}
	return QVariant();
}

bool SDR::Set(STANDARD_KEYS _key, QVariant _value, quint16 _option) {
	switch (_key) {
		case PluginName:
			break;
		case PluginDescription:
			break;
		case PluginNumDevices:
			break;
		case DeviceName:
			break;
		case DeviceDescription:
			break;
		case DeviceNumber:
			deviceNumber = _value.toInt();
			break;
		case DeviceType:
			break;
		case DeviceSampleRates:
			break;
		case InputDeviceName:
			break;
		case OutputDeviceName:
			break;
		case HighFrequency:
			break;
		case LowFrequency:
			break;
		case FrequencyCorrection:
			//Device specific
			break;
		case IQGain:
			iqGain = _value.toDouble();
			break;
		case SampleRate:
			break;
		case StartupType:
			break;
		case StartupMode:
			break;
		case StartupFrequency:
			break;
		case LastMode:
			lastMode = _value.toInt();
			break;
		case LastFrequency:
			lastFreq = _value.toDouble();
			break;
		case LastSpectrumMode:
			lastSpectrumMode = _value.toInt();
			break;
		case UserMode:
			break;
		case UserFrequency:
			break;
		case IQOrder:
			iqOrder = (IQORDER)_value.toInt();
			break;
		case IQBalanceEnabled:
			break;
		case IQBalanceGain:
			break;
		case IQBalancePhase:
			break;
		default:
			break;
	}
	return false;
}

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
