//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "fft.h"
#include "db.h"
#include <QDebug>

//Forward declarations of supported subclasses in Factory
#include "fftw.h"
#include "fftcute.h"
#include "fftooura.h"
#include "fftaccelerate.h"

FFT::FFT() :
    useIntegerFFT(false)
{
    if (useIntegerFFT) {
        ampMax = 32767.0;	//maximum sin wave Pk for 16 bit input data
        overLimit = 32000.0;	//limit for detecting over ranging inputs
    } else {
        ampMax = 1.0; //+/- 1 format
        overLimit = 0.9;
    }


    timeDomain = NULL;
    freqDomain = NULL;
    workingBuf = NULL;
    overlap = NULL;

    fftParamsSet = false; //Only set to true in FFT::FFTParams(...)

    invert = false;
	dBCompensation = DB::maxDb;


	fftBinToX = NULL;

    //These will be init on first use in CalcAverages
    bufferCnt = 0;
    averageCnt = 0;
    FFTPwrAvgBuf = NULL;
    FFTAvgBuf = NULL;
    FFTPwrSumBuf = NULL;
    movingAvgLimit = 2;


}
FFT::~FFT()
{
    if (timeDomain) CPXBuf::free(timeDomain);
    if (freqDomain) CPXBuf::free(freqDomain);
    if (workingBuf) CPXBuf::free(workingBuf);
    if (overlap) CPXBuf::free(overlap);
	if (fftBinToX) delete fftBinToX;
    if (FFTPwrAvgBuf != NULL)
        delete FFTPwrAvgBuf;
    if (FFTAvgBuf != NULL)
        delete FFTAvgBuf;
    if (FFTPwrSumBuf != NULL)
        delete FFTPwrSumBuf;
}

FFT* FFT::Factory()
{
#ifdef USE_FFTW
	qDebug()<<"Using FFTW";
	return new FFTfftw();
#endif
#ifdef USE_FFTCUTE
	qDebug()<<"Using FFT CUTE";
	return new CFft();
#endif
#ifdef USE_FFTOOURA
	qDebug()<<"Using FFT Ooura";
	return new FFTOoura();
#endif
#ifdef USE_FFTACCELERATE
	qDebug()<<"Using FFT Accelerate";
	return new FFTAccelerate();
#endif

	qDebug()<<"Error in FFT configuration";
	return NULL;
}

void FFT::FFTParams(quint32 _size, bool _invert, double _dBCompensation, double _sampleRate)
{
    if (_size == 0)
        return; //Error
    else if( _size < minFFTSize )
        fftSize = minFFTSize;
    else if( _size > maxFFTSize )
        fftSize = maxFFTSize;
    else
        fftSize = _size;

	//Reset to actual fft size
	DB::dbOffset = DB::maxDb - 20 * log10(fftSize * 1.0 / 2.0);
	DB::pwrOffset = pow(10, (DB::minDb - DB::dbOffset) / 10.0);

    invert = _invert;
    dBCompensation = _dBCompensation;
    sampleRate = _sampleRate;

    timeDomain = CPXBuf::malloc(fftSize);
    freqDomain = CPXBuf::malloc(fftSize);
    workingBuf = CPXBuf::malloc(fftSize);
    overlap = CPXBuf::malloc(fftSize);
    CPXBuf::clear(overlap, fftSize);

    if (FFTPwrAvgBuf != NULL)
        delete FFTPwrAvgBuf;
    FFTPwrAvgBuf = new double[fftSize];
    for (int i=0; i<fftSize; i++)
        FFTPwrAvgBuf[i] = 0.0;

    if (FFTAvgBuf != NULL)
        delete FFTAvgBuf;
    FFTAvgBuf = new double[fftSize];
    for (int i=0; i<fftSize; i++)
        FFTAvgBuf[i] = 0.0;

    if (FFTPwrSumBuf != NULL)
        delete FFTPwrSumBuf;
    FFTPwrSumBuf = new double[fftSize];
    for (int i=0; i<fftSize; i++)
        FFTPwrSumBuf[i] = 0.0;


	if (fftBinToX != NULL)
		delete fftBinToX;
	fftBinToX = new qint32[fftSize];

    fftParamsSet = true;
}

