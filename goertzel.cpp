#include "goertzel.h"
#include "math.h"
#include "defs.h"
#include "QtDebug"
#include "FIRFilter.h"

/*
  From SILabs DTMF paper http://www.silabs.com/Support%20Documents/TechnicalDocs/an218.pdf

The Goertzel algorithm works on blocks of ADC samples.
Samples are processed as they arrive and a result is produced every N samples, where N is the number of samples in each block.
If the sampling rate is fixed, the block size determines the frequency resolution or “bin width” of the resulting power measurement.
The example below shows how to calculate the frequency resolution and time required to capture N samples:

Sampling rate: 8 kHz
Block size (N): 200 samples
Frequency Resolution: sampling rate/block size = 40 Hz
Time required to capture N samples: block size/sampling rate = 25 ms

The tradeoff in choosing an appropriate block size is between frequency resolution and the time required to capture a block of samples.
Increasing the output word rate (decreasing block size) increases the “bin width” of the resulting power measurement.
The bin width should be chosen to provide enough frequency resolution to differentiate between the DTMF frequencies and be able to detect DTMF tones with reasonable delay.
See [2] for additional information about choosing a block size.


*/

Goertzel::Goertzel(int _sampleRate, int _numSamples)
{
    sampleRate = _sampleRate;
    numSamples = _numSamples;

    realW = 0;
    binaryOutput = false;
    binaryThreshold = 0;
    noiseThreshold = 0.5;
    delay0 = delay1 = delay2 = 0;
    sampleCount = 0;
    binWidthHz = 0;
    samplesPerBin = 0;
    avgTonePower = 0;
    peakPower = 0;
    scale = 1000; //So we don't lose precision?
    powerBuf = NULL;
    window = NULL;
}

