#ifndef SDRFILE_H
#define SDRFILE_H

#include "SDR.h"
#include "QFile"
#include "NCO.h"
#include "wavfile.h"


class SDRFile : public SDR
{
public:
    SDRFile(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
    ~SDRFile(void);

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

protected:
    QSettings *qSettings;

    CPX *inBuffer;
    CPX *outBuffer;
    int framesPerBuffer;
    virtual void StopProducerThread();
    virtual void RunProducerThread();
    virtual void StopConsumerThread();
    virtual void RunConsumerThread();

    QString fileName;

    WavFile wavFile;

    //Testing NCO
    NCO *nco;


};

#endif // SDRFILE_H
