//GPL license and attributions are in gpl.h and terms are included in this file by reference

#include "iqbalance.h"

IQBalance::IQBalance(int sr, int fc):SignalProcessing(sr, fc)
{
	//Default no gain, no phase
	gainFactor=1;
	phaseFactor= 0;
	enabled = false;
}

double IQBalance::getGainFactor() {return gainFactor;}
double IQBalance::getPhaseFactor() {return phaseFactor;}
bool IQBalance::getEnabled() {return enabled;}
void IQBalance::setGainFactor(double g) {gainFactor=g;}
void IQBalance::setPhaseFactor(double f) {phaseFactor=f;}
void IQBalance::setEnabled(bool b) {enabled = b;}
void IQBalance::setAutomatic(bool b) {automatic = b;}

//Correct IM relative to RE using phase and gain factors to reduce images
//Time domain, before spectrum and mixer
#if(0)
CPX *IQBalance::ProcessBlock(CPX *in)
{
	if (!enabled)
		return in;

	//Can we calc a valid gain value by simply averaging across sample buffer?
	if (automatic) {
		float Ival=0;
		float Qval=0;
		for (int i=0; i<numSamples;i++) {
			Ival += fabs(in[i].re);
			Qval += fabs(in[i].im);
		}
		gainFactor = Ival/Qval;
	}
	for (int i = 0; i < numSamples; i++) {
		//Adj .re gain relative to .im
		out[i].re = in[i].re * gainFactor;
		//Adj .im with a portion of .re
		out[i].im = in[i].im + (in[i].re * phaseFactor);
	}

	return out;
}
#else
/*
 From Bob, N4HY.  AB2KT branch of Dttsp
 */
CPX *IQBalance::ProcessBlock(CPX *in)
{
	if (!enabled)
		return in;

	CPX t1(0,0);
	CPX t2(0,0);
	float mu = 0.0025;
	for (int i = 0; i < numSamples; i++) {
		//Standard math for adj iq bal, used in several algorithms
		//Adj .re gain relative to .im
		out[i].re = in[i].re * gainFactor;
		//Adj .im with a portion of .re
		out[i].im = in[i].im + (in[i].re * phaseFactor);

		t1 = out[i] + (t2 * out[i].conj());
		t2 = (t2.scale(1.0 - mu * 0.000001)) - ((t1*t1).scale(mu));
		out[i] = t1;
	}
	return out;
}

#endif

/*
 This will be the automatic algorithm described by Alex VE3NEA for Rocky and Bob McGwier N4HY in
 his 2007 DCC paper.

 This is inserted in the chain after we convert to Freq domain in order to display spectrum.
 The entire FFT is examined to find significant signals and related images and a correction value
 is calculated for each FFT bin.
 The correction is then applied before the Inverse FFT returns us to Time Domain

CPX *IQBalance::BalanceInFrequencyDomain(CPX *in)
{
}
*/
