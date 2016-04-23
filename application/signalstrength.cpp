//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalstrength.h"

SignalStrength::SignalStrength(quint32 _sampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	m_squelchDb = DB::minDb;
	reset();
}

SignalStrength::~SignalStrength(void)
{
}

void SignalStrength::reset()
{
	m_peakDb = DB::minDb;
	m_peakAmplitude = 0;
	m_avgDb = DB::minDb;
	m_rms = 0;
	m_rmsDb = DB::minDb;
	m_extValue = DB::minDb;

	m_runningMean = 0;
	m_stdDev = 0;
	m_variance = 0;
	m_signal = 0;
	m_noise = 0;
}

/*
S-Meter represents signal amplitude

A fast way to estimate without sqrt() overhead: http://dspguru.com/dsp/tricks/magnitude-estimator

From Lynn book pg 443-445: 
Avg power for any given freq = 1/2 * (B^2 + C^2) 'This is mean of the squares'
Total power = sum of power for each freq in fourier series
or
Total power = mean square value of time domain waveform

RMS (Root Mean Square) = sqrt(mean of the squares)

From Rhodes&Schwartz (Spectrum Analyzer) application notes
https://www.rohde-schwarz.com/us/applications/importing-data-in-arb-custom-digital-modulation-and-rf-list-mode-application-note_56280-15778.html
https://www.tablix.org/~avian/blog/archives/2015/04/correcting_the_gnu_radio_power_measurements/
The RMS, Peak and Crest Factor of a complex signal buffer is defined as
follows:
rms = sqrt(1/n * sum(0:n-1) (I[n]^2 + Q[n]^2))
peak = max(0:n-1)sqrt(I[n]^2 + Q[n]^2)
crest=20*log10 (Peak/RMS)

For complex signals with a constant envelope (vector length) the peak and RMS
value is identical. In this case the crest factor becomes zero. If the envelope
is exactly 1.0, which means that the full signal range is used, both numbers of
the level tag need to be set to zero. However, it is possible to use other
values that will either scale down the signal or lead to clipping. Both
situations might be desirable under certain circumstances. An example could be
a signal that generally uses low levels. Once in a while high peaks exist where
a clipping situation can be tolerated. Under these circumstances a negative
value for the peak offset will scale up the signal and result in a higher
resolution of the low level signal sections.

FullScale = 1, but can be changed to scale the signal for better resolution of low level signals
peakDbOffset = 20*log10(FullScale/Peak)
rmsDbOffset = 20*log10(FullScale/RMS)

From DSPGuide http://www.dspguide.com/ch10/7.htm
Parseval's Theorem (power in time and freq domain must be the same (after adj for window and FFT gain/loss)
x is a time domain vector of length N
X is a freq domain vector of length N with modifications to the DC element [0] and the last element [N-1]
TODO: Verify this with matlab
X = fft(x)
			 Time Domain    =  Freq Domain
totalPower = sum(x[i]^2)       =  sum(abs(X[i])^2) / N
avgPower   = totalPower / N    =  totalPower / N
rms        = sqrt(avgPower)    =  sqrt(avgPower)

--------------------------
GNURadio Noise experiment, effect of decimation on noise calculation.  Confirmed with Pebble and TestBench experiments

Decimation has minimal impact on the amplitude and power of the signal of interest.
But noise calculations are impacted by the number of FFT bins the noise is universally distributed across freq.
Each decimate by 2 stage cuts the number of samples in half, and therefore the total power in the buffer by half.
A 50% reduction in power equates to 3dB per stage
A decimation factor of 32 requires 5 (2^5) decimate-by-2 stages and should show a 15db reduction in measured noise
	if this is correct.
A GNURadio experiment with a noise generator added to a cosine generator, decimated, and displayed pre and post
	decimation confirmed this (see noiseDecimationTest.grc).  The same test with Pebble and TestBench set to generate a
	-50db cosine signal mixed with a -75db noise signal showed the same results.

Noise source amplitude set to 100m (adjustable to see if linear)
Noise power = 10*log10(amplitude * amplitude)

Cosine source amplitude set to 500m (0.500)
SampleRate set to 192k
FFT Plot set to average results, FFT size 1024 (default), peak to peak reference = 2
RMS results displayed in db (20*log10(rms))

								  Noise  Noise  Signal
	Noise Noise Signal Signal     FFT    FFT    FFT    RMS Meter
#   Amp   Power Amp    Power  Dec Pre    Post   Peak   Peak
 1  0.10  0.01  0.50   0.25       -50db  -50db  - 9.5db  - 5.90db
 2  0.10  0.01  0.50   0.25    2  -50db  -53db  - 9.5db  - 5.90db
 3  0.10  0.01  0.50   0.25    4  -50db  -56db  - 9.5db  - 5.90db
 4  0.10  0.01  0.50   0.25    8  -50db  -59db  - 9.5db  - 5.90db
 5  ... skip                  16
 6  0.10  0.01  0.50   0.25   32  -50db  -65db  - 9.5db  - 5.90db
 7  0.20  0.04  0.50   0.25   32  -44db  -59db  - 9.5db  - 5.90db
 8  0.40  0.16  0.50   0.25   32  -38db  -53db  - 9.5db  - 5.90db
 9  0.40  0.16  0.25   0.06   32  -38db  -53db  -15.5db  -11.90db
10  0.40  0.16  1.00   1.00   32  -38db  -53db  - 3.5db  - 0.06db
11  1.00  1.00  1.00   1.00   32  -30db  -45db  - 3.5db  - 0.19db

Notes:
#1 -  FFT Signal Peak = 10*log10(signal power) = 10*log10(.25) = 6.02db but displays as -9.5 due to FFT bin distribution
	If the entire signal was in a single bin, the FFT peak would match the expected, but assume its in 2 bins due to
	spectral leakage.  Then, just like noise, the expected peak has to be normalized across the number of bins - 2.
	So 10*log10(signalPower / 2) = -9.03db
	If it crossed 3 bins, 10*log10(signalPower / 3) = -10.79db
	So the displayed FFT peak will always be around 3db less than what would be expected for a given test signal
#1 -  FFT Noise Pre = 10*log10(noise power/bins) = 10*log10(.01/1024) = -50.1db (see below for just noise tests)
#2  - RMS = 20*log10(signal amp) = 20*log10(.5) = 6.02db
#6  - 5 stages * 3db per stage = 15db delta pre vs post
#7  - 2x noise amp = 4x noise power = +6db
#8  - 2x noise amp = 4x noise power = +6db
#9  - 1/2 signal amp = 1/4 signal power = -6db peak and -6db rms
#10 - FS generated signal = full scale (0db) RMS because signal full scale
#11 - FS generated noise = -30db on FFT plot, not 0db, because noise is uniformly distributed across all freq and FFT bins
	Expected FFT powerDb = 10*log10(Noise power / Number FFT bins) Confirmed with noiseDecimationTest.grc

	Noise Noise  FFT    FFT
	Amp   Power  Bins   Power (db)
	1.0   1.00   1024  -30.10 db
	1.0   1.00   2048  -33.11 db (2x bins = 1/2 power per bin = -3db delta)
	1.0   1.00   4096  -36.12 db
	0.5   0.25   4096  -42.13 db (1/2 noise amp = 1/4 power = -6db delta)

-------------------------------
4/18/16 GNURadio Signal strength test
Similar to noise generator test
2 signal sources added together and viewed with GUISink FFT and RMS block to 10*log10() to GUINumberSink
Amplitude changes were also observed in time domain with GUISink and matched set amplitude peak-to-peak
	ie set F1 Amp = 0.5 Time domain shows -0.5 to +0.5 peak to peak
Blackman-Harris window, 1024 bin FFT

	 F1     F2     F1   F2  RMS
	 Amp    Amp    dB   dB  dB
-------------------------------
1   .00001 .00000  -108 NA -99.99
2   .0001  .00000  - 88 NA -79.99
3   .0156  .00000  - 45 NA -36.13
4   .0312  .00000  - 39 NA -30.11
5   .0625  .00000  - 32 NA -24.00
6   .125   .00000  - 26 NA -18.06
7   .25    .00000  - 21 NA -12.00
8   .50    .00000  - 15 NA - 6.00
9  1.00    .00000  -  9 NA   0.00
Notes:
#1 - Smallest value tested
#2:9 - 2x amplitude = 4x power = 6db relative gain in both FFT and RMS as expected
#9 - Full scale, shows 0db RMS as expected, but -9db on FFT.  GNURadio FFT may not compensate for window type loss
	Switching to Hamming or Rectangular window shows expected 0db on FFT

When we add a 2nd signal, the amplitude of the combined signals is the sum of each
To keep full scale signals +/- 1, we have to keep each signal less than .5
So F1Amp = .50 f2Amp = .50 = 1.0 amplitude (+/- 1.0 peak to peak)

	 F1     F2     F1   F2  RMS
	 Amp    Amp    dB   dB  dB
-------------------------------
1   .50    .0625  -15  -32  -5.95
2   .50    .125   -15  -26  -5.75
3   .50    .25    -15  -21  -5.05
4   .50    .50    -15  -15  -3.00
5   .25    .25    -21  -21  -9.02

Notes:
Adding 2nd signal does not change FFT displayed dB as expected, F1 is the same
RMS is no longer representative of a single test frequency and therefore can't be compared to FT db values.
Since RMS is the total power in the buffer, if our buffer has been filtered to only contain desired signal, it is also
	representative of the relative strength of the signal.  But again can not be compared to the FFT plot of the signal
So Peak and Average readings in Pebble are only accurate when we're feeding a single test signal, otherwise they are only relative
#4 has 2 signals at .50 amplitude and therefore twice the total buffer power of #8 above.  This is reflected in the increase
	from -6db to -3db (3db = 2x power)
#5 is the same, 2 signals at .25 amplitude equates to +3db (-12 to -9)
Adding a 3rd, 4th etc signal should show similar RMS increases

*/

