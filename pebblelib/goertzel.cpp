//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "goertzel.h"
#include "math.h"
#include "QtDebug"
#include "firfilter.h"

const DTMF DTMF::DTMF_0 = DTMF(1336,941);
const DTMF DTMF::DTMF_1 = DTMF(1209,697);
const DTMF DTMF::DTMF_2 = DTMF(1336,697);
const DTMF DTMF::DTMF_3 = DTMF(1477,697);
const DTMF DTMF::DTMF_4 = DTMF(1209,770);
const DTMF DTMF::DTMF_5 = DTMF(1336,770);
const DTMF DTMF::DTMF_6 = DTMF(1477,770);
const DTMF DTMF::DTMF_7 = DTMF(1209,852);
const DTMF DTMF::DTMF_8 = DTMF(1336,852);
const DTMF DTMF::DTMF_9 = DTMF(1477,852);
const DTMF DTMF::DTMF_A = DTMF(1633,697);
const DTMF DTMF::DTMF_B = DTMF(1633,770);
const DTMF DTMF::DTMF_C = DTMF(1633,852);
const DTMF DTMF::DTMF_D = DTMF(1633,941);
const DTMF DTMF::DTMF_STAR = DTMF(1209,941);
const DTMF DTMF::DTMF_POUND = DTMF(1477,941);

const CTCSS CTCSS::CTCSS_1 = CTCSS("XY",67.0,0);
const CTCSS CTCSS::CTCSS_2 = CTCSS("XA",71.9,4.9);
const CTCSS CTCSS::CTCSS_3 = CTCSS("WA",74.4,2.5);
const CTCSS CTCSS::CTCSS_4 = CTCSS("??",0,0);//Not defined??
const CTCSS CTCSS::CTCSS_5 = CTCSS("SP",79.7,2.7);
const CTCSS CTCSS::CTCSS_6 = CTCSS("YZ",82.5,2.8);
const CTCSS CTCSS::CTCSS_7 = CTCSS("YA",85.4,2.9);
const CTCSS CTCSS::CTCSS_8 = CTCSS("YB",88.5,3.1);
const CTCSS CTCSS::CTCSS_9 = CTCSS("ZZ",91.5,3.0);
const CTCSS CTCSS::CTCSS_10 = CTCSS("ZA",94.8,3.3);
const CTCSS CTCSS::CTCSS_11 = CTCSS("ZB",97.4,2.6);
const CTCSS CTCSS::CTCSS_12 = CTCSS("1Z",100.0,2.6);
const CTCSS CTCSS::CTCSS_13 = CTCSS("1A",103.5,3.5);
const CTCSS CTCSS::CTCSS_14 = CTCSS("1B",107.2,3.7);
const CTCSS CTCSS::CTCSS_15 = CTCSS("2Z",110.9,3.7);
const CTCSS CTCSS::CTCSS_16 = CTCSS("2A",114.8,3.9);
const CTCSS CTCSS::CTCSS_17 = CTCSS("2B",118.8,4.0);
const CTCSS CTCSS::CTCSS_18 = CTCSS("3Z",123.0,4.2);
const CTCSS CTCSS::CTCSS_19 = CTCSS("3A",127.3,4.3);
const CTCSS CTCSS::CTCSS_20 = CTCSS("3B",131.8,4.5);
const CTCSS CTCSS::CTCSS_21 = CTCSS("4Z",136.5,4.7);
const CTCSS CTCSS::CTCSS_22 = CTCSS("4A",141.3,4.8);
const CTCSS CTCSS::CTCSS_23 = CTCSS("4B",146.2,4.9);
const CTCSS CTCSS::CTCSS_24 = CTCSS("5Z",151.4,5.2);
const CTCSS CTCSS::CTCSS_25 = CTCSS("5A",156.7,5.3);
const CTCSS CTCSS::CTCSS_26 = CTCSS("5B",162.2,5.5);
const CTCSS CTCSS::CTCSS_27 = CTCSS("6Z",167.9,5.7);
const CTCSS CTCSS::CTCSS_28 = CTCSS("6A",173.8,5.9);
const CTCSS CTCSS::CTCSS_29 = CTCSS("6B",179.9,6.1);
const CTCSS CTCSS::CTCSS_30 = CTCSS("7Z",186.2,6.3);
const CTCSS CTCSS::CTCSS_31 = CTCSS("7A",192.8,6.6);
const CTCSS CTCSS::CTCSS_32 = CTCSS("M1",203.5,10.7);

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

