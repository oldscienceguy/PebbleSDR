#ifndef GOERTZEL_H
#define GOERTZEL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "delayline.h"
#include "QtGlobal"
#include "cpx.h"
#include "windowfunction.h"
#include "qbitarray.h"

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

//Testing, 1 or 2 tone goertzel with updated model
class NewGoertzel
{
public:
	NewGoertzel(quint32 sampleRate, quint32 numSamples);
	~NewGoertzel();

	//Threshold determines the boundary between 'present' and 'not present' tones
	//TH_COMPARE uses 2 extra bins, above and below the target freq
	//TH_AVERAGE uses the average power in the buffer
	//TH_MANUAL uses specific target
	enum ThresholdType {TH_COMPARE, TH_AVERAGE, TH_MANUAL};
	void setThresholdType(ThresholdType t);
	ThresholdType thresholdType() {return m_thresholdType;}
	void setThreshold(double threshold);
	double threshold(){return m_threshold;}

	void setFreq(quint32 freq, quint32 N, quint32 debounce);

	//Updates tone1Power and tone2Power
	double updateTonePower(CPX *cpxIn);

	//Updates threshold power between 0 and 1 bit
	void updateToneThreshold();

	//Processes tone1Power and tone2Power to update high/low bits
	void updateBitDetection();

	//Call one or the other to set N
	quint32 estNForShortestBit(double msShortestBit);
	quint32 estNForBinBandwidth(quint32 bandwidth);

	void setTargetSampleRate(quint32 targetSampleRate);
	bool processSample(double x_n, double &power);
private:
	//All of the internal data needed to decode a tone
	//Allows us to specify multiple tones that can be decoded at the same time
	class Tone{
	public:
		Tone();
		void setFreq(quint32 freq, quint32 N, quint32 sampleRate);
		bool processSample (double x_n); //For audio data
		bool processSample (CPX cpx); //For IQ data - Not implemented yet

		//Use simplified goertzel or full which includes phase information
		bool m_usePhaseAlgorithm;

		//Is the result valid, ie processed enough samples
		bool m_isValid;

		//Saved results, do we need?
		QBitArray *m_bits;

		quint32 m_freq;
		quint32 m_bandwidth;
		double m_power; //Power of last bin
		double m_phase; //Phase of last bin

		//Using Lyons terminology (pg 740)
		double m_coeff; //For power
		double m_coeff2; //For phase if needed
		int m_N; //binWidth = #samples per bin
		double m_wn; //Lyons w(n), Wikipedia s
		double m_wn1; //Lyons w(n-1), Wikipedia s_prev
		double m_wn2; //Lyons w(n-2), Wikipedia s_prev2
		int m_nCount; //# samples processed

	};

	Tone m_mainTone;
	Tone m_lowCompareTone;
	Tone m_highCompareTone;

	quint32 m_externalSampleRate;
	quint32 m_internalSampleRate;
	int m_decimate;
	int m_decimateCount;

	int m_numSamples;
	ThresholdType m_thresholdType;
	double m_threshold;

	bool m_lastResult; //For debounce counting
	quint32 m_debounce; //#results needed to determine tone is present
	quint32 m_debounceCounter;
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

class Goertzel
{
public:
    Goertzel(int _sampleRate, int _numSamples);
    ~Goertzel();

    //Sets internal values (coefficient) for freq

	int setFreqHz(int _freq, int _bwHz, int _goertzelSampleRate);
	void setBinaryThreshold(float t);
    //Processes next sample and returns true if we've accumulated enough for an output change
	bool newSample(float s, float &p);
	//toneBuf is returned with bool values indicating tone/no-tone
	CPX * processBlock(CPX *in, bool *toneBuf);
	//Need function to return all of powerBuf or just next result
	float getNextPowerResult();

private:
	bool calcThreshold_MinMax(float p);
	bool calcThreshold_Average(float p);

	int m_freqHz;
	int m_gSampleRate;
    //output values
	CPX m_cpx;
	float m_avgTonePower;
	float m_avgNoisePower;
	float m_peakPower;
	bool m_binaryOutput;
	bool m_lastBinaryOutput;

	float m_binaryThreshold; //above is true, below or = is false
	float m_noiseThreshold; //Ignore results below this level
	int m_noiseTimer; //If we get noise for some time, reset all averages, peaks, etc
	int m_noiseTimerThreshold; //How much noise triggers a reset.  Specified in block counts

    //#samples we need to process throught filter to get accurate result
	int m_samplesPerBin;
	int m_timePerBin; //in ms


	int m_sampleRate; //Source
	int m_numSamples; //Source
	int m_decimateFactor;
	int m_numToneResults;

    //Filter coefficient, calculate with CalcCoefficient or table lookup
	float m_realW;
	float m_imagW;
	float m_w;

	int m_resultCounter;

    //Circular buffer for saving power readings
	DelayLine *m_powerBuf;
	int m_powerBufSize;

    //Internal values per EE Times article
	int m_k;
	int m_binWidthHz;
	int m_scale;

	WindowFunction *m_windowFunction; //Precalculated array of window values for specified freq,

    //These could be static in FPNextSample, but here for debugging and for use
    //if we try different algorithms
    //iteration counter
	int m_sampleCount;
    //Delay loop for filter
	float m_delay0;
	float m_delay1;
	float m_delay2;

	float m_maxPower;
	float m_minPower;

	float m_maxSample;
	float m_minSample;

};

#endif // GOERTZEL_H
