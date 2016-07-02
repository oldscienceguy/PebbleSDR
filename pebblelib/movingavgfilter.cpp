#include "movingavgfilter.h"

MovingAvgFilter::MovingAvgFilter(quint32 filterLen, bool useRunningMean)
{
	if (filterLen == 0)
		filterLen = 1;
	m_delayBuf = new double[filterLen];
	m_filterLen = filterLen;
	m_useRunningMean = useRunningMean;
	reset();
}

MovingAvgFilter::~MovingAvgFilter()
{
	delete[] m_delayBuf;
}

double MovingAvgFilter::newSample(double sample)
{
	if (m_useRunningMean) {
		//See Knuth TAOCP vol 2, 3rd edition, page 232
		//for each incoming sample x:
		//    prev_mean = m;
		//    n = n + 1;
		//    m = m + (x-m)/n;
		//    S = S + (x-m)*(x-prev_mean);
		//	standardDev = sqrt(S/n) or sqrt(S/n-1) depending on who you listen to
		double prevMean = m_runningMean;
		m_runningMeanCount++;
		m_runningMean += (sample - m_runningMean) / m_runningMeanCount;
		m_S += (sample - m_runningMean) * (sample - prevMean);
		//Dividing by N-1 (Bessel's correction) returns unbiased variance, dividing by N returns variance across the entire sample set
		m_variance = m_S / (m_runningMeanCount - 1);
		m_stdDev = sqrt(m_variance);
		//Not tested, here for future use
		return m_runningMean;

	} else {
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
}

void MovingAvgFilter::reset()
{
	m_ringIndex = 0;
	m_delayBufSum = 0;
	m_movingAvg = 0;
	m_primed = false;

	//Running mean
	m_runningMean = 0;
	m_runningMeanCount = 0;
	m_S = 0;
	m_stdDev = 0;
	m_variance = 0;

}

double MovingAvgFilter::weightedAvg(double prevAvg, double sample, quint32 weight)
{
	if (weight <= 1.0)
		return sample;
	return sample * (1.0 / weight) + prevAvg * (1.0 - (1.0 / weight));
}
