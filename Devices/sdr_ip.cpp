#include "sdr_ip.h"


SDR_IP::SDR_IP(Receiver *_receiver, SDR::SDRDEVICE dev, Settings *_settings)
    :SDR(_receiver, dev, _settings)
{
}

bool SDR_IP::Connect()
{
    return false;
}

bool SDR_IP::Disconnect()
{
    return false;
}

double SDR_IP::SetFrequency(double fRequested, double fCurrent)
{
    return fRequested;
}

void SDR_IP::ShowOptions()
{
}

void SDR_IP::Start()
{
}

void SDR_IP::Stop()
{
}

double SDR_IP::GetStartupFrequency()
{
    return 10000000;
}

int SDR_IP::GetStartupMode()
{
    return 0;
}

double SDR_IP::GetHighLimit()
{
    return 30000000;
}

double SDR_IP::GetLowLimit()
{
    return 500000;
}

double SDR_IP::GetGain()
{
    return 1;
}

QString SDR_IP::GetDeviceName()
{
    return "SDR_IP";
}
