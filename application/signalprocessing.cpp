//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include "string.h"

SignalProcessing::SignalProcessing(int sr, int ns)
{
	sampleRate = sr;
	numSamples = ns;
	numSamplesX2 = ns*2; //Frequency used
	out = CPX::memalign(numSamples);
}

SignalProcessing::~SignalProcessing(void)
{
	if (out != NULL)
		free(out);
}

