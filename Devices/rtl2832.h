#ifndef RTL2832_H
#define RTL2832_H

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
    int GetSampleRate(); //Returns current sample rate
    int* GetSampleRates(int &len);  //Returns array of allowable rates

    //bool SetSampleRate(quint32 sampleRate);


private:
    double sampleGain; //Factor to normalize output

    CPXBuf *inBuffer;
    CPXBuf *outBuffer;

    int framesPerBuffer;
    qint16 rtlGain; //in 10ths of a db
    quint32 rtlFrequency;
    quint32 rtlSampleRate;
    quint16 rtlDecimate;

    rtlsdr_dev_t *dev;

    USBUtil *usb;


    virtual void StopProducerThread();
    virtual void RunProducerThread();
    virtual void StopConsumerThread();
    virtual void RunConsumerThread();


};

#endif // RTL2832_H
