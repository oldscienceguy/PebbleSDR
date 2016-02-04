//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "nco.h"

NCO::NCO(quint32 _sampleRate, quint32 _bufferSize) :
	ProcessStep(_sampleRate,_bufferSize)
{
}

NCO::~NCO(void)
{
}
void NCO::SetFrequency(double f)
{
	//Eveeything related to frequency has to be set together, no interupts that could change freq 
	//before step is set
	mutex.lock();
	//Saftey net, make sure freq is less than nyquist limit
	int nyquist = (sampleRate / 2 ) -1;
	if (f < - nyquist)
		f = -nyquist;
	else if (f > nyquist)
		f = nyquist;

	frequency = f;
	//Everytime freq changes, we restart synchronized NCO
	//step is how much we have to increment each sample in sin(sample) to get a waveform that
	//is perfectly synchronized to samplerate
	//w = 2 * PI * f where f is normalized (f/sampleRate) and w is angular frequency
	//x[t] = cos(wt)
	//NOTE: The only difference between a negative freq and a positive is the direction of the 'rotation'
	//ie angularIncrement will be neg for neg freq and pos for pos freq
	angularIncrement = TWOPI * frequency / sampleRate;
	nextAngularFreq = 0;

	mutex.unlock();
}
void NCO::SetStartingPhase(double p)
{
	nextAngularFreq = p;
}
/*
From Lynn 2nd ed pg 20
x[n] = sin(n * 2 * Pi * f / fs)
f = desired frequency
fs = sampling frequency or 1/sample rate

We calculate 2Pif/fs once and save it as step
so
x[n] = sin(n * stepTone)
----------------
The number of samples per cycle is fixed, but the samples will be taken at different points in
each cycle.  We need to make sure that the spacing between each sample is consistent with the
sample rate and frequency.
A 10hz signal completes a full cycle ever 1/10 sec (1/Freg)
At 48000 sps, this means we need 4800 samples for a complete cytle (sampleRate / freq)
So a 24khz signal will have 2 samples per cycle (48000/24000), which is the Nyquist limit
-----------------
*/
CPX * NCO::GenSamples()
{
	mutex.lock();

	for (int i = 0; i < numSamples; i++)
	{
		//cos() is the real component
		out[i].im = sin(nextAngularFreq);
		//cos is 90% out of phase of sin
		out[i].re = cos(nextAngularFreq);
		nextAngularFreq += angularIncrement;
		//We're basically plotting sin(x) where x can range from 0-360deg or 0 to 2pi radians
		//x steps in angularIncrement increments, and we want to keep in the range or 0-2pi
		if (nextAngularFreq >= TWOPI)
			nextAngularFreq -= TWOPI;
		else if (nextAngularFreq < 0.0)
			nextAngularFreq += TWOPI;
	}
	//next loop starts where we left off as if i=numSamples; i<numSamples*loop; ...
	mutex.unlock();
	return out;
}
//Returns next sample at specified freq and phase
//Same logic as GenSample, just loop can be external to NCO
CPX NCO::NextSample()
{
	CPX cx(cos(nextAngularFreq),sin(nextAngularFreq));
	//DTTSP does an additional mult by cos and sin or increment, why?
	//CPX cxDelta(cos(angularIncrement),sin(angularIncrement));

	nextAngularFreq += angularIncrement;
	if (nextAngularFreq >= TWOPI)
		nextAngularFreq -= TWOPI;
	else if (nextAngularFreq < 0)
		nextAngularFreq += TWOPI;
	//return cx * cxDelta;
	return cx;
}
