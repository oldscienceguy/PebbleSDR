#ifndef DEMOD_NFM_H
#define DEMOD_NFM_H

#include "demod.h"

class Demod_NFM : public Demod
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod_NFM(int _inputRate, int _numSamples);
    virtual ~Demod_NFM();

    void ProcessBlock(CPX *in, CPX *out, int demodSamples);

    void SimpleFM(CPX *in, CPX *out, int demodSamples);
    void SimpleFM2(CPX *in, CPX *out, int demodSamples);

    void PllFMN(CPX *in, CPX *out, int demodSamples);

    void SimplePhase(CPX *in, CPX *out, int _numSamples);
private:
    //Previous I/Q values, used in simpleFM
    float fmIPrev;
    float fmQPrev;

    //FMN PLL config (reference)
    float fmLoLimit;
    float fmHiLimit;
    float fmBandwidth;
    float fmDCOffset;
    //PLL variables
    float pllFrequency;
    float pllPhase;
    float pllAlpha,pllBeta;

};

#endif // DEMOD_NFM_H
