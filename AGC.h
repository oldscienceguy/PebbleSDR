#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"

class AGC :
	public SignalProcessing
{
public:
	enum AGCMODE {OFF,SLOW,MED,FAST,LONG};

	AGC(int sampleRate, int nSamples);
	~AGC(void);
	void setAgcMode(AGCMODE m);
	void setAgcGainTop(int g);
	int getAgcGainTop();

	CPX * ProcessBlock(CPX *in);
	CPX * ProcessBlock2(CPX *in);

private:
	AGCMODE agcMode;
	DelayLine *agcDelay;

	//These values are used to configure agc for different modes, they are not used directly
	float modeHangTimeMs;
	float modeFastHangTimeMs;
	float modeDecayTimeMs;
	float modeAttackTimeMs;
	float modeHangWeight;

	float gainNow;
	float gainPrev;
	float gainLimit;
	//Gain limits
	float gainFix;
	float gainTop;
	float gainBottom;

	float hangThreshold;
	//Times are all normalized to number of samples
	int hangSamples;
	int fastHangSamples;
	int hangCount;

	int attackSamples;
	float attackWeightNew;
	float attackWeightLast;
	float fastAttackWeightNew;
	float fastAttackWeightLast;

	int decaySamples;
	float decayWeightNew;
	float decayWeightLast;
	float fastDecayWeightNew;
	float fastDecayWeightLast;

	int delayTime;
	int fastDelayTime;


	//Utility function, used in many calculations exp(x) = e^x
	inline float eVal(float x) {return exp(-1000.0 / (x * sampleRate));}
};
