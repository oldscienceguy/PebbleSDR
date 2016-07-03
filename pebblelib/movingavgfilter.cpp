//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "movingavgfilter.h"

//Cumulative moving average, no storage for samples needed
MovingAvgFilter::MovingAvgFilter()
{
	m_movingAverageType = CumulativeMovingAverage;
	m_filterLen = 0; //Not used
	m_sampleBuf = NULL;
	m_prodBuf = NULL;
	m_coeffBuf = NULL;
	reset();
}

/*
   Simple moving average is the optimal and fastest filter for removing white noise
   Filter length is based on the number of samples for the rise phase of the signal
   ie if there are 10 samples during the rise interval, then filter should be 10
  */
MovingAvgFilter::MovingAvgFilter(quint32 filterLen)
{
	m_movingAverageType = SimpleMovingAverage;
	if (filterLen == 0)
		filterLen = 1;
	m_filterLen = filterLen;
	m_sampleBuf = new double[filterLen];
	m_prodBuf = new double[filterLen];
	m_coeffBuf = NULL; //Not used
	reset();
}
//Weighted moving average constructor
MovingAvgFilter::MovingAvgFilter(quint32 filterLen, double coeff[])
{
	m_filterLen = filterLen;
	if (filterLen == 0) {
		//Special case for decay moving average
		m_movingAverageType = DecayMovingAverage;
		m_sampleBuf = NULL;
		m_prodBuf = NULL;
		m_coeffBuf = NULL;
	} else {
		m_movingAverageType = WeightedMovingAverage;
		m_sampleBuf = new double[filterLen];
		m_prodBuf = NULL; //Not used
		m_coeffBuf = new double[filterLen];
		for (quint32 i=0; i<filterLen; i++)
			m_coeffBuf[i] = coeff[i]; //Local copy
	}
	reset();
}

MovingAvgFilter::~MovingAvgFilter()
{
	if (m_sampleBuf != NULL)
		delete[] m_sampleBuf;
	if (m_prodBuf != NULL)
		delete[] m_prodBuf;
	if (m_coeffBuf != NULL)
		delete[] m_coeffBuf;
}

//Allows caller to set different weights for the same average on different calls
double MovingAvgFilter::newSample(double sample, double weight)
{
	if (m_movingAverageType != DecayMovingAverage)
		return sample;
	m_weightedMovingAverage = sample * weight + m_weightedMovingAverage * (1-weight);
	return m_weightedMovingAverage;
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
		} else {
			//New moving average = (oldMovingAverage - oldestSample + newSample) / filterLength
			m_sampleBufSum = m_sampleBufSum - m_sampleBuf[m_ringIndex] + sample;
			//new sum of the products for variance and stdDev
			m_prodBufSum = m_prodBufSum - m_prodBuf[m_ringIndex] + sampleX2;

			//Replace oldestSample with new sample
			m_sampleBuf[m_ringIndex] = sample;
			m_prodBuf[m_ringIndex] = sampleX2;

			//Update ring buffer and wrap
			m_ringIndex = ++m_ringIndex % m_filterLen;
		}

		m_simpleMovingAvg = m_sampleBufSum / m_filterLen;
		m_variance = (m_filterLen * m_prodBufSum - (m_sampleBufSum * m_sampleBufSum)) / (m_filterLen * (m_filterLen -1));
		m_stdDev = sqrt(m_variance);

		return m_simpleMovingAvg;
	} else if (m_movingAverageType == WeightedMovingAverage){
		//Special handling for first sample.  Otherwise average will not be correct
		double weightedSample;
		if (!m_primed) {
			m_sampleBufSum = 0;
			m_prodBufSum = 0;
			for (quint32 i =  0; i<m_filterLen; i++) {
				m_sampleBuf[i] = sample;
				weightedSample = sample * m_coeffBuf[i];
				m_sampleBufSum += weightedSample;
				m_prodBufSum += weightedSample * weightedSample;
			}
			m_primed = true;
		} else {
			//Brute force mac with weights.  Same as FIR filter
			//Replace oldestSample with new sample
			m_sampleBuf[m_ringIndex] = sample;

			m_sampleBufSum = 0;
			quint16 index = m_ringIndex;
			for (quint32 i=0; i<m_filterLen; i++) {
				weightedSample = m_sampleBuf[index] * m_coeffBuf[i]; //Weighted
				m_sampleBufSum += weightedSample;
				m_prodBufSum += weightedSample * weightedSample;
				index = ++index % m_filterLen;
			}

			//Update ring buffer and wrap
			m_ringIndex = ++m_ringIndex % m_filterLen;
		}

		m_weightedMovingAverage = m_sampleBufSum / m_filterLen;
		m_variance = (m_filterLen * m_prodBufSum - (m_sampleBufSum * m_sampleBufSum)) / (m_filterLen * (m_filterLen -1));
		m_stdDev = sqrt(m_variance);

		return m_weightedMovingAverage;

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

	m_weightedMovingAverage = 0;
}
