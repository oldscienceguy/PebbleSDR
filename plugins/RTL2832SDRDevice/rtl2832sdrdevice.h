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
#include "deviceinterfacebase.h"
#include "producerconsumer.h"
#include "cpx.h"

class RTL2832SDRDevice: public QObject, public DeviceInterfaceBase
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DeviceInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DeviceInterface)

public:
#define K_RTLSampleRate DeviceInterface::CustomKey1
#define K_RTLTunerGainMode DeviceInterface::CustomKey2
#define K_RTLTunerGain DeviceInterface::CustomKey3
#define K_RTLAgcMode DeviceInterface::CustomKey4
#define K_RTLSampleMode DeviceInterface::CustomKey5
#define K_RTLOffsetMode DeviceInterface::CustomKey6

    RTL2832SDRDevice();
    ~RTL2832SDRDevice();

    //DeviceInterface abstract methods that must be implemented
    bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer);
    bool Connect();
    bool Disconnect();
    void Start();
    void Stop();

    void ReadSettings();
    void WriteSettings();

    double GetStartupFrequency();
    double GetHighLimit();
    double GetLowLimit();

	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);

    //Display device option widget in settings dialog
    void SetupOptionUi(QWidget *parent);

protected:
    void producerWorker(cbProducerConsumerEvents _event);
    void consumerWorker(cbProducerConsumerEvents _event);

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
    void TunerGainChanged(int _selection);
    //void IFGainChanged(int _selection);
    void FreqCorrectionChanged(int _correction);
    void SamplingModeChanged(int _samplingMode);
    void AgcModeChanged(bool _selected);
    void OffsetModeChanged(bool _selected);

private:
    enum PEBBLE_DEVICES {RTL_USB = 0,RTL_TCP=1};
    enum SAMPLING_MODES {NORMAL=0, DIRECT_I=1, DIRECT_Q=2};

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
    const quint8 TCP_SET_DIRECT_SAMPLING = 0x09;
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
    bool SetRtlSampleRate(quint64 _sampleRate);
    bool GetRtlValidTunerGains(); //Tuner type must be known before this is called
    bool SetRtlTunerGain(quint16 _gain);
    bool SetRtlTunerMode(quint16 _mode);
    bool SetRtlIfGain(quint16 _stage, quint16 _gain); //Not used
    bool SetRtlFrequencyCorrection(qint16 _correction);
    bool SetRtlSampleMode(SAMPLING_MODES _sampleMode);
    bool SetRtlAgcMode(bool _on);
    bool SetRtlOffsetMode(bool _on);

    void InitSettings(QString fname);
    ProducerConsumer producerConsumer;

    rtlsdr_dev_t *dev;

    //Needed to determine when it's safe to fetch options for display
    bool running; //Set after Start

    double sampleGain; //Factor to normalize output

    SAMPLING_MODES rtlSampleMode;

    CPXBuf *inBuffer;

    RTLSDR_TUNERS rtlTunerType;
    int rtlTunerGainCount;
    int rtlTunerGains[50];
    quint16 rtlTunerGainMode;
    qint16 rtlTunerGain; //in 10ths of a db
    //qint16 rtlIfGain; //Not used
    quint32 rtlSampleRate;
    quint16 rtlDecimate;

    QTcpSocket *rtlTcpSocket;
    QMutex rtlTcpSocketMutex; //Control access to rtltcpSocket across thread

    Ui::RTL2832UI *optionUi;
    QHostAddress rtlServerIP;
    quint32 rtlServerPort;
    qint16 rtlFreqencyCorrection; //+ or -
    bool rtlAgcMode;
    bool rtlOffsetMode;

    //Settings for each device
    QSettings *usbSettings;
    QSettings *tcpSettings;

    //Flag to determine if we're received dongle information from rtl_tcp
    bool haveDongleInfo;
    DongleInfo tcpDongleInfo;


    quint16 readBufferIndex; //Used to track whether we have full buffer or not, 0 to readBufferSize-1

    //These are used in the TCP worker thread and created in that thread to prevent QTcpSocket errors
    QTcpSocket *tcpThreadSocket;
    char *producerFreeBufPtr;
};

#endif // RTL2832SDRDEVICE_H
