#ifndef IQBALANCE_H
#define IQBALANCE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference

#include "SignalProcessing.h"

class IQBalance : public SignalProcessing
{
public:
	IQBalance(int sr, int fc);
	CPX *ProcessBlock(CPX *in);

	double getGainFactor();
	double getPhaseFactor();
	bool getEnabled();
	void setGainFactor(double g);
	void setPhaseFactor(double f);
	void setEnabled(bool b);
	void setAutomatic(bool b);

private:
	double gainFactor;
	double phaseFactor;
	bool enabled;
	bool automatic;
};

#endif // IQBALANCE_H
