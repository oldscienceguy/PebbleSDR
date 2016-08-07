#ifndef GOERTZEL_H
#define GOERTZEL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "delayline.h"
#include "QtGlobal"
#include "cpx.h"
#include "windowfunction.h"
#include "movingavgfilter.h"
#include <complex>

//Core Goertzel to process a tone or FFT bin
class Goertzel
{
public:
	Goertzel();
	void setFreq(qint32 freq, quint32 N, quint32 sampleRate);
	bool processSample (double x_n); //For audio data
	bool processSample (CPX x_n); //For IQ data - Not implemented yet

	//Use simplified goertzel or full which includes phase information
	bool m_usePhaseAlgorithm;

	//Is the result valid, ie processed enough samples
	bool m_isValid;

	qint32 m_freq;
	quint32 m_bandwidth;
	quint32 m_usecPerResult;
	double m_power; //Power of last bin
	double m_phase; //Phase of last bin

	//Using Lyons terminology (pg 740)
	double m_coeff; //For power
	double m_coeff2; //For phase if needed
	int m_samplesPerResult; //N = binWidth = #samples per bin
	double m_wn; //Lyons w(n), Wikipedia s
	double m_wn1; //Lyons w(n-1), Wikipedia s_prev
	double m_wn2; //Lyons w(n-2), Wikipedia s_prev2
	int m_nCount; //# samples processed

	//6/14/16: New CPX Goertzel algorithm supporting non-integer 'k'

	//Using terminology from article http://asp.eurasipjournals.springeropen.com/articles/10.1186/1687-6180-2012-56
	//Constants
	double c_A;
	double c_B;
	CPX c_C;
	CPX c_D;
	//State variables (delay line)
	CPX m_s0;
	CPX m_s1;
	CPX m_s2;

	double avgPower() {return m_avgPower;}
	double variance() {return m_avgFilter->variance();}
	double stdDev() {return m_avgFilter->stdDev();}
private:
	MovingAvgFilter *m_avgFilter;
	double m_avgPower;

};

//TBD, should have better time resolution and noise rejection
class SlidingGoertzel
{
public:
	SlidingGoertzel();
	~SlidingGoertzel();
};

//Goertzel class for on/off keying like morse
class GoertzelOOK
{
public:
	GoertzelOOK(quint32 sampleRate, quint32 numSamples);
	~GoertzelOOK();

	//Threshold determines the boundary between 'present' and 'not present' tones
	//TH_COMPARE uses 2 extra bins, above and below the target freq
	//TH_AVERAGE uses the average power in the buffer
	//TH_MANUAL uses specific target
	enum ThresholdType {TH_COMPARE, TH_AVERAGE, TH_PEAK, TH_MIN_MAX, TH_MANUAL, TH_NOISE, TH_UP, TH_DOWN};
	void setThresholdType(ThresholdType t);
	ThresholdType thresholdType() {return m_thresholdType;}
	void setThreshold(double threshold, ThresholdType thType);
	double threshold(){return m_compareThreshold;}

	void setTargetSampleRate(quint32 targetSampleRate);

	//Set the size of the jitter filter (moving average)
	//Typically based on the rise/fall time of the signal since that's where jitter usually occurs
	void setFreq(quint32 freq, quint32 N, quint32 jitterCount);

	//Updates threshold power between 0 and 1 bit
	void updateToneThreshold();

	//Processes tone1Power and tone2Power to update high/low bits
	void updateBitDetection();

	//Call one or the other to set N
	quint32 estNForShortestBit(double msShortestBit);
	quint32 estNForBinBandwidth(quint32 bandwidth);

	bool processSample(double x_n, double &power, bool &aboveThreshold);
	bool processSample(CPX x_n, double &retPower, bool &aboveThreshold, double &peakPower);
private:
	bool processResult(double &retPower, bool &aboveThreshold);

	Goertzel m_mainTone;
	Goertzel m_lowCompareTone;
	Goertzel m_highCompareTone;

	quint32 m_sampleRate;

	int m_numSamples;
	ThresholdType m_thresholdType;
	double m_noiseThreshold; //Used to squelch signals below a specified SNR
	double m_compareThreshold;
	double m_avgThreshold;
	double m_thresholdUp;
	double m_thresholdDown;
	double m_peakPower;
	double m_minPower;

