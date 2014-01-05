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
#include <QtCore>
#include "pebblelib_global.h"
#include <functional>

class CPX;

//using std::placeholders;
//ProcessIQData callback: Call with CPX buffer and number of samples
typedef std::function<void(CPX *, quint16)> cbProcessIQData;

class PEBBLELIBSHARED_EXPORT DeviceInterface
{
friend class SDR; //Temp while we're transitioning from internal to plugins to sdr has access to protected

public:
    typedef enum IQORDER {IQ,QI,IONLY,QONLY} IQORDER;
    typedef enum STARTUP {SETFREQ = 0, LASTFREQ, DEFAULTFREQ} STARTUP;
    enum SDRDEVICE {SR_LITE=1, SR_V9, SR_ENSEMBLE, SR_ENSEMBLE_2M,
        SR_ENSEMBLE_4M, SR_ENSEMBLE_6M, SR_ENSEMBLE_LF,
        ELEKTOR, ELEKTOR_PA, SDR_IQ_USB,
        HPSDR_USB, HPSDR_TCP, SPARE1, FUNCUBE,
        NOSDR, FILE, DVB_T, FUNCUBE_PLUS, SDR_IP_TCP, FiFi};

    DeviceInterface() {};
    virtual ~DeviceInterface() {};
    //Interface must be all pure virtual functions
    //Info - return plugin name for menus
    virtual QString GetPluginName() = 0;
    virtual QString GetPluginDescription() = 0;

    virtual bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer) = 0;
    virtual bool Connect() = 0;
    virtual bool Disconnect() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

    virtual double SetFrequency(double fRequested,double fCurrent) = 0;
    //Display device option widget in settings dialog
    virtual void SetupOptionUi(QWidget *parent) = 0;

    //Device doesn't have to implement this.  Called by receiver to bring up SDR options dialog
    virtual void ShowSdrOptions(bool b) {b=true;}

    virtual void ReadSettings() = 0;
    virtual void WriteSettings() = 0;

    virtual double GetStartupFrequency() = 0;
    virtual int GetStartupMode() = 0;
    virtual double GetHighLimit() = 0;
    virtual double GetLowLimit() = 0;
    virtual double GetGain() = 0;
    virtual QString GetDeviceName() = 0;
    virtual int GetSampleRate() = 0;
    virtual int* GetSampleRates(int &len) = 0; //Returns array of allowable rates and length of array as ref
    virtual bool UsesAudioInput() = 0;
    virtual bool GetTestBenchChecked() {return isTestBenchChecked;}


    virtual void StopProducerThread() = 0;
    virtual void RunProducerThread() = 0;
    virtual void StopConsumerThread() = 0;
    virtual void RunConsumerThread() = 0;

    cbProcessIQData ProcessIQData;

    virtual int GetLastDisplayMode() {return lastDisplayMode;}
    virtual void SetLastDisplayMode(int mode) {lastDisplayMode = mode;}

    virtual IQORDER GetIQOrder() {return iqOrder;}
    virtual void SetIQOrder(IQORDER o) {iqOrder = o;}
    virtual bool GetIQBalanceEnabled() {return iqBalanceEnable;}
    virtual bool GetIQBalanceGain() {return iqBalanceGain;}
    virtual bool GetIQBalancePhase() {return iqBalancePhase;}

    virtual double GetFreqToSet() {return freqToSet;}
    virtual double GetLastFreq() {return lastFreq;}
    virtual void SetLastFreq(double f) {lastFreq = f;}

    virtual int GetLastMode() {return lastMode;}
    virtual void SetLastMode(int mode) {lastMode = mode;}

    virtual STARTUP GetStartup() {return startup;}
    virtual QString GetInputDeviceName() {return inputDeviceName;}
    virtual QString GetOutputDeviceName() {return outputDeviceName;}

    virtual double GetIQGain() {return iqGain;}
    virtual void SetIQGain(double g) {iqGain = g;}

    //These only apply to old internal
    SDRDEVICE GetSDRDevice() {return sdrDevice;}
    void SetSDRDevice(SDRDEVICE dev) {sdrDevice = dev;}

    virtual QSettings *GetQSettings() {return qSettings;}

protected:
    //Todo: Flag which of these is just a convenience for Pebble, vs required for the interface
    quint16 framesPerBuffer;

    int lastDisplayMode; //Spectrum, waterfall, etc
    STARTUP startup;
    double freqToSet;
    QString inputDeviceName;
    QString outputDeviceName;
    int sampleRate;

    double iqGain; //Normalize device so incoming IQ levels are consistent
    IQORDER iqOrder;
    //Image rejection (iqbalance) factors for this device
    double iqBalanceGain;
    double iqBalancePhase;
    bool iqBalanceEnable;

    double lastFreq;
    int lastMode;

    //Device needs to manage QSettings since it knows its own settings file name
    QSettings *qSettings;

    bool isTestBenchChecked;

    SDRDEVICE sdrDevice;

};

//How best to encode version number in interface
#define DeviceInterface_iid "N1DDY.Pebble.DeviceInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DeviceInterface, DeviceInterface_iid)


#endif // DEVICE_INTERFACES_H
