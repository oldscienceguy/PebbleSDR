#ifndef RTL2832_H
#define RTL2832_H

#include "SDR.h"

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
    bool SetSampleRate(quint32 sampleRate);


private:
    CPX *inBuffer;
    CPX *outBuffer;
    unsigned char* producerBuffer;
    int producerBufferSize;
    int framesPerBuffer;
    quint32 rtlSampleRate;
    quint32 sampleRate; //Pebble rate from settings or rtl option box eventually
    quint16 rtlDecimate;

    bool RTL_I2CRead(quint8 addr, quint8 *buffer, int len);
    int RTL_I2CWrite(quint8 i2c_addr, quint8 *buffer, int len);
    bool RTL_ReadArray(quint8 block, quint16 addr, quint8 *array, quint8 len);
    int RTL_WriteArray(quint8 block, quint16 addr, quint8 *array, quint8 len);
    bool RTL_WriteReg(quint16 block, quint16 address, quint16 value, quint16 length);
    bool DemodWriteReg(quint16 page, quint16 address, quint16 value, quint16 length);
    bool InitTuner();
    bool SetI2CRepeater(bool on);

    bool RTL_Init();
    bool E4000_Init();
    bool E4000_I2CReadByte(quint8 addr, quint8 *byte);
    bool E4000_I2CWriteByte(quint8 addr, quint8 byte);
    bool E4000_I2CWriteArray(quint8 addr, quint8 *array, quint8 len);
    bool E4000_ResetTuner();
    bool E4000_TunerClock();
    bool E4000_Qpeak();
    bool E4000_DCoffloop();
    bool E4000_GainControlinit();
    bool E4000_SetBandwidthHz(unsigned long bandwidthHz);
    bool E4000_IFFilter(int bandwidth);
    bool E4000_SetRfFreqHz(unsigned long RfFreqHz);
    bool E4000_GainManual();
    bool E4000_GainFreq(int freq);
    bool E4000_PLL(int ref_clk, int freq);
    bool E4000_LNAFilter(int Freq);
    bool E4000_FreqBand(int Freq);
    bool E4000_DCoffLUT();
    bool E4000_GainControlAuto();
    bool E4000_Sensitivity(int Freq, int bandwidth);
    bool E4000_Linearity(int Freq, int bandwidth);
    bool E4000_Nominal(int Freq, int bandwidth);

    QSettings *qSettings;
    USBUtil *usb;

    quint32 crystalFreqHz;

    virtual void StopProducerThread();
    virtual void RunProducerThread();
    virtual void StopConsumerThread();
    virtual void RunConsumerThread();


};

#endif // RTL2832_H
