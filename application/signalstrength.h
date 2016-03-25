#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"

#define SPECDBMOFFSET 100.50

class SignalStrength :
	public ProcessStep
{
public:
	SignalStrength(quint32 _sampleRate, quint32 _bufferSize);
	~SignalStrength(void);
	inline double peakdB() {return m_peakdB;}
	inline double peakPower() {return m_peakPower;}
	inline double avgdB() {return m_avgdB;}
	inline double avgPower() {return m_avgPower;}
	inline double snr() {return m_snr;}

	inline double extValue() {return m_extValue;}

	void setExtValue(double v);

	CPX * ProcessBlock(CPX *in, int numSamples, double squelchdB);


private:
	double m_peakdB;
	double m_avgdB;

	double m_peakPower;
	double m_avgPower;

	double m_rms;
	double m_rmsdB;

	double m_snr;

	double m_extValue; //Used for other power readings, like goretzel (cw) output

};
