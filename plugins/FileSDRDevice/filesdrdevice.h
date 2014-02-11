#ifndef FILEDEVICE_H
#define FILEDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "../pebblelib/device_interfaces.h"
#include "producerconsumer.h"
#include "wavfile.h"

class FileSDRDevice : public QObject, public DeviceInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DeviceInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DeviceInterface)

public:
    FileSDRDevice();
    ~FileSDRDevice();

    bool Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer);
    bool Connect();
    bool Disconnect();
    void Start();
    void Stop();

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

	virtual QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);

    //Display device option widget in settings dialog
    void SetupOptionUi(QWidget *parent);

    void InitSettings(QString fname);

protected:
    void producerWorker(cbProducerConsumerEvents _event);
    void consumerWorker(cbProducerConsumerEvents _event);

private:
    ProducerConsumer producerConsumer;
    QString fileName;
    QString recordingPath;

    WavFile wavFileRead;
    WavFile wavFileWrite;
    bool copyTest; //True if we're reading from one file and writing to another file for testing

};



#endif // FILEDEVICE_H
