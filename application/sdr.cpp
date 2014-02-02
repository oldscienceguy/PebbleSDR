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
#include "testbench.h"

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

QString SDR::GetPluginName(int _devNum)
{
    if (DelegateToPlugin())
        return plugin->GetPluginName();
    else
        return GetDeviceName(); //Use device name as pseudo plugin name
}

QString SDR::GetPluginDescription(int _devNum)
{
    if (DelegateToPlugin())
        return plugin->GetPluginDescription();
    else
        return "";
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
    if (DelegateToPlugin())
        return plugin->GetStartupFrequency();
    else
        return 0;
}

int SDR::GetStartupMode()
{
    if (DelegateToPlugin())
        return plugin->GetStartupMode();
    else
        return 0;
}

double SDR::GetHighLimit()
{
    if (DelegateToPlugin())
        return plugin->GetHighLimit();
    else
        return 0;
}

double SDR::GetLowLimit()
{
    if (DelegateToPlugin())
        return plugin->GetLowLimit();
    else
        return 0;
}

double SDR::GetGain()
{
    if (DelegateToPlugin())
        return plugin->GetGain();
    else
        return 1;
}

QString SDR::GetDeviceName()
{
    if (DelegateToPlugin())
        return plugin->GetDeviceName();
    else
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
    if (!b) {
        if (sdrOptions != NULL) {
            sdrOptions->setVisible(b); //hide
            delete sdrOptions;
            sdrOptions = NULL;
        }
        return;
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
        if (di->UsesAudioInput()) {
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
                if (dn == di->inputDeviceName)
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

        int numSr;
        int *sr = di->GetSampleRates(numSr);
        sd->sampleRateBox->blockSignals(true);
        sd->sampleRateBox->clear();
        for (int i=0; i<numSr; i++) {
            sd->sampleRateBox->addItem(QString::number(sr[i]),sr[i]);
            if (di->sampleRate == sr[i])
                sd->sampleRateBox->setCurrentIndex(i);
        }
        sd->sampleRateBox->blockSignals(false);
        connect(sd->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

        connect(sd->closeButton,SIGNAL(clicked(bool)),this,SLOT(CloseOptions(bool)));
        connect(sd->resetAllButton,SIGNAL(clicked(bool)),this,SLOT(ResetAllSettings(bool)));

        sd->testBenchBox->setChecked(di->isTestBenchChecked);
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
    di->isTestBenchChecked = b;
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
    di->iqOrder = (IQORDER)sd->IQSettings->itemData(i).toInt();
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

        QString fName = GetQSettings()->fileName();
        QFile f(fName);
        f.remove();
    }
}

//Settings common to all devices
//Make sure to call SDR::ReadSettings() in any derived class
void SDR::ReadSettings()
{
    QSettings *qs = GetQSettings();
    if (DelegateToPlugin()) {
        plugin->ReadSettings();
    }
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    qDebug()<<"Reading settings for "<<di->GetPluginName(pluginDeviceNumber);

    di->startup = (STARTUP)qs->value("Startup", DEFAULTFREQ).toInt();
    di->freqToSet = qs->value("StartupFreq", 10000000).toDouble();
    di->inputDeviceName = qs->value("InputDeviceName", "").toString();
    di->outputDeviceName = qs->value("OutputDeviceName", "").toString();
    di->sampleRate = qs->value("SampleRate", 48000).toInt();
    di->iqGain = qs->value("iqGain",1).toDouble();
    di->iqOrder = (IQORDER)qs->value("IQOrder", SDR::IQ).toInt();
    di->iqBalanceGain = qs->value("iqBalanceGain",1).toDouble();
    di->iqBalancePhase = qs->value("iqBalancePhase",0).toDouble();
    di->iqBalanceEnable = qs->value("iqBalanceEnable",false).toBool();
    di->lastFreq = qs->value("LastFreq", 10000000).toDouble();
    di->lastMode = qs->value("LastMode",0).toInt();
    di->lastDisplayMode = qs->value("LastDisplayMode",0).toInt();
    di->isTestBenchChecked = qs->value("TestBench",false).toBool();

    qs->beginGroup(tr("Testbench"));

    global->testBench->m_SweepStartFrequency = qs->value(tr("SweepStartFrequency"),0.0).toDouble();
    global->testBench->m_SweepStopFrequency = qs->value(tr("SweepStopFrequency"),1.0).toDouble();
    global->testBench->m_SweepRate = qs->value(tr("SweepRate"),0.0).toDouble();
    global->testBench->m_DisplayRate = qs->value(tr("DisplayRate"),10).toInt();
    global->testBench->m_VertRange = qs->value(tr("VertRange"),10000).toInt();
    global->testBench->m_TrigIndex = qs->value(tr("TrigIndex"),0).toInt();
    global->testBench->m_TrigLevel = qs->value(tr("TrigLevel"),100).toInt();
    global->testBench->m_HorzSpan = qs->value(tr("HorzSpan"),100).toInt();
    global->testBench->m_Profile = qs->value(tr("Profile"),0).toInt();
    global->testBench->m_TimeDisplay = qs->value(tr("TimeDisplay"),false).toBool();
    global->testBench->m_GenOn = qs->value(tr("GenOn"),false).toBool();
    global->testBench->m_PeakOn = qs->value(tr("PeakOn"),false).toBool();
    global->testBench->m_PulseWidth = qs->value(tr("PulseWidth"),0.0).toDouble();
    global->testBench->m_PulsePeriod = qs->value(tr("PulsePeriod"),0.0).toDouble();
    global->testBench->m_SignalPower = qs->value(tr("SignalPower"),0.0).toDouble();
    global->testBench->m_NoisePower = qs->value(tr("NoisePower"),-70.0).toDouble();
    global->testBench->m_UseFmGen = qs->value(tr("UseFmGen"),false).toBool();

    qs->endGroup();


}
//Make sure to call SDR::WriteSettings() in any derived class
void SDR::WriteSettings()
{
    QSettings *qs = GetQSettings();
    if (DelegateToPlugin()) {
        plugin->WriteSettings();
    }
    DeviceInterface *di = (plugin == NULL) ? this:plugin;
    qDebug()<<"Writing settings for "<<di->GetPluginName();

    qs->setValue("Startup",di->startup);
    qs->setValue("StartupFreq",di->freqToSet);
    qs->setValue("InputDeviceName", di->inputDeviceName);
    qs->setValue("OutputDeviceName", di->outputDeviceName);
    qs->setValue("SampleRate",di->sampleRate);
    qs->setValue("iqGain",di->iqGain);
    qs->setValue("IQOrder", di->iqOrder);
    qs->setValue("iqBalanceGain", di->iqBalanceGain);
    qs->setValue("iqBalancePhase", di->iqBalancePhase);
    qs->setValue("iqBalanceEnable", di->iqBalanceEnable);
    qs->setValue("LastFreq",di->lastFreq);
    qs->setValue("LastMode",di->lastMode);
    qs->setValue("LastDisplayMode",di->lastDisplayMode);
    qs->setValue("TestBench",di->isTestBenchChecked);

    qs->beginGroup(tr("Testbench"));

    qs->setValue(tr("SweepStartFrequency"),global->testBench->m_SweepStartFrequency);
    qs->setValue(tr("SweepStopFrequency"),global->testBench->m_SweepStopFrequency);
    qs->setValue(tr("SweepRate"),global->testBench->m_SweepRate);
    qs->setValue(tr("DisplayRate"),global->testBench->m_DisplayRate);
    qs->setValue(tr("VertRange"),global->testBench->m_VertRange);
    qs->setValue(tr("TrigIndex"),global->testBench->m_TrigIndex);
    qs->setValue(tr("TimeDisplay"),global->testBench->m_TimeDisplay);
    qs->setValue(tr("HorzSpan"),global->testBench->m_HorzSpan);
    qs->setValue(tr("TrigLevel"),global->testBench->m_TrigLevel);
    qs->setValue(tr("Profile"),global->testBench->m_Profile);
    qs->setValue(tr("GenOn"),global->testBench->m_GenOn);
    qs->setValue(tr("PeakOn"),global->testBench->m_PeakOn);
    qs->setValue(tr("PulseWidth"),global->testBench->m_PulseWidth);
    qs->setValue(tr("PulsePeriod"),global->testBench->m_PulsePeriod);
    qs->setValue(tr("SignalPower"),global->testBench->m_SignalPower);
    qs->setValue(tr("NoisePower"),global->testBench->m_NoisePower);
    qs->setValue(tr("UseFmGen"),global->testBench->m_UseFmGen);

    qs->endGroup();

    qs->sync();

}

//Devices may override this and return a rate based on other settings
int SDR::GetSampleRate()
{

    if (DelegateToPlugin())
        return plugin->GetSampleRate();
    else
        return sampleRate;
}

int *SDR::GetSampleRates(int &len)
{
    if (DelegateToPlugin())
        return plugin->GetSampleRates(len);
    else {
        len = 3;
        //Ugly, but couldn't find easy way to init with {1,2,3} array initializer
        sampleRates[0] = 48000;
        sampleRates[1] = 96000;
        sampleRates[2] = 192000;
        return sampleRates;
    }
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

void SDR::WriteOptionUi()
{
    if (DelegateToPlugin())
        return plugin->WriteOptionUi();
    else
        return;
}

bool SDR::UsesAudioInput()
{
    if (DelegateToPlugin())
        return plugin->UsesAudioInput();
    else
        return true;
}

bool SDR::GetTestBenchChecked()
{
    if (DelegateToPlugin())
        return plugin->isTestBenchChecked;
    else
        return isTestBenchChecked;

}

DeviceInterface::STARTUP SDR::GetStartup()
{
  if (DelegateToPlugin())
    return plugin->startup;
  else
    return startup;
}

int SDR::GetLastDisplayMode()
{
    if (DelegateToPlugin())
      return plugin->lastDisplayMode;
    else
        return lastDisplayMode;
}

void SDR::SetLastDisplayMode(int mode)
{
    if (DelegateToPlugin())
        plugin->lastDisplayMode = mode;
    else
        lastDisplayMode = mode;
}

DeviceInterface::IQORDER SDR::GetIQOrder()
{
    if (DelegateToPlugin())
        return plugin->iqOrder;
    else
        return iqOrder;
}

void SDR::SetIQOrder(DeviceInterface::IQORDER o)
{
    if (DelegateToPlugin())
        plugin->iqOrder = o;
    else
        iqOrder = o;
}

bool SDR::GetIQBalanceEnabled()
{
    if (DelegateToPlugin())
        return plugin->iqBalanceEnable;
    else
        return iqBalanceEnable;
}

bool SDR::GetIQBalanceGain()
{
    if (DelegateToPlugin())
        return plugin->iqBalanceGain;
    else
        return iqBalanceGain;
}

bool SDR::GetIQBalancePhase()
{
    if (DelegateToPlugin())
        return plugin->iqBalancePhase;
    else
        return iqBalancePhase;
}

double SDR::GetFreqToSet()
{
    if (DelegateToPlugin())
        return plugin->GetFreqToSet();
    else
        return freqToSet;
}

double SDR::GetLastFreq()
{
    if (DelegateToPlugin())
        return plugin->GetLastFreq();
    else
        return lastFreq;
}

void SDR::SetLastFreq(double f)
{
    if (DelegateToPlugin())
        plugin->SetLastFreq(f);
    else
        lastFreq = f;
}

int SDR::GetLastMode()
{
    if (DelegateToPlugin())
        return plugin->GetLastMode();
    else
        return lastMode;
}

void SDR::SetLastMode(int mode)
{
    if (DelegateToPlugin())
        plugin->SetLastMode(mode);
    else
        lastMode = mode;
}

QString SDR::GetInputDeviceName()
{
    if (DelegateToPlugin())
        return plugin->inputDeviceName;
    else
        return inputDeviceName;
}

QString SDR::GetOutputDeviceName()
{
    if (DelegateToPlugin())
        return plugin->outputDeviceName;
    else
        return outputDeviceName;
}

double SDR::GetIQGain()
{
    if (DelegateToPlugin())
        return plugin->iqGain;
    else
        return iqGain;
}

void SDR::SetIQGain(double g)
{
    if (DelegateToPlugin())
        plugin->iqGain = g;
    else
        iqGain = g;
}

QSettings *SDR::GetQSettings()
{
    if (DelegateToPlugin())
        return plugin->GetQSettings();
    else
        return qSettings;
}

//Call this in every deviceInterface method to see if we should delegate, and if so make sure deviceNumber is in sync
bool SDR::DelegateToPlugin()
{
    if (plugin == NULL)
        return false;

    plugin->SetDeviceNumber(pluginDeviceNumber);
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

void SDR::StopProducerThread(){}
void SDR::RunProducerThread(){}
void SDR::StopConsumerThread(){}
void SDR::RunConsumerThread(){}

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
