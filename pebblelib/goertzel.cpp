//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "goertzel.h"
#include "math.h"
#include "QtDebug"
#include "firfilter.h"


/*
From SILabs DTMF paper http://www.silabs.com/Support%20Documents/TechnicalDocs/an218.pdf

The Goertzel algorithm works on blocks of ADC samples. Samples are processed as
they arrive and a result is produced every N samples, where N is the number of
samples in each block. If the sampling rate is fixed, the block size determines
the frequency resolution or “bin width” of the resulting power measurement. The
example below shows how to calculate the frequency resolution and time required
to capture N samples:

Sampling rate: 8 kHz
Block size (N): 200 samples
Frequency Resolution: sampling rate/block size = 40 Hz
Time required to capture N samples: block size/sampling rate = 25 ms

The tradeoff in choosing an appropriate block size is between frequency
resolution and the time required to capture a block of samples. Increasing the
output word rate (decreasing block size) increases the “bin width” of the
resulting power measurement. The bin width should be chosen to provide enough
frequency resolution to differentiate between the DTMF frequencies and be able
to detect DTMF tones with reasonable delay. See [2] for additional information
about choosing a block size.

The Goertzel algorithm is derived from the DFT and exploits the periodicity of
the phase factor, exp(-j*2k/N) to reduce the computational complexity
associated with the DFT, as the FFT does. With the Goertzel algorithm only 16
samples of the DFT are required for the 16 tones (\Links\Goertzel Theory.pdf)
Goertzel is a 2nd order IIR filter or a single bin DFT

http://en.wikipedia.org/wiki/Goertzel_algorithm

//Simplest best explanation
http://www.eetimes.com/design/embedded/4024443/The-Goertzel-Algorithm

*/

void CTCSS::Tone::init(const char *d, float f, float s)
{
	strncpy(m_designation,d,2);
	m_freq = f;
	m_spacing = s;
}

#if 0
//From TI DSP Course Chapter 17
//Q15 Math
void Q15NextSample (float sample)
{
    static short delay;
    static short delay_1 = 0;
    static short delay_2 = 0;
    static int N = 0;
    static int Goertzel_Value = 0;
    int prod1, prod2, prod3, R_in, output;
    short input;
    short coef_1 = 0x4A70;		// For detecting 1209 Hz

    //R_in = mcbsp0_read();		// Read the signal in

    input = (short) R_in;
    input = input >> 4;   		// Scale down input to prevent overflow

    prod1 = (delay_1*coef_1)>>14;
    delay = input + (short)prod1 - delay_2;
    delay_2 = delay_1;
    delay_1 = delay;
    N++;

    if (N==206)
    {
        prod1 = (delay_1 * delay_1);
        prod2 = (delay_2 * delay_2);
        prod3 = (delay_1 *  coef_1)>>14;
        prod3 = prod3 * delay_2;
        Goertzel_Value = (prod1 + prod2 - prod3) >> 15;
        Goertzel_Value <<= 4;   	// Scale up value for sensitivity
        N = 0;
        delay_1 = delay_2 = 0;
    }

    output = (((short) R_in) * ((short)Goertzel_Value)) >> 15;

        //mcbsp0_write(output& 0xfffffffe);	// Send the signal out

   return;
  }
#endif

/*
Reference from mstarlabs article
Coding the Goertzel Algorithm

For applications with a real-valued measurement stream, the general complex-valued computations reduce to real-valued computations, and the coding can be implemented like this:

realW = 2.0*cos(2.0*pi*k/N);
imagW = sin(2.0*pi*k/N);

d1 = 0.0;
d2 = 0.0;
for (n=0; n<N; ++n)
  {
    y  = x(n) + realW*d1 - d2;
    d2 = d1;
    d1 = y;
  }
resultr = 0.5*realW*d1 - d2;
resulti = imagW*d1;
The results are the real and imaginary parts of the DFT transform for frequency k.


*/

ToneBit::ToneBit()
{
	m_freq = 0;
	m_bitSamples = 0;
	m_bandwidth = 0;
	m_wn = 0;
	m_wn1 = 0;
	m_wn2 = 0;
	m_nCount = 0;
	m_usePhaseAlgorithm = false;
	m_isValid = false;

	m_calcRunningMean = true;
	m_runningMean = 0;
	m_runningMeanCount = 1; //Avoid div by zero error
	m_S = 0;
	m_stdDev = 0;
	m_variance = 0;
}

/*
	Uses Lyons pg 738, 13.17.1
	Same as Wikipedia, just differnt nomenclature
	Wikipedia algorithm ...
	s_prev = 0
	s_prev2 = 0
	normalized_frequency = target_frequency / sample_rate;
	coeff = 2*cos(2*PI*normalized_frequency);
	for each sample, x[n],
	  s = x[n] + coeff*s_prev - s_prev2;
	  s_prev2 = s_prev;
	  s_prev = s;
	end
	power = s_prev2*s_prev2 + s_prev*s_prev - coeff*s_prev*s_prev2;

*/

