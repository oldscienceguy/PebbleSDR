//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include "string.h"

ProcessStep::ProcessStep(int sr, int ns)
{
	sampleRate = sr;
	numSamples = ns;
	numSamplesX2 = ns*2; //Frequency used
	out = CPX::memalign(numSamples);
}

ProcessStep::~ProcessStep(void)
{
	if (out != NULL)
		free(out);
}

void ProcessStep::enableStep(bool _enableStep)
{
	enabled = _enableStep;
}