	bool m_lastTone; //For debounce counting
	quint32 m_attackCount;
	quint32 m_attackCounter;
	quint32 m_decayCount;
	quint32 m_decayCounter;

	quint32 m_debounceCounter;
	bool debounce(bool aboveThreshold);

	MovingAvgFilter *m_peakFilter;
	MovingAvgFilter *m_minFilter;

	//Todo: Goertzel agc options: off, fast, slow
	//Increasing signal raises agc fast (20 results)
	const double m_attackWeight = 1.0 / 20.0;
	//Decreasing signal lowers agc slow (100 results
	//Original value of 800 on decay seems way too high, means we need 800 space results before agc reflects
	const double m_decayWeight = 1.0 / 500.0;

};


//For reference, goertzel from Nue-Psk
class NuePskGoertzel
{
public:
	NuePskGoertzel();
	void do_goertzel (qint16 f_samp);

private:
	// The Goretzel sample block size determines the CW bandwidth as follows:
	// CW Bandwidth :  100, 125, 160, 200, 250, 400, 500, 800, 1000 Hz.
	const int cw_bwa[9] = {80,  64,  50,  40,  32,  20,  16,  10,    8};
	const int cw_bwa_index = 3;

	int		cw_n; // Number of samples used for Goertzel algorithm

	double	cw_f;		// CW frequency offset (start of bin)
	double	g_coef;	// 2*cos(PI2*(cw_f+7.1825)/SAMPLING_RATE);
	double  q0;
	double	q1;
	double	q2;
	double	current;
	int		g_t_lock;			// Goertzel threshold lock
	int		cspm_lock;			// Character SPace Multiple lock
	int		wspm_lock;			// Word SPace Multiple lock
	int 	g_sample_count;
	double 	g_sample;
	double	g_current;
	double	g_scale_factor;
	volatile long	g_s;
	volatile long g_ra;
	volatile long g_threshold;
	volatile int	do_g_ave;
	int		g_dup_count;
	int 	preKey;

	//RL fixes
	int RXKey;
	int last_trans;
	int cwPractice;


};

//Standard Reference Tones

class DTMF
{
public:
	DTMF();
	struct Tone {
		void init(quint16 h, quint16 l);
		quint16 m_hi;
		quint16 m_lo;

	};
	//Look for all tone bits in parallel
	//Strongest two that are above threshold are DTMF
	Goertzel m_697;
	Goertzel m_770;
	Goertzel m_852;
	Goertzel m_941;
	Goertzel m_1209;
	Goertzel m_1336;
	Goertzel m_1477;
	Goertzel m_1633;

	Tone DTMF_0;
	Tone DTMF_1;
	Tone DTMF_2;
	Tone DTMF_3;
	Tone DTMF_4;
	Tone DTMF_5;
	Tone DTMF_6;
	Tone DTMF_7;
	Tone DTMF_8;
	Tone DTMF_9;
	Tone DTMF_A;
	Tone DTMF_B;
	Tone DTMF_C;
	Tone DTMF_D;
	Tone DTMF_STAR;
	Tone DTMF_POUND;
};


class CTCSS
{
public:
	CTCSS();
	struct Tone {
		void init(const char *d, float f, float s);
		char m_designation[2];
		float m_freq;
		float m_spacing;
	};

	Tone CTCSS_1;
	Tone CTCSS_2;
	Tone CTCSS_3;
	Tone CTCSS_4;//Not defined??
	Tone CTCSS_5;
	Tone CTCSS_6;
	Tone CTCSS_7;
	Tone CTCSS_8;
	Tone CTCSS_9;
	Tone CTCSS_10;
	Tone CTCSS_11;
	Tone CTCSS_12;
	Tone CTCSS_13;
	Tone CTCSS_14;
	Tone CTCSS_15;
	Tone CTCSS_16;
	Tone CTCSS_17;
	Tone CTCSS_18;
	Tone CTCSS_19;
	Tone CTCSS_20;
	Tone CTCSS_21;
	Tone CTCSS_22;
	Tone CTCSS_23;
	Tone CTCSS_24;
	Tone CTCSS_25;
	Tone CTCSS_26;
	Tone CTCSS_27;
	Tone CTCSS_28;
	Tone CTCSS_29;
	Tone CTCSS_30;
	Tone CTCSS_31;
	Tone CTCSS_32;
};


#endif // GOERTZEL_H
