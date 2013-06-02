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
#include "sdr-ip/sdrinterface.h"


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

    int GetSampleRate();
    int* GetSampleRates(int &len);  //Returns array of allowable rates

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
    qint32 m_RfGain;

    QSettings *qSettings;
    quint32 sampleRate; //Pebble rate from settings or option box eventually
    int framesPerBuffer;

    //Producer/Consumer
    //SDR overrides
    //We use CNetio thread as our producer thread and just provide an enqueue method for it to call
    void PutInProducerQ(CPX *cpxBuf, quint32 numCpx);
    //void StopProducerThread();
    //void RunProducerThread();
    void StopConsumerThread();
    void RunConsumerThread();

    int msTimeOut;

    static const int numDataBufs = 50; //Producer/Consumer buffers

    CPX *outBuffer;
    CPX **dataBuf;  //Array of CPX arrays
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
    quint32 producerOverflowCount; //Number of times we'ver overflowed producer, use to test 1.8m sampleRate
    void ProcessDataBlocks();


};

#endif // SDR_IP_H
