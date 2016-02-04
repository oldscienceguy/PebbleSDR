//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "mixer.h"

Mixer::Mixer(int sr, int ns):ProcessStep(sr,ns)
{
	//Test
	nco = new NCO(sr,ns);
	SetFrequency(0);
	gain = 1; //Testing shows no loss
}

Mixer::~Mixer(void)
{
}
//This sets the frequency we want to mix, typically -48k to +48k
void Mixer::SetFrequency(double f)
{
	nco->SetFrequency(f);

	//Bug: there's a problem if abs(f) is close to sampleRate which seems to be the same as f=0;
	frequency = f;
}
/*
From Rick Muething, KN6KB DCC 2010 DSP course and texts noted in GPL.h 
For j = 0 to Number of Samples -1
    USB Out I (j) = NCOCos(j) * I(j) NCOSine(j) * Q(j)
	LSB Out Q(j) = NCOSin(j) * I(j) + NCOCos(j) * Q(j)
Next j
*/
CPX *Mixer::ProcessBlock(CPX *in)
{
	//If no mixing, just copy in to out
	if (frequency == 0) {
		return in;
	}
#if(0)
	//Experiment: Does it make a difference if we do this in blocks or sample by sample?
	mix = nco->GenSamples();
	CPX::multCPX(out,in,mix,numSamples);
#else
	CPX cx;
	for (int i = 0; i < numSamples; i++)
	{
		cx = nco->NextSample();
		//NOTE: sin(a+b) and cos(a+b) are handled by complex mult, see nco.cpp for more details
		if (gain != 1)
			out[i] = cx * in[i] * gain;
		else
			out[i] = cx * in[i];
	}
#endif
	return out;
}
