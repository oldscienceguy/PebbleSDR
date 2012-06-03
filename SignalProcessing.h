#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
#include <QMutex>
#include "../fftw-3.3.1/api/fftw3.h"
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
	CPX MAC(float *coeff, int numCoeff);
	CPX MAC(CPX *coeff, int numCoeff);


private:
	CPX *buffer;
	int size; //#elements in delay line
	int delay; //How long is the delay
	int head; //Insert index
	int last; //Last new value index
	QMutex mutex; //for MAC atomic

};
class FFT 
{    
public:
    FFT(int size);
    ~FFT();    
    void DoFFTWForward(CPX * in, CPX * out, int size);    
	void DoFFTWMagnForward(CPX * in,int size,float baseline,float correction,float *fbr);
    void DoFFTWInverse(CPX * in, CPX * out, int size);
	void FreqDomainToMagnitude(CPX * freqBuf, int size, float baseline, float correction, float *fbr);
	void OverlapAdd(CPX *out, int size);

	CPX *timeDomain;
	CPX *freqDomain;
	int fftSize;

private:

	fftwf_plan plan_fwd;
	fftwf_plan plan_rev;
	CPX *buf;
	CPX *overlap;
	int half_sz;

};

class SignalProcessing : public QObject
{
    Q_OBJECT

public:
	SignalProcessing(int sr, int fc);
	~SignalProcessing(void);

	//Useful conversion functions
	static inline float cpxToWatts(CPX cx) //Returns power (watts) for sample c
		{return (cx.re * cx.re + cx.im * cx.im)/2;}
	//Calculates the total power of all samples in buffer
	static float totalPower(CPX *in, int bsize);

	static double dBm_2_Watts(double dBm);
	static double watts_2_dBm(double watts);
	static double dBm_2_RMSVolts(double dBm, double impedance);
	static double rmsVolts_2_dBm(double volts, double impedance);
	
	//db conversion functions from Steven Smith book
	static float powerToDb(float p);
	static float dbToPower(float db);

	static float amplitudeToDb(float a);
	static float dbToAmplitude(float db);

	static int dbToSUnit(float db);

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
