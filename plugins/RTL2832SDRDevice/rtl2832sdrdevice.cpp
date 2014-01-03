//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rtl2832sdrdevice.h"


RTL2832SDRDevice::RTL2832SDRDevice()
{
    InitSettings("RTL2832SDR");

    //framesPerBuffer = 2048; //Temp till we pass

}

RTL2832SDRDevice::~RTL2832SDRDevice()
{
    Stop();
    Disconnect();
}

QString RTL2832SDRDevice::GetPluginName()
{
    return "RTL2832";
}

QString RTL2832SDRDevice::GetPluginDescription()
{
    return "RTL2832 Devices";
}

bool RTL2832SDRDevice::Initialize(cbProcessIQData _callback)
{
    ProcessIQData = _callback;
    producerConsumer.Initialize(this,1,framesPerBuffer * sizeof(CPX),0);

    return true;
}

bool RTL2832SDRDevice::Connect()
{
    return false;
}

bool RTL2832SDRDevice::Disconnect()
{
    return false;
}

void RTL2832SDRDevice::Start()
{
    producerConsumer.Start();
}

void RTL2832SDRDevice::Stop()
{
    producerConsumer.Stop();
}

double RTL2832SDRDevice::SetFrequency(double fRequested,double fCurrent)
{
    return fCurrent;
}

void RTL2832SDRDevice::ShowOptions()
{

}

void RTL2832SDRDevice::ReadSettings()
{
}

void RTL2832SDRDevice::WriteSettings()
{
}

double RTL2832SDRDevice::GetStartupFrequency()
{
    return 0;
}

int RTL2832SDRDevice::GetStartupMode()
{
    return lastMode;
}

double RTL2832SDRDevice::GetHighLimit()
{
    return 0;
}

double RTL2832SDRDevice::GetLowLimit()
{
    return 0;
}

double RTL2832SDRDevice::GetGain()
{
    return 1;
}

QString RTL2832SDRDevice::GetDeviceName()
{
    return "RTL2832"; //Add device specifics
}

int RTL2832SDRDevice::GetSampleRate()
{
    return 0;
}

int *RTL2832SDRDevice::GetSampleRates(int &len)
{
    return NULL;
}

bool RTL2832SDRDevice::UsesAudioInput()
{
    return false;
}

void RTL2832SDRDevice::SetupOptionUi(QWidget *parent)
{
    //Nothing to do
}

void RTL2832SDRDevice::StopProducerThread(){}
void RTL2832SDRDevice::RunProducerThread()
{
    producerConsumer.AcquireFreeBuffer();
    //Insert code to put data from device in buffer
    producerConsumer.SupplyProducerBuffer();
    producerConsumer.ReleaseFilledBuffer();

}
void RTL2832SDRDevice::StopConsumerThread(){}
void RTL2832SDRDevice::RunConsumerThread()
{
    producerConsumer.AcquireFilledBuffer();
    CPX *buf = producerConsumer.GetNextConsumerBuffer();
    //Insert code to retrieve data from buffer and pass to receiver for processing
    ProcessIQData(buf,framesPerBuffer);
    producerConsumer.SupplyConsumerBuffer();
    producerConsumer.ReleaseFreeBuffer();
}

//These need to be moved to a DeviceInterface implementation class, equivalent to SDR
//Must be called from derived class constructor to work correctly
void RTL2832SDRDevice::InitSettings(QString fname)
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
