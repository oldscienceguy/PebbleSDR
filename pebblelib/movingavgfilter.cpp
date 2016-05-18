#include "movingavgfilter.h"

MovingAvgFilter::MovingAvgFilter(quint32 filterLen)
{
	m_delayBuf = new double[filterLen];
	m_filterLen = filterLen;
	reset();
}

MovingAvgFilter::~MovingAvgFilter()
{
	delete[] m_delayBuf;
}

double MovingAvgFilter::newSample(double sample)
{
	//Special handling for first sample.  Otherwise average will not be correct
	if (!m_primed) {
		for (quint32 i =  0; i<m_filterLen; i++)
			m_delayBuf[i] = sample;
		m_primed = true;
		m_delayBufSum = sample * m_filterLen;
		m_movingAvg = sample;
		return m_movingAvg;
	}

	//New moving average = (oldMovingAverage - oldestSample + newSample) / filterLength
	m_delayBufSum = m_delayBufSum - m_delayBuf[m_ringIndex] + sample;

	//Replace oldestSample with new sample
	m_delayBuf[m_ringIndex] = sample;

	//Update ring buffer and wrap
	m_ringIndex = ++m_ringIndex % m_filterLen;

	m_movingAvg = m_delayBufSum / m_filterLen;
	return m_movingAvg;
}

void MovingAvgFilter::reset()
{
	m_ringIndex = 0;
	m_delayBufSum = 0;
	m_movingAvg = 0;
	m_primed = false;
}

double MovingAvgFilter::weightedAvg(double prevAvg, double sample, quint32 weight)
{
	if (weight <= 1.0)
		return sample;
	return sample * (1.0 / weight) + prevAvg * (1.0 - (1.0 / weight));
}
