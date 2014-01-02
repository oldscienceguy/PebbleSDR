#ifndef DEVICE_INTERFACES_H
#define DEVICE_INTERFACES_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
 * WIP to develop an extio.dll equivalent for Pebble, leveraging what we learned from digital modem plugins
 * First example will be to move File device  to this
 *
 * We want devices to be as ignorant as possible about caller so there is no tight coupling to Pebble Receiver class.
 * This allows devices to be used in other programs eventually.
 *
 * We want to support the producer/consumer thread model to support high sample rate devices
 *
 */
#include "pebblelib_global.h"
#include <functional>

class CPX;

//using std::placeholders;
//ProcessIQData callback: Call with CPX buffer and number of samples
typedef std::function<void(CPX *, quint16)> cbProcessIQData;

class PEBBLELIBSHARED_EXPORT DeviceInterface
{
public:
    //Interface must be all pure virtual functions
    //Info - return plugin name for menus
    virtual QString GetPluginName() = 0;
    virtual QString GetPluginDescription() = 0;

    virtual bool Initialize(cbProcessIQData _callback) = 0;
    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual void StartData() = 0;
    virtual void StopData() = 0;

    virtual double SetFrequency(double fRequested,double fCurrent) = 0;
    virtual void ShowOptions() = 0;
    virtual void ReadSettings() = 0;
    virtual void WriteSettings() = 0;

    virtual double GetStartupFrequency() = 0;
    virtual int GetStartupMode() = 0;
    virtual double GetHighLimit() = 0;
    virtual double GetLowLimit() = 0;
    virtual double GetGain() = 0;
    virtual QString GetDeviceName() = 0;
    virtual int GetSampleRate() = 0;
    virtual bool UsesAudioInput() = 0;

protected:
    virtual void StopProducerThread() = 0;
    virtual void RunProducerThread() = 0;
    virtual void StopConsumerThread() = 0;
    virtual void RunConsumerThread() = 0;

    cbProcessIQData ProcessIQData;

};

//How best to encode version number in interface
#define DeviceInterface_iid "N1DDY.Pebble.DeviceInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DeviceInterface, DeviceInterface_iid)


#endif // DEVICE_INTERFACES_H
