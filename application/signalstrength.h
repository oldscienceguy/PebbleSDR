#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "processstep.h"
#include <QElapsedTimer>

class SignalStrength :
	public ProcessStep
{
	//This needs to included in any class that uses signals or slots
	Q_OBJECT

public:
	SignalStrength(quint32 _sampleRate, quint32 _bufferSize);
	~SignalStrength(void);

	void setSquelch(int squelchDb) {m_squelchDb = squelchDb;}

	void reset(); //Initialize all running mean variables

	inline double peakDb() {return m_peakDb;}
	inline double peakPower() {return m_peakAmplitude;}
	inline double avgDb() {return m_avgDb;}
	inline double snrDb() {return m_snrDb;}
	inline double floorDb() {return m_floorDb;}

	inline double extValue() {return m_extValue;}

	void setExtValue(double v);

	CPX *tdEstimate(CPX *in, quint32 numSamples, bool estNoise = false, bool estSignal = false);

	void fdEstimate(double *spectrum, int spectrumBins, quint32 spectrumSampleRate,
		float bpLowFreq, float bpHighFreq, double mixerFreq);
signals:
	void newSignalStrength(double peakDb, double avgDb, double snrDb, double floorDb, double extValue);

private:
	int m_squelchDb;

	//ms between updates
	const quint32 updateInterval = 100; //10 updates per sec
	QElapsedTimer updateTimer;

	double m_peakAmplitude;
	double m_peakDb;

	//For running mean, variance, stdDev
	double m_runningMean;
	double m_avgDb;

	double m_rms;
	double m_rmsDb;

	double m_snrDb;
	double m_floorDb; //Noise floor

	double m_extValue; //Used for other power readings, like goretzel (cw) output


	double m_stdDev;
	double m_variance;
	double m_signal;
	double m_noise;


};
