#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
Simple DSP Mixer
*/
//#include "nco.h"
#include "processstep.h"

class Mixer:public ProcessStep
{
public:
	Mixer(quint32 _sampleRate, quint32 _bufferSize);
	~Mixer(void);
	CPX * ProcessBlock(CPX *in);
	void SetFrequency(double f);
private:
	double frequency;
	double gain; //Adjust for mixer loss so we keep constant gain
	//NCO *nco;
	//CPX *mix;

	//Testing inline osc instead of calling NCO
	double oscInc;
	double oscCos;
	double oscSin;
	CPX lastOsc;

};
