//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SignalProcessing.h"
#include "string.h"
#include "global.h"

SignalProcessing::SignalProcessing(int sr, int ns)
{
	sampleRate = sr;
	numSamples = ns;
	numSamplesX2 = ns*2; //Frequency used
	out = CPXBuf::malloc(numSamples);
}

SignalProcessing::~SignalProcessing(void)
{
	if (out != NULL)
		CPXBuf::free(out);
}

//Returns the total power in the sample buffer, using Lynn formula
float SignalProcessing::totalPower(CPX *in, int bsize)
{
	float tmp = 0.0;
	//sum(re^2 + im^2)
    for (int i = 0; i < bsize; i++) {
		//Total power of this sample pair
        tmp += cpxToWatts(in[i]);
    }
	return tmp;
}
/*
db tutorial from Steven Smith
'bel' means power has changed by factor of 10
'decibel' is 0.1 of a bell, so 10db means power changed by factor of 10
60db = power changed by 10^3 = 1000
40db = power changed by 10^2 = 100
20db = power changed by 10^1 = 10
0db = power changed by 10^0 = 1
*/
//Power same as cpx.mag()
//Convert Power to Db
float SignalProcessing::powerToDb(float p)
{
    //For our purposes -127db is the lowest we'll ever see.  Handle special case of 0 directly
    if (p==0)
        return -127;

	//Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
    //Voltage = 20 * log10(V2/V1)
	//  + ALMOSTZERO avoid problem if p==0 but does not impact result
    return  10.0 * log10(p + ALMOSTZERO);
}
float SignalProcessing::dbToPower(float db)
{
	return pow(10, db/10.0);
}
//Steven Smith pg 264
float SignalProcessing::amplitudeToDb(float a)
{
	return 20.0 * log10(a + ALMOSTZERO);
}
//Steven Smith pg 264
float SignalProcessing::dbToAmplitude(float db)
{
	return pow(10, db/20.0);
}
/*
	One S unit = 6db
	s0 = -127, s1=-121, s2=-115, ... s9 = -73db (-93dbVHF)
	+10 = -63db, +20 = -53, ... +60 = -13db
*/
int SignalProcessing::dbToSUnit(float db)
{
	int unit = db/6;
	return unit;
}
double SignalProcessing::dBm_2_Watts(double dBm)
{
	return pow(10.0, (dBm - 30.0) / 10.0);
}

double SignalProcessing::watts_2_dBm(double watts)
{
	if (watts < 10.0e-32) 
		watts = 10.0e-32;
	return (10.0 * log10(watts)) + 30.0;
}

double SignalProcessing::dBm_2_RMSVolts(double dBm, double impedance)
{
	return sqrt(dBm_2_Watts(dBm) * impedance);
}

double SignalProcessing::rmsVolts_2_dBm(double volts, double impedance)
{
	return watts_2_dBm((volts * volts) / impedance);
}




DelayLine::DelayLine(int s, int d) 
{
	size = s;
	delay = d;
	buffer = CPXBuf::malloc(size);
	CPXBuf::clear(buffer,size);
	head = 0;
	last = 0;
}
DelayLine::~DelayLine()
{
	if (buffer != NULL) CPXBuf::free(buffer);
}
void DelayLine::NewSample(CPX c)
{
	buffer[head] = c;
	last = head;
	//buffer order is newest to oldest, so head is decremented modulo size
	head = (head==0) ? size-1 : --head;
}
//Next sample in delay line, mod size of delay line to wrap (ring buffer)
CPX DelayLine::NextDelay(int i)
{
	//Last new value, plus delay, plus passed index, mod line size
	int next = (last + delay + i) % size;
	return buffer[next];
}
CPX DelayLine::operator [] (int i)
{
	return buffer[i];
}
//Convolution sum or Multiply And Accumulate (MAC)
//Key component for filter math
CPX DelayLine::MAC(float *coeff, int numCoeff)
{
	mutex.lock();
	int next;
	CPX mac(0,0);
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) % size;
		//This is a MAC operation and is NOT the same as cpx1 * cpx2 (see cpx.h)
		mac.re += buffer[next].re * coeff[i];
		mac.im += buffer[next].im * coeff[i];
	}
	mutex.unlock();
	return mac;
}
CPX DelayLine::MAC(CPX *coeff, int numCoeff)
{
	mutex.lock();
	int next;
	CPX mac(0,0);
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) % size;
		//This is a MAC operation and is NOT the same as cpx1 * cpx2 (see cpx.h)
		mac.re += buffer[next].re * coeff[i].re;
		mac.im += buffer[next].im * coeff[i].im;
	}
	mutex.unlock();
	return mac;
}
#if (0)
void LMS(int numCoeff)
{
	int next;
	float sos = 0;
	//Compute the average squared value
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) & mask;
		sos += buffer[next].re;
	}
	float avgSOS = sos / numCoeff;

}
#endif
FFT::FFT(int size) 
{
	fftSize = size;
	half_sz = size/2;

	timeDomain = CPXBuf::malloc(size);
	freqDomain = CPXBuf::malloc(size);

	//These are inplace transforms,ie out = in, of 1 dimensional data (dft_1d)
	plan_fwd = fftwf_plan_dft_1d(size , (float (*)[2])timeDomain, (float (*)[2])freqDomain, FFTW_FORWARD, FFTW_MEASURE);
	plan_rev = fftwf_plan_dft_1d(size , (float (*)[2])freqDomain, (float (*)[2])timeDomain, FFTW_BACKWARD, FFTW_MEASURE);
    buf = CPXBuf::malloc(size);
	CPXBuf::clear(buf, size);
	overlap = CPXBuf::malloc(size);
	CPXBuf::clear(overlap, size);
}

