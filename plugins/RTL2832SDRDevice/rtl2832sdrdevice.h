#ifndef RTL2832SDRDEVICE_H
#define RTL2832SDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rtl-sdr.h"

/*
  http://sdr.osmocom.org/trac/wiki/rtl-sdr for latest library
  cd rtl-sdr/
  autoreconf -i
  ./configure
  make
  sudo make install
  sudo ldconfig
  ./src/rtl_test to verify working
  Libraries are in rtl-sdr/src/.libs

  http://sdr.osmocom.org/trac/wiki/GrOsmoSDR for TCP client examples and utilities
  sudo port install boost  //Macports - Boost is required to build grOsmoSDR

*/

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

    bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer);
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
    ProducerConsumer producerConsumer;

    rtlsdr_dev_t *dev;
    //Needed to determine when it's safe to fetch options for display
    bool connected;

    double sampleGain; //Factor to normalize output

    CPXBuf *inBuffer;

    qint16 rtlGain; //in 10ths of a db
    quint32 rtlFrequency;
    quint32 rtlSampleRate;
    quint16 rtlDecimate;

    int sampleRates[10]; //Max 10 for testing


};

#endif // RTL2832SDRDEVICE_H
