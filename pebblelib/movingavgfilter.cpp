//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "movingavgfilter.h"

/*
   Simple moving average is the optimal and fastest filter for removing white noise
   Filter length is based on the number of samples for the rise phase of the signal
   ie if there are 10 samples during the rise interval, then filter should be 10
  */
MovingAvgFilter::MovingAvgFilter(quint32 filterLen, MOVING_AVG_TYPE movingAverageType)
{
	if (filterLen == 0)
		filterLen = 1;
	m_sampleBuf = new double[filterLen];
	m_prodBuf = new double[filterLen];
	m_filterLen = filterLen;
	m_movingAverageType = movingAverageType;
	reset();
}

MovingAvgFilter::~MovingAvgFilter()
{
	delete[] m_sampleBuf;
	delete[] m_prodBuf;
}

double MovingAvgFilter::newSample(double sample)
{
	double sampleX2 = sample * sample;

	if (m_movingAverageType == CumulativeMovingAverage) {
		//See Knuth TAOCP vol 2, 3rd edition, page 232
		//for each incoming sample x:
		//    prev_mean = m;
		//    n = n + 1;
		//    m = m + (x-m)/n;
		//    S = S + (x-m)*(x-prev_mean);
		//	standardDev = sqrt(S/n) or sqrt(S/n-1) depending on who you listen to
		double prevCumulativeMovingAverage = m_cumulativeMovingAverage;
		m_cumulativeMovingAverageCount++;
		m_cumulativeMovingAverage += (sample - m_cumulativeMovingAverage) / m_cumulativeMovingAverageCount;
		m_S += (sample - m_cumulativeMovingAverage) * (sample - prevCumulativeMovingAverage);
		//Dividing by N-1 (Bessel's correction) returns unbiased variance, dividing by N returns variance across the entire sample set
		m_variance = m_S / (m_cumulativeMovingAverageCount - 1);
		m_stdDev = sqrt(m_variance);
		return m_cumulativeMovingAverage;

	} else if (m_movingAverageType == SimpleMovingAverage){
		//Special handling for first sample.  Otherwise average will not be correct
		if (!m_primed) {
			for (quint32 i =  0; i<m_filterLen; i++) {
				m_sampleBuf[i] = sample;
				m_prodBuf[i] = sampleX2;
			}
			m_primed = true;
			m_sampleBufSum = sample * m_filterLen;
			m_prodBufSum = sampleX2 * m_filterLen;
			m_simpleMovingAvg = sample;
			m_variance = 0; //Check
			m_stdDev = 0;
			return m_simpleMovingAvg;
		}

		//New moving average = (oldMovingAverage - oldestSample + newSample) / filterLength
		m_sampleBufSum = m_sampleBufSum - m_sampleBuf[m_ringIndex] + sample;
		//new sum of the products for variance and stdDev
		m_prodBufSum = m_prodBufSum - m_prodBuf[m_ringIndex] + sampleX2;

		//Replace oldestSample with new sample
		m_sampleBuf[m_ringIndex] = sample;
		m_prodBuf[m_ringIndex] = sampleX2;

		//Update ring buffer and wrap
		m_ringIndex = ++m_ringIndex % m_filterLen;

		m_simpleMovingAvg = m_sampleBufSum / m_filterLen;

		m_variance = (m_filterLen * m_prodBufSum - (m_sampleBufSum * m_sampleBufSum)) / (m_filterLen * (m_filterLen -1));
		m_stdDev = sqrt(m_variance);

		return m_simpleMovingAvg;
	} else if (m_movingAverageType == WeightedMovingAverage){
		//Not implemented yet
	}
	return 0; //Error condition, should never get here
}

void MovingAvgFilter::reset()
{
	m_ringIndex = 0;
	m_sampleBufSum = 0;
	m_simpleMovingAvg = 0;
	m_primed = false;

	//Running mean
	m_cumulativeMovingAverage = 0;
	m_cumulativeMovingAverageCount = 0;
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
