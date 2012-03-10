#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include <iostream>
#include <QString>

enum DEMODMODE {
    dmAM,
    dmSAM,
    dmFMN,
    dmFMW,
    dmDSB,
    dmLSB,
    dmUSB,
    dmCWL,
    dmCWU,
    dmDIGL,
    dmDIGU,
	dmNONE
};

class Demod : public SignalProcessing
{
public:
    Demod(int samplerate, int size); 
	~Demod();

    CPX * ProcessBlock(CPX * in);
    DEMODMODE demodMode() const;
    void setDemodMode(DEMODMODE mode);
	static DEMODMODE StringToMode(QString m);
	static QString ModeToString(DEMODMODE dm);

private:
    DEMODMODE mode;
    
    float pllPhase;
    float samLockCurrent;
    float samLockPrevious;
    float samDc;
    float fmAfc;
	//Previous I/Q values, used in simpleFM
	float fmIPrev;
	float fmQPrev;
    
	//PLL variables
    float pllFrequency;
	float alpha,beta;

	//Moving averages for smooth am detector
	float amDc;
	float amSmooth;

    
	//SAM config
	float samLoLimit; //PLL range
	float samHiLimit;
	float samBandwidth;

	//FMN config
	float fmLoLimit;
	float fmHiLimit;
	float fmBandwidth;

	CPX PLL(CPX sig, float loLimit, float hiLimit);

    void PllSAM(CPX * in, CPX * out );    
    void PllFMN(CPX * in, CPX * out);    
    void PllFMW(CPX * in, CPX * out);

	//Simple time domain demod algorithms for testing and comparison
	void SimpleAM(CPX *in, CPX *out);
	void SimpleAMSmooth(CPX *in, CPX *out);
	void SimpleUSB(CPX *in, CPX *out);
	void SimpleLSB(CPX *in, CPX *out);
	void SimplePhase(CPX *in, CPX *out);
	void SimpleFM(CPX *in, CPX *out);
	void SimpleFM2(CPX *in, CPX *out);

};

