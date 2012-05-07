#include "rtl2832.h"

RTL2832::RTL2832 (Receiver *_receiver,SDRDEVICE dev,Settings *_settings): SDR(_receiver, dev,_settings)
{
}

RTL2832::~RTL2832(void)
{

}

bool RTL2832::Connect()
{
}

bool RTL2832::Disconnect()
{
}

double RTL2832::SetFrequency(double fRequested, double fCurrent)
{
}

void RTL2832::ShowOptions()
{
}

void RTL2832::Start()
{
}

void RTL2832::Stop()
{
}

void RTL2832::ReadSettings()
{
}

void RTL2832::WriteSettings()
{
}

double RTL2832::GetStartupFrequency()
{
}

int RTL2832::GetStartupMode()
{
}

double RTL2832::GetHighLimit()
{
}

double RTL2832::GetLowLimit()
{
}

double RTL2832::GetGain()
{
}

QString RTL2832::GetDeviceName()
{
}

int RTL2832::GetSampleRate()
{
}
