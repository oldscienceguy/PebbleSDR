#ifndef RTL2832SDRDEVICE_H
#define RTL2832SDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rtl-sdr.h"
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include "ui_rtl2832sdrdevice.h"

/*
  http://sdr.osmocom.org/trac/wiki/rtl-sdr for latest library
  cd rtl-sdr/
  autoreconf -i
  ./configure
     To enable auto-disconnect of running instances of librtlsdr (used to default to on)
    ./configure --enable-driver-detach

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
    QString GetPluginName(int _devNum = 0);
    QString GetPluginDescription(int _devNum = 0);

    bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer);
    bool Connect();
    bool Disconnect();
    void Start();
    void Stop();

    QSettings *GetQSettings();

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
    bool GetTestBenchChecked();
    quint16 GetNumDevices() {return 2;}

    //Display device option widget in settings dialog
    void SetupOptionUi(QWidget *parent);
    //Called by settings to write device options to ini file
    void WriteOptionUi();

protected:
    void StopProducerThread();
    void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();
signals:
    void reset(); //When no recourse, signal this to restart

private slots:
    void Reset();

    void TCPSocketError(QAbstractSocket::SocketError socketError);
    void TCPSocketConnected();
    void TCPSocketDisconnected();
    void TCPSocketNewData();

    void IPAddressChanged();
    void IPPortChanged();
    void SampleRateChanged(int _index);
    void GainModeChanged(bool _clicked);
    void GainChanged(int _selection);
    void FreqCorrectionChanged(int _correction);

private:
    enum PEBBLE_DEVICES {RTL_USB = 0,RTL_TCP=1};
    //This comes from rtl-sdr.h in library, update if new devices are supported
    enum RTLSDR_TUNERS {
        RTLSDR_TUNER_UNKNOWN = 0,
        RTLSDR_TUNER_E4000,
        RTLSDR_TUNER_FC0012,
        RTLSDR_TUNER_FC0013,
        RTLSDR_TUNER_FC2580,
        RTLSDR_TUNER_R820T,
        RTLSDR_TUNER_R828D
    };

    const quint8 TCP_SET_FREQ = 0x01;
    const quint8 TCP_SET_SAMPLERATE = 0x02;
    const quint8 TCP_SET_GAIN_MODE = 0x03;
    const quint8 TCP_SET_TUNER_GAIN = 0x04;
    const quint8 TCP_SET_FREQ_CORRECTION = 0x05;
    const quint8 TCP_SET_IF_GAIN = 0x06;
    const quint8 TCP_SET_TEST_MODE = 0x07;
    const quint8 TCP_SET_AGC_MODE = 0x08;
    const quint8 TCP_SET_DIRECTS_AMPLING = 0x09;
    const quint8 TCP_SET_OFFSET_TUNING = 0x0a;
    const quint8 TCP_SET_RTL_XTAL = 0x0b;
    const quint8 TCP_SET_TUNER_XTAL = 0x0c;
    const quint8 TCP_SET_TUNER_GAIN_BY_INDEX = 0x0d;

    const quint8 GAIN_MODE_AUTO = 0;
    const quint8 GAIN_MODE_MANUAL = 1;

//Technique rtl-tcp uses to make sure struct is packed on windows and mac
#ifdef _WIN32
#define __attribute__(x)
#pragma pack(push, 1)
#endif
    struct RTL_CMD {
        quint8 cmd;
        quint32 data;
    }__attribute__((packed));
#ifdef _WIN32
#pragma pack(pop)
#endif

    //First bytes from rtl_tcp contain dongle information
    typedef struct { /* structure size must be multiple of 2 bytes */
        char magic[4];
        quint32 tunerType;
        quint32 tunerGainCount;
    } DongleInfo;

    bool SendTcpCmd(quint8 _cmd, quint32 _data);
    bool SetRtlGain(quint16 _mode, quint16 _gain);
    bool SetRtlFrequencyCorrection(qint16 _correction);


    void InitSettings(QString fname);
    ProducerConsumer producerConsumer;

    rtlsdr_dev_t *dev;

    //Needed to determine when it's safe to fetch options for display
    bool connected; //Set after Connect
    bool running; //Set after Start

    double sampleGain; //Factor to normalize output

    CPXBuf *inBuffer;

    qint16 rtlGain; //in 10ths of a db
    quint32 rtlFrequency;
    quint32 rtlSampleRate;
    quint16 rtlDecimate;

    int sampleRates[10]; //Max 10 for testing

    QTcpSocket *rtlTcpSocket;
    QMutex rtlTcpSocketMutex; //Control access to rtltcpSocket across thread

    Ui::RTL2832UI *optionUi;
    QHostAddress rtlServerIP;
    quint32 rtlServerPort;
    quint16 rtlGainMode;
    qint16 rtlFreqencyCorrection; //+ or -

    //Settings for each device
    QSettings *usbSettings;
    QSettings *tcpSettings;

    //Flag to determine if we're received dongle information from rtl_tcp
    bool haveDongleInfo;
    DongleInfo tcpDongleInfo;

    RTLSDR_TUNERS rtlTunerType;
    quint32 rtlTunerGainCount;

    int readBufferSize;
    quint16 numProducerBuffers; //For faster sample rates, may need more producer buffers to handle
    quint16 readBufferIndex; //Used to track whether we have full buffer or not, 0 to readBufferSize-1

};

#endif // RTL2832SDRDEVICE_H
