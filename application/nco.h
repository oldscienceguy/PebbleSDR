#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include <QMutex>
/*
Numerically Controlled Oscillator
Used in balanced mixer, PLLs, etc
Important Trig Identities
sin(a) = sqrt( 1 - cos(a)^2)
tan(a) = sin(a) / cos(a)
//Frequency mixing is the same as complex mult where re=cos and im=sin
sin(a+b) = sin(a) * cos(b) + cos(a) * sin(b)
cos(a+b) = cos(a) * cos(b) - sin(a) * sin(b)
http://en.wikipedia.org/wiki/List_of_trigonometric_identities
*/
class NCO : public ProcessStep
{
public:
	NCO(quint32 _sampleRate, quint32 _bufferSize);
	~NCO(void);
	void SetFrequency(double f);
	void SetStartingPhase (double p);
	//Fills out buffer with samples
	CPX * GenSamples();
	//Returns next sample for use in continuous loops
	CPX NextSample();

private:
	//Phase is always an increment of step
	double nextAngularFreq;
	double angularIncrement; //Angular frequency in radians
	QMutex mutex;

	double frequency;

};
