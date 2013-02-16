//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SignalProcessing.h"
#include "string.h"

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
double SignalProcessing::totalPower(CPX *in, int bsize)
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
double SignalProcessing::powerToDb(double p)
{
    //For our purposes -127db is the lowest we'll ever see.  Handle special case of 0 directly
    if (p==0)
        return global->minDb;

	//Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
    //Voltage = 20 * log10(V2/V1)
	//  + ALMOSTZERO avoid problem if p==0 but does not impact result
    return  qBound(global->minDb, 10.0 * log10(p + ALMOSTZERO), global->maxDb);
}
double SignalProcessing::dbToPower(double db)
{
	return pow(10, db/10.0);
}
//Steven Smith pg 264
double SignalProcessing::amplitudeToDb(double a)
{
    return qBound(global->minDb, 20.0 * log10(a + ALMOSTZERO), global->maxDb);
}
//Steven Smith pg 264
double SignalProcessing::dbToAmplitude(double db)
{
	return pow(10, db/20.0);
}
/*
	One S unit = 6db
	s0 = -127, s1=-121, s2=-115, ... s9 = -73db (-93dbVHF)
	+10 = -63db, +20 = -53, ... +60 = -13db
*/
int SignalProcessing::dbToSUnit(double db)
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
    return qBound(global->minDb, (10.0 * log10(watts)) + 30.0, global->maxDb);
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
CPX DelayLine::MAC(double *coeff, int numCoeff)
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
FFT::FFT(int size):perform()
{
	fftSize = size;
	half_sz = size/2;

	timeDomain = CPXBuf::malloc(size);
	freqDomain = CPXBuf::malloc(size);

    plan_fwd = fftw_plan_dft_1d(size , (double (*)[2])timeDomain, (double (*)[2])freqDomain, FFTW_FORWARD, FFTW_MEASURE);
    plan_rev = fftw_plan_dft_1d(size , (double (*)[2])freqDomain, (double (*)[2])timeDomain, FFTW_BACKWARD, FFTW_MEASURE);
    buf = CPXBuf::malloc(size);
	CPXBuf::clear(buf, size);
	overlap = CPXBuf::malloc(size);
	CPXBuf::clear(overlap, size);

#if 0
    //Testing Ooura
    //These are inplace transforms,ie out = in, of 1 dimensional data (dft_1d)
    offt = new FFTOoura();
    offtWorkArea = new int[fftSize]; //Work buffer for bit reversal, size at least 2+sqrt(n)
    offtWorkArea[0] = 0; //Initializes w with sine/cosine values.  Only do this once
    offtSinCosTable = new double[fftSize*5/4]; //sine/cosine table.  Check size
    offtBuf = CPXBuf::malloc(fftSize+1);
#endif
}

FFT::~FFT()
{
    fftw_destroy_plan(plan_fwd);
    fftw_destroy_plan(plan_rev);


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

    fftw_execute(plan_fwd);

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
    fftw_execute(plan_rev);

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
void FFT::DoFFTWMagnForward (CPX * in, int size, double baseline, double correction, double *fbr)
{
	if (size < fftSize)
		//Make sure that buffer which does not have samples is zero'd out
		CPXBuf::clear(timeDomain, fftSize);

    //perform.StartPerformance();
    //FFTW averages 25-30us (relative) between calls
    //Ooura averages 35-40us (relative) between calls
    //Confirmed with Mac CPU usage: avg 90% with Ooura vs 80% with FFTW, just using Ooura for Spectrum FFT!
    //So Ooura is significantly slower!
#if 1
    //Use FFTW
    CPXBuf::copy(timeDomain, in, size);
    // For Ref plan_fwd = fftwf_plan_dft_1d(size , (float (*)[2])timeDomain, (float (*)[2])freqDomain, FFTW_FORWARD, FFTW_MEASURE);
    fftw_execute(plan_fwd);
    //FFT output is now in freqDomain
    //FFTW does not appear to be in order as documented.  On-going mystery
    /*
     *From FFTW documentation
     *From above, an FFTW_FORWARD transform corresponds to a sign of -1 in the exponent of the DFT.
     *Note also that we use the standard “in-order” output ordering—the k-th output corresponds to the frequency k/n
     *(or k/T, where T is your total sampling period).
     *For those who like to think in terms of positive and negative frequencies,
     *this means that the positive frequencies are stored in the first half of the output
     *and the negative frequencies are stored in backwards order in the second half of the output.
     *(The frequency -k/n is the same as the frequency (n-k)/n.)
     */
    CPX temp;
    for (int i=0, j = size-1; i < size/2; i++, j--) {
        temp = freqDomain[i];
        freqDomain[i] = freqDomain[j];
        freqDomain[j] = temp;
    }

    FreqDomainToMagnitude(freqDomain, size, baseline, correction, fbr);
#else
    //CPXBuf::copy(timeDomain, in, size);
    CPXBuf::copy(offtBuf, in, size);

    //Size is 2x fftSize because offt works on double[] re-im-re-im et
    offt->cdft(2*size, +1, (double*)offtBuf, offtWorkArea, offtSinCosTable);
    FreqDomainToMagnitude(offtBuf, size, baseline, correction, fbr);

#endif
    //perform.StopPerformance(5);
}

/*
 * Spectrum Folding Example for ooura FFT
 * Buffer[]:        0           N/2-1   N/2                 N-1
 *
 * FFT bins:        0           Most Positive (MP)                                  //Positive Freq
 *                                      Most Negative (MN)  Least Negative(LN)      //Negative Freq
 *
 * Folded:          MN          LN                                                  // +/- freq order
 *                                      0                   MP
 *
 * FFTW should be in same order, but is reversed, ie
 * FFT bins:        LN          MN      MP                  0
 */

//This can be called directly if we've already done FFT
//WARNING:  fbr must be large enough to hold 'size' values
void FFT::FreqDomainToMagnitude(CPX * freqBuf, int size, double baseline, double correction, double *fbr)
{
    //calculate the magnitude of your complex frequency domain data (magnitude = sqrt(re^2 + im^2))
    //convert magnitude to a log scale (dB) (magnitude_dB = 20*log10(magnitude))

    // FFT output index 0 to N/2-1 - frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term )
    //This puts 0 to size/2 into size/2 to size-1 position
    for (int i=0, j=size/2; i<size/2; i++,j++) {
        fbr[j] = SignalProcessing::amplitudeToDb(freqBuf[i].mag() + baseline) + correction;
    }
    // FFT output index N/2 to N-1 - frequency output -Fs/2 to 0
    // This puts size/2 to size-1 into 0 to size/2
    //Works correctly with Ooura FFT
    for (int i=size/2, j=0; i<size; i++,j++) {
        fbr[j] = SignalProcessing::amplitudeToDb(freqBuf[i].mag() + baseline) + correction;
    }
}
