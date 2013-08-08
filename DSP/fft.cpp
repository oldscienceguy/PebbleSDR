#include "fft.h"

FFT::FFT() :
    useIntegerFFT(false)
{
    maxFFTSize = 65536;
    minFFTSize = 512;
    maxDb = 0.0;			//specifies total range of FFT
    minDb = -220.0;
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
    dBCompensation = maxDb;


    plotTranslateTable = NULL;
    binLow = 0;		//force recalculation of plot variables
    binHigh = 0;

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
    if (plotTranslateTable) delete plotTranslateTable;
    if (FFTPwrAvgBuf != NULL)
        delete FFTPwrAvgBuf;
    if (FFTAvgBuf != NULL)
        delete FFTAvgBuf;
    if (FFTPwrSumBuf != NULL)
        delete FFTPwrSumBuf;
}

//////////////////////////////////////////////////////////////////////
// A pure input sin wave ... Asin(wt)... will produce an fft output
//   peak of (N*A/1)^2  where N is FFT_SIZE.
//		Kx = 2 for complex, 4 for real FFT
// To convert to a Power dB range:
//   PdBmax = 10*log10( (N*A/Kx)^2 + K_C ) + K_B
//   PdBmin = 10*log10( 0 + K_C ) + K_B
//  if (N*A/Kx)^2 >> K_C
//  Then K_B = PdBmax - 20*log10( N*A/Kx )
//       K_C = 10 ^ ( (PdBmin-K_B)/10 )
//  for power range of 0 to 100 dB with input(A) of 32767 and N=262144
//			K_B = -86.63833  and K_C = 4.6114145e8
// To eliminate the multiply by 10, divide by 10 so for an output
//		range of 0 to -120dB the stored value is 0.0 to -12.0
//   so final constant K_B = -8.663833
///////////////////////////////////////////////////////////////////////

void FFT::FFTParams(qint32 _size, bool _invert, double _dBCompensation, double _sampleRate)
{
    if (_size == 0)
        return; //Error
    else if( _size < minFFTSize )
        fftSize = minFFTSize;
    else if( _size > maxFFTSize )
        fftSize = maxFFTSize;
    else
        fftSize = _size;

    invert = _invert;
    dBCompensation = _dBCompensation;
    sampleRate = _sampleRate;

    timeDomain = CPXBuf::malloc(fftSize);
    freqDomain = CPXBuf::malloc(fftSize);
    workingBuf = CPXBuf::malloc(fftSize);
    overlap = CPXBuf::malloc(fftSize);
    CPXBuf::clear(overlap, fftSize);

    //Init K_B and K_C constants for power conversion
    K_B = dBCompensation - 20 * log10( (double)fftSize * ampMax/2.0 );
    K_C = pow( 10.0, (minDb - K_B)/10.0 );
    K_B = K_B/10.0;

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


    if (plotTranslateTable != NULL)
        delete plotTranslateTable;
    plotTranslateTable = new qint32[fftSize];

    lastStartFreq = 0;
    lastStopFreq = 0;
    lastPlotWidth = 0;

    fftParamsSet = true;
}

