#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
#include "db.h" //From pebblelib with static db conversions
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
