//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "filesdrdevice.h"


FileSDRDevice::FileSDRDevice()
{
}

FileSDRDevice::~FileSDRDevice()
{

}

QString FileSDRDevice::GetPluginName()
{
    return "WAV File";
}

QString FileSDRDevice::GetPluginDescription()
{
    return "Plays back I/Q WAV file";
}

bool FileSDRDevice::Initialize(cbProcessIQData _callback)
{
    ProcessIQData = _callback;
    return true;
}

bool FileSDRDevice::Connect()
{
    return false;
}

bool FileSDRDevice::Disconnect()
{
    return false;
}

void FileSDRDevice::Start()
{

}

void FileSDRDevice::Stop()
{

}

double FileSDRDevice::SetFrequency(double fRequested,double fCurrent)
{
    return fCurrent;
}

void FileSDRDevice::ShowOptions()
{

}

void FileSDRDevice::ReadSettings()
{

}

void FileSDRDevice::WriteSettings()
{

}

double FileSDRDevice::GetStartupFrequency()
{
    return 10000;
}

int FileSDRDevice::GetStartupMode()
{
    return 0;
}

double FileSDRDevice::GetHighLimit()
{
    return 30000000;
}

double FileSDRDevice::GetLowLimit()
{
    return 150000;
}

double FileSDRDevice::GetGain()
{
    return 1.0;
}

QString FileSDRDevice::GetDeviceName()
{
    return "Device";
}

int FileSDRDevice::GetSampleRate()
{
    return 0;
}

bool FileSDRDevice::UsesAudioInput()
{
    return false;
}

void FileSDRDevice::StopProducerThread()
{

}

void FileSDRDevice::RunProducerThread()
{

}

void FileSDRDevice::StopConsumerThread()
{

}

void FileSDRDevice::RunConsumerThread()
{

}
