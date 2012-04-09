#include "goertzel.h"
#include "math.h"
#include "defs.h"
#include "QtDebug"

#define POWERBUFSIZE 2048
Goertzel::Goertzel()
{
    coeff = 0;
    power = 0;
    binaryOutput = false;
    binaryThreshold = 0;
    delay0 = delay1 = delay2 = 0;
    sampleCount = 0;
    binWidthHz = 0;
    samplesPerBin = 0;
    avgPower = 0;
    nextIn = nextOut = numPowerResults = 0;
    //Todo: How long should this be
    powerBuf = new float[POWERBUFSIZE];
}
Goertzel::~Goertzel()
{
    if (powerBuf != NULL)
        free (powerBuf);
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

//Note: We'll use table of coefficients, this is just here as a utility and to test forumlas
//fTone is the freq in hz we want to detect
//SampleRate 48000, 96000, etc
//Test with Freq from DTMF and sampleRate 8000 and compare with table above
void Goertzel::SetFreqHz(int fTone, int sr, int bw)
{
    freqHz = fTone;
    sampleRate = sr;
    binWidthHz = bw; //equivalent to filter bandwidth
    //Set blockSize based on sampleRate and desired filter bandwidth
    //The more samples we process, the 'sharper' the filter
    /*
      From nue-psk
        # Samples:       80,  64,  50,  40,  32,  20,  16,  10,   8
        Bandwidth:      100, 125, 160, 200, 250, 400, 500, 800, 1000 Hz.

    */
    //How many samples do we need to accumulate to get desired bandwidth
    samplesPerBin = sampleRate / binWidthHz;

    //What is the time, in ms, that each bin represents
    timePerBin = 1.0/sampleRate * samplesPerBin * 1000;

    //Calc the filter coefficient
    //Round up
    k = (float)(samplesPerBin * freqHz)/(float)(sampleRate) + 0.5;
    w = (TWOPI / samplesPerBin) * k;
    coeff = 2 * cos(w);
    qDebug("Goertzel: freq = %d binWidth = %d sampleRate = %d samplesPerBin = %d coefficient = %.12f timePerBin = %d",
           freqHz,binWidthHz,sampleRate,samplesPerBin,coeff,timePerBin);
    return;
}
void Goertzel::SetBinaryThreshold(float t)
{
    binaryThreshold = t;
}

//FP Math
//Tone detection occurs every Nth sample
bool Goertzel::FPNextSample(float sample)
{
    //Internal products - shown for clarity not speed
    float prod1, prod2, prod3;

    //TI algorithm
    //prod1 = coeff * delay1;
    //delay0 = sample + prod1 - delay2;
    //delay2 = delay1;
    //delay1 = delay0;

    //Wikipedia algorithm
    delay0 = sample + coeff * delay1 - delay2;

    delay2 = delay1;
    delay1 = delay0;

    if(++sampleCount >= samplesPerBin) {
        //We've accumulated enough to change output
        sampleCount = 0;
        //Filter output is the total 'energy' in the delay loop
        //Precalc cos(w) and sin(w) for performance
#if 0
        cpx.re = delay1 - delay2 * cos(w);
        cpx.im = delay2 * sin(w);
        power = cpx.mag();
#else
        //Simpler way if we don't need re and im data
        power = delay1*delay1 + delay2*delay2 - delay1*delay2*coeff;
#endif

        //qDebug() << "pwr" << power;
        //We want our threshold to adjust to some percentage of average
        avgPower = (power * .01) + (avgPower * 0.99);

        //Threshold should be some factor of average so we get clear distinction between tone / no tone
        binaryThreshold = avgPower * 0.75;

        if (power > binaryThreshold)
            binaryOutput = true;
        else
            binaryOutput = false;

        delay2 = 0;
        delay1 = 0;

        //Save in circular buffer
        powerBuf[nextIn] = power;
        nextIn = (nextIn + 1) % POWERBUFSIZE;
        //Handle the case where we wrap circular buffer without reading results
        if (numPowerResults < POWERBUFSIZE)
            numPowerResults++;

        return true;
    }

    return false;
}

float Goertzel::GetNextPowerResult()
{
    //Make sure we never try to get more results than we have
    if (numPowerResults == 0)
        return 0.0;

    float power = powerBuf[nextOut];
    nextOut = (nextOut + 1) % POWERBUFSIZE;
    numPowerResults--;
    return power;
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


//From TI DSP Course Chapter 17
//Q15 Math
void Q15NextSample (float sample)
{
    static short delay;
    static short delay_1 = 0;
    static short delay_2 = 0;
    static int N = 0;
    static int Goertzel_Value = 0;
    int I, prod1, prod2, prod3, sum, R_in, output;
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