FFT::~FFT()
{

	fftwf_destroy_plan(plan_fwd);
	fftwf_destroy_plan(plan_rev);


	if (buf) CPXBuf::free(buf);
	if (overlap) CPXBuf::free(overlap);
	if (timeDomain) CPXBuf::free(timeDomain);
	if (freqDomain) CPXBuf::free(freqDomain);
}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFT::DoFFTWForward(CPX * in, CPX * out, int size)
{
	//If in==NULL, use whatever is in timeDomain buffer
	if (in != NULL ) {
		if (size < fftSize)
			//Make sure that buffer which does not have samples is zero'd out
			//We can pad samples in the time domain because it does not impact frequency results in FFT
			CPXBuf::clear(timeDomain,fftSize);

		//Put the data in properly aligned FFTW buffer
		CPXBuf::copy(timeDomain, in, size);
	}

    fftwf_execute(plan_fwd);

	//If out == NULL, just leave result in freqDomain buffer and let caller get it
	if (out != NULL)
		CPXBuf::copy(out, freqDomain, fftSize);

}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFT::DoFFTWInverse(CPX * in, CPX * out, int size)
{
	//If in==NULL, use whatever is in freqDomain buffer
	if (in != NULL) {
		if (size < fftSize)
			//Make sure that buffer which does not have samples is zero'd out
			CPXBuf::clear(freqDomain,fftSize);

		CPXBuf::copy(freqDomain, in, size);
	}
    fftwf_execute(plan_rev);

	if (out != NULL)
		CPXBuf::copy(out, timeDomain, fftSize);

}
//Utility to handle overlap/add using FFT buffers
void FFT::OverlapAdd(CPX *out, int size)
{
	//Do Overlap-Add to reduce from 1/2 fftSize

	//Add the samples in 'in' to last overlap
	CPXBuf::add(out, timeDomain, overlap, size);

	//Save the upper 50% samples to  overlap for next run
	CPXBuf::copy(overlap, (timeDomain+size), size);

}

//NOTE: size= # samples in 'in' buffer, 'out' must be == fftSize (set on construction) which is #bins
void FFT::DoFFTWMagnForward (CPX * in, int size, float baseline, float correction, float *fbr)
{
	if (size < fftSize)
		//Make sure that buffer which does not have samples is zero'd out
		CPXBuf::clear(timeDomain, fftSize);

	CPXBuf::copy(timeDomain, in, size);

    fftwf_execute(plan_fwd);
	FreqDomainToMagnitude(freqDomain, fftSize, baseline, correction, fbr);

}

//This can be called directly if we've already done FFT
//WARNING:  fbr must be large enough to hold 'size' values
void FFT::FreqDomainToMagnitude(CPX * freqBuf, int size, float baseline, float correction, float *fbr)
{
	/*
	freqBuf[0] = DC component, or what we expect to see in the middle of spectrum
	This puts things in the right order by first reversing the buffer so what was highest is now lowest
	Then splitting so that db output array is -f..0..+f
	*/
    for (int i=0, j = size-1; i < size; i++, j--) {
		buf[j] = freqBuf[i];
    }

	//Convert to db and order correctly
    //Limit output to -150db to 60db
	for (int i=0, j = size/2; i < size/2; i++, j++) {
        fbr[i] = qBound(-150.0, 10.0 * log10((buf[j]).sqrMag() + baseline) + correction, 60.0); //global->MIN_DB);

        fbr[j] = qBound(-150.0, 10.0 * log10((buf[i]).sqrMag() + baseline) + correction, 60.0); //global->MIN_DB);

    }
}
