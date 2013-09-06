#ifndef SDRFILE_H
#define SDRFILE_H

#include "sdr.h"
#include "QFile"
#include "nco.h"
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
    bool UsesAudioInput() {return false;}

protected:

    CPX *outBuffer;
    int framesPerBuffer;
    virtual void StopProducerThread();
    virtual void RunProducerThread();
    virtual void StopConsumerThread();
    virtual void RunConsumerThread();

    QString fileName;

    WavFile wavFileRead;
    WavFile wavFileWrite;

    //Testing NCO
    NCO *nco;

    bool copyTest; //True if we're reading from one file and writing to another file for testing
};

#endif // SDRFILE_H