//Call setTargetSampleRate before setToneFreq
void ToneBit::setFreq(quint32 freq, quint32 N, quint32 sampleRate)
{
	m_freq = freq;
	m_bitSamples = N;
	m_bandwidth = sampleRate / m_bitSamples;
	//time, in ms, that each bin represents
	m_msPerBit = 1.0/sampleRate * m_bitSamples * 1000;

#if 1
	//Lyons coeff same result as Wikipedia coeff, but Wikipedia not dependent on N
	m_coeff = 2 * cos(TWOPI * m_freq / sampleRate); //Wikipedia
	m_coeff2 = sin(TWOPI * m_freq / sampleRate);
#else
	//Alternate, should be same results
	//Think of the goertzel as 1 bin of an N bin FFT
	//Each bin 'holds' a frequency range, specified by bandwidth or binwidth
	//So for binwidth = 200hz, bin1=0-199, bin2=200-399, bin3=400-599, bin4=600-799
	//If we're looking for 700hz in this example, it would be in the middle of bin 4, perfect!
	//k represents the fft bin that would contain the frequency of interest
	m_k = (float)(m_samplesPerBin * m_freqHz)/(float)(m_gSampleRate) + 0.5; // + 0.5 rounds up

	m_w = TWOPI * m_k / m_samplesPerBin;

	m_realW = 2.0 * cos(m_w);
	m_imagW = sin(m_w);

#endif

}

#if 0
	We can also process complex input, like spectrum data, not just audio
	From Wikipedia ...
	Complex signals in real arithmetic[edit]
	Since complex signals decompose linearly into real and imaginary parts, the Goertzel algorithm can be computed in real arithmetic separately over the sequence of real parts, yielding y_r(n); and over the sequence of imaginary parts, yielding y_i(n). After that, the two complex-valued partial results can be recombined:

	(15) y(n) = y_r(n) + i\ y_i(n)
#endif

bool ToneBit::processSample(CPX cpx)
{
	Q_UNUSED(cpx);
	return false;
}

/*
	Wikipedia algorithm
	s_prev = 0
	s_prev2 = 0
	normalized_frequency = target_frequency / sample_rate;
	coeff = 2*cos(2*PI*normalized_frequency);
	for each sample, x[n],
	  s = x[n] + coeff*s_prev - s_prev2;
	  s_prev2 = s_prev;
	  s_prev = s;
	end
	power = s_prev2*s_prev2 + s_prev*s_prev - coeff*s_prev*s_prev2;

*/
/*
	We can add a window function, just like FFT, to improve results at the low and high edges of the
	bandwidth.  Without this, the results 'ripple', meaning we might get a -80db result at the edge
	then a -70db result further past the edge.  See the test results for low/main/high tones in Gertzel()
	for an example.

	http://www.mstarlabs.com/dsp/goertzel/goertzel.html has a good example of how to use a window
	Hamming Window
		Terms are generated using the formula
		0.54 - 0.46 * cos(2*pi*n/N)

	Pass band spans primary 2 bins plus first neighbors on each side.
	Out of band rejection minimum -43 dB.

	Exact Blackman
		Terms are generated using the formula
		0.426591 - .496561*cos(2*pi*n/N) +.076848*cos(4*pi*n/N)

	Pass band spans primary 2 bins plus first 2 neighbors on each side.
	Out of band rejection minimum -66 dB.
*/

/*
	We also calculate average signal and stdDev (noise) for possible use in threshold
	Todo: Test SNR threshold for Goertzel
*/
//Process a single sample, return true if we've processed enough for a result
bool ToneBit::processSample(double x_n)
{
	if (m_calcRunningMean) {
		double power = x_n*x_n;
		//See Knuth TAOCP vol 2, 3rd edition, page 232
		//for each incoming sample x:
		//    prev_mean = m;
		//    n = n + 1;
		//    m = m + (x-m)/n;
		//    S = S + (x-m)*(x-prev_mean);
		//	standardDev = sqrt(S/n) or sqrt(S/n-1) depending on who you listen to
		double prevMean = 0;
		prevMean = m_runningMean;
		m_runningMean += (power - m_runningMean) / m_runningMeanCount;
		m_S += (power - m_runningMean) * (power - prevMean);
		//Dividing by N-1 (Bessel's correction) returns unbiased variance, dividing by N returns variance across the entire sample set
		m_variance = m_S / (m_runningMeanCount - 1);
		m_stdDev = sqrt(m_variance);
		m_runningMeanCount++;
		//Not tested, here for future use
	}

	//m_wn = x_n + m_wn1 * (2 * cos((TWOPI * m_m )/ N)) - m_wn2; //Lyons 13-80 re-arranged
	//Same as m_wn = x+n + m_coeff * w_wn1 - m_wn2;
	//m_wn = x_n + (m_wn1 * m_coeff) + (m_wn2 * -1); //Same as simplified version below
	m_wn = x_n + (m_wn1 * m_coeff) - m_wn2;

	//Update delay line
	m_wn2 = m_wn1;
	m_wn1 = m_wn;

	double normalize = m_bitSamples / 2;

	if (m_nCount++ >= m_bitSamples) {
		//We have enough samples to calculate a result
		if (m_usePhaseAlgorithm) {
			//Full algorithm with phase
			double re = (0.5 * m_coeff * m_wn1 - m_wn2);
			double im = (0.5 * m_coeff2 * m_wn1 - m_wn2);
			m_power = re * re + im * im;
			m_power /= normalize; //Normalize?
			//Phase compared to prev result phase, for phase change detection
			m_phase = atan2(im, re); //Result not tested or confirmed
		} else {
			//Simplified algorithm without phase
			m_power = (m_wn1 * m_wn1) + (m_wn2 * m_wn2) - (m_wn1 * m_wn2 * m_coeff); //Lyons 13-83
			m_power /= normalize; //Normalize?
			m_phase = 0;
		}
		m_wn1 = m_wn2 = 0;
		m_nCount = 0;

		//m_runningMean = 0;
		//m_S = 0;
		//m_runningMeanCount = 1;
		return true;
	}
	return false;
}