///////////////////////////////////////////////////////////////////
//FFT initialization and parameter setup function
///////////////////////////////////////////////////////////////////
void FFT::SetMovingAvgLimit( quint32 ave)
{
    if(movingAvgLimit != ave)
    {
        if(ave>0)
            movingAvgLimit = ave;
        else
            movingAvgLimit = 1;
    }
    ResetFFT();
}

//Subclasses should call in case we need to do anything
void FFT::ResetFFT()
{
    //Clear buffers?
}


//!!Compare with cuteSDR logic, replace if necessary
//This can be called directly if we've already done FFT
//WARNING:  fbr must be large enough to hold 'size' values
void FFT::FreqDomainToMagnitude(CPX * freqBuf, int size, double baseline, double correction, double *fbr)
{
    //calculate the magnitude of your complex frequency domain data (magnitude = sqrt(re^2 + im^2))
    //convert magnitude to a log scale (dB) (magnitude_dB = 20*log10(magnitude))

    // FFT output index 0 to N/2-1 - frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term )
    //This puts 0 to size/2 into size/2 to size-1 position
    for (int i=0, j=size/2; i<size/2; i++,j++) {
		fbr[j] = DB::amplitudeToDb(freqBuf[i].mag() + baseline) + correction;
    }
    // FFT output index N/2 to N-1 - frequency output -Fs/2 to 0
    // This puts size/2 to size-1 into 0 to size/2
    //Works correctly with Ooura FFT
    for (int i=size/2, j=0; i<size; i++,j++) {
		fbr[j] = DB::amplitudeToDb(freqBuf[i].mag() + baseline) + correction;
    }
}

//Utility to handle overlap/add using FFT buffers
void FFT::OverlapAdd(CPX *out, int size)
{
    //Do Overlap-Add to reduce from 1/2 fftSize

    //Add the samples in 'in' to last overlap
    CPXBuf::add(out, timeDomain, overlap, size);

    //Save the upper 50% samples to  overlap for next run
    CPXBuf::copy(overlap, (timeDomain+size), size);

}

