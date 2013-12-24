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

    void ProcessBlockFM1(CPX *in, CPX *out, int demodSamples);
    void ProcessBlockFM2(CPX *in, CPX *out, int demodSamples);
    void ProcessBlockNCO(CPX *in, CPX *out, int demodSamples);

    void PllFMN(CPX *in, CPX *out, int demodSamples);

    void SimplePhase(CPX *in, CPX *out, int demodSamples);
    void Init(double samplerate);
private:
    //Previous I/Q values, used in simpleFM
    float fmIPrev;
    float fmQPrev;

    //FMN PLL config (reference)
    float fmBandwidth;
    float pllFreqErrorDC;
    float ncoFrequency;
    float ncoLLimit;
    float ncoHLimit;
    float pllPhase;
    float pllAlpha,pllBeta;
    float pllDcAlpha;
    float pllOutGain;

    CFir lpFilter;

};

#endif // DEMOD_NFM_H