Goertzel::Goertzel(quint32 sampleRate, quint32 numSamples)
{
	m_sampleRate  = sampleRate;
	m_numSamples = numSamples;

	m_thresholdType = TH_COMPARE;
	//Higher thresholds mean stonger signals are required
	//Low thresholds will be more sensitive, but more susceptible to noise and spikes
	//Tested:2, 4, 8(no)
	m_compareThreshold = 4;

	m_debounceCounter = 0;
	m_attackCounter = 0;
	m_decayCounter = 0;

	m_lastTone = false;
	m_mainJitterFilter = NULL;
	m_highJitterFilter = NULL;
	m_lowJitterFilter = NULL;
}

Goertzel::~Goertzel()
{
	if (m_mainJitterFilter != NULL)
		delete m_mainJitterFilter;
	if (m_lowJitterFilter != NULL)
		delete m_lowJitterFilter;
	if (m_highJitterFilter != NULL)
		delete m_highJitterFilter;
}

void Goertzel::setThresholdType(Goertzel::ThresholdType t)
{
	m_thresholdType = t;
}

void Goertzel::setThreshold(double threshold)
{
	m_compareThreshold = threshold;
}

/*
	Returns an optimal bin size
	Determine an optimum bin width, or number of samples needed for each result
	Lyons: The larger N is, the better the frequency resolution and noise immunity,
		but the more calculations and CPU and the longer it takes to return each result.

	Banks http://www.embedded.com/design/configurable-systems/4024443/The-Goertzel-Algorithm
	Goertzel block size N is like the number of points in an equivalent FFT.
	It controls the frequency resolution (also called bin width).
	For example, if your sampling rate is 8kHz and N is 100 samples, then your bin width is 80Hz.
	Ideally you want the frequencies to be centered in their respective bins.
	In other words, you want the target frequencies to be integer multiples of sample_rate/N.

	For morse code, the shortest 'bit' is 100ms @ 12wpm and 10ms @120wpm for example.  See morse.cpp
	For morse code, typical bandwidths run from 50hz to 1000hz
	See morse.cpp

*/

//Returns an estimated N based on shortest bit length
quint32 Goertzel::estNForShortestBit(double msShortestBit)
{
	//How many ms does each sample represent?
	//ie for 8k sample rate and 2048 buffer, each sample is .125ms
	double msPerSample = 1e3 / m_sampleRate; //result in ms, not seconds
	//N has to be less than the shortest 'bit' or we lose data
	//ie maxN for 120wpm morse = 10ms / .125ms = 80
	return msShortestBit / msPerSample;
}

//Returns an estimated N based on bin bandwidth
quint32 Goertzel::estNForBinBandwidth(quint32 bandwidth)
{
	//N has to be large enough for desired bandwidth
	//ie minN for bandwidth of 100 = 100 / 3.90 = 25.6
	double binWidth = m_sampleRate / m_numSamples;
	return bandwidth / binWidth;
}

void Goertzel::setTargetSampleRate(quint32 targetSampleRate)
{
	//Testing without decimation, caller decimates
	m_sampleRate = targetSampleRate;
}

