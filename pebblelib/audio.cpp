//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "audio.h"
#include "soundcard.h"
#include "audioqt.h"
#include "QDebug"

bool Audio::useQtAudio = false;

Audio::Audio(QObject *parent) : QObject(parent)
{
}

Audio::~Audio()
{
}

Audio *Audio::Factory(cbProcessIQData cb, int framesPerBuffer)
{
	if (Audio::useQtAudio) {
		qDebug()<<"Using QTAudio";
		return new AudioQT(cb, framesPerBuffer);
	} else {
		qDebug()<<"Using PortAudio";
		return new SoundCard(cb, framesPerBuffer);
	}

}


QStringList Audio::InputDeviceList()
{
    if (Audio::useQtAudio){
        return AudioQT::InputDeviceList();
    } else {
        return SoundCard::InputDeviceList();
    }

}
QStringList Audio::OutputDeviceList()
{
    if (Audio::useQtAudio)
        return AudioQT::OutputDeviceList();
    else
        return SoundCard::OutputDeviceList();
}
