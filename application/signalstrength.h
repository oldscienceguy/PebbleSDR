#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"

#define SPECDBMOFFSET 100.50

class SignalStrength :
	public ProcessStep
{
public:
	SignalStrength(int sr, int ns);
	~SignalStrength(void);
	float instFValue();
	float avgFValue();
    float extFValue();
    void setExtValue(float v);
	char instCValue(); //Char representing db?
	char avgCValue();
	void setCorrection(const float value);
    CPX * ProcessBlock(CPX *in, int downConvertLen, int squelch);


private:
	float instValue; //Instantaneous value
    float avgValue; //90/10 weighted average
    float extValue; //Used for other power readings, like goretzel (cw) output
	float correction; //Constant value to make things fit

};
