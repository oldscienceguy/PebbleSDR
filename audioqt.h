#ifndef AUDIOQT_H
#define AUDIOQT_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "audio.h"
#include <QAudioOutput>
#include <QAudioFormat>

class AudioQT : public Audio
{
public:
    AudioQT(Receiver *r, int fpb, Settings *s);
	//Virtual functions
	//We may get input from some other source, if so inputSampleRate = 0
    int StartInput(int inputSampleRate);
    int StartOutput(int outputSampleRate);
	int Stop();
	int Flush();
	int Pause();
	int Restart();
    void SendToOutput(CPX *, int outSamples);
	void ClearCounts();
    QAudioDeviceInfo FindInputDeviceByName(QString name);
    QAudioDeviceInfo FindOutputDeviceByName(QString name);

    static QStringList InputDeviceList();
    static QStringList OutputDeviceList();

private:
    QAudioDeviceInfo qaInputDevice;
    QAudioDeviceInfo qaOutputDevice;
	QAudioOutput*    qaAudioOutput;
	//QIODevice*       qaOutput; // not owned
	QAudioFormat     qaFormat;
	QIODevice *dataSource;

    float *outStreamBuffer;

	void DumpDeviceInfo(QAudioDeviceInfo device);


};

#endif // AUDIOQT_H
