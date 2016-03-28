//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalstrength.h"

SignalStrength::SignalStrength(quint32 _sampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	reset();
}

SignalStrength::~SignalStrength(void)
{
}

void SignalStrength::reset()
{
	m_peakDb = DB::minDb;
	m_peakPower = 0;
	m_avgDb = DB::minDb;
	m_avgPower = 0;
	m_rms = 0;
	m_rmsDb = DB::minDb;
	m_extValue = DB::minDb;

	m_runningMean = 0;
	m_meanCounter = 0;
	m_prevMean = 0;
	m_prevS = 0;
	m_stdDev = 0;
	m_variance = 0;
	m_signal = 0;
	m_noise = 0;
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

CPX * SignalStrength::processBlock(CPX *in, int numSamples, double squelchDb)
{
	if (!updateTimer.isValid()) {
		updateTimer.start();
		return in; //First time we're called
	}

	if (updateTimer.elapsed() < updateInterval) {
		return in; //Waiting
	}
	updateTimer.start(); //Reset

	double pwr = 0;
	double totalPwr = 0;

	m_peakPower = 0;
	m_peakDb = DB::minDb;

	double S = 0;

	for (int i = 0; i < numSamples; i++) {
		//Total power of this sample pair
		pwr = DB::power(in[i]);
		totalPwr += pwr;

		if (pwr > m_peakPower)
			m_peakPower = pwr;

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
#if 0
		d_y1 is prevMean
		m is runningMean
		d_y2 is S
		int i = 0;
			  if(d_counter == 0) {
				d_y1 = abs(input[0]);
				d_y2 = 0;
				d_counter++;
				i++;
			  }

			  for(; i < noutput_items; i++) {
				double x = abs(input[i]);
				double m = d_y1 + (x - d_y1)/d_counter;
				d_y2 = d_y2 + (x - d_y1)*(x - m);
				d_y1 = m;
				d_counter++;
			  }
			  return noutput_items;
			  ...
			  d_signal = d_y1;
			  d_noise = 2 * d_y2 / (d_counter - 1);

			  return 10.0*log10(d_signal/d_noise);


#endif
		//Map to Knuth p216 2nd edition
		//meanCouner = k
		//runnintMean = M(k)
		//prevMean = M(k-1)
		//prevS = S(k-1)
		if (m_meanCounter > 0) {
			m_runningMean = m_prevMean + ((pwr - m_prevMean) / m_meanCounter);
			//S is the delta which will become variance
			S = m_prevS + ((pwr - m_prevMean) * (pwr - m_runningMean));
		} else {
			//Special case for first sample, mean and pwr are the same
			m_runningMean = pwr;
			S = 0;
		}
		//Testing wrapping meanCounter to average over specific number of samples
		if (m_meanCounter < 4 * numSamples) {
			m_meanCounter++;
		} else {
			reset();
		}
		m_prevMean = m_runningMean;
		m_prevS = S;
#endif
    }

	m_avgPower = totalPwr/numSamples;
	//m_avgPower = m_runningMean;


	m_variance = S / m_meanCounter;
	m_stdDev = sqrt(m_variance);

	//Different interpretations for signal
	//m_signal = m_avgPower;
	m_signal = m_runningMean;  //Should be approx same as avgPower

	//Different interpretations for noise, which is correct?
	m_noise = m_stdDev; //From texts
	//m_noise = 2 * m_variance; //From GNURadio block

	//Compare m_avgPower with m_runningMean, should be close for stable signal
	//qDebug()<<"Avg:"<<m_avgPower<<" Mean:"<<m_runningMean<<" Delta:"<<(m_avgPower - m_runningMean);

	//Test SNR
	//SNR is the power of the wanted signal divided by the noise power.
	m_snrDb = DB::powerTodB(m_signal/m_noise);
	//m_snr = DB::powerRatioToDb(m_signal, m_noise); //Same as powerTodB(m_signal/m_noise)
	m_floorDb = DB::powerTodB(m_noise);

	//Todo: peak db and average db are 10db different when looking at noise, why?
	m_peakDb = DB::powerTodB(m_peakPower);
	m_avgDb = DB::powerTodB(m_signal); //Same as rmsdB
	m_rms = sqrt(m_avgPower);
	m_rmsDb = DB::amplitudeTodB(m_rms); //Same as avgdB

	//squelch is a form of AGC and should have an attack/decay component to smooth out the response
	//we fudge that by just looking at the average of the entire buffer
	if (m_avgDb < squelchDb) {
		CPX::clearCPX(out,numSamples);
	} else {
		CPX::copyCPX(out, in, numSamples);
	}

	emit newSignalStrength(m_peakDb, m_avgDb, m_snrDb, m_floorDb, m_extValue);

	return out;
}

//Used for testing with other values
void SignalStrength::setExtValue(double v)
{
	m_extValue = v;
}
