#ifndef RTL2832_H
#define RTL2832_H

#include "SDR.h"

class RTL2832 : public SDR
{
public:
    RTL2832(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
    ~RTL2832(void);

    bool Connect();
    bool Disconnect();
    double SetFrequency(double fRequested,double fCurrent);
    void ShowOptions();
    void Start();
    void Stop();
    void ReadSettings();
    void WriteSettings();

    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();
    int GetSampleRate();

private:

};

#endif // RTL2832_H