void Goertzel::setFreq(quint32 freq, quint32 N, quint32 jitterCount)
{
	m_attackCount = jitterCount;
	m_attackCounter = 0;
	m_decayCount = jitterCount;
	m_decayCounter = 0;
	m_jitterCount = jitterCount;

	// bit filter based on 10 msec rise time of CW waveform
	//int bfv = (int)(m_sampleRate * .010 / N); //8k*.01/16 = 5
	m_mainJitterFilter = new MovingAvgFilter(m_jitterCount);
	m_lowJitterFilter = new MovingAvgFilter(m_jitterCount);
	m_highJitterFilter = new MovingAvgFilter(m_jitterCount);

	/*

		KA7OEI differential goertzel algorithm compares 5% below and 4% above bins
		But comparison seems to work better with non-overlapping bins.
		Tested with bw, bw/2, bw/4 using signal strength meter and receiver.cpp processIq() testing

		We use 3 bins.  If main bin is 200hz wide and centered at 800hz
					  Low		    Main          High
							  |---- 200 ----|
				|---- 200 ----|             |----- 200 -----|
			   500hz  600hz  700hz  800hz  900hz  1000hz  1100hz   db over avg
		  Mixer
		-----------------------------------------------------------------
		1: 8500      -31db         - 40db         - 43db           4db
		2: 8600      -28db(1)      -106db(2)      -106db(2)
								   - 36db(4)      - 42.4(4)
		3: 8700      -31db(3)      - 32db(3)      - 42db           4db
		4: 8800      -78db(2)      - 28db(1)      - 78db(2)
					 -36db(4)                     - -36db(4)       8db
		5: 8900      -42db         - 32db(3)      - 31db(3)        4db
		6: 9000      -78db(2)      - 78db(2)      - 28db(1)
					 -43db(4)      - 36db(4)                       2db
		7: 9100      -44db         - 40db         - 31db           4db
		Test results with TestBench generating an 8khz signal at -40db, lsb, SR 8000, N=44
		(1) Highest db of -28 is at center frequency for each bin, as expected.
		(2) Results at the edge of each bin are not accurate without windowing
		(3) Results right between 2 bins give equal results for each bin
		(4) Estimated accurate result if we used windowing
	*/
	m_mainTone.setFreq(freq, N, m_sampleRate);
	quint32 bwDelta = m_mainTone.m_bandwidth *.75;
	m_highCompareTone.setFreq(freq + bwDelta, N, m_sampleRate);
	m_lowCompareTone.setFreq(freq - bwDelta, N, m_sampleRate);
	//KA7OEI differential goertzel algorithm compares 5% below and 4% above bins
	//m_lowCompareTone.setFreq(freq * 0.95, N, m_internalSampleRate);
	//m_highCompareTone.setFreq(freq * 1.04, N, m_internalSampleRate);
}

/*
	Just like a switch, the threshold between a tone being on and off is subject to a bounce effect
	debounce() gives us some control over how fast an off-tone is recognized as on-tone (m_attackCount) and
	how fast an on-tone is recognized as off-tone (m_decayCount)

	Debounce examples: m_attackCount = 2, m_decayCount = 2;
	B = bitAboveThreshold, A = m_attackCount, D = m_decayCount, T = resulting tone on/off

	Clean signal, no noise.
	Note that timing is correct, but delayed by 1 bit on attack and decay
	B: 0 0 0 0  1 1 1 1  1 1 1 1  0 0 0 0  0 0 0 0
	A: 0 0 0 0  1 2 2 2  2 2 2 2  1 0 0 0  0 0 0 0
	D: 1 2 2 2  0 0 0 0  0 0 0 0  1 2 2 2  2 2 2 2
	T: 0 0 0 0  0 1 1 1  1 1 1 1  1 0 0 0  0 0 0 0

	Alternating bits example (noise)
	B: 0 1 0 1  0 1 0 1
	A: 0 1 0 1  0 1 0 1
	D: 1 0 1 0  1 0 1 0
	T: 0 0 0 0  0 0 0 0 attackCount never gets above 2

	Tone with short drops
	B: 0 0 0 0  1 1 1 1  0 1 0 1  0 0 0 0
	A: 0 0 0 0  1 2 2 2  0 1 0 1  0 0 0 0
	D: 1 2 2 2  0 0 0 0  1 0 1 0  1 2 2 2
	T: 0 0 0 0  0 1 1 1  1 1 1 1  1 0 0 0 Tone is on until decayCounter==2

	Random noise example, but where 3 out of 4 bits could be recognized as a tone
	B: 0 1 1 0  1 1 1 0  0 1 1 1  0 0 0 0
	A: 0 1 2 0  1 2 2 0  0 1 2 2  0 0 0 0
	D: 1 0 0 1  0 0 0 1  2 0 0 0  1 2 2 2
	T: 0 0 1 1  1 1 1 1  0 0 1 1  1 0 0 0

	Same example with decayCount = 3
	B: 0 1 1 0  1 1 1 0  0 1 1 1  0 0 0 0
	A: 0 1 2 0  1 2 2 0  0 1 2 2  0 0 0 0
	D: 1 0 0 1  0 0 0 1  2 0 0 0  1 2 3 3
	T: 0 0 1 1  1 1 1 1  0 0 1 1  1 1 0 0 Last tone is 1 bit longer than decayCount = 2
*/
//Replaced with moving average filter, here for reference during testing
bool Goertzel::debounce(bool aboveThreshold)
{
	bool isTone = false;
	if (aboveThreshold) {
		m_attackCounter++;
		m_decayCounter = 0;
		//If lastTone was off, do we have enough 'on' bits to say we have an 'on' tone?
		if (!m_lastTone && m_attackCounter >= m_attackCount) {
			isTone = true;
		} else {
			//Even though we have aboveThreshold, not enough sequential on bits
			isTone = m_lastTone;
		}
		//Keep at threshold
		if (m_attackCounter >= m_attackCount)
			m_attackCounter = m_attackCount;
	} else {
		m_decayCounter++;
		m_attackCounter = 0;
		//Do we have enough 'off' bits to say the tone is 'off'?
		if (m_lastTone && m_decayCounter >= m_decayCount) {
			//Transition from on to off
			isTone = false;
		} else {
			//Tone is still on. Even though we are not aboveThreshold, we don't have enough 'off bits'
			isTone = m_lastTone; //No change
		}
		//Keep at threshold
		if (m_decayCounter >= m_decayCount)
			m_decayCounter = m_decayCount;
	}
	m_lastTone = isTone;
	return isTone;
}

