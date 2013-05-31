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
#include "sdrinterface.h"

class SDR_IP : public SDR
{
    Q_OBJECT
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


signals:
    
public slots:

private slots:
    void OnStatus(int status);

private:
    CSdrInterface *m_pSdrInterface;
    CSdrInterface::eStatus m_Status;
    CSdrInterface::eStatus m_LastStatus;
    QHostAddress m_IPAdr;
    quint32 m_Port;
    qint32 m_RadioType;
    qint64 m_CenterFrequency;
    QSettings *qSettings;
    quint32 sampleRate; //Pebble rate from settings or option box eventually
    int framesPerBuffer;

    //Producer/Consumer
    //SDR overrides
    void StopProducerThread();
    void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();

    int msTimeOut;

    static const int numDataBufs = 50; //Producer/Consumer buffers
    unsigned char **producerBuffer; //Array of buffers

    short **dataBuf;
    int nextProducerDataBuf;
    int nextConsumerDataBuf;
    /*
      NumFreeBuffers starts at NUMDATABUFS and is decremented (acquire()) everytime the producer thread has new data.
      If it ever gets to zero, it will block forever and program will hang until consumer thread catches up.
      NumFreeBuffers is incremented (release()) in consumer thread when a buffer has been processed and can be reused.
    */
    QSemaphore *semNumFreeBuffers; //Init to NUMDATABUFS
    QSemaphore *semNumFilledBuffers;
    bool dataBufOverflow;
    void ProcessDataBlocks();


};

#endif // SDR_IP_H
