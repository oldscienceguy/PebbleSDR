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

class ProcessStep : public QObject
{
    Q_OBJECT

public:
	ProcessStep(int sr, int fc);
	~ProcessStep(void);

	//int sampleRate() {return sampleRate;}
	//int numSamples() {return numSamples;}
	void enableStep(bool _enableStep);
	bool isEnabled(){return enabled;}


protected:
	int numSamples;
	int numSamplesX2;
	int sampleRate;
	//Each process block returns it's output to a module specific buffer
	//So mixer has it's own output buffer, noise blanker has its own, etc.
	//This is not absolutely necessary, but makes it easier to test, debug, and add new blocks
	CPX *out;

	//Every step can be enabled/disabled
	bool enabled;


};
