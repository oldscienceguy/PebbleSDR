//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "NoiseFilter.h"

NoiseFilter::NoiseFilter(int sr, int ns):
	SignalProcessing(sr,ns)
{

	anfDelaySize = 512; //dttsp 512, SDRMax 1024
	anfAdaptiveFilterSize = 45; //dttsp 45, SDRMax 256
	anfAdaptationRate = 0.01; //dttsp 0.01, SDRMax 0.005
	anfLeakage = 0.00001; //dttsp 0.00001, SDRMax 0.01
	anfDelay = new DelayLine(anfDelaySize, 64); //64 sample delay line
    anfCoeff = new double[anfAdaptiveFilterSize];
	anfEnabled = false;
}

NoiseFilter::~NoiseFilter(void)
{
}
void NoiseFilter::setAnfEnabled(bool b)
{
	if (b)
		//Clean out any garbage from prior filtering
		memset(anfCoeff,0.0,sizeof(float) * anfAdaptiveFilterSize);

	anfEnabled = b;
}

//Called from main recieve chain if enabled
/*
Algorithm references
	dttsp lmadf.c

*/
CPX * NoiseFilter::ProcessBlock(CPX *in)
{
	int size = numSamples;
	//anfEnabled = false;
	//Works,  but takes a few seconds to stabilize
	if (!anfEnabled)
	{
		return in;
	}
	float sos = 0; //Sum of squares
	
	float errorSignal = 0;
	float scl1 = 0; 
	//float factor2 = 0;
	CPX nxtDelay;
	//How rapidly do we adjust for changing signals, bands, etc
	scl1 = 1.0 - anfAdaptationRate * anfLeakage;

	for (int i = 0; i < size; i++)
	{
		//Add the current sample to the delay line
		anfDelay->NewSample(in[i]);

		sos = 0.0;
		//We don't have to run through entire delay line, just enough to calc noise data
		//For each sample, accumulate 256 (adapt size) samples, delayed by 64 samples
		//This is the basic filter step and the coefficients tell us how to weigh historical samples
		//or,if preset, filter for desired frequencies.
		for (int j = 0; j < anfAdaptiveFilterSize; j++)
		{
			nxtDelay = anfDelay->NextDelay(j);

			sos += (nxtDelay.re * nxtDelay.re);
			//sos += nxtDelay.mag();
		}
		//Test
		CPX cpxMAC = anfDelay->MAC(anfCoeff,anfAdaptiveFilterSize);
		out[i] = in[i] - cpxMAC;

		//This is the delta between the actual current sample, and MAC from delay line (reference signal)
		//or if we're comparing to a desired CW sine wave, the diff from that
		//It drives the Least Means Square (LMS) algorithm
		//errorSignal = (in[i] - MAC * adaptionRate) / SOS
#if (0)
		float avgMag = sos / anfAdaptiveFilterSize;
		errorSignal = in[i].mag() - avgMag;
#else
		errorSignal = (in[i].re - cpxMAC.re) * (anfAdaptationRate / (sos + 1e-10));
#endif
		//And calculate tne new coefficients for next sample
		/*
		See Doug Smith, pg 8-2 for Adaptive filters, Least Mean Squared, Leakage, etc
		h[t+1] = h[t] + 2 * adaptationRate * errorSignal * sample
		*/

		for (int j = 0; j < anfAdaptiveFilterSize; j++)
		{
			nxtDelay = anfDelay->NextDelay(j);
			//Weighted average based on anfLeakage
			//AdaptationRate is between 0 and 1 and determines how fast we stabilize filter
			//anfCoeff[j] = anfCoeff[j-1] * anfLeakage + (1.0 - anfLeakage) * anfAdaptationRate * nxtDelay.re * errorSignal;
			anfCoeff[j] = anfCoeff[j] * scl1 + errorSignal * nxtDelay.re;
		}
	}
	return out;
}