/*
	Excellent practical information from KA7OE1 on mcHf SDR DTMF implementation
		http://ka7oei.blogspot.com/2015/11/fm-squelch-and-subaudible-tone.html
	"Differential Goertzel" detection:

	I chose to use a "differential" approach in which I set up three separate
	Goertzel detection algorithms: One operating at the desired frequency, another
	operating at 5% below the desired frequency and the third operating at 4% above
	the desired frequency and processed the results as follows:

		Sum the amplitude results of the -5% Goertzel and +4% Goertzel detections.
		Divide the results of the sum, above, by two.
		Divide the amplitude results of the on-frequency Goertzel by the above sum.

	RL Note: Setting with +/- freq doesn't take into account the bin width, ie the higher and lower
	freq may still be in the main bin.  Try setting based on bin bandwidth.  +/- 50% of bandwidth maybe.

	The result is a ratio, independent of amplitude, that indicates the amount of
	on-frequency energy. In general, a ratio higher than "1" would indicate that
	"on-frequency" energy was present. A threshold of 1.75 is a good place to start.
	By having the two additional Goertzel detectors (above and below) frequency we accomplish
	several things at once:

		*We obtain a "reference" amplitude that indicates how much energy there is that is
		not on the frequency of the desired tone as a basis of comparison.
		*By measuring the amplitude of adjacent frequencies the frequency discrimination capability
		of the decoder is enhanced without requiring narrower detection bandwidth and the necessarily
		"slower" detection response that this would imply.

	In the case of the last point, above, if we were looking for a 100 Hz tone and
	a 103 Hz tone was present, our 100 Hz decoder would "weakly-to-medium" detect
	the 103 Hz tone as well but the +4% decoder (at 104 Hz) would more strongly
	detect it, but since its value is averaged in the numerator it would reduce the
	ratiometric output and prevent detection.

*/
//Process one sample at a time
//3 states: Not enough samples for result, result below threshold, result above threshold
bool Goertzel::processSample(double x_n, double &retPower)
{
	//Caller is responsible for decimation, if needed

	//All the tones have the same N, just check the result of the main
	bool result = m_mainTone.processSample(x_n);
	m_highCompareTone.processSample(x_n);
	m_lowCompareTone.processSample(x_n);
	m_mainTone.m_isValid = false;
	retPower = 0;
	double mainPower = 0;
	double highPower = 0;
	double lowPower = 0;
	double bufMean = 0;
	double bufDev = 0;
	double snr = 0;
	if (result) {
		//If we have a detectable tone, it should be strong in the main bin compared with surrounding bins

		//Handle jitter
		//Todo: move jitter process to ToneBit?
		mainPower = m_mainJitterFilter->newSample(m_mainTone.m_power);
		lowPower = m_lowJitterFilter->newSample(m_lowCompareTone.m_power);
		highPower = m_highJitterFilter->newSample(m_highCompareTone.m_power);
		bufDev = m_mainTone.m_stdDev;
		bufMean = m_mainTone.m_runningMean;
		snr = mainPower/bufMean;

#if 1
		if (false)
			qDebug()<<"Power:"<<
					  DB::powerTodB(mainPower)<<" Mean:"<<
					  DB::powerTodB(bufMean)<<" Dev:"<<
					  DB::powerTodB(bufDev);
		if (snr > 2 )
			retPower = mainPower;

#else
		//lowPower = m_lowCompareTone.m_power;
		//highPower = m_highCompareTone.m_power;

		double avgComparePower = (highPower + lowPower) / 2;

		//KA7OEI differential goertzel
		double compareRatio = 0;
		if (avgComparePower > 0)
			//Avoid dev by zero for Nan
			compareRatio = mainPower / avgComparePower;

		if (false)
			qDebug()<<"Low:"<<
					  DB::powerTodB(lowPower)<<" Main:"<<
					  DB::powerTodB(mainPower)<<" High:"<<
					  DB::powerTodB(highPower)<<" Ratio:"<<
					  compareRatio;

		//if (debounce(compareRatio > m_compareThreshold)) {
		if (compareRatio > m_compareThreshold) {
			retPower = mainPower;
		}
#endif
		return true; //Valid result

	} //End if (result)
	return false; //Needs more samples
}

