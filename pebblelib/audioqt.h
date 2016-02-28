#ifndef AUDIOQT_H
#define AUDIOQT_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "audio.h"
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>
#include "producerconsumer.h"

class AudioQT : public Audio
{
	Q_OBJECT
public:
	AudioQT(cbAudioProducer cb, int fpb);
    ~AudioQT();

	//Virtual functions
	//We may get input from some other source, if so inputSampleRate = 0
    int StartInput(QString inputDeviceName, int inputSampleRate);
    int StartOutput(QString outputDeviceName, int outputSampleRate);
	int Stop();
	int Flush();
	int Pause();
	int Restart();
    void SendToOutput(CPX *, int outSamples, float gain = 1.0, bool mute = false);
	void ClearCounts();
    QAudioDeviceInfo FindInputDeviceByName(QString name);
    QAudioDeviceInfo FindOutputDeviceByName(QString name);

    static QStringList InputDeviceList();
    static QStringList OutputDeviceList();

public slots:
    void ProcessInputData();

private:
    QAudioDeviceInfo qaInputDevice;
    QAudioDeviceInfo qaOutputDevice;
    QAudioInput *qaAudioInput;
    QAudioOutput *qaAudioOutput;
	//QIODevice*       qaOutput; // not owned
	QAudioFormat     qaFormat;
    QIODevice *outputDataSource;
    QIODevice *inputDataSource;

    float *inStreamBuffer;
    float *outStreamBuffer;
    CPX *cpxOutBuffer;

	QMutex mutex;

	void DumpDeviceInfo(QAudioDeviceInfo device);

	ProducerConsumer producerConsumer;
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);


};

#endif // AUDIOQT_H
