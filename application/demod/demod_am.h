#ifndef DEMOD_AM_H
#define DEMOD_AM_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

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
    //DC Filtering
    double amDc;
    double amDcLast;

    CFir lpFilter;

};

#endif // DEMOD_AM_H
