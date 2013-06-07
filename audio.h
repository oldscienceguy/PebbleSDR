#ifndef AUDIO_H
#define AUDIO_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include <QObject>
#include <QstringList>
#include "cpx.h"

class Settings;
class Receiver;

class Audio : public QObject
{
    Q_OBJECT
public:
    explicit Audio(QObject *parent = 0);
    virtual int StartInput(int inputSampleRate)=0;
    virtual int StartOutput(int outputSampleRate)=0;
	virtual int Stop()=0;
	virtual int Flush()=0;
	virtual int Pause()=0;
	virtual int Restart()=0;
    virtual void SendToOutput(CPX *_cpxBuf,int _numSamples)=0;
	virtual void ClearCounts()=0;

    //Creates either PortAudio or QTAudio device
    static Audio *Factory(Receiver *rcv, int framesPerBuffer,Settings *settings);

    //Returns a list of input devices for settings to chose from
    static QStringList InputDeviceList();
    static QStringList OutputDeviceList();

	static int DefaultOutputDevice();
	static int DefaultInputDevice();

	double inBufferUnderflowCount;
	double inBufferOverflowCount;
	double outBufferUnderflowCount;
	double outBufferOverflowCount;

	//Utility functions to convert samples
	float Int2Float(int sample) {return sample / 32767;}
	float UInt2Float(int sample) {return sample / 65535;}
	qint16 Float2Int(float sample) {return sample * 32767;}
	quint16 Float2UInt(float sample) {return sample * 65535;}

    static bool useQtAudio;

signals:

public slots:

protected:
	Settings *settings;
	Receiver *receiver;
	int inputSampleRate;
	int outputSampleRate;
	int framesPerBuffer; //#samples in each callback

};

#endif // AUDIO_H
