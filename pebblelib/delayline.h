#ifndef DELAYLINE_H
#define DELAYLINE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

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

#endif // DELAYLINE_H