*/

Goertzel::Goertzel(int _sampleRate, int _numSamples)
{
	m_sampleRate = _sampleRate;
	m_numSamples = _numSamples;

	m_realW = 0;
	m_binaryOutput = false;
	m_binaryThreshold = 0;
	m_noiseThreshold = 0.5;
	m_delay0 = m_delay1 = m_delay2 = 0;
	m_sampleCount = 0;
	m_binWidthHz = 0;
	m_samplesPerBin = 0;
	m_avgTonePower = 0;
	m_peakPower = 0;
	m_scale = 1000; //So we don't lose precision?
	m_powerBuf = NULL;
	m_windowFunction = NULL;
}

Goertzel::~Goertzel()
{
	if (m_powerBuf != NULL)
		delete (m_powerBuf);
}

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

Nue-Psk coefficient for 796.875hz is 1.6136951071 as another example
*/

/*
The Goertzel algorithm is derived from the DFT and exploits the periodicity of
the phase factor, exp(-j*2k/N) to reduce the computational complexity
associated with the DFT, as the FFT does. With the Goertzel algorithm only 16
samples of the DFT are required for the 16 tones (\Links\Goertzel Theory.pdf)
Goertzel is a 2nd order IIR filter or a single bin DFT

http://en.wikipedia.org/wiki/Goertzel_algorithm

//Simplest best explanation
http://www.eetimes.com/design/embedded/4024443/The-Goertzel-Algorithm
*/


//fTone is the freq in hz we want to detect
//SampleRate 48000, 96000, etc
//Test with Freq from DTMF and sampleRate 8000 and compare with table above
//returns numToneResults for sizing toneBuffer
int Goertzel::setFreqHz(int fTone, int bw, int gsr)
{
	m_freqHz = fTone;
	m_gSampleRate = gsr;
	m_decimateFactor = m_sampleRate / m_gSampleRate;

	m_binWidthHz = bw; //equivalent to filter bandwidth
    //Set blockSize based on sampleRate and desired filter bandwidth
    //How many samples do we need to accumulate to get desired bandwidth
	m_samplesPerBin = m_gSampleRate / m_binWidthHz;

    //What is the time, in ms, that each bin represents
	m_timePerBin = 1.0/m_gSampleRate * m_samplesPerBin * 1000;

    //How many tone results per sample buffer = num decimated samples / samples per bin
	m_numToneResults = (m_numSamples / m_decimateFactor) / m_samplesPerBin;

    //Calc the filter coefficient
    //Think of the goertzel as 1 bin of an N bin FFT
    //Each bin 'holds' a frequency range, specified by bandwidth or binwidth
    //So for binwidth = 200hz, bin1=0-199, bin2=200-399, bin3=400-599, bin4=600-799
    //If we're looking for 700hz in this example, it would be in the middle of bin 4, perfect!
    //k represents the fft bin that would contain the frequency of interest
	m_k = (float)(m_samplesPerBin * m_freqHz)/(float)(m_gSampleRate) + 0.5; // + 0.5 rounds up

	m_w = TWOPI * m_k / m_samplesPerBin;

	m_realW = 2.0 * cos(m_w);
	m_imagW = sin(m_w);

    //qDebug("Goertzel: freq = %d binWidth = %d sampleRate = %d samplesPerBin = %d coefficient = %.12f timePerBin = %d",
    //       freqHz,binWidthHz,sampleRate,samplesPerBin,coeff,timePerBin);

	m_avgTonePower = 0.001;
	m_avgNoisePower = 0.000001;
	m_binaryThreshold = (m_avgTonePower - m_avgNoisePower) / 2.0;

	m_noiseThreshold = 1.0/(m_scale*1000); //Lowest possible value for binaryThreshold?

	m_peakPower=0;
	m_binaryOutput=0;
	m_lastBinaryOutput = 0;
	m_noiseTimer = 0;
	m_noiseTimerThreshold = 1000 / m_timePerBin; //1 second for now

	m_powerBufSize = 1000 / m_timePerBin; //2 sec of data
	if (m_powerBuf != NULL)
		delete m_powerBuf;
	m_powerBuf = new DelayLine(m_powerBufSize);

	m_resultCounter = 0;

    //See http://www.mstarlabs.com/dsp/goertzel/goertzel.html for details on using a window for better detection
	if (m_windowFunction != NULL)
		delete m_windowFunction;
	m_windowFunction = new WindowFunction(WindowFunction::BLACKMANHARRIS, m_samplesPerBin);
#if 0
    for (int n = 0; n<samplesPerBin; n++) {
        //Hamming: Pass band spans primary 2 bins plus first neighbors on each side.
        //      Out of band rejection minimum -43 dB.
        window[n] = 0.54 - 0.46 * cos(TWOPI * n / samplesPerBin);
        //Exact Blackman: Pass band spans primary 2 bins plus first 2 neighbors on each side.
        //      Out of band rejection minimum -66 dB.
        //window[n] = 0.426591 - .496561*cos(TWOPI * n / samplesPerBin) +.076848*cos(4*pi * n /samplesPerBin);
    }
#endif
	return m_numToneResults;
}
void Goertzel::setBinaryThreshold(float t)
{
	m_binaryThreshold = t;
}

