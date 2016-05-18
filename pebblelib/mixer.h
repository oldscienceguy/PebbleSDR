#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
/*
Simple DSP Mixer
*/
//#include "nco.h"

class Mixer
{
public:
	Mixer(quint32 _sampleRate, quint32 _bufferSize);
	~Mixer(void);
	CPX * processBlock(CPX *in);
	void setFrequency(double f);
private:
	quint32 m_sampleRate;
	quint32 m_numSamples;
	CPX* m_out;

	double m_frequency;
	double m_gain; //Adjust for mixer loss so we keep constant gain
	//NCO *m_nco;
	//CPX *m_mix;

	//Testing inline osc instead of calling NCO
	double m_oscInc;
	double m_oscCos;
	double m_oscSin;
	CPX m_lastOsc;

	double m_oscTime; //For alternate implementation

};
