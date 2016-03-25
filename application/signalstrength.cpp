//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalstrength.h"

SignalStrength::SignalStrength(quint32 _sampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	m_peakdB = DB::minDb;
	m_peakPower = 0;
	m_avgdB = DB::minDb;
	m_avgPower = 0;
	m_rms = 0;
	m_rmsdB = DB::minDb;
	m_extValue = DB::minDb;
}

SignalStrength::~SignalStrength(void)
{
}
/*
From Lynn book pg 443-445: 
Avg power for any given freq = 1/2 * (B^2 + C^2) 'This is mean of the squares'
Total power = sum of power for each freq in fourier series
or
Total power = mean square value of time domain waveform

RMS (Root Mean Square) = sqrt(mean of the squares)

ProcessBlock is called after we've mixed and filtered our samples, so *in contains desired signal
Compute the total power of all the samples, convert to dB and save
Background thread will pick up instValue or avgValue and display
*/

CPX * SignalStrength::ProcessBlock(CPX *in, int numSamples, double squelchdB)
{
    //Squelch values are global->mindB to global->maxdB

	double pwr = 0;
	m_peakPower = 0;
	double totalPwr = 0;

	m_peakdB = DB::minDb;

	//Testing standard dev
	double mean = 0;
	double prevMean = 0;
	double S = 0;
	double prevS = 0;
	double stdDev = 0;
	double variance = 0;

	for (int i = 0, k=1; i < numSamples; i++,k++) {
		//Total power of this sample pair
		pwr = DB::power(in[i]);
		if (pwr > m_peakPower)
			m_peakPower = pwr;

		totalPwr += pwr;

		//Running mean, variance & standard deviation.  Needed for SNR calculations
		//Knuth algorithm which handles a standard deviation which is a small fraction of the mean
		//See Knuth TAOCP vol 2, 3rd edition, page 232
		//for each incoming sample x:
		//    prev_mean = m;
		//    n = n + 1;
		//    m = m + (x-m)/n;
		//    S = S + (x-m)*(x-prev_mean);
		//	standardDev = sqrt(S/n) or sqrt(S/n-1) depending on who you listen to
#if 0
		prevMean = mean;
		mean = mean + (pwr - mean) / j; //no divide by 0
		S = S + (pwr - mean) * (pwr - prevMean);
#else
		//Map to Knuth p216 2nd edition
		//mean = M(k)
		//prevMean = M(k-1)
		//prevS = S(k-1)
		if (k > 1) {
			mean = prevMean + ((pwr - prevMean) / k);
			S = prevS + ((pwr - prevMean) * (pwr - mean));
		} else {
			//Special case for first sample, mean and pwr are the same
			mean = pwr;
			S = 0;
		}
		prevMean = mean;
		prevS = S;
#endif
		variance = S / k;
		stdDev = sqrt(variance);

        //Verified 8/13 comparing testbench gen output at all db levels
		//instValue = DB::clip(DB::powerTodB(pwr)); //Watts to db
		//instValue = DB::clip(DB::amplitudeTodB(DB::amplitude(in[i])));
        //Weighted average 90/10
		//avgValue = 0.99 * avgValue + 0.01 * instValue;

    }
	m_peakdB = DB::powerTodB(peakPower());

	//Test SNR
	//SNR is the power of the wanted signal divided by the noise power.
	m_snr = DB::powerTodB(mean / stdDev);
	//m_snr = DB::powerRatioToDb(mean, stdDev); //Same as powerTodB(mean/stdDev)

	//Todo: peak db and average db are 10db different when looking at noise, why?
	m_avgPower = totalPwr/numSamples;
	m_avgdB = DB::powerTodB(m_avgPower); //Same as rmsdB
	m_rms = sqrt(m_avgPower);
	m_rmsdB = DB::amplitudeTodB(m_rms); //Same as avgdB

	//squelch is a form of AGC and should have an attack/decay component to smooth out the response
	//we fudge that by just looking at the average of the entire buffer
	if (m_avgdB < squelchdB) {
		CPX::clearCPX(out,numSamples);
	} else {
		CPX::copyCPX(out, in, numSamples);
	}

	return out;
}

//Used for testing with other values
void SignalStrength::setExtValue(double v)
{
	m_extValue = v;
}
