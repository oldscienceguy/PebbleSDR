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
	void setFrequency(double f);
	//Fills out buffer with samples
	CPX * genSamples();

	//Returns next sample for use in continuous loops
	inline void nextSample(CPX &osc) {
		//More efficient code that doesn't recalc expensive sin/cos over and over
		double oscGn;
		//We could make osc complex and use CPX::convolution method with gain, same code
		osc.re = lastOsc.re * oscCos - lastOsc.im * oscSin;
		osc.im = lastOsc.im * oscCos + lastOsc.re * oscSin;
		oscGn = 1.95 - (lastOsc.re * lastOsc.re + lastOsc.im * lastOsc.im);
		lastOsc.re = oscGn * osc.re;
		lastOsc.im = oscGn * osc.im;
	}

private:
	double oscInc;
	double oscCos;
	double oscSin;
	CPX lastOsc;

	QMutex mutex;

	double frequency;

};
