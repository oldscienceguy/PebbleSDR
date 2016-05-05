#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include "delayline.h"

class AGC :
	public ProcessStep
{
public:
	enum AgcMode {AGC_OFF, ACG_FAST, AGC_MED, AGC_SLOW, AGC_LONG};

	AGC(quint32 _sampleRate, quint32 _bufferSize);
	~AGC(void);
	void setAgcMode(AgcMode m);
    void setAgcThreshold(int g);
    int getAgcThreshold();

	CPX * processBlock(CPX *in);
	//CPX * processBlock2(CPX *in);

	//CPX *processBlock3(CPX *in); //Testing cuteSDR algorithm

	void setParameters(bool useHang, int threshold, int manualGainDb, int slopeFactor, int decay);
private:
	static const int DEFAULT_THRESHOLD = 20;
	static const int MAX_DELAY_BUF = 2048;
	//CuteSDR algorithm constants converted for FP CPX samples and predefined modes
	//signal delay line time delay in seconds.
	//adjust to cover the impulse response time of filter
	static constexpr float DELAY_TIMECONST = .015;

	//Peak Detector window time delay in seconds.
	static constexpr float WINDOW_TIMECONST = .018;

	//attack time constant in seconds
	//just small enough to let attackave charge up within the DELAY_TIMECONST time
	static constexpr float ATTACK_RISE_TIMECONST = .002;
	static constexpr float ATTACK_FALL_TIMECONST = .005;

	//ratio between rise and fall times of Decay time constants
	//adjust for best action with SSB
	static constexpr float DECAY_RISEFALL_RATIO = .3;

	// hang timer release decay time constant in seconds
	static constexpr float RELEASE_TIMECONST = .05;

	//limit output to about 3db of max
	static constexpr float AGC_OUTSCALE = 0.7;

	//keep max in and out the same (was 32767 for cuteSDR, but we're FP cpx
	static constexpr float MAX_AMPLITUDE = 1.0;
	static constexpr float MAX_MANUAL_AMPLITUDE = 1.0; //Same, was 32767

	//const for calc log() so that a value of 0 magnitude == -8
	//corresponding to -160dB
	//K = 10^( -8 + log(1) )
	//#define MIN_CONSTANT 3.2767e-4
	static constexpr float MIN_CONSTANT = 1e-8;

	AgcMode m_agcMode;
	DelayLine *m_agcDelay;

	//These values are used to configure agc for different modes, they are not used directly
	//float modeHangTimeMs;
	//float modeFastHangTimeMs;
	//float modeDecayTimeMs;
	//float modeAttackTimeMs;
	//float modeHangWeight;

	//float gainNow;
	//float gainPrev;
	//float gainLimit;
	//Gain limits
	//float gainFix;
	//float gainTop;
	//float gainBottom;

	//float hangThreshold;
	//Times are all normalized to number of samples
	//int hangSamples;
	//int fastHangSamples;
	//int hangCount;

	//int attackSamples;
	//float attackWeightNew;
	//float attackWeightLast;
	//float fastAttackWeightNew;
	//float fastAttackWeightLast;

	//int decaySamples;
	//float decayWeightNew;
	//float decayWeightLast;
	//float fastDecayWeightNew;
	//float fastDecayWeightLast;

	//int delayTime;
	//int fastDelayTime;

    //CuteSDR implementation
    int thresholdFromUi;

    //internal copy of AGC settings parameters
	bool m_useHang;
	int m_threshold;
	int m_manualGainDb;
	double m_manualGainAmp;

	//int m_Slope;
	int m_decay;
	double m_sampleRate;

	double m_slopeFactor;

	double m_decayAvg;
	double m_attackAvg;

	double m_attackRiseAlpha;
	double m_attackFallAlpha;
	double m_decayRiseAlpha;
	double m_decayFallAlpha;

	double m_fixedGain;
	double m_knee;
	double m_gainSlope;
	double m_peak;

	int m_sigDelayPtr;
	int m_magBufPos;
	//int m_delaySize;
	int m_delaySamples;
	int m_windowSamples;
	int m_hangTime;
	int m_hangTimer;

	QMutex m_mutex;		//for keeping threads from stomping on each other
	CPX m_sigDelayBuf[MAX_DELAY_BUF];
	double m_magBuf[MAX_DELAY_BUF];

	//Utility function, used in many calculations exp(x) = e^x
	inline float eVal(float x) {return exp(-1000.0 / (x * sampleRate));}
};
