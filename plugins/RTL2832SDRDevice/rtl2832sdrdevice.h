#ifndef RTL2832SDRDEVICE_H
#define RTL2832SDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "../pebblelib/device_interfaces.h"
#include "producerconsumer.h"
#include "cpx.h"

class RTL2832SDRDevice: public QObject, public DeviceInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DeviceInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DeviceInterface)

public:
    RTL2832SDRDevice();
    ~RTL2832SDRDevice();

    //DeviceInterface abstract methods that must be implemented
    QString GetPluginName();
    QString GetPluginDescription();

    bool Initialize(cbProcessIQData _callback);
    bool Connect();
    bool Disconnect();
    void Start();
    void Stop();

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
    int* GetSampleRates(int &len); //Returns array of allowable rates and length of array as ref
    bool UsesAudioInput();

    //Display device option widget in settings dialog
    void SetupOptionUi(QWidget *parent);

protected:
    void StopProducerThread();
    void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();

private:
    void InitSettings(QString fname);
    //Things every device needs, move to interface
    int framesPerBuffer;
    ProducerConsumer producerConsumer;


};

#endif // RTL2832SDRDEVICE_H
