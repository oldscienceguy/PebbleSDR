#ifndef GOERTZEL_H
#define GOERTZEL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "delayline.h"
#include "QtGlobal"
#include "cpx.h"
#include "windowfunction.h"

//Standard Reference Tones

class DTMF
{
public:
	DTMF(quint16 h, quint16 l) {m_hi =h; m_lo=l;}
	quint16 m_hi;
	quint16 m_lo;

	static const DTMF DTMF_1;
	static const DTMF DTMF_2;
	static const DTMF DTMF_3;
	static const DTMF DTMF_A;
	static const DTMF DTMF_4;
	static const DTMF DTMF_5;
	static const DTMF DTMF_6;
	static const DTMF DTMF_B;
	static const DTMF DTMF_7;
	static const DTMF DTMF_8;
	static const DTMF DTMF_9;
	static const DTMF DTMF_C;
	static const DTMF DTMF_STAR;
	static const DTMF DTMF_0;
	static const DTMF DTMF_POUND;
	static const DTMF DTMF_D;

};


class CTCSS
{
public:
    CTCSS(const char *d, float f, float s) {
		strncpy(m_designation,d,2);
		m_freq = f;
		m_spacing = s;
    }
	char m_designation[2];
	float m_freq;
	float m_spacing;
	static const CTCSS CTCSS_1;
	static const CTCSS CTCSS_2;
	static const CTCSS CTCSS_3;
	static const CTCSS CTCSS_4;//Not defined??
	static const CTCSS CTCSS_5;
	static const CTCSS CTCSS_6;
	static const CTCSS CTCSS_7;
	static const CTCSS CTCSS_8;
	static const CTCSS CTCSS_9;
	static const CTCSS CTCSS_10;
	static const CTCSS CTCSS_11;
	static const CTCSS CTCSS_12;
	static const CTCSS CTCSS_13;
	static const CTCSS CTCSS_14;
	static const CTCSS CTCSS_15;
	static const CTCSS CTCSS_16;
	static const CTCSS CTCSS_17;
	static const CTCSS CTCSS_18;
	static const CTCSS CTCSS_19;
	static const CTCSS CTCSS_20;
	static const CTCSS CTCSS_21;
	static const CTCSS CTCSS_22;
	static const CTCSS CTCSS_23;
	static const CTCSS CTCSS_24;
	static const CTCSS CTCSS_25;
	static const CTCSS CTCSS_26;
	static const CTCSS CTCSS_27;
	static const CTCSS CTCSS_28;
	static const CTCSS CTCSS_29;
	static const CTCSS CTCSS_30;
	static const CTCSS CTCSS_31;
	static const CTCSS CTCSS_32;
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
