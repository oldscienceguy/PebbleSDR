#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include "delayline.h"

class NoiseFilter :
	public ProcessStep
{
public:
	NoiseFilter(quint32 _sampleRate, quint32 _bufferSize);
	~NoiseFilter(void);
    CPX * ProcessBlock(CPX *in);

private:
	//ANF Delay line, todo: make DelayLine class
	int anfAdaptiveFilterSize;
    double anfAdaptationRate;
    double anfLeakage;
	int anfDelaySize; //Size of delay buffer
	DelayLine *anfDelay;
    quint16 anfDelaySamples;
	//ANF filter coefficients
    CPX *anfCoeff;

};
