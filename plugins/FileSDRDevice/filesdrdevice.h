#ifndef FILEDEVICE_H
#define FILEDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "../pebblelib/device_interfaces.h"

class FileSDRDevice : public QObject, public DeviceInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DeviceInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DeviceInterface)

public:
    FileSDRDevice();
    bool Initialize(cbProcessIQData _callback);
    bool Connect();
    bool Disconnect();
    void StartData();
    void StopData();

    double SetFrequency(double fRequested,double fCurrent);
    void ShowOptions();
    void ReadSettings();
    void WriteSettings();

    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();
    int GetSampleRate();
    bool UsesAudioInput();

protected:
    void StopProducerThread();
    void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();

};

#endif // FILEDEVICE_H
