#ifndef DEMOD_AM_H
#define DEMOD_AM_H

#include "demod.h"

class Demod_AM : public Demod
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod_AM(int _inputRate, int _numSamples);
    ~Demod_AM();

    void ProcessBlock(CPX *in, CPX *out, int demodSamples);
    void ProcessBlockFiltered(CPX *in, CPX *out, int demodSamples);
    void SetBandwidth(double bandwidth);

private:
    //Moving averages for smooth am detector
    double amDc;
    double amDcLast;
    double amSmooth;

    CFir lpFilter;

};

#endif // DEMOD_AM_H
