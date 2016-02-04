#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include "delayline.h"

class NoiseBlanker :
	public ProcessStep
{
public:
	NoiseBlanker(quint32 _sampleRate, quint32 _bufferSize);
	~NoiseBlanker(void);
	void setNbEnabled(bool b);
	void setNb2Enabled(bool b);

	CPX * ProcessBlock(CPX *in);
	CPX * ProcessBlock2(CPX *in); //dttsp calls this SDR OM noise blanker

private:
	bool nbEnabled;
	bool nb2Enabled;
	DelayLine *nbDelay;
	float nbAverageMag;
	float nb2AverageMag;
	CPX nbAverageCPX;
	CPX nb2AverageCPX;
	int nbSpikeCount;
	int nbSpike; //# samples we consider to be a spike, typically 7
	double nbThreshold; //Adjustable, typically 3.3;

};
