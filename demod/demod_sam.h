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

private:
    float samLockCurrent;
    float samLockPrevious;
    float samDc;
    //SAM config
    float samLoLimit; //PLL range
    float samHiLimit;
    float samBandwidth;


};

#endif // DEMOD_SAM_H
