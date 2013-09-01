#ifndef DEMOD_SAM_H
#define DEMOD_SAM_H
#include "demod.h"

class Demod_SAM: public Demod
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod_SAM(int _inputRate, int _numSamples);
    ~Demod_SAM();
    void ProcessBlock(  CPX * in, CPX * out, int demodSamples );

    CPX PLL(CPX sig, float loLimit, float hiLimit);
private:
    float pllLowLimit; //PLL range
    float pllHighLimit;
    //PLL variables
    float pllFrequency;
    float pllPhase;
    float pllAlpha,pllBeta;


    //Like AM, but separate re and im for left/right
    double amDcRe;
    double amDcReLast;
    double amDcIm;
    double amDcImLast;

    CFir bpFilter;

};

#endif // DEMOD_SAM_H
