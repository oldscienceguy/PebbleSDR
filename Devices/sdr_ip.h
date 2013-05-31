#ifndef SDR_IP_H
#define SDR_IP_H

#include "gpl.h"
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QSettings>
#include <QtNetwork>
#include "sdr.h"
#include "cpx.h"

class SDR_IP : public SDR
{
    Q_OBJECT
public:
    SDR_IP(Receiver *_receiver, SDRDEVICE dev, Settings *_settings);

    //SDR class overrides
    bool Connect();
    bool Disconnect();
    double SetFrequency(double fRequested, double fCurrent);
    void ShowOptions();

    void Start(); //Start stop thread
    void Stop();

    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();


signals:
    
public slots:
    
};

#endif // SDR_IP_H
