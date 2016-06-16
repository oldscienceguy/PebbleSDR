#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
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
class NCO
{
public:
	NCO(quint32 _sampleRate, quint32 _bufferSize);
	~NCO(void);
	void setFrequency(double f);

	//Modifies _in with a single freq
	void genSingle(CPX *_in, quint32 _numSamples, double _dbGain, bool _mix);

	//Returns next sample for use in continuous loops
	//Quadrature oscillator
	inline void nextSample(CPX &osc) {
		//More efficient code that doesn't recalc expensive sin/cos over and over
		double oscGn;
		//We could make osc complex and use CPX::convolution method with gain, same code
		osc.real(m_lastOsc.real() * m_oscCos - m_lastOsc.im * m_oscSin);
		osc.im = m_lastOsc.im * m_oscCos + m_lastOsc.real() * m_oscSin;
		oscGn = 1.95 - (m_lastOsc.real() * m_lastOsc.real() + m_lastOsc.im * m_lastOsc.im);
		m_lastOsc.real(oscGn * osc.real());
		m_lastOsc.im = oscGn * osc.im;
	}

	//Alternate implementation, much slower than quad oscillator because we recalc sin/cos every sample
	inline void nextSampleAlt(CPX &osc) {
		osc.real(cos(m_oscTime));
		osc.im = sin(m_oscTime);
		m_oscTime += m_oscInc;
		m_oscTime = fmod(m_oscTime, TWOPI);	//Keep bounded

	}

	//Modifies _in
	void genNoise(CPX* _in, quint32 _numSamples, double _dbGain, bool _mix);

	enum SweepType {SINGLE, REPEAT, REPEAT_REVERSE};
	//Must be called before genSweep
	void initSweep(double _sweepStartFreq, double _sweepStopFreq, double _sweepRate,
		double _pulseWidth, double _pulsePeriod, SweepType _sweepType = SINGLE);
	//Modifies _in
	void genSweep(CPX *_in, quint32 _numSamples, double _dbGain, bool _mix);

	//Todo: genFM()
	//Todo: genMorse()
	//Other test modulations?

private:
	quint32 m_sampleRate;
	quint32 m_bufferSize;

	double m_oscInc;
	double m_oscCos;
	double m_oscSin;
	double m_oscTime; //For alternate implementation
	CPX m_lastOsc;

	QMutex m_mutex;

	double m_frequency;

	//For sweep
	bool m_sweepInitialized;
	double m_sweepStartFreq;
	double m_sweepStopFreq;
	double m_sweepFreq; //Frequency being generated
	double m_sweepRate;
	double m_sweepAcc;
	double m_sweepRateInc;
	double m_sweepFreqNorm;
	SweepType m_sweepType;
	bool m_sweepUp;

	double m_sweepPulseWidth;
	double m_sweepPulsePeriod;
	double m_sweepPulseTimer;

};
