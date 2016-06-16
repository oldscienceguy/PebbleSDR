//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "iqbalance.h"
//Testing
//Look at ghpsdr3 correctiq
#include "medianfilter.h"

IQBalance::IQBalance(quint32 _sampleRate, quint32 _bufferSize):ProcessStep(_sampleRate, _bufferSize)
{
	//Default no gain, no phase
	gainFactor=1;
	phaseFactor= 0;
	medianBuf.reserve(_bufferSize);  //To avoid constant resizing on push()
	medianBuf.assign(_bufferSize,0.0); //Initialize
	snrSquared = new double[_bufferSize];

	//Testing
	//MedianFilter<qint16> mf(9);
	//mf.test();

}
IQBalance::~IQBalance()
{
	delete snrSquared;
}

double IQBalance::getGainFactor() {return gainFactor;}
double IQBalance::getPhaseFactor() {return phaseFactor;}
void IQBalance::setGainFactor(double g) {gainFactor=g;}
void IQBalance::setPhaseFactor(double f) {phaseFactor=f;}
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

//Algorithm and pascal code references from Rocky's iqbal.pas (Alex Shovkoplyas, VE3NEA)

//Relationship between power and amplitude we can get back from FFT
//From Wikipedia: http://en.wikipedia.org/wiki/Signal-to-noise_ratio
//SNR = Psignal / Pnoise = (Asignal / Anoise)^2
//Where P is power, and A is amplitude.

//http://www.mathworks.com/help/signal/ref/snr.html for snr calculation reference


//Iterates through FFT output and updates an array of SNR ratios for each bin
//Todo: We need FFT output before averaging and conversion to db (see fft.cpp) or change algorithm to work with db not power
void IQBalance::CalcNoise(CPX *inFreqDomain)
{
	double noise;
	for (int i = 0; i < numSamples; i++) {
		//Magnitude squared (power at ith frequency bin)
		snrSquared[i] = medianBuf[i] = sqrMagCpx(inFreqDomain[i]);
	}
	//Update median, which is our noise figure.  Half the bins have lower and half have higher power levels
	std::nth_element(medianBuf.begin(), medianBuf.begin() + medianBuf.size()/2, medianBuf.end());
	noise = medianBuf[medianBuf.size()/2];
	//Calculate signal to noise for each bin
	//double normalize = 1.0 / median;
	for (int i = 0; i < numSamples; i++) {
		snrSquared[i] = snrSquared[i] / noise; //Signal / Noise
		qDebug() << "SNR ["<< i <<"] "<< snrSquared[i];
	}
}

//Find the strongest SNR
void IQBalance::FindPeak()
{
	//Todo: Keep bin size in hz in FFT base class, then use it to determine minimum signal width in # of bins
	//Use this when looking for signals > 30db (SNR_THRESHOLD) to ignore spikes that are not real signals

}
