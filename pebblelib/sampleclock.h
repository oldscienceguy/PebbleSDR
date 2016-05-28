#ifndef SAMPLECLOCK_H
#define SAMPLECLOCK_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "QtCore"

//General purpose class for using incoming sample count for timing signals
class SampleClock
{
public:
	SampleClock(quint32 sampleRate);
	~SampleClock();

	//Call this on every new sample to update m_clock
	void inline tick() {m_clock++;}

	//Returns uSec delta between current clock and earlier clock
	quint32 uSecToCurrent(quint32 earlierClock);

	//Returns uSec delta between two clocks
	quint32 uSecDelta(quint32 earlierClock, quint32 laterClock);

	//Returns uSec from clock start
	quint32 uSec();

	//Returns current m_clock
	quint32 inline clock() {return m_clock;}

	//Starts everything over
	void reset();

	//Saves current clock for interval timing
	void startInterval();
	//Returns interval
	quint32 uSecInterval();

private:
	quint32 m_sampleRate;
	//quint32 can clock for 4294 sec (4,294,000,000 uSec) or 71 minutes
	//So don't worry about wrap
	quint32 m_clock;
	quint32 m_startInterval;

};

#endif // SAMPLECLOCK_H