//Called post decimate, but pre bandpass, so we can get a reasonable floor either side of the bandpass filtered buffer
//Then called post bandpass to get signal estimate
CPX* SignalStrength::estimate(CPX *in, quint32 numSamples, bool estNoise, bool estSignal)
{

	if (!updateTimer.isValid()) {
		updateTimer.start(); //First time we're called
	}

	quint32 noiseFudgeDb = -30; //Observed delta between FFT display and noise calculated from time domain, why?

	//Running mean, variance & standard deviation.  Needed for SNR calculations
	//Knuth algorithm which handles a standard deviation which is a small fraction of the mean
	//See Knuth TAOCP vol 2, 3rd edition, page 232
	//for each incoming sample x:
	//    prev_mean = m;
	//    n = n + 1;
	//    m = m + (x-m)/n;
	//    S = S + (x-m)*(x-prev_mean);
	//	standardDev = sqrt(S/n) or sqrt(S/n-1) depending on who you listen to

	//Map to Knuth p216 2nd edition (based on Welford's algorithm)
	//meanCouner = k
	//runningMean = M(k)
	//prevMean = M(k-1)
	//prevS = S(k-1)

	double pwr = 0;
	double S = 0;
	double prevMean = 0;
	double sumOfTheSquares = 0;
	double peak;

	//prev and running are initialized to zero in constructor and reset()
	pwr = DB::power(in[0]); //Special case for first sample
	m_runningMean = pwr;
	m_peakAmplitude = sqrt(pwr);
	sumOfTheSquares = pwr;
	//Rest of samples
	for (int i = 1; i<numSamples; i++) {
		pwr = DB::power(in[i]);
		if (estSignal) {
			sumOfTheSquares += pwr; //DB::power is i^2 + q^2
			peak = sqrt(pwr); //Rhodes and Schwartz
			if (peak > m_peakAmplitude)
				m_peakAmplitude = peak;
		}
		if (estNoise) {
			prevMean = m_runningMean;
			m_runningMean += (pwr - m_runningMean) / i;
			S += (pwr - m_runningMean) * (pwr - prevMean);
		}
	}

	if (estNoise) {
		//Dividing by N-1 (Bessel's correction) returns unbiased variance, dividing by N returns variance across the entire sample set
		m_variance = S / (numSamples - 1);
		m_stdDev = sqrt(m_variance);
		//GNURadio uses 2 * S for noise instead of stdDev
		m_noise = (2 * S) / (numSamples - 1);
		//m_noise = m_stdDev * DB::dBToPower(noiseFudgeDb); //Fudge not applied correctly
		m_floorDb = DB::clip(DB::powerTodB(m_noise));
	}
	if (estSignal) {
		m_peakDb = DB::clip(DB::amplitudeTodB(m_peakAmplitude));

		m_rms = sqrt(sumOfTheSquares / numSamples);
		m_rmsDb = DB::clip(DB::amplitudeTodB(m_rms));

		m_signal = sumOfTheSquares / numSamples;
		m_avgDb = DB::powerTodB(m_signal);

		m_snrDb = DB::powerRatioToDb(m_signal, m_noise); //Same as powerTodB(m_signal/m_noise)
		m_snrDb = qBound(0.0,m_snrDb,120.0);

		//squelch is a form of AGC and should have an attack/decay component to smooth out the response
		//we fudge that by just looking at the average of the entire buffer
		if (m_avgDb < m_squelchDb) {
			CPX::clearCPX(out,numSamples);
		} else {
			CPX::copyCPX(out, in, numSamples);
		}

		if (updateTimer.elapsed() > updateInterval) {
			//Only update when we have new signal, not noise
			emit newSignalStrength(m_peakDb, m_avgDb, m_snrDb, m_floorDb, m_extValue);
			updateTimer.start(); //Reset
		}
		return out;
	}

	return in; //No buffer change
}

//Used for testing with other values
void SignalStrength::setExtValue(double v)
{
	m_extValue = v;
}
