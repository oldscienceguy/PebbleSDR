#ifndef MOVINGAVGFILTER_H
#define MOVINGAVGFILTER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

/*
	Simple moving average for doubles (templatize if needed for other types)
	See http://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch15.pdf
	Could be extended to support weighted moving average if needed
	Moving average is a simple low pass FIR filter that is the fastest possible FIR filter algorithm
	Useful for on/off keying like CW or RTTY where there is a sharp transition on the rising signal
	The size of the filter is based on the estimated rise or fall time of the signal, ie 10ms for CW
	m_filterLen = sampleRate * msRiseTime -> 8000sps * .010 = 80

	See https://en.wikipedia.org/wiki/Moving_average for different variations
	Simple Moving Average (SMA)
	Cumulative Moving Average (CMA)
	Weighted Moving Average (WMA)

	See http://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch15.pdf
	Optimal filter for removing white noise from a sample
*/
class MovingAvgFilter
{
public:
	enum MOVING_AVG_TYPE {SimpleMovingAverage, CumulativeMovingAverage, WeightedMovingAverage, DecayMovingAverage};

	MovingAvgFilter(); //Cumulative moving average
	MovingAvgFilter(quint32 filterLen); //Simple moving average
	MovingAvgFilter(quint32 filterLen, double coeff[]); //Weighted moving average
	~MovingAvgFilter();

	//Puts new sample into delay buffer and returns new moving average
	double newSample(double sample);

	//Special call for decay average where len==0.  Allows 1st coeff to be specified on each sample
	//Simple 2 element weighted average, used in multiple places
	//Weight is the factor to weight the input when calculating the average
	//Weight=20: (input * 1/20) + average * (19/20) - Short average, only 20 loops to make average == steady input
	//Weight=800: (input * 1/800) + average * (799/800) - Long average, stead input would take 800 to == average
	double newSample(double sample, double weight);

	//Clears moving average and starts over
	void reset();

	double simpleMovingAvg() {return m_simpleMovingAvg;}
	double variance() {return m_variance;}
	double stdDev() {return m_stdDev;}
	double cumulativeMovingAverage() {return m_cumulativeMovingAverage;}
	double weightedMovingAverage() {return m_weightedMovingAverage;}
private:
	MOVING_AVG_TYPE m_movingAverageType;

	//For simple moving average
	bool m_primed; //Flag for special handling of first samples
	quint32 m_filterLen; //Number of samples to average
	double *m_sampleBuf; //Ring buffer for samples
	double *m_prodBuf; //Ring buffer for products
	double *m_coeffBuf; //For weighted averages
	quint32 m_ringIndex; //Points to oldest sample in buffer
	double m_sampleBufSum; //Sum of all the entries in the delay buffer
	double m_prodBufSum; //mac
	double m_simpleMovingAvg; //Last average returned

	//For cumulative moving average
	double m_cumulativeMovingAverage;
	quint32 m_cumulativeMovingAverageCount;
	double m_S;

	double m_weightedMovingAverage;

	//Common
	double m_stdDev;
	double m_variance;

};

#endif // MOVINGAVGFILTER_H