void FFT::CalcPowerAverages(CPX* in, double *out, int size)
{

#if 0


    // FFT output index 0 to N/2-1
    // is frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term )
    for( l=0,j=n/2; j<n; l+=2,j++)
    {
        x0r = (a[l]*a[l]) + (a[l+1]*a[l+1]);
        //perform moving average on power up to m_AveSize then do exponential averaging after that
        if(m_TotalCount <= m_AveSize)
            m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + x0r;
        else
            m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + x0r;

        m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(double)m_AveCount;

        m_pFFTAveBuf[j] = log10( m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
    }

#endif

    //This is called from code that is locked with fftMutex, don't re-lock or will hang

    double samplePwr;

    bufferCnt++;
    if(averageCnt < movingAvgLimit)
        averageCnt++;

    // FFT output index 0 to N/2-1
    // is frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term )
    // Buffer is already unfolded, unlike in cuteSDR implementation, so simple loop
    for( int i = 0; i < size; i++){
        //re^2 + im^2
        samplePwr = in[i].norm(); // or .sqrMag() = Power in sample

        //perform moving average on power up to m_AveSize then do exponential averaging after that
        if(bufferCnt <= movingAvgLimit)
            FFTPwrSumBuf[i] = FFTPwrSumBuf[i] + samplePwr;
        else
            //Weighted by previous average
            FFTPwrSumBuf[i] = FFTPwrSumBuf[i] - FFTPwrAvgBuf[i] + samplePwr;

        //Average buffer = sum buffer / average counter
        FFTPwrAvgBuf[i] = FFTPwrSumBuf[i]/(double)averageCnt;

		//Convert to db
		//FFTAvgBuf[i] = 10 * log10( FFTPwrAvgBuf[i] + K_C) + K_B;
		FFTAvgBuf[i] = DB::powerToDbAdjusted(FFTPwrAvgBuf[i]);

        //Skip copying to out if null
        if (out != NULL)
            out[i] = FFTAvgBuf[i];
    }

}

#if 0
Returns an array of plotable y values for each element of the array (x value)
Auto scales so startFreq and stopFreq fill the array (zoom)
Auto scales to fit xPixels, yPixels, mindB and maxdB
Handles the case where are more fft bins to plot than pixels available (average fft bins)
Also handles the case where there are not enough fft bins to fill pixels available

If startFreq or endFreq are outside the FFT range, maps the pixels to DB::minDb ie no signal
#endif

bool FFT::MapFFTToScreen(
		double *inBuf,

		qint32 yPixels, //Height of the plot area
		qint32 xPixels, //Width of the plot area
		double maxdB, //Fft value corresponding to output value 0
		double mindB, //FFT value corresponding to output value DB::maxDb
		qint32 startFreq, //In +/- hZ, relative to 0
		qint32 stopFreq, //In +/- hZ, relative to 0
		qint32* outBuf )
{

	qint32 binLow;
	qint32 binHigh;
	//float hzPerBin = (float)sampleRate / (float)fftSize; //hZ per fft bin
	float binsPerHz = (float)fftSize / (float)sampleRate;
	//
	//qint32 fftLowFreq = -sampleRate / 2;
	//qint32 fftHighFreq = sampleRate / 2;

	//Find the beginning and ending FFT bins that correspond to Start and StopFreq
	//
	//Ex: If no zoom, binLow will be 1/2 of fftSize or -1024
	binLow = (qint32)(startFreq * binsPerHz);
	//Ex: makes it zero relative, so will be 0
	binLow += (fftSize/2);
	//Ex: 1024 for full spectrum
	binHigh = (qint32)(stopFreq * binsPerHz);
	//Ex: now 2048
	binHigh += (fftSize/2);

	//Note: binLow can be less than zero, indicating we have plots that are below fft
	//binHigh can be greater than fftSize for the same reason, indicating invalid fft bins
	//We handle this in the output loop

	qint32 fftBinsToPlot = binHigh - binLow;

	//If more pixels than bins, pixelsPerBin will be > 1
	float pixelsPerBin = (float)xPixels / (float)fftBinsToPlot;
	float binsPerPixel = (float)fftBinsToPlot / (float)xPixels;

	//dbGainFactor is used to scale min/max db values so they fit in N vertical pixels
	float dBGainFactor = -1 / (maxdB - mindB);

	qint32 fftBin; //Bin corresponding to freq being processed

	qint32 totalBinPower = 0;
	qint32 powerdB = 0;

	fftMutex.lock();

	qint32 lastFftBin = -1; //Flag as invalid
	//if more FFT bins than plot bins, averge skipped fftBins to match plots bins
	if(fftBinsToPlot > xPixels ) {
		//f is in range, find fft bin that corresponds to it
		for (qint32 i=0; i < xPixels; i++) {
			totalBinPower = 0;
			//binsPerPixel is a float, be careful about integer math that loses precision and plot points
			fftBin = binLow + (i*binsPerPixel);
			//Check fftBin before we look for invert, otherwise won't be negative
			//If freq for this pixel is outside fft range, output 0 until we are in range
			if (fftBin < 0) {
				powerdB = DB::minDb; //Out of bin range
				lastFftBin = fftBin;
			} else if (fftBin >= fftSize) {
				powerdB = DB::minDb;
				lastFftBin = fftBin;
			} else if (invert) {
				//Lowest freq is in highest bin
				fftBin = fftSize - fftBin - 1; //0 maps to 2047

				//we start from fftBin[fftSize] and go to zero
				if (lastFftBin > 0 && fftBin != lastFftBin - 1) {
					//We skipped some bins, average the ones we skipped
					qint32 skippedBins = lastFftBin - fftBin;
					for (int j=0; j<skippedBins; j++) {
						totalBinPower += inBuf[lastFftBin - j] - maxdB;
					}
					powerdB = totalBinPower / skippedBins;
					lastFftBin = fftBin;
				} else {
					powerdB = inBuf[fftBin] - maxdB;
				}

			} else if (lastFftBin > 0 && fftBin != lastFftBin + 1) {
				//Lowest freq is in fftBin[0]
				//We skipped some bins, average the ones we skipped
				qint32 skippedBins = fftBin - lastFftBin;
				for (int j=0; j<skippedBins; j++) {
					totalBinPower += inBuf[lastFftBin + j] - maxdB;
				}
				powerdB = totalBinPower / skippedBins;
				lastFftBin = fftBin;
			} else {
				powerdB = inBuf[fftBin] - maxdB;
			}

			outBuf[i] = (qint32)(yPixels * dBGainFactor * powerdB);
		} //End xPixel loop

	} else {
		//if more plot bins than FFT bins, each fft bin maps to multiple plot bins
		for (qint32 i=0; i < xPixels; i++) {
			//ie inBuf[0] maps to outBuf[0..3]
			// inBuf[1] maps to outBuf [4..7]
			fftBin = binLow + (i / pixelsPerBin);
			//Check fftBin before we look for invert, otherwise won't be negative
			if (fftBin < 0) {
				powerdB = DB::minDb; //Out of bin range
			} else if (fftBin >= fftSize) {
				powerdB = DB::minDb;
			} else if (invert) {
				//Lowest freq is in highest bin
				fftBin = fftSize - fftBin;
				powerdB = inBuf[fftBin] - maxdB;
			} else {
				powerdB = inBuf[fftBin] - maxdB;
			}
			outBuf[i] = (qint32)(yPixels * dBGainFactor * powerdB);
			lastFftBin = fftBin;

		} //End xPixel loop
	}

	fftMutex.unlock();

	return false; //True if y has to be truncated (overloaded)
}

#if 0
//Save for reference, replaced 1/20/16
//Private variables
//qint32 binLow; //lowest frequency to be displayed
//qint32 binHigh; //highest frequency to be displayed
//qint32 plotStartFreq;
//qint32 plotStopFreq;
//qint32 plotWidth;

//Common routine borrowed from cuteSDR
//Scales X axis to fit more FFT data in fewer pixels or fewer FFT data in more pixels
//For cah pixel bin, calculates the scaled db y value for FFTAvgBuf
//After calling this, we can plot directly from OutBuf[0 to plotWidth]

//////////////////////////////////////////////////////////////////////
// The bin range is "start" to "stop" Hz.
// The range of start to stop frequencies are mapped to the users
// plot screen size so the users buffer will be filled with an array
// of integers whos value is the pixel height and the index of the
//  array is the x pixel coordinate.
// The function returns true if the input is overloaded
//   This routine converts the data to 32 bit integers and is useful
//   when displaying fft data on the screen.
//		MaxHeight = Plot height in pixels(zero is top and increases down)
//		MaxWidth =  Plot width in pixels
//		StartFreq = freq in Hz (relative to 0, should always be neg)
//		StopFreq = freq in Hz (should always be pos)
//		MaxdB = FFT dB level corresponding to output value == 0
//			must be <= to K_MAXDB
//		MindB = FFT dB level  corresponding to output value == MaxHeight
//			must be >= to K_MINDB
//////////////////////////////////////////////////////////////////////
/// \brief FFT::MapFFTToScreen
/// \param inBuf
/// \param yPixels
/// \param xPixels
/// \param maxdB
/// \param mindB
/// \param startFreq
/// \param stopFreq
/// \param outBuf
/// \return

bool FFT::MapFFTToScreen(
		double *inBuf,
		qint32 yPixels, //Height
		qint32 xPixels, //Width
		double maxdB,
		double mindB,
		qint32 startFreq,
		qint32 stopFreq,
		qint32* outBuf )
{

    if (!fftParamsSet)
        return false;

	//Passed as arg so we can use with remote dsp servers
	//double *inBuf = FFTAvgBuf;

	qint32 y;
	qint32 xCoord;
    qint32 m;
    qint32 ymax = -10000;
    qint32 xprev = -1;
	qint32 numPlotBins = binHigh - binLow;
    const qint32 binMax = fftSize -1;
	//dBmaxOffset is used to adjust inbound db up/down by fixed offset which can be set by user in Spectrum
	double dBmaxOffset = maxdB;
	//dbGainFactor is used to scale min/max db values so they fit in N vertical pixels
	double dBGainFactor = -1/(maxdB-mindB);

    //qDebug()<<"maxoffset dbgaindfact "<<dBmaxOffset << dBGainFactor;

    fftMutex.lock();
    //If zoom has changed (startFreq-stopFreq) or width of plot area (window resize)
    //then recalculate
	if( (plotStartFreq != startFreq) ||
		(plotStopFreq != stopFreq) ||
		(plotWidth != xPixels) ) {
        //if something has changed need to redo translate table
		plotStartFreq = startFreq;
		plotStopFreq = stopFreq;
		plotWidth = xPixels;

		//Find the beginning and ending FFT bins that correspond to Start and StopFreq
        //
        //Ex: If no zoom, binLow will be 1/2 of fftSize or -1024
		binLow = (qint32)((double)plotStartFreq*(double)fftSize/sampleRate);
        //Ex: makes it zero relative, so will be 0
        binLow += (fftSize/2);
        //Ex: 1024 for full spectrum
		binHigh = (qint32)((double)plotStopFreq*(double)fftSize/sampleRate);
        //Ex: now 2048
        binHigh += (fftSize/2);
        if(binLow < 0)	//don't allow these go outside the translate table
            binLow = 0;
        if(binLow >= binMax)
            binLow = binMax;
        if(binHigh < 0)
            binHigh = 0;
        if(binHigh >= binMax)
            binHigh = binMax;
		numPlotBins = (binHigh - binLow);

		//fftBinTable is the same size as FFT and each FFT index has a corresponding fftBinTable value
		//fftBinTable[n] = fft bin to map to x-coordinate n
		//To know where to plot fftBin[n] on plotFrame x axis, get the value from fftBinTable[n]
        //Example where plot width 1/3 of FFT size
		//FFT:			0   1   2   3   4   5   6   7   8   9   ...
		//fftBinTable:	0   0   0   1   1   1   2   2   2   3   ...
		//xCoord of 0,1,2 all map to FFT[0]

		if( (numPlotBins) >= plotWidth )
        {
			//if more FFT bins than plot bins, map min to max FFT to plot
			for(int i=binLow; i<=binHigh; i++)
				fftBinToX[i] = ( (i-binLow)*plotWidth )/numPlotBins;
        }
        else
        {
			//if more plot bins than FFT bins, fft bins map to multiple x bins
			for(int i=0; i<plotWidth; i++)
				fftBinToX[i] = binLow + ( i*numPlotBins )/plotWidth;
        }
    } //End recalc after change in window size etc

    m = fftSize - 1; //cuteSDR bug, in loop below if m=fftSize then m -i (when i==0) is outside of inBuf size
	if( numPlotBins > plotWidth )
    {
		//if more FFT points than plot points, process every bin
		//Some FFT bins will be skipped
		for(int i=binLow; i<=binHigh; i++ )
        {
            if(invert)
				y = (qint32)((double)yPixels * dBGainFactor * (inBuf[(m-i)] - dBmaxOffset));
            else
				y = (qint32)((double)yPixels * dBGainFactor * (inBuf[i] - dBmaxOffset));
            if(y < 0)
                y = 0;
			if(y > yPixels)
				y = yPixels;
			xCoord = fftBinToX[i];	//get fft bin to plot x coordinate transform
			if( xCoord == xprev )	// still mappped to same fft bin coordinate
            {
				//Each fft bin will map to multiple plot bins since we're throwing away some fft bins
				//in order to fit.  Make sure the plot bin always has the largest y value for the
				//set of fft bins that map to it
				if(y > ymax) {		//store only if we have a new max value
					outBuf[xCoord] = y;
					ymax = y; //New yMax
                }
			} else {
				//New x coord, reset yMax for this coord
				outBuf[xCoord] = y;
				xprev = xCoord;
				ymax = y; //New yMax
            }

		}
	} //End FFT bins > plot bins
    else
    {
		//if more plot points than FFT points
		//Map plot bin to fft Bin, ie iterate over plot bins
		//We throw away fft bins, but could average to get a slightly better result
		for( int i=0; i<plotWidth; i++ ) {
			xCoord = fftBinToX[i];	//get plot to fft bin coordinate transform
            if(invert)
				y = (qint32)((double)yPixels * dBGainFactor * (inBuf[(m-xCoord)] - dBmaxOffset));
            else
				y = (qint32)((double)yPixels * dBGainFactor * (inBuf[xCoord] - dBmaxOffset));
            if(y<0)
                y = 0;
			if(y > yPixels)
				y = yPixels;
			outBuf[i] = y;

        }
	} //End FFT bins <= plot bins
    fftMutex.unlock();

    //!!return m_Overload;
    return false; //No overload
}
#endif
