#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
#include <QMutex>
#include "global.h"

#include "QObject"

/*

ANF algorithm from SuperMax, DTTSP, Doug Smith book, Andy Talbot book, etc
*/

/*
Diagram buffer behavior
Samples are in time order, newest first.  Note: sound buffer is in time order, oldest first

*/
class DelayLine
{
public:
	//Delay is normally 0, ie use the most current sample and n prev, except in special algorithms
	//like adaptive filters
	DelayLine(int numSamples, int delay=0);
	~DelayLine();

	//Adds new sample to line
	void NewSample(CPX v);
	//Returns delayed sample # i
	CPX NextDelay(int i);
	//Array access
	CPX operator [] (int i);
	//Multiply and Accumulate or convolution sum
    CPX MAC(double *coeff, int numCoeff);
	CPX MAC(CPX *coeff, int numCoeff);


private:
	CPX *buffer;
	int size; //#elements in delay line
	int delay; //How long is the delay
	int head; //Insert index
	int last; //Last new value index
	QMutex mutex; //for MAC atomic

};

class SignalProcessing : public QObject
{
    Q_OBJECT

public:
	SignalProcessing(int sr, int fc);
	~SignalProcessing(void);

	//Useful conversion functions
    static inline double cpxToWatts(CPX cx) //Returns power (watts) for sample c
		{return (cx.re * cx.re + cx.im * cx.im)/2;}
	//Calculates the total power of all samples in buffer
    static double totalPower(CPX *in, int bsize);

	static double dBm_2_Watts(double dBm);
	static double watts_2_dBm(double watts);
	static double dBm_2_RMSVolts(double dBm, double impedance);
	static double rmsVolts_2_dBm(double volts, double impedance);
	
	//db conversion functions from Steven Smith book
    static double powerToDb(double p);
    static double dbToPower(double db);

    static double amplitudeToDb(double a);
    static double dbToAmplitude(double db);

    static int dbToSUnit(double db);

	int SampleRate() {return sampleRate;}
	int NumSamples() {return numSamples;}


protected:
	int numSamples;
	int numSamplesX2;
	int sampleRate;
	//Each process block returns it's output to a module specific buffer
	//So mixer has it's own output buffer, noise blanker has its own, etc.
	//This is not absolutely necessary, but makes it easier to test, debug, and add new blocks
	CPX *out;


};
