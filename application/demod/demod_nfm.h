#ifndef DEMOD_NFM_H
#define DEMOD_NFM_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "demod.h"

class Demod_NFM : public Demod
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod_NFM(int _inputRate, int _numSamples);
    virtual ~Demod_NFM();

	void processBlockFM1(CPX *in, CPX *out, int demodSamples);
	void processBlockFM2(CPX *in, CPX *out, int demodSamples);
	void processBlockNCO(CPX *in, CPX *out, int demodSamples);

	void pllFMN(CPX *in, CPX *out, int demodSamples);

	void simplePhase(CPX *in, CPX *out, int demodSamples);
	void init(double samplerate);
private:
    //Previous I/Q values, used in simpleFM
	float m_fmIPrev;
	float m_fmQPrev;

    //FMN PLL config (reference)
	float m_fmBandwidth;
	float m_pllFreqErrorDC;
	float m_ncoFrequency;
	float m_ncoLLimit;
	float m_ncoHLimit;
	float m_pllPhase;
	float m_pllAlpha;
	float m_pllBeta;
	float m_pllDcAlpha;
	float m_pllOutGain;

	CFir m_lpFilter;

};

#endif // DEMOD_NFM_H