/*
  Several algorithms for determining the threshold
  1. Explicit: User adjusts slider or something.  Ughh
  2. Dynamic Median:  Look for lowest and highest power returned over some interval and split
  3. Dynamic Comparison: Process 2 goertzel filters some freqency apart and compare results.  Signal should only be
  in one or the other, not both.  Similar to the technique used by SiLabs paper on DTMF detection.
  If power in 1 filter is 8x greater than sum of all the others, that filter must have a tone
  Note: Tried comparison with 1 other filter value and could not achieve consistent results
*/

void Goertzel::updateToneThreshold()
{
	switch (m_thresholdType) {
		case TH_COMPARE:
			break;
		case TH_AVERAGE:
#if 0
		//From old Goertzel
			//WIP What should binary threshold be initialized to?
			m_binaryOutput = (power > m_binaryThreshold) ? true : false;

			//Only update average on transition and only if binaryOutput same for at least 3 results
			if (m_binaryOutput == m_lastBinaryOutput) {
				//No transition, keep counting
				m_resultCounter++;
				if (m_binaryOutput)
					m_avgTonePower = (power * .01) + (m_avgTonePower * 0.99);
				else
					m_avgNoisePower = (power * .01) + (m_avgNoisePower * 0.99);

			} else {
				//Transition, see if it was long enough to average
				//Only update average if we have N consecutive marks or spaces
				if (m_resultCounter > 3) {
					//We want our threshold to adjust to some percentage of average
					//Calc average over N seconds, based on current settings
					m_binaryThreshold = (m_avgTonePower - m_avgNoisePower) / 2.0;
				}
				m_resultCounter = 0;
				m_lastBinaryOutput = m_binaryOutput;
			}
			//Timeout if we're constantly above or below threshold
			if (m_resultCounter > (2000 / m_timePerBin)) {
				m_resultCounter = 0;
				//Or should we just reset to default values??
				m_binaryThreshold *= 0.5; //Cut in half and try again
				m_avgTonePower = m_binaryThreshold * 1.5;
				m_avgNoisePower = m_binaryThreshold * 0.5;
			}


#endif
			break;
		case TH_MIN_MAX:
#if 0
		//From old Goertzel
			//Only check ever n sec
			if (++m_resultCounter < 1000/m_timePerBin)
				return true;
			m_resultCounter = 0;

			//Initial values such that first result will always be > max and < min
			m_maxPower = 0.0;
			m_minPower = 1.0;

			m_minSample = 1.0;
			m_maxSample = -1.0;

			//Todo: Investigate running min/max and running median algorithms
			//This adjusts for varying signal strength and should allow weak signals to still have a clear
			//threshold between tone or no-tone
			//Find min/max in last N results
			//We don't know state of circular buffer, so just check all
			for (int i = 0; i<m_powerBufSize; i++) {
				power = (*m_powerBuf)[i].re;
				if (power > m_maxPower)
					m_maxPower = power;
				else if (power>0 && power < m_minPower)
					m_minPower = power;
			}

			//We need at least some variation in results in order to determine valid threshold
			//If we're just sitting on noise, we'll have a min/max, but delta won't be enough for threshold
			//So we need a min delta in order to reliably detect tone presence
			//Some example Min values: 0.000000010636
			//Min 0.000000000040 Max 0.000472313695
			//Min 0.000000000048 Max 0.000000665952 Tone or noise? mag 4 diff
			//Min 0.000000000040 Max 0.000442085729 Clearly a tone mag 7 diff
			//Min 0.000000007617 Max 0.000403082959 Mag 5 diff
			//Min 0.000000010636 Max 0.000361439010
			//Just noise on 40m
			//Min 0.000000001696 Max 0.000012756922
			//Noise on 20m
			//Min 0.000000186417 Max 0.000070004084
			//Signal on 20m
			//Min 0.000000069135 Max 0.00311974407
			m_binaryThreshold = m_maxPower * 0.60; //n% of max above min

			return true;

#endif
			break;
		case TH_MANUAL:
			break;
		default:
			break;
	}
}

void Goertzel::updateBitDetection()
{

}

//Todo:  Look at realtime algorithms from https://netwerkt.wordpress.com/2011/08/25/goertzel-filter/
#if 0
Efficiently detecting a frequency using a Goertzel filter
Posted on August 25, 2011 by Wilfried Elmenreich

The Goertzel algorithm detects a specific, predetermined frequency in a signal.
This can be used to analyze a sound source for the presence of a particular
tone. The algorithm is simpler than an FFT and therefore a candidate for small
embedded systems.

With the following C code you can analyze an array of samples for a given frequency:

double goertzelFilter(int samples[], double freq, int N) {
	double s_prev = 0.0;
	double s_prev2 = 0.0;
	double coeff,normalizedfreq,power,s;
	int i;
	normalizedfreq = freq / SAMPLEFREQUENCY;
	coeff = 2*cos(2*M_PI*normalizedfreq);
	for (i=0; i<N; i++) {
		s = samples[i] + coeff * s_prev - s_prev2;
		s_prev2 = s_prev;
		s_prev = s;
	}
	power = s_prev2*s_prev2+s_prev*s_prev-coeff*s_prev*s_prev2;
	return power;
}

