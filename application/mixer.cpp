//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "mixer.h"

Mixer::Mixer(quint32 _sampleRate, quint32 _bufferSize):ProcessStep(_sampleRate,_bufferSize)
{

	//nco = new NCO(_sampleRate,_bufferSize);
	SetFrequency(0);
	gain = 1; //Testing shows no loss
}

Mixer::~Mixer(void)
{
}

//This sets the frequency we want to mix, typically -48k to +48k
void Mixer::SetFrequency(double f)
{
	frequency = f;
	//nco->setFrequency(f);

	oscInc = TWOPI * frequency / sampleRate;
	oscCos = cos(oscInc);
	oscSin = sin(oscInc);
	lastOsc.re = 1.0;
	lastOsc.im = 0.0;

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
	CPX osc;
	double oscGn = 1;
	for (int i = 0; i < numSamples; i++) {
		//nco->nextSample(osc);
		//This is executed at the highest sample rate and every usec counts
		//Pulled code from NCO for performance gain
		osc.re = lastOsc.re * oscCos - lastOsc.im * oscSin;
		osc.im = lastOsc.im * oscCos + lastOsc.re * oscSin;
		oscGn = 1.95 - (lastOsc.re * lastOsc.re + lastOsc.im * lastOsc.im);
		lastOsc.re = oscGn * osc.re;
		lastOsc.im = oscGn * osc.im;

		out[i].convolution(osc, in[i]); //Inline code
	}

	return out;
}