///////////////////////////////////////////////////////////////////
//FFT initialization and parameter setup function
///////////////////////////////////////////////////////////////////
void FFT::SetMovingAvgLimit( qint32 ave)
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
        fbr[j] = SignalProcessing::amplitudeToDb(freqBuf[i].mag() + baseline) + correction;
    }
    // FFT output index N/2 to N-1 - frequency output -Fs/2 to 0
    // This puts size/2 to size-1 into 0 to size/2
    //Works correctly with Ooura FFT
    for (int i=size/2, j=0; i<size; i++,j++) {
        fbr[j] = SignalProcessing::amplitudeToDb(freqBuf[i].mag() + baseline) + correction;
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

        FFTAvgBuf[i] = log10( FFTPwrAvgBuf[i] + K_C) + K_B;

        //Skip copying to out if null
        if (out != NULL)
            out[i] = FFTAvgBuf[i];

    }

}

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
bool FFT::MapFFTToScreen(qint32 maxHeight,
                                qint32 maxWidth,
                                double maxdB,
                                double mindB,
                                qint32 startFreq,
                                qint32 stopFreq,
                                qint32* outBuf )
{

    if (!fftParamsSet)
        return false;

    double *inBuf = FFTAvgBuf;

    qint32 i;
    qint32 y;
    qint32 x;
    qint32 m;
    qint32 ymax = -10000;
    qint32 xprev = -1;
    const qint32 binMax = fftSize -1;
    double dBmaxOffset = maxdB/10.0;
    double dBGainFactor = -10.0/(maxdB-mindB);

    //qDebug()<<"maxoffset dbgaindfact "<<dBmaxOffset << dBGainFactor;

    fftMutex.lock();
    //If zoom has changed (startFreq-stopFreq) or width of plot area (window resize)
    //then recalculate
    if( (lastStartFreq != startFreq) ||
        (lastStopFreq != stopFreq) ||
        (lastPlotWidth != maxWidth) ) {
        //if something has changed need to redo translate table
        lastStartFreq = startFreq;
        lastStopFreq = stopFreq;
        lastPlotWidth = maxWidth;

        //Find the beginning and ending FFT bins that correspond to Start and StopFreq
        //
        //Ex: If no zoom, binLow will be 1/2 of fftSize or -1024
        binLow = (qint32)((double)startFreq*(double)fftSize/sampleRate);
        //Ex: makes it zero relative, so will be 0
        binLow += (fftSize/2);
        //Ex: 1024 for full spectrum
        binHigh = (qint32)((double)stopFreq*(double)fftSize/sampleRate);
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

        //plotTranslateTable is the same size as FFT and each FFT index has a corresponding plotTranslateTable value
        //The values in plotTranslateTable are the x indexes to the actual plot screen
        //To know where to plot fftBin[n] on plotFrame x axis, get the value from plotTranslateTable[n]
        //Example where plot width 1/3 of FFT size
        //FFT:  0   1   2   3   4   5   6   7   8   9   ...
        //Plot: 0   0   0   1   1   1   2   2   2   3   ...
        if( (binHigh-binLow) > lastPlotWidth )
        {
            //if more FFT points than plot points, map min to max FFT to plot
            for( i=binLow; i<=binHigh; i++)
                plotTranslateTable[i] = ( (i-binLow)*lastPlotWidth )/(binHigh - binLow);
        }
        else
        {
            //if more plot points than FFT points
            for( i=0; i<lastPlotWidth; i++)
                plotTranslateTable[i] = binLow + ( i*(binHigh - binLow) )/lastPlotWidth;
        }
    } //End recalc after change in window size etc

    m = fftSize - 1; //cuteSDR bug, in loop below if m=fftSize then m -i (when i==0) is outside of inBuf size
    if( (binHigh-binLow) > lastPlotWidth )
    {
        //if more FFT points than plot points
        for( i=binLow; i<=binHigh; i++ )
        {
            if(invert)
                y = (qint32)((double)maxHeight * dBGainFactor * (inBuf[(m-i)] - dBmaxOffset));
            else
                y = (qint32)((double)maxHeight * dBGainFactor * (inBuf[i] - dBmaxOffset));
            if(y < 0)
                y = 0;
            if(y > maxHeight)
                y = maxHeight;
            x = plotTranslateTable[i];	//get fft bin to plot x coordinate transform
            if( x == xprev )	// still mappped to same fft bin coordinate
            {
                if(y < ymax)		//store only the max value
                {
                    outBuf[x] = y;
                    ymax = y;
                }
            }
            else
            {
                outBuf[x] = y;
                xprev = x;
                ymax = y;
            }

        }
    }
    else
    {
        //if more plot points than FFT points fill with average
        for( x=0; x<lastPlotWidth; x++ ) {
            i = plotTranslateTable[x];	//get plot to fft bin coordinate transform
            if(invert)
                y = (qint32)((double)maxHeight * dBGainFactor * (inBuf[(m-i)] - dBmaxOffset));
            else
                y = (qint32)((double)maxHeight * dBGainFactor * (inBuf[i] - dBmaxOffset));
            if(y<0)
                y = 0;
            if(y > maxHeight)
                y = maxHeight;
            outBuf[x] = y;

        }
    }
    fftMutex.unlock();

    //!!return m_Overload;
    return false; //No overload
}

