#include "sampleclock.h"

//Returns uSec between clocks
SampleClock::SampleClock(quint32 sampleRate)
{
	m_sampleRate = sampleRate;
}

SampleClock::~SampleClock()
{

}

quint32 SampleClock::uSecToCurrent(quint32 earlierClock)
{
	return uSecDelta(earlierClock, m_clock);
}

quint32 SampleClock::uSecDelta(quint32 earlierClock, quint32 laterClock)
{
	if (earlierClock >= laterClock)
		return 0; //earlier can't be > later
	quint64 delta = ((double)(laterClock - earlierClock) * 1.0e6) / m_sampleRate;
	return delta;
}

quint32 SampleClock::uSec()
{
	return uSecDelta(0, m_clock);
}

void SampleClock::reset()
{
	m_clock = 0;
	m_startInterval = 0;
}

void SampleClock::startInterval()
{
	m_startInterval = m_clock;
}

quint32 SampleClock::uSecInterval()
{
	//Don't reset interval so we can call again to compare
	return uSecDelta(m_startInterval, m_clock);
}
