#ifndef IQBALANCE_H
#define IQBALANCE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "processstep.h"

class IQBalance : public ProcessStep
{
public:
	IQBalance(quint32 _sampleRate, quint32 _bufferSize);
	~IQBalance();
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
	double *snrSquared;
	//std::vector has nth element function which will give us fast median
	std::vector<double> medianBuf;
	void CalcNoise(CPX *inFreqDomain);
	void FindPeak();
};

#endif // IQBALANCE_H
