#ifndef DECIMATOR_H
#define DECIMATOR_H

#include "processstep.h"
/*
   This class implements a decimation (anti-alias filter + downsampling) step.

*/
class Decimator : public ProcessStep
{
public:
	Decimator(quint32 _sampleRate, quint32 _bufferSize);

	CPX *process(CPX *in, quint32 _numSamples);

};

#endif // DECIMATOR_H
