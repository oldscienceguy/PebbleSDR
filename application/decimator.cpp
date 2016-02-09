#include "decimator.h"

Decimator::Decimator(quint32 _sampleRate, quint32 _bufferSize) :
	ProcessStep(_sampleRate, _bufferSize)
{

}

CPX *Decimator::process(CPX *in, quint32 _numSamples)
{

}
