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

	void processBlock(CPX *in, CPX *out, int demodSamples);
	void processBlockFiltered(CPX *in, CPX *out, int demodSamples);
	void setBandwidth(double bandwidth);

private:
    //DC Filtering
	double m_amDc;
	double m_amDcLast;

	CFir m_lpFilter;

};

#endif // DEMOD_AM_H