bool Goertzel::calcThreshold_Average(float power)
{
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


#if 0
    if (power > noiseThreshold) {
        if (avgPower > peakPower) {
            //When we get a stronger signal, move our noise and binary thresholds to keep up
            peakPower = avgPower;

            //Threshold should be some factor of average so we get clear distinction between tone / no tone
            binaryThreshold = peakPower * 0.50;

            noiseThreshold = peakPower * 0.05;
        }
        if (power > binaryThreshold)
            binaryOutput = true;
        else
            binaryOutput = false;
        noiseTimer = 0; //Start looking for noise again
    } else {
        power = 0.0;
        binaryOutput = false;
        //If noise for some period, reset avgPower and peakPower
        if (noiseTimer++ > noiseTimerThreshold) {
            //No tones and no signal above noiseThreshold for some time
            //Reset noiseThreshold to averagePower and keep checking
            noiseThreshold = avgPower;
            peakPower = 0;
            noiseTimer = 0;
        }
    }
#endif
    return true;
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


//Sets threshold by comparing min/max values of last N samples
//Returns true if there is enough variation to indicate that there are some tones in sample buffer
bool Goertzel::calcThreshold_MinMax(float power)
{
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
}

CPX * Goertzel::processBlock(CPX *in, bool *toneBuf)
{
    bool res;
    float power;
    int toneBufCounter = 0;
    float normalize = 0.0;
    float dcBias = 0.0;
    float sample;

    //If we have processed at least 1 block use min/max from last block to normalize this block
	if (m_maxSample > -1.0 ) {
        //Remove DC bias per SILabs algorithm
        //In a pure sine wave, bias is the offset to make the sine cross at exactly zero
        //Removing the dc bias from the input signal provides consistent power measurements and minimizes errors
		dcBias = (m_maxSample - m_minSample) / 2.0;
        //Scale to 50% max - what should this be?
		normalize = 0.001 / (m_maxSample-dcBias);
    }
    //Reset to get new min/max
	m_minSample = 1.0;
	m_maxSample = -1.0; //Remember fp is -1 to +1

    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
	for (int i=0; i<m_numSamples; i=i+m_decimateFactor) {
        sample = in[i].re;
		if (sample > m_maxSample)
			m_maxSample = sample;
		else if (sample < m_minSample)
			m_minSample = sample;
        //Normalize
        sample -= dcBias;
        //sample *= normalize;

		res = newSample(sample, power);

        if (res) {
			toneBuf[toneBufCounter ++] = m_binaryOutput;
        }
    }
    return in;
}

//Tone detection occurs every Nth sample
bool Goertzel::newSample(float sample, float &power)
{

    CPX cpx;

    //TI algorithm
    //Internal products - shown for clarity not speed
    //float prod1, prod2, prod3;
    //prod1 = coeff * delay1;
    //delay0 = sample + prod1 - delay2;
    //delay2 = delay1;
    //delay1 = delay0;

    //Apply window for better discrimination
	if (m_windowFunction != NULL)
		sample = sample * m_windowFunction->window[m_sampleCount];

#if 0
    float phase;
    //mstarlabs algorithm realW = 2*cos(w)
    delay0 = sample + (realW * delay1) - delay2;
    delay2 = delay1;
    delay1 = delay0;

    power = 0;
    if(++sampleCount >= samplesPerBin) {
        cpx.re = 0.5 * realW * delay1 - delay2;
        cpx.im = imagW * delay1;
        power = cpx.mag();
        phase = cpx.phase();
        delay2 = 0;
        delay1 = 0;

        //Add new result and throw away oldest result
        powerBuf->NewSample(CPX(power,0));

        bool valid = true;
        //valid = CalcThreshold_MinMax(power);
        valid = CalcThreshold_Average(power);

        qDebug("Thresh %.12f Power %.12f Phase %.12f Sample %.12f",binaryThreshold, power, phase, sample);

        return true;
    }
#endif

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
	m_delay0 = sample + (m_realW * m_delay1) - m_delay2;
	m_delay2 = m_delay1;
	m_delay1 = m_delay0;

	if(++m_sampleCount >= m_samplesPerBin) {
        //We've accumulated enough to change output
		m_sampleCount = 0;
        //Filter output is the total 'energy' in the delay loop
		power = m_delay2*m_delay2 + m_delay1 * m_delay1 - m_realW * m_delay1 * m_delay2;
        //power *= scale;

		m_delay2 = 0;
		m_delay1 = 0;

        //Add new result and throw away oldest result
		m_powerBuf->NewSample(CPX(power,0));

        bool valid;
        //Check threshold every N second
        //valid = CalcThreshold_MinMax(power);
		valid = calcThreshold_Average(power);

        //qDebug("Thresh %.12f Power %.12f Phase %.12f",binaryThreshold, power, phase);

        return true;
    }

    return false;
}

float Goertzel::getNextPowerResult()
{
	return m_powerBuf->NextDelay(0).re;
}

//From Nue-PSK
/*************************************************************************
 *			G O E R T Z E L  F I L T E R   P R O C E S S I N G           *
 *************************************************************************/
// The Goretzel sample block size determines the CW bandwidth as follows:
// CW Bandwidth :  100, 125, 160, 200, 250, 400, 500, 800, 1000 Hz.
int		cw_bwa[] = {80,  64,  50,  40,  32,  20,  16,  10,    8};
int		cw_bwa_index = 3;
int		cw_n =  40; // Number of samples used for Goertzel algorithm

double	cw_f = 796.8750;		// CW frequency offset (start of bin)
double	g_coef = 1.6136951071;	// 2*cos(PI2*(cw_f+7.1825)/SAMPLING_RATE);
double  q0=0, q1=0, q2=0, current;
int		g_t_lock = 0;			// Goertzel threshold lock
int		cspm_lock = 0;			// Character SPace Multiple lock
int		wspm_lock = 0;			// Word SPace Multiple lock
int 	g_sample_count = 0;
double 	g_sample;
double	g_current;
double	g_scale_factor = 1000.0;
volatile long	g_s, g_ra = 10000, g_threshold = 1000;
volatile int	do_g_ave = 0;
int		g_dup_count = 0;
int 	preKey;

//RL fixes
int RXKey;
int last_trans;
int cwPractice;

void do_goertzel (qint16 f_samp)
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

NewGoertzel::NewGoertzel(quint32 sampleRate, quint32 numSamples)
{
	m_externalSampleRate  = sampleRate;
	m_internalSampleRate = m_externalSampleRate; //No decimation for now
	m_numSamples = numSamples;

	m_decimate = 1;
	m_decimateCount = 0;

	m_thresholdType = TH_COMPARE;
	m_threshold = 0.5;

	m_debounceCounter = 0;
}

NewGoertzel::~NewGoertzel()
{
}

void NewGoertzel::setThresholdType(NewGoertzel::ThresholdType t)
{
	m_thresholdType = t;
}

void NewGoertzel::setThreshold(double threshold)
{
	m_threshold = threshold;
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

	(move to morse.cpp)
	For reference and testing, these are correct values for 8khz sample rate (note doesn't change with fTone)
	The more samples we process, the 'sharper' the filter but the more time per result

	Tcw = 60/(wpm*50) //See morse.cpp for details
	msPerSample = 1 / sampleRate, 1/8000 = .125ms
	maxN = Tcw / msPerSample, any bigger and we miss Tcw's (aka bits)
	N = maxN / 4, For better granularity 4 bits = 1 tcw
	N must be an integer, truncate if necessary
	bandwidth = sampleRate / N
	Ideal freq is one that is centered in the bin, ie an even multiple of binWidth/2
	Max  Tcw   Max       BW     Ideal
	WPM  ms    N    N    for N  freq
	-------------------------------
	 20  60.0  480  120   66hz  792
	 30  40.0  320   80  100hz  800 (Ideal)
	 40  30.0  240   60  133hz  798
	 50  24.0  192   48  166hz  830
	 60  20.0  160   40  200hz  800 (Ideal)
	 70  17.1  137   34  235hz  705
	 80  15.0  120   30  266hz  798
	 90  13.3  106   26  307hz  614 or 921 (bad choice)
	100  12.0   96   24  333hz  666 or 999 (bad choice)
	120  10.0   80   20  400hz  800  (Ideal - Good default to start auto wpm)
	200   6.0   48   12  666hz  666 or 1332 (bad choice)
	240   5.0   40   10  800hz  800 (Ideal)
	300   4.0   32    8  1khz   500 or 1000 (bad choice)
*/

//Returns an estimated N based on shortest bit length
quint32 NewGoertzel::estNForShortestBit(double msShortestBit)
{
	//How many ms does each sample represent?
	//ie for 8k sample rate and 2048 buffer, each sample is .125ms
	double msPerSample = 1e3 / m_internalSampleRate; //result in ms, not seconds
	//N has to be less than the shortest 'bit' or we lose data
	//ie maxN for 120wpm morse = 10ms / .125ms = 80
	return msShortestBit / msPerSample;
}

//Returns an estimated N based on bin bandwidth
quint32 NewGoertzel::estNForBinBandwidth(quint32 bandwidth)
{
	//N has to be large enough for desired bandwidth
	//ie minN for bandwidth of 100 = 100 / 3.90 = 25.6
	double binWidth = (m_internalSampleRate / (m_numSamples/m_decimate));
	return bandwidth / binWidth;
}

void NewGoertzel::setTargetSampleRate(quint32 targetSampleRate)
{
	//Find internal sample rate closest to intSampleRate, but with even decimation factor
	quint32 dec = m_externalSampleRate / targetSampleRate;
	//We process sample by sample cross buffer, so decimate can be any integer
	m_decimate = dec;
#if 0
	//m_decimate = dec + (dec % 2);

	if (dec > 2)
		//Round up to nearest power of 2 so we can decimae numSamples evenly
		m_decimate =  pow(2,(int(log2(dec-1))+1));
	else
		m_decimate = 2;
#endif

	m_internalSampleRate = m_externalSampleRate / m_decimate;

}

void NewGoertzel::setFreq(quint32 freq, quint32 N, quint32 debounce)
{
	m_debounce = debounce;

	/*
		This creates 3 overlapping bins.  If main bin is 200hz wide
				   Main
			   |--- 200 ---|
		|--- 200 ---||--- 200 ---|
			 Low          High
		With the averaging comparison, the effective bandwidth of Main is 150hz
		Better than just using frequency to set high/low?
	*/
	m_mainTone.setFreq(freq, N, m_internalSampleRate);
	//Try bw, bw/2, bw/4
	quint32 bwDelta = m_mainTone.m_bandwidth / 2;
	m_highCompareTone.setFreq(freq + bwDelta, N, m_internalSampleRate);
	m_lowCompareTone.setFreq(freq - bwDelta, N, m_internalSampleRate);
	//KA7OEI differential goertzel algorithm compares 5% below and 4% above bins
	//m_lowCompareTone.setFreq(freq * 0.95, N, m_internalSampleRate);
	//m_highCompareTone.setFreq(freq * 1.04, N, m_internalSampleRate);
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
bool NewGoertzel::processSample(double x_n, double &power)
{
	//We decimate here so caller doesn't need to know details
	//Ignore samples between decimation
	if (m_decimateCount++ % m_decimate > 0) {
		power = 0;
		return false;
	}
	//All the tones have the same N, just check the result of the main
	bool result = m_mainTone.processSample(x_n);
	m_highCompareTone.processSample(x_n);
	m_lowCompareTone.processSample(x_n);
	m_mainTone.m_isValid = false;
	power = 0;
	if (result) {
		//If we have a detectable tone, it should be strong in the main bin compared with surrounding bins
		double avgComparePower = (m_highCompareTone.m_power + m_lowCompareTone.m_power) / 2;
#if 1
		//KA7OEI differential goertzel
		double compareRatio = m_mainTone.m_power / avgComparePower;
		if (compareRatio > 1.75) {
			//We have a tone, check to see if it's consistent across debounce
			if (m_lastResult) {
				//If last result was a tone, see if we have enough in a row
				m_debounceCounter++;
			} else {
				//Last result was unknown or no-tone, restart debounce counter
				m_debounceCounter = 1;
			}
			m_lastResult = true;
			//This will return valid result only if we have at least m_debounce consistent results
			if (m_debounceCounter >= m_debounce) {
				power = m_mainTone.m_power;
				m_debounceCounter = 0;
				m_mainTone.m_isValid = true;
				return true; //Valid
			}
			return false; //No result yet
		} else {
			//We need consistent timing for both on and off keying
			if (!m_lastResult)
				m_debounceCounter++;
			else
				m_debounceCounter = 1;
			m_lastResult = false;
			if (m_debounceCounter >= m_debounce) {
				power = 0;
				m_debounceCounter = 0;
				m_mainTone.m_isValid = false;
				return true;
			}
			return false;
		}

#else
		//We want to see a 6db difference or 4x
		m_threshold = avgComparePower * 16;
		if (m_mainTone.m_power > m_threshold)
			power = m_mainTone.m_power;
		else
			power = 0;
#endif
	} //End if (result)
	return false; //Needs more samples
}

//For future use if we need to detect all the bits in a buffer at once
double NewGoertzel::updateTonePower(CPX *cpxIn)
{
	if (m_mainTone.m_N == 0 || m_mainTone.m_freq == 0)
		//Not initialized
		return 0;

	//We make sure that decimate an even divisor of m_numSamples
	for (int i = 0; i < m_numSamples; i += m_decimate) {
		//Todo: Do we need decimation filtering here?
		double power;
		bool result = processSample(cpxIn[i].re, power);
		if (result) {
			//power == 0 or power > 0 for on-off
			//Save bits
		}
	}
	return 0;
}

NewGoertzel::Tone::Tone()
{
	m_freq = 0;
	m_N = 0;
	m_bandwidth = 0;
	m_wn = 0;
	m_wn1 = 0;
	m_wn2 = 0;
	m_nCount = 0;
	m_usePhaseAlgorithm = false;
	m_isValid = false;
	//Todo: construct power and bit arrays
}

//Call setTargetSampleRate before setToneFreq
void NewGoertzel::Tone::setFreq(quint32 freq, quint32 N, quint32 sampleRate)
{
	m_freq = freq;
	m_N = N;
	m_bandwidth = sampleRate / m_N;
	//Lyons coeff same result as Wikipedia coeff, but Wikipedia not dependent on N
	m_coeff = 2 * cos(TWOPI * m_freq / sampleRate); //Wikipedia
	m_coeff2 = 2 * sin(TWOPI * m_freq / sampleRate);
}

#if 0
	We can also process complex input, like spectrum data, not just audio
	From Wikipedia ...
	Complex signals in real arithmetic[edit]
	Since complex signals decompose linearly into real and imaginary parts, the Goertzel algorithm can be computed in real arithmetic separately over the sequence of real parts, yielding y_r(n); and over the sequence of imaginary parts, yielding y_i(n). After that, the two complex-valued partial results can be recombined:

	(15) y(n) = y_r(n) + i\ y_i(n)
#endif

bool NewGoertzel::Tone::processSample(CPX cpx)
{
	Q_UNUSED(cpx);
	return false;
}

//Process a single sample, return true if we've processed enough for a result
bool NewGoertzel::Tone::processSample(double x_n)
{
	//m_wn = x_n + m_wn1 * (2 * cos((TWOPI * m_m )/ N)) - m_wn2; //Lyons 13-80 re-arranged
	//Same as m_wn = x+n + m_coeff * w_wn1 - m_wn2;
	//m_wn = x_n + (m_wn1 * m_coeff) + (m_wn2 * -1); //Same as simplified version below
	m_wn = x_n + (m_wn1 * m_coeff) - m_wn2;

	//Update delay line
	m_wn2 = m_wn1;
	m_wn1 = m_wn;

	int normalize = m_N / 2;

	if (m_nCount++ >= m_N) {
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
		return true;
	}
	return false;
}

void NewGoertzel::updateToneThreshold()
{

}

void NewGoertzel::updateBitDetection()
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