However, there are two issues with that approach: first, the samples need to be
stored, which requires a lot of RAM memory that might not be easily available
on a microcontroller. Second, the detection of a signal might be delayed until
the sample buffer is full and gets analyzed. As an improvement, we can
formulate the filter also as a real time algorithm that analyzes one sample at
a time:

double RTgoertzelFilter(int sample, double freq) {
	static double s_prev = 0.0;
	static double s_prev2 = 0.0;
	static double totalpower = 0.0;
	static int N = 0;
	double coeff,normalizedfreq,power,s;
	normalizedfreq = freq / SAMPLEFREQUENCY;
	coeff = 2*cos(2*M_PI*normalizedfreq);
	s = sample + coeff * s_prev - s_prev2;
	s_prev2 = s_prev;
	s_prev = s;
	N++;
	power = s_prev2*s_prev2+s_prev*s_prev-coeff*s_prev*s_prev2;
	totalpower += sample*sample;
	if (totalpower == 0) totalpower=1;
	return power / totalpower / N;
}

Note that the initialization of the static variables takes place only at the
first time when the function is called. The return value has been normalized
using the total power and number of samples. This filter delivers a result
after each sample without storing the samples, but it considers the whole
history of the signal. If you want to detect the sudden presence of a tone, it
is better to limit the history of the filter. This can be done using the
tandemRTgoertzelFilter:

double tandemRTgoertzelFilter(int sample, double freq) {
	static double s_prev[2] = {0.0,0.0};
	static double s_prev2[2] = {0.0,0.0};
	static double totalpower[2] = {0.0,0.0};
	static int N=0;
	double coeff,normalizedfreq,power,s;
	int active;
	static int n[2] = {0,0};
	normalizedfreq = freq / SAMPLEFREQUENCY;
	coeff = 2*cos(2*M_PI*normalizedfreq);
	s = sample + coeff * s_prev[0] - s_prev2[0];
	s_prev2[0] = s_prev[0];
	s_prev[0] = s;
	n[0]++;
	s = sample + coeff * s_prev[1] - s_prev2[1];
	s_prev2[1] = s_prev[1];
	s_prev[1] = s;
	n[1]++;
	N++;
	active = (N / RESETSAMPLES) & 0x01;
	if  (n[1-active] >= RESETSAMPLES) { // reset inactive
		s_prev[1-active] = 0.0;
		s_prev2[1-active] = 0.0;
		totalpower[1-active] = 0.0;
		n[1-active]=0;
	}
	totalpower[0] += sample*sample;
	totalpower[1] += sample*sample;
	power = s_prev2[active]*s_prev2[active]+s_prev[active]
	  * s_prev[active]-coeff*s_prev[active]*s_prev2[active];
	return power / (totalpower[active]+1e-7) / n[active];
}

The tandem filter is the combination of two real-time filters, which are reset
alternatively every RESETSAMPLES. Except for the first RESETSAMPLES, the active
filter always has a history between RESETSAMPLES and 2 * RESETSAMPLES samples.
Meanwhile the inactive filter is building up its history again. This is
necessary because the algorithm needs some time to reach a steady state. In my
test runs, I successfully used a value of 200 samples for RESETSAMPLES in order
to detect a 440 Hz signal in a 44kHz audio sample. Even for an 8 bit
Microcontroller without an FPU, the tandem implementation is fast enough. For
high performance computation, I further recommend to translate the algorithm to
fixed point arithmetic.

#endif

//From Nue-PSK for reference
NuePskGoertzel::NuePskGoertzel()
{

	// The Goretzel sample block size determines the CW bandwidth as follows:
	// CW Bandwidth :  100, 125, 160, 200, 250, 400, 500, 800, 1000 Hz.
	//cw_bwa[] = {80,  64,  50,  40,  32,  20,  16,  10,    8};
	//cw_bwa_index = 3;
	cw_n =  40; // Number of samples used for Goertzel algorithm
	//For sample rate = 8000
	cw_f = 796.8750;		// CW frequency offset (start of bin)
	g_coef = 1.6136951071;	// 2*cos(PI2*(cw_f+7.1825)/SAMPLING_RATE);
	q0 = 0;
	q1 = 0;
	q2 = 0;
	current = 0;
	g_t_lock = 0;			// Goertzel threshold lock
	cspm_lock = 0;			// Character SPace Multiple lock
	wspm_lock = 0;			// Word SPace Multiple lock
	g_sample_count = 0;
	g_sample = 0;
	g_current = 0;
	g_scale_factor = 1000.0;
	g_s = 0;
	g_ra = 10000;
	g_threshold = 1000;
	do_g_ave = 0;
	g_dup_count = 0;
	preKey = 0;
	//RL fixes
	RXKey = 0;
	last_trans = 0;
	cwPractice = 0;
}