Goertzel::~Goertzel()
{
    if (powerBuf != NULL)
        delete (powerBuf);
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

Note: Q15 format means that 15 decimal places are assumed.  Used for situations where there is no floating point
or only integer math is used.
See http://en.wikipedia.org/wiki/Q_(number_format)

Nue-Psk coefficient for 796.875hz is 1.6136951071 as another example
*/
/*
The Goertzel algorithm is derived from the DFT and exploits the periodicity of the phase factor, exp(-j*2k/N) to reduce the computational complexity associated with the DFT, as the FFT does.
With the Goertzel algorithm only 16 samples of the DFT are required for the 16 tones (\Links\Goertzel Theory.pdf)
Goertzel is a 2nd order IIR filter or a single bin DFT

http://en.wikipedia.org/wiki/Goertzel_algorithm

//Simplest best explanation
http://www.eetimes.com/design/embedded/4024443/The-Goertzel-Algorithm
*/

/*
  For reference and testing, these are correct values for 8khz sample rate (note doesn't change with fTone)
  The more samples we process, the 'sharper' the filter but the more time per result
  max wpm for each setting is 1200 / (4 * MsPerBin) min 4 results per element
  Bandwidth #Samples    MsPerBin    MaxWPM
  100       80          10          30
  125       64          8           37.5
  160       50          6.25        48
  200       40          5           60      (Default value)
  250       32          4           75
  400       20          2.5         120
  500       16          2           150
  800       10          1.25        240
  1000      8           1           300

*/
//fTone is the freq in hz we want to detect
//SampleRate 48000, 96000, etc
//Test with Freq from DTMF and sampleRate 8000 and compare with table above
//returns numToneResults for sizing toneBuffer
int Goertzel::SetFreqHz(int fTone, int bw, int gsr)
{
    freqHz = fTone;
    gSampleRate = gsr;
    decimateFactor = sampleRate / gSampleRate;

    binWidthHz = bw; //equivalent to filter bandwidth
    //Set blockSize based on sampleRate and desired filter bandwidth
    //How many samples do we need to accumulate to get desired bandwidth
    samplesPerBin = gSampleRate / binWidthHz;

    //What is the time, in ms, that each bin represents
    timePerBin = 1.0/gSampleRate * samplesPerBin * 1000;

    //How many tone results per sample buffer = num decimated samples / samples per bin
    numToneResults = (numSamples / decimateFactor) / samplesPerBin;

    //Calc the filter coefficient
    //Think of the goertzel as 1 bin of an N bin FFT
    //Each bin 'holds' a frequency range, specified by bandwidth or binwidth
    //So for binwidth = 200hz, bin1=0-199, bin2=200-399, bin3=400-599, bin4=600-799
    //If we're looking for 700hz in this example, it would be in the middle of bin 4, perfect!
    //k represents the fft bin that would contain the frequency of interest
    k = (float)(samplesPerBin * freqHz)/(float)(gSampleRate) + 0.5; // + 0.5 rounds up

    w = TWOPI * k / samplesPerBin;

    realW = 2.0 * cos(w);
    imagW = sin(w);

    //qDebug("Goertzel: freq = %d binWidth = %d sampleRate = %d samplesPerBin = %d coefficient = %.12f timePerBin = %d",
    //       freqHz,binWidthHz,sampleRate,samplesPerBin,coeff,timePerBin);

    avgTonePower = 0.001;
    avgNoisePower = 0.000001;
    binaryThreshold = (avgTonePower - avgNoisePower) / 2.0;

    noiseThreshold = 1.0/(scale*1000); //Lowest possible value for binaryThreshold?

    peakPower=0;
    binaryOutput=0;
    lastBinaryOutput = 0;
    noiseTimer = 0;
    noiseTimerThreshold = 1000 / timePerBin; //1 second for now

    powerBufSize = 1000 / timePerBin; //2 sec of data
    if (powerBuf != NULL)
        delete powerBuf;
    powerBuf = new DelayLine(powerBufSize);

    resultCounter = 0;

    //See http://www.mstarlabs.com/dsp/goertzel/goertzel.html for details on using a window for better detection
    if (window != NULL)
        free (window);
    window = new float[samplesPerBin];
    FIRFilter::MakeWindow(FIRFilter::BLACKMANHARRIS, samplesPerBin, window);
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
    return numToneResults;
}
void Goertzel::SetBinaryThreshold(float t)
{
    binaryThreshold = t;
}

bool Goertzel::CalcThreshold_Average(float power)
{
    //WIP What should binary threshold be initialized to?
    binaryOutput = (power > binaryThreshold) ? true : false;

    //Only update average on transition and only if binaryOutput same for at least 3 results
    if (binaryOutput == lastBinaryOutput) {
        //No transition, keep counting
        resultCounter++;
        if (binaryOutput)
            avgTonePower = (power * .01) + (avgTonePower * 0.99);
        else
            avgNoisePower = (power * .01) + (avgNoisePower * 0.99);

    } else {
        //Transition, see if it was long enough to average
        //Only update average if we have N consecutive marks or spaces
        if (resultCounter > 3) {
            //We want our threshold to adjust to some percentage of average
            //Calc average over N seconds, based on current settings
            binaryThreshold = (avgTonePower - avgNoisePower) / 2.0;
        }
        resultCounter = 0;
        lastBinaryOutput = binaryOutput;
    }
    //Timeout if we're constantly above or below threshold
    if (resultCounter > (2000 / timePerBin)) {
        resultCounter = 0;
        //Or should we just reset to default values??
        binaryThreshold *= 0.5; //Cut in half and try again
        avgTonePower = binaryThreshold * 1.5;
        avgNoisePower = binaryThreshold * 0.5;
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
bool Goertzel::CalcThreshold_MinMax(float power)
{
    //Only check ever n sec
    if (++resultCounter < 1000/timePerBin)
        return true;
    resultCounter = 0;

    //Initial values such that first result will always be > max and < min
    maxPower = 0.0;
    minPower = 1.0;

    minSample = 1.0;
    maxSample = -1.0;

    //Todo: Investigate running min/max and running median algorithms
    //This adjusts for varying signal strength and should allow weak signals to still have a clear
    //threshold between tone or no-tone
    //Find min/max in last N results
    //We don't know state of circular buffer, so just check all
    for (int i = 0; i<powerBufSize; i++) {
        power = (*powerBuf)[i].re;
        if (power > maxPower)
            maxPower = power;
        else if (power>0 && power < minPower)
            minPower = power;
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
    binaryThreshold = maxPower * 0.60; //n% of max above min

    return true;
}

CPX * Goertzel::ProcessBlock(CPX *in, bool *toneBuf)
{
    bool res;
    float power;
    int toneBufCounter = 0;
    float normalize = 0.0;
    float dcBias = 0.0;
    float sample;

    //If we have processed at least 1 block use min/max from last block to normalize this block
    if (maxSample > -1.0 ) {
        //Remove DC bias per SILabs algorithm
        //In a pure sine wave, bias is the offset to make the sine cross at exactly zero
        //Removing the dc bias from the input signal provides consistent power measurements and minimizes errors
        dcBias = (maxSample - minSample) / 2.0;
        //Scale to 50% max - what should this be?
        normalize = 0.001 / (maxSample-dcBias);
    }
    //Reset to get new min/max
    minSample = 1.0;
    maxSample = -1.0; //Remember fp is -1 to +1

    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
    for (int i=0; i<numSamples; i=i+decimateFactor) {
        sample = in[i].re;
        if (sample > maxSample)
            maxSample = sample;
        else if (sample < minSample)
            minSample = sample;
        //Normalize
        sample -= dcBias;
        //sample *= normalize;

        res = NewSample(sample, power);

        if (res) {
            toneBuf[toneBufCounter ++] = binaryOutput;
        }
    }
    return in;
}

//Tone detection occurs every Nth sample
bool Goertzel::NewSample(float sample, float &power)
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
    if (window != NULL)
        sample = sample * window[sampleCount];

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
    delay0 = sample + (realW * delay1) - delay2;
    delay2 = delay1;
    delay1 = delay0;

    if(++sampleCount >= samplesPerBin) {
        //We've accumulated enough to change output
        sampleCount = 0;
        //Filter output is the total 'energy' in the delay loop
        power = delay2*delay2 + delay1 * delay1 - realW * delay1 * delay2;
        //power *= scale;

        delay2 = 0;
        delay1 = 0;

        //Add new result and throw away oldest result
        powerBuf->NewSample(CPX(power,0));

        bool valid;
        //Check threshold every N second
        //valid = CalcThreshold_MinMax(power);
        valid = CalcThreshold_Average(power);

        //qDebug("Thresh %.12f Power %.12f Phase %.12f",binaryThreshold, power, phase);

        return true;
    }

    return false;
}

float Goertzel::GetNextPowerResult()
{
    return powerBuf->NextDelay(0).re;
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
