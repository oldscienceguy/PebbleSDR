#ifndef RTL2832_H
#define RTL2832_H

#include "QSemaphore"
#include "SDR.h"
#include "rtl-sdr/rtl-sdr.h"

class RTL2832 : public SDR
{
public:
    RTL2832(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
    ~RTL2832(void);

    bool Connect();
    bool Disconnect();
    double SetFrequency(double fRequested,double fCurrent);
    void ShowOptions();
    void Start();
    void Stop();
    void ReadSettings();
    void WriteSettings();

    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();
    int GetSampleRate();
    //bool SetSampleRate(quint32 sampleRate);


private:
    double sampleGain; //Factor to normalize output

    CPX *inBuffer;
    CPX *outBuffer;

    int numDataBufs; //Producer/Consumer buffers
    unsigned char **producerBuffer; //Array of buffers
    int producerBufferSize;
    int nextProducerDataBuf;
    int nextConsumerDataBuf;
    /*
      NumFreeBuffers starts at NUMDATABUFS and is decremented (acquire()) everytime the producer thread has new data.
      If it ever gets to zero, it will block forever and program will hang until consumer thread catches up.
      NumFreeBuffers is incremented (release()) in consumer thread when a buffer has been processed and can be reused.


    */
    QSemaphore *semNumFreeBuffers; //Init to NUMDATABUFS
    QSemaphore *semNumFilledBuffers;


    int framesPerBuffer;
    quint16 rtlGain; //in 10ths of a db
    quint32 rtlFrequency;
    quint32 rtlSampleRate;
    quint32 sampleRate; //Pebble rate from settings or rtl option box eventually
    quint16 rtlDecimate;

    rtlsdr_dev_t *dev;

    QSettings *qSettings;
    USBUtil *usb;


    virtual void StopProducerThread();
    virtual void RunProducerThread();
    virtual void StopConsumerThread();
    virtual void RunConsumerThread();


};

#endif // RTL2832_H