void NuePskGoertzel::do_goertzel (qint16 f_samp)
{
	//(fractional f_samp){

	g_sample = (((double)f_samp)/32768.0);
	q0 = g_coef*q1 - q2 + g_sample;
	q2 = q1;
	q1 = q0;

	if(!cwPractice && (++g_sample_count >= cw_n)){
		g_sample_count = 0;
		cw_n = cw_bwa[cw_bwa_index];
		g_current = q1*q1 + q2*q2 - q1*q2*g_coef; 	// ~ energy^2
		q2 = 0;
		q1 = 0;
		g_s = (long)(g_current*g_scale_factor);	//scale and convert to long
		preKey = (g_s > g_threshold) ? 1 : 0;		//sample MARK or SPACE?
		if(preKey == RXKey){		//state transition?
			last_trans = 0;			//no, clear timer
			g_dup_count++;			//and count duplicate
		}else{
			g_dup_count = 0;		//yes, clear duplicate count
									//and let timer run
		}
		//Change receive "key" if last change was over 15 clocks ago.
		//This adds some of latency but should not have a major
		//affect on durations.
		if(last_trans > 15){
			RXKey = preKey;
			last_trans = 0;
		}
		//add sample to running average if no change in 3 samples
		if(g_dup_count > 3){
			do_g_ave = 1;
			g_ra = (999*g_ra + g_s)/1000;
		}
		if(g_t_lock){
			g_ra = g_threshold;
		}else{
			g_threshold = g_ra > 200 ? g_ra : 200;
		}
	}
}

//For future experimentation
/*
DTMF Tone Pairs to test algorithm
		1209hz  1336hz  1477hz  1633hz
697hz   1       2       3       A
770hz   4       5       6       B
852hz   7       8       9       C
941hz   *       0       #       D

For N=205 and fs=8000
Freq    k       Coeff           Coeff
				decimal         Q15
697     18      1.703275        0x6D02
770     20      1.635585        0x68B1
852     22      1.562297        0x63FC
941     24      1.482867        0x5EE7
1209    31      1.163138        0x4A70
1336    34      1.008835        0x4090
1477    38      0.790074        0x6521
1633    42      0.559454        0x479C

Note: Q15 format means that 15 decimal places are assumed.
Used for situations where there is no floating point or only integer math is used.
See http://en.wikipedia.org/wiki/Q_(number_format)

*/

DTMF::DTMF()
{
	DTMF_0.init(1336,941);
	DTMF_1.init(1209,697);
	DTMF_2.init(1336,697);
	DTMF_3.init(1477,697);
	DTMF_4.init(1209,770);
	DTMF_5.init(1336,770);
	DTMF_6.init(1477,770);
	DTMF_7.init(1209,852);
	DTMF_8.init(1336,852);
	DTMF_9.init(1477,852);
	DTMF_A.init(1633,697);
	DTMF_B.init(1633,770);
	DTMF_C.init(1633,852);
	DTMF_D.init(1633,941);
	DTMF_STAR.init(1209,941);
	DTMF_POUND.init(1477,941);
}

void DTMF::Tone::init(quint16 h, quint16 l)
{
	m_hi = h;
	m_lo = l;
}

CTCSS::CTCSS()
{
	CTCSS_1.init("XY",67.0,0);
	CTCSS_2.init("XA",71.9,4.9);
	CTCSS_3.init("WA",74.4,2.5);
	CTCSS_4.init("??",0,0);//Not defined??
	CTCSS_5.init("SP",79.7,2.7);
	CTCSS_6.init("YZ",82.5,2.8);
	CTCSS_7.init("YA",85.4,2.9);
	CTCSS_8.init("YB",88.5,3.1);
	CTCSS_9.init("ZZ",91.5,3.0);
	CTCSS_10.init("ZA",94.8,3.3);
	CTCSS_11.init("ZB",97.4,2.6);
	CTCSS_12.init("1Z",100.0,2.6);
	CTCSS_13.init("1A",103.5,3.5);
	CTCSS_14.init("1B",107.2,3.7);
	CTCSS_15.init("2Z",110.9,3.7);
	CTCSS_16.init("2A",114.8,3.9);
	CTCSS_17.init("2B",118.8,4.0);
	CTCSS_18.init("3Z",123.0,4.2);
	CTCSS_19.init("3A",127.3,4.3);
	CTCSS_20.init("3B",131.8,4.5);
	CTCSS_21.init("4Z",136.5,4.7);
	CTCSS_22.init("4A",141.3,4.8);
	CTCSS_23.init("4B",146.2,4.9);
	CTCSS_24.init("5Z",151.4,5.2);
	CTCSS_25.init("5A",156.7,5.3);
	CTCSS_26.init("5B",162.2,5.5);
	CTCSS_27.init("6Z",167.9,5.7);
	CTCSS_28.init("6A",173.8,5.9);
	CTCSS_29.init("6B",179.9,6.1);
	CTCSS_30.init("7Z",186.2,6.3);
	CTCSS_31.init("7A",192.8,6.6);
	CTCSS_32.init("M1",203.5,10.7);
}

