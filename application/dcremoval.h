#ifndef DCREMOVAL_H
#define DCREMOVAL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "cpx.h"
#include "iir.h"
#include "processstep.h"

class DCRemoval : public ProcessStep
{
public:
	DCRemoval(quint32 _sampleRate, quint32 _frameCount);
	void process(CPX *in, CPX *out, quint32 _numSamples);

private:
	CIir dcHpFilter;
};

#endif // DCREMOVAL_H
