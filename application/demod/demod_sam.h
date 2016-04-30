#ifndef DEMOD_SAM_H
#define DEMOD_SAM_H
#include "demod.h"
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

class Demod_SAM: public Demod
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod_SAM(int _inputRate, int _numSamples);
    ~Demod_SAM();
    void processBlock(  CPX * in, CPX * out, int demodSamples );

	CPX pll(CPX sig, float loLimit, float hiLimit);
private:
	float m_pllLowLimit; //PLL range
	float m_pllHighLimit;
    //PLL variables
	float m_pllFrequency;
	float m_pllPhase;
	float m_pllAlpha;
	float m_pllBeta;


    //Like AM, but separate re and im for left/right
	double m_amDcRe;
	double m_amDcReLast;
	double m_amDcIm;
	double m_amDcImLast;

	CFir m_bpFilter;

};

#endif // DEMOD_SAM_H
