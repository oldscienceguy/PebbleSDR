#ifndef SDRFILE_H
#define SDRFILE_H

#include "SDR.h"
class SDRFile : public SDR
{
public:
    SDRFile(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
    ~SDRFile(void);

    bool Connect();
    bool Disconnect();
    double SetFrequency(double fRequested,double fCurrent);
    void ShowOptions();
    void Start();
    void Stop();
    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();

};

#endif // SDRFILE_H
