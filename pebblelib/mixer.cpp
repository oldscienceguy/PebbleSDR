//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "mixer.h"

Mixer::Mixer(quint32 _sampleRate, quint32 _bufferSize)
{
	m_sampleRate = _sampleRate;
	m_numSamples = _bufferSize;

	m_out = CPX::memalign(m_numSamples);

	//nco = new NCO(_sampleRate,_bufferSize);
	SetFrequency(0);
	gain = 1; //Testing shows no loss
	oscTime = 0.0;

}

Mixer::~Mixer(void)
{
	delete m_out;
}

//This sets the frequency we want to mix, typically -48k to +48k
void Mixer::SetFrequency(double f)
{
	//3/17/16: For some reason, mixer is inverting
	//1. Instead of re:im = cos:sin, we can switch to re:im = sin:cos
	//2. We can reverse the freq
	//Todo: Understand why this is so
	frequency = -f;
	//nco->setFrequency(f);

	oscInc = TWOPI * frequency / m_sampleRate;
	oscCos = cos(oscInc);
	oscSin = sin(oscInc);
	lastOsc.re = 1.0;
	lastOsc.im = 0.0;

}
/*
From Rick Muething, KN6KB DCC 2010 DSP course and texts noted in GPL.h 
For j = 0 to Number of Samples -1
	USB Out I(j) = NCOCos(j) * I(j) - NCOSine(j) * Q(j)
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
	for (quint32 i = 0; i < m_numSamples; i++) {
#if 1
		//nco->nextSample(osc);
		//This is executed at the highest sample rate and every usec counts
		//Pulled code from NCO for performance gain
		osc.re = lastOsc.re * oscCos - lastOsc.im * oscSin;
		osc.im = lastOsc.re * oscSin + lastOsc.im * oscCos;
		//oscGn is used to account for variations in the amplitude output as time increases
		//Since oscillator variables are not bounded as they are in alternate implementation (fmod(...))
		oscGn = 1.95 - (lastOsc.re * lastOsc.re + lastOsc.im * lastOsc.im);
		lastOsc.re = oscGn * osc.re;
		lastOsc.im = oscGn * osc.im;
#else
		//Putting sin in re works with standard unfold, cos doesn't.  why?
		osc.re = cos(oscTime);
		osc.im = sin(oscTime);
		oscTime += oscInc;
		oscTime = fmod(oscTime, TWOPI);	//Keep bounded

#endif
		//out[i]= in[i] * osc;
		m_out[i].convolution(osc, in[i]); //Inline code
	}

	return m_out;
}
