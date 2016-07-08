//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "movingavgfilter.h"

//Cumulative moving average, no storage for samples needed
MovingAvgFilter::MovingAvgFilter()
{
	m_movingAverageType = CumulativeMovingAverage;
	m_filterLen = 0; //Not used
	m_sampleBuf = NULL;
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
		m_coeffBuf = NULL;
	} else {
		m_movingAverageType = WeightedMovingAverage;
		m_sampleBuf = new double[filterLen];
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
	if (m_coeffBuf != NULL)
		delete[] m_coeffBuf;
}

//Allows caller to set different weights for the same average on different calls
double MovingAvgFilter::newSample(double sample, double weight)
{
	if (m_movingAverageType != DecayMovingAverage)
		return sample;
	m_movingAvg = sample * weight + m_movingAvg * (1-weight);
	return m_movingAvg;
}

double MovingAvgFilter::newSample(double sample)
{
	if (m_movingAverageType == CumulativeMovingAverage) {
		//See Knuth TAOCP vol 2, 3rd edition, page 232
		//for each incoming sample x:
		//    prev_mean = m;
		//    n = n + 1;
		//    m = m + (x-m)/n;
		//    S = S + (x-m)*(x-prev_mean);
		//	standardDev = sqrt(S/n) or sqrt(S/n-1) depending on who you listen to
		double prevCumulativeMovingAverage = m_movingAvg;
		m_cumulativeMovingAverageCount++;
		m_movingAvg += (sample - m_movingAvg) / m_cumulativeMovingAverageCount;
		m_varianceSum += (sample - m_movingAvg) * (sample - prevCumulativeMovingAverage);
		//Dividing by N-1 (Bessel's correction) returns unbiased variance, dividing by N returns variance across the entire sample set
		//m_variance = m_varianceSum / (m_cumulativeMovingAverageCount - 1); //Defer to caller requests
		//m_stdDev = sqrt(m_variance); //Defer to caller requests it
		return m_movingAvg;

	} else if (m_movingAverageType == SimpleMovingAverage){
		//Special handling for first sample.  Otherwise average will not be correct
		if (!m_primed) {
			for (quint32 i =  0; i<m_filterLen; i++) {
				m_sampleBuf[i] = sample;
			}
			m_primed = true;
			m_sampleBufSum = sample * m_filterLen;
			m_movingAvg = sample;
			m_varianceSum = 0;
			m_variance = 0;
		} else {
			//New moving average = (oldMovingAverage - oldestSample + newSample) / filterLength
			double oldestSample = m_sampleBuf[m_ringIndex];
			double oldAvg = m_movingAvg;
			m_sampleBufSum = m_sampleBufSum - oldestSample + sample;
			m_movingAvg = m_sampleBufSum / m_filterLen;

			//Variance is the average of the squared difference from the mean
			//https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#On-line_algorithm
			//If variance is ever negative, we have an error in formula and stdDev will be nan
			//Lots of discussion about how to calculate variance for moving average
			//This came from Dan S at http://stackoverflow.com/questions/5147378/rolling-variance-algorithm
			/*
				Mean is simple to compute iteratively, but you need to keep the complete history of values in a circular buffer.

				next_index = (index + 1)/window_size;    // oldest x value is at next_index.

				new_mean = mean + (x_new - xs[next_index])/window_size;
				I have adapted Welford's algorithm and it works for all the values that I have tested with.

				varSum = var_sum + (x_new - mean) * (x_new - new_mean) - (xs[next_index] - mean) * (xs[next_index] - new_mean);

				xs[next_index] = x_new;
				index = next_index;
				To get the current variance just divide varSum by the window size: variance = varSum / window_size;
			  */
			m_varianceSum += (sample - oldAvg) * (sample - m_movingAvg) - (oldestSample - oldAvg) * (oldestSample - m_movingAvg);
#if 0
			//Only calculate variance and stdDev when caller asks for it, not every sample, for efficiency
			m_variance = m_varianceSum / m_filterLen;
			//Variance can still be a verrrrrry small negative number in some extreme cases, correct
			//Don't allow it to go to zero, or we'll get divide by zero errors for snr (mean / stdDev)
			if (m_variance <= 0)
				m_variance = ALMOSTZERO; //Defined in cpx.h
#endif
			//Replace oldestSample with new sample
			m_sampleBuf[m_ringIndex] = sample;

			//Update ring buffer and wrap
			m_ringIndex = ++m_ringIndex % m_filterLen;
		}

		return m_movingAvg;
	} else if (m_movingAverageType == WeightedMovingAverage){
		//Special handling for first sample.  Otherwise average will not be correct
		double weightedSample;
		if (!m_primed) {
			m_sampleBufSum = 0;
			for (quint32 i =  0; i<m_filterLen; i++) {
				m_sampleBuf[i] = sample;
				weightedSample = sample * m_coeffBuf[i];
				m_sampleBufSum += weightedSample;
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
				index = ++index % m_filterLen;
			}

			//Update ring buffer and wrap
			m_ringIndex = ++m_ringIndex % m_filterLen;
		}

		m_movingAvg = m_sampleBufSum / m_filterLen;
		//Todo: Update to use var and stdDev algorithm from simple moving avg

		return m_movingAvg;

	}
	return 0; //Error condition, should never get here
}

void MovingAvgFilter::reset()
{
	m_ringIndex = 0;
	m_sampleBufSum = 0;
	m_movingAvg = 0;
	m_primed = false;
	m_stdDev = 0;
	m_variance = 0;

	//Running mean
	m_cumulativeMovingAverageCount = 0;
}

//Only calculate variance on demand for efficiency
double MovingAvgFilter::variance()
{
	//Dividing by N-1 (Bessel's correction) returns unbiased variance, dividing by N returns variance across the entire sample set
	if (m_movingAverageType == CumulativeMovingAverage)
		m_variance = m_varianceSum / (m_cumulativeMovingAverageCount - 1);
	else
		m_variance = m_varianceSum / m_filterLen;
	//Variance can still be a verrrrrry small negative number in some extreme cases, correct
	//Don't allow it to go to zero, or we'll get divide by zero errors for snr (mean / stdDev)
	if (m_variance <= 0)
		m_variance = ALMOSTZERO; //Defined in cpx.h
	return m_variance;
}

//Only calc stdDev on demand for efficiency
double MovingAvgFilter::stdDev()
{
	m_stdDev = sqrt(m_variance);

	return m_stdDev;
}
