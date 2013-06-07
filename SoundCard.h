#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "audio.h"
#include "../portaudio/include/portaudio.h"
#include "settings.h"

using namespace std;

class Receiver; //Forward declaration since soundcard and receiver are dependent on each other

class SoundCard:public Audio
{
public:
    SoundCard(Receiver *r,int fpb, Settings *s);
	~SoundCard(void);
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

    //Return device index for matching device
    int FindDeviceByName(QString name, bool inputDevice);

	//Returns a list of input devices for settings to chose from
    static QStringList InputDeviceList();
    static QStringList OutputDeviceList();
    static int DefaultOutputDevice();
	static int DefaultInputDevice();


private:

    static QStringList DeviceList(bool input);

	//Input may come from another source, so we manage these streams separately
	PaStream *inStream;
	PaStream *outStream;
	float *outStreamBuffer; 
	PaSampleFormat sampleFormat;
	PaError error;
	static int streamCallback(
		const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData );
	CPX *inBuffer;
	CPX *outBuffer;

};
