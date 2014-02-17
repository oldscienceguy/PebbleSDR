//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include <QLibrary>
#include "sdr.h"
#include "settings.h"
#include "devices/elektorsdr.h"
#include "devices/sdr_iq.h"
#include "devices/funcube.h"
#include "devices/sdr_ip.h"
#include "soundcard.h"
#include "receiver.h"
#include "QMessageBox"

using namespace std::placeholders;

//Called from plugins.cpp to create an SDR object that delegates to a device plugin
//Necessary while we are in transition from internal to plugins
SDR::SDR(DeviceInterface *_plugin, int _devNum, Receiver *_receiver, Settings *_settings)
{
    plugin = _plugin;
    pluginDeviceNumber = _devNum; //Critical - this is used everywhere to disambiguate multi device plugins
	receiver = _receiver;
	settings = _settings;
	audioInput = NULL;

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
    connected = false;

    //Setup callback for device plugins to use when they have new IQ data
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
	if (DelegateToPlugin()) {
		if (plugin->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ) {
			audioInput = Audio::Factory(receiver,_framesPerBuffer,settings);
		}
		return plugin->Initialize(_callback, _framesPerBuffer);
	}
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
	return fCurrent;
}

void SDR::Start()
{
	if (DelegateToPlugin()) {
		if (plugin->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ) {
			//We handle audio
			audioInput->StartInput(plugin->Get(DeviceInterface::InputDeviceName).toString(),
				plugin->Get(DeviceInterface::DeviceSampleRate).toUInt());

		}
		return plugin->Start();
	}
}

void SDR::Stop()
{
    if (DelegateToPlugin()) {
		if (plugin->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ) {
			if (audioInput != NULL)
				audioInput->Stop();
		}
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

//Settings common to all devices
//Make sure to call SDR::ReadSettings() in any derived class
void SDR::ReadSettings()
{
    if (DelegateToPlugin()) {
        plugin->ReadSettings();
    } else {
		startupType = (STARTUP_TYPE)qSettings->value("StartupType", DEFAULTFREQ).toInt();
		userFrequency = qSettings->value("StartupFreq", 10000000).toDouble();
		inputDeviceName = qSettings->value("InputDeviceName", "").toString();
		outputDeviceName = qSettings->value("OutputDeviceName", "").toString();
		sampleRate = qSettings->value("SampleRate", 48000).toInt();
		iqGain = qSettings->value("IQGain",1).toDouble();
		iqOrder = (IQORDER)qSettings->value("IQOrder", SDR::IQ).toInt();
		iqBalanceGain = qSettings->value("IQBalanceGain",1).toDouble();
		iqBalancePhase = qSettings->value("IQBalancePhase",0).toDouble();
		iqBalanceEnable = qSettings->value("IQBalanceEnable",false).toBool();
		lastFreq = qSettings->value("LastFreq", 10000000).toDouble();
		lastDemodMode = qSettings->value("LastDemodMode",0).toInt();
		lastSpectrumMode = qSettings->value("LastSpectrumMode",0).toInt();
    }
}
//Make sure to call SDR::WriteSettings() in any derived class
void SDR::WriteSettings()
{
    if (DelegateToPlugin()) {
        plugin->WriteSettings();
    } else {
        //Plugins are responsible for these, but if no plugin, write them here
		qSettings->setValue("StartupType",startupType);
		qSettings->setValue("StartupFreq",userFrequency);
		qSettings->setValue("InputDeviceName", inputDeviceName);
		qSettings->setValue("OutputDeviceName", outputDeviceName);
		qSettings->setValue("SampleRate",sampleRate);
		qSettings->setValue("IQGain",iqGain);
		qSettings->setValue("IQOrder", iqOrder);
		qSettings->setValue("IQBalanceGain", iqBalanceGain);
		qSettings->setValue("IQBalancePhase", iqBalancePhase);
		qSettings->setValue("IQBalanceEnable", iqBalanceEnable);
		qSettings->setValue("LastFreq",lastFreq);
		qSettings->setValue("LastDemodMode",lastDemodMode);
		qSettings->setValue("LastSpectrumMode",lastSpectrumMode);
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

//Call this in every deviceInterface method to see if we should delegate, and if so make sure deviceNumber is in sync
bool SDR::DelegateToPlugin()
{
    if (plugin == NULL)
        return false;
	//Plugins with multiple device support are still only 1 object
	//So we have to set device number every time we use it
	//Should be better place to do it, but this is defensive
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
		case DeviceType:
			return UsesAudioInput();
			break;
		case DeviceSampleRates:
			return GetSampleRates();
			break;
		case HighFrequency:
			return GetHighLimit();
			break;
		case LowFrequency:
			return GetLowLimit();
			break;
		case StartupDemodMode:
			return GetStartupMode();
			break;
		case StartupFrequency:
			return GetStartupFrequency();
			break;
		case UserFrequency:
			//If freq is outside of mode we are in return default
			if (userFrequency > GetHighLimit() || userFrequency < GetLowLimit())
				return GetStartupFrequency();
			else
				return userFrequency;
			break;
		case DeviceSampleRate:
			return sampleRate;
			break;
		case DeviceFrequency:
			return deviceFrequency;
			break;
		case InputDeviceName:
			return inputDeviceName;
			break;
		case OutputDeviceName:
			return outputDeviceName;
			break;
		case IQGain:
			return iqGain;
			break;
		case StartupType:
			return startupType;
			break;
		case LastDemodMode:
			return lastDemodMode;
			break;
		case LastSpectrumMode:
			return lastSpectrumMode;
			break;
		case LastFrequency:
			//If freq is outside of mode we are in return default
			if (lastFreq > Get(DeviceInterface::HighFrequency).toDouble() || lastFreq < Get(DeviceInterface::LowFrequency).toDouble())
				return Get(DeviceInterface::StartupFrequency).toDouble();
			else
				return lastFreq;
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
		case IQOrder:
			return iqOrder;
			break;
		default:
			Q_UNREACHABLE(); //We have to handle it all, no impl in interface
			break;
	}
	return QVariant();
}

bool SDR::Set(STANDARD_KEYS _key, QVariant _value, quint16 _option) {
	if (DelegateToPlugin())
		return plugin->Set(_key, _value, _option);

	switch (_key) {
		case DeviceFrequency: {
			double fRequested = _value.toDouble();
			deviceFrequency = SetFrequency(fRequested,deviceFrequency);
			lastFreq = deviceFrequency;
			break;
		}
		case DeviceSampleRate:
			sampleRate = _value.toInt();
			break;
		case InputDeviceName:
			inputDeviceName = _value.toString();
			break;
		case OutputDeviceName:
			outputDeviceName = _value.toString();
			break;
		case IQGain:
			iqGain = _value.toDouble();
			break;
		case StartupType:
			startupType = (STARTUP_TYPE)_value.toInt();
			break;
		case LastDemodMode:
			lastDemodMode = _value.toInt();
			break;
		case LastSpectrumMode:
			lastSpectrumMode = _value.toInt();
			break;
		case LastFrequency:
			lastFreq = _value.toDouble();
			break;
		case UserFrequency:
			userFrequency = _value.toDouble();
			break;
		case IQOrder:
			iqOrder = (IQORDER)_value.toInt();
			break;
		case IQBalanceEnabled:
			iqBalanceEnable = _value.toBool();
			break;
		case IQBalanceGain:
			iqBalanceGain = _value.toDouble();
			break;
		case IQBalancePhase:
			iqBalancePhase = _value.toDouble();
			break;
		default:
			Q_UNREACHABLE(); //We have to handle it all, no impl in interface
	}
	return true;
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
