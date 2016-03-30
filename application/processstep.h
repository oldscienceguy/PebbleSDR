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
	ProcessStep(quint32 _sampleRate, quint32 _bufferSize);
	virtual ~ProcessStep(void);

	virtual CPX *process(CPX *in, quint32 _numSamples);

	quint32 getSampleRate() {return sampleRate;}
	quint32 getBufferSize() {return bufferSize;}
	void enableStep(bool _enableStep);
	bool isEnabled(){return enabled;}


protected:
	quint32 numSamples;
	quint32 bufferSize;
	quint32 numSamplesX2;
	quint32 sampleRate;
	//Each process block returns it's output to a module specific buffer
	//So mixer has it's own output buffer, noise blanker has its own, etc.
	//This is not absolutely necessary, but makes it easier to test, debug, and add new blocks
	CPX *out;

	//Every step can be enabled/disabled
	bool enabled;


};
