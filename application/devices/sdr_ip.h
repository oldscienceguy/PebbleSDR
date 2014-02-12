#ifndef SDR_IP_H
#define SDR_IP_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include "gpl.h"
#include <QThread>
#include <QMutex>
#include <QSettings>
#include <QtNetwork>
#include "sdr.h"
#include "cpx.h"
#include "sdr-ip/sdrinterface.h"
#include "ui_sdr-ip.h"


class SDR_IP : public SDR
{
    Q_OBJECT
    friend class CSdrInterface; //Allows callback to our private PutInQ()
public:
    SDR_IP(Receiver *_receiver, SDRDEVICE dev, Settings *_settings);
    ~SDR_IP();

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
    void SetupOptionUi(QWidget *parent);
    bool UsesAudioInput() {return false;}

    int GetSampleRate();
	QStringList GetSampleRates();  //Returns array of allowable rates

    void ReadSettings();
    void WriteSettings();

signals:
    
public slots:

private slots:
    void OnStatus(int status);
    void OnNewInfo();

private:
    CSdrInterface *m_pSdrInterface;
    CSdrInterface::eStatus m_Status;
    CSdrInterface::eStatus m_LastStatus;
    QHostAddress m_IPAdr;
    quint32 m_Port;
    qint32 m_RadioType;
    qint64 m_CenterFrequency;
    qint32 m_RfGain;

    int framesPerBuffer;

    Ui::SDRIP *optionUi;

    //Producer/Consumer
    //SDR overrides
    //We use CNetio thread as our producer thread and just provide an enqueue method for it to call
    void PutInProducerQ(CPX *cpxBuf, quint32 numCpx);
    //void StopProducerThread();
    //void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();

    int msTimeOut;

    CPX *outBuffer;

    quint32 producerOverflowCount; //Number of times we'ver overflowed producer, use to test 1.8m sampleRate
    void ProcessDataBlocks();


};

#endif // SDR_IP_H
