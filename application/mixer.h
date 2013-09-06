#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
Simple DSP Mixer
*/
#include "nco.h"

class Mixer:public SignalProcessing
{
public:
	Mixer(int sampleRate, int nSamples);
	~Mixer(void);
        CPX * ProcessBlock(CPX *in);
	void SetFrequency(double f);
private:
	double frequency;
	int gain; //Adjust for mixer loss so we keep constant gain
	NCO *nco;
	CPX *mix;


};
