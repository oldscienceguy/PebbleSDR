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

FFT::FFT()
{
	m_timeDomain = NULL;
	m_freqDomain = NULL;
	m_workingBuf = NULL;
	m_overlap = NULL;
	m_windowFunction = NULL;

	m_fftParamsSet = false; //Only set to true in FFT::FFTParams(...)

	m_fftPower = NULL;
	m_fftAmplitude = NULL;
	m_fftPhase = NULL;
	m_isOverload = false;

}
FFT::~FFT()
{
	if (m_timeDomain) free(m_timeDomain);
	if (m_freqDomain) free(m_freqDomain);
	if (m_workingBuf) free(m_workingBuf);
	if (m_overlap) free(m_overlap);
	if (m_windowFunction != NULL)
		delete m_windowFunction;
	if (m_fftPower != NULL)
		delete[] m_fftPower;
	if (m_fftAmplitude != NULL)
		delete[] m_fftAmplitude;
	if (m_fftPhase != NULL)
		delete[] m_fftPhase;
}

FFT* FFT::factory(QString _label)
{
	FFT *fft = NULL;
#if defined USE_FFTW
	qDebug()<<"Using FFTW for "<<_label;
	fft = new FFTfftw();
#elif defined USE_FFTCUTE
	qDebug()<<"Using FFT CUTE for "<<_label;
	fft = new CFft();
#elif defined USE_FFTOOURA
	qDebug()<<"Using FFT Ooura for "<<_label;
	fft = new FFTOoura();
#elif defined USE_FFTACCELERATE
	qDebug()<<"Using FFT Accelerate for "<<_label;
	fft = new FFTAccelerate();
#else
	qDebug()<<"Error in FFT configuration";
#endif

	return fft;
}

void FFT::fftParams(quint32 _fftSize, double _dBCompensation, double _sampleRate, int _samplesPerBuffer,
					WindowFunction::WINDOWTYPE _windowType)
{
	Q_UNUSED(_dBCompensation); //Remove?, not used anywhere.  Maybe use for db to dbm calibration?

	if (_fftSize == 0)
        return; //Error
	else if( _fftSize < m_minFFTSize )
		m_fftSize = m_minFFTSize;
	else if( _fftSize > m_maxFFTSize )
		m_fftSize = m_maxFFTSize;
    else
		m_fftSize = _fftSize;

	m_binWidth = m_sampleRate / m_fftSize;

	m_samplesPerBuffer = _samplesPerBuffer;
	m_maxBinPower = m_ampMax * m_samplesPerBuffer;

	m_windowType = _windowType;

	m_sampleRate = _sampleRate;

	m_timeDomain = CPX::memalign(m_fftSize);
	m_freqDomain = CPX::memalign(m_fftSize);
	m_workingBuf = CPX::memalign(m_fftSize);
	m_overlap = CPX::memalign(m_fftSize);
	CPX::clearCPX(m_overlap, m_fftSize);

	if (m_windowFunction != NULL) {
		delete m_windowFunction;
		m_windowFunction = NULL;
	}

	//Window are sized to expected samples per buffer, which may be smaller than fftSize
	m_windowFunction = new WindowFunction(m_windowType,m_samplesPerBuffer);

	//Todo: Add UI to switch
	m_isAveraged = true;

	if (m_fftPower != NULL)
		delete[] m_fftPower;
	m_fftPower = new double[m_fftSize];
	if (m_fftAmplitude != NULL)
		delete[] m_fftAmplitude;
	m_fftAmplitude = new double[m_fftSize];
	if (m_fftPhase != NULL)
		delete[] m_fftPhase;
	m_fftPhase = new double[m_fftSize];

	m_fftParamsSet = true;
}

//Subclasses should call in case we need to do anything
void FFT::resetFFT()
{
    //Clear buffers?
}

//Common function used in FFTForward by all fft versions
//Applies window function and checks for overload
//Returns windowed data, padded to fftSize in timeDomain
bool FFT::m_applyWindow(const CPX *in, int numSamples)
{
	//If in==NULL, use whatever is in timeDomain buffer
	if (in != NULL ) {
		if (m_windowType != WindowFunction::NONE && numSamples == m_samplesPerBuffer) {
			//Smooth the input data with our window
			m_isOverload = false;
			//CPX::multCPX(timeDomain, in, windowFunction->windowCpx, samplesPerBuffer);
			for (int i=0; i<m_samplesPerBuffer; i++) {
				if (fabs(in[i].re) > m_overLimit || fabs(in[i].im) > m_overLimit) {
					m_isOverload = true;
					//Don't do anything, just flag that we're in the danger zone so UI can display it
				}
				m_timeDomain[i] = in[i] * m_windowFunction->windowCpx[i];
			}
			//Zero pad remainder of buffer if needed
			for (int i = m_samplesPerBuffer; i<m_fftSize; i++) {
				m_timeDomain[i] = 0;
			}
		} else {
			//Make sure that buffer which does not have samples is zero'd out
			//We can pad samples in the time domain because it does not impact frequency results in FFT
			CPX::clearCPX(m_timeDomain,m_fftSize);
			//Put the data in properly aligned FFTW buffer
			CPX::copyCPX(m_timeDomain, in, numSamples);
		}
	}
	return m_isOverload;
}

// Unfolds output of fft to -f to +f order
/*
 (From FFTW documentation)
 Note also that we use the standard “in-order” output ordering—the k-th output corresponds to the frequency k/n
 (or k/T, where T is your total sampling period).
 For those who like to think in terms of positive and negative frequencies,
 this means that the positive frequencies are stored in the first half of the output
 and the negative frequencies are stored in backwards order in the second half of the output.
 (The frequency -k/n is the same as the frequency (n-k)/n.)

 (from FFTW doc, alternative to unfolding output by pre-processing input)
 For human viewing of a spectrum, it is often convenient to put the origin in
 frequency space at the center of the output array, rather than in the zero-th
 element (the default in FFTW). If all of the dimensions of your array are
 even, you can accomplish this by simply multiplying each element of the input
 array by (-1)^(i + j + ...), where i, j, etcetera are the indices of the
 element. (This trick is a general property of the DFT, and is not specific to
 FFTW.)

	for (int i=0; i<numSamples; i++) {
		buf[i] = in[i] * pow(-1,i);
	}

*/
void FFT::m_unfoldInOrder(CPX *inBuf, CPX *outBuf)
{
	int fftMid = m_fftSize/2;

	//In Accelerate, the DC bin has a special purpose, check other implementations.  From gerrybeauregard ...
	// Bins 0 and N/2 both necessarily have zero phase, so in the packed format
	// only the real values are output, and these are stuffed into the real/imag components
	// of the first complex value (even though they are both in fact real values).
	//bin.re = magnitude at 0
	//bin.im = magnitude at n/2 (instead of phase at 0)
	//out[0] = in[0].re / windowFunction->coherentGain;
	//out[0] /= maxBinPower;
	//Todo: Not sure if this above is really true, for future research
#if 0
	if (inBuf[0].im != 0) {
		//qDebug()<<inBuf[0].re<<" "<<inBuf[0].im<<" "<<inBuf[fftMid].re<<" "<<inBuf[fftMid].im;
		inBuf[0].im = 0;
	}
#endif


#if 1
	//For fftSize 2048, fftMid=1024
	// move in[0] to in[1023] to out[1024] to out[2047]
	// move in[1024] to in[2047] to out[0] to out[1023]

	CPX::copyCPX(&outBuf[fftMid], &inBuf[0], fftMid);
	CPX::copyCPX(&outBuf[0], &inBuf[fftMid], fftMid);

#else
	// FFT output index 0 to N/2-1 - frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term, positive frequencies )
	// This puts 0 to size/2 into size/2 to size-1 position
	for( int unfolded = 0, folded = fftSize/2 ; folded < fftSize; unfolded++, folded++) {
		outBuf[unfolded] = inBuf[folded]; //1024 to 2047 folded -> 0 to 1023 unfolded
	}
	// FFT output index N/2 to N-1 - frequency output -Fs/2 to 0 (negative frequencies)
	// This puts size/2 to size-1 into 0 to size/2
	// Works correctly with Ooura FFT
	for( int unfolded = fftSize/2, folded = 0; unfolded < fftSize; unfolded++, folded++) {
		outBuf[unfolded] = inBuf[folded]; //0 to 1023 folded -> 1024 to 2047 unfolded
	}
#endif
}

//!!Compare with cuteSDR logic, replace if necessary
//This can be called directly if we've already done FFT
//WARNING:  fbr must be large enough to hold 'fftSize' values
void FFT::freqDomainToMagnitude(CPX * freqBuf, double baseline, double correction, double *fbr)
{
    //calculate the magnitude of your complex frequency domain data (magnitude = sqrt(re^2 + im^2))
    //convert magnitude to a log scale (dB) (magnitude_dB = 20*log10(magnitude))

	m_unfoldInOrder(freqBuf,m_workingBuf);
	for (int i=0; i<m_fftSize; i++) {
		fbr[i] = DB::powerTodB(DB::power(m_workingBuf[i]) + baseline) + correction;
	}
}

//Utility to handle overlap/add using FFT buffers
void FFT::overlapAdd(CPX *out, int numSamples)
{
    //Do Overlap-Add to reduce from 1/2 fftSize

    //Add the samples in 'in' to last overlap
	CPX::addCPX(out, m_timeDomain, m_overlap, numSamples);

    //Save the upper 50% samples to  overlap for next run
	CPX::copyCPX(m_overlap, (m_timeDomain+numSamples), numSamples);

}

/*

 All SDR devices are normalized to produce an output of -1.0 to +1.0, so the max sample value (maxSamp) of
	x in any sample is 1 and the min value is -1.
 The largest theoretical CPX is therefore CPX(1,1)

 The max power (maxPower) in any one sample is therefore cx.re^2 + cx.im^2 = 1^2 + 1^2 = 2;
 The max amplitude (maxAmp) in any one sample is therefore sqrt(maxPower) = 1.414213562

 Assume a pure sine wave input where each sample.re = maxAmp*cos(x) & sample.im = maxAmp*sin(x) and
	there are no other signals present, just the single sine wave.

 For maxAmp = 1, fullscale, and any value of x (ie any freq)
	the power in that sample is 1 since since 1*cos(x)^2 + 1*sin(x)^2 is always equal to 1.
 The amplitude of that sample is also 1 (sqrt(power))

 If we analyze a time domain buffer that contains the pure sine wave (typically generated by TestBench),
 the total power in the buffer is therefore 1 * numSamples or 2048 for a 2048 frames per buffer configuration

 This is the largest power value we will ever see and equates to 0db (full scale)
 dB = 10*log10(power) = 10*log10(1) = 0db


 According to Parseval's theorem, the energy in the time domain must be equal to the energy in the frequency domain.
 Since we have a single frequency in this example, all of the power in the input signal should be represented
 in a single fft bin (freq of our sin wave) with the same power.
 The rest of the bins should be close to zero.

 The maximum power value for any FFT bin maxBinPower = ampMax * samplesPerBuffer and is our normalizing factor
 Note that this is based on the number of samples, NOT the number of bins.  We normalize to the input buffer.

 The input signal is pre-processed by a window function which has the effect of reducing the power before being
 handled by the FFT.  We need to compensate for this.
 For example, a BlackmanHarris window has a gain factor of 0.36.
 This means that our FFT power should be scaled up by this amount (power / window gain)
 So the expected uncompensated power in our 1 bin should be 2048 * 0.36 = 737.28
 Any variation from this is due to other fft losses such as fftSize and scalloping, which we can calculate or just plug.

 Finally, we take the output of the fft, which represents  amplitude, and convert it using DB::amplitude();
 One we have the amplitude, we can also get the power using DB::amplitudeToPower()

 Examples with our standard 2048 sample buffer size (fpb). Full scale reduced by 10% (10db) each step
 Power = 2048, normalized = 2048/2048 = 1, 10*log10(1) = 0dB (our maxdB limit)
 Power = 204.8, normalized = 204.8/2048 = 0.1, 10*log10(0.1) = -10dB
 Power = 20.48, normalized = 20.48/2048 = 0.01, 10*log10(0.01) = -20dB
 ...
 Power = 2.048e-12, normalized = 2.048e-12/2048 = 1e-12, 10*log10(1e-12) = -120db (our mindB limit)

 Example of a -10db signal generated by testbench and reported by DB::analyzeCPX()
 The first result is the timedomain, the second is the unprocessed fft output
 Note powerMax is -10db in the timedomain, and also -10db in the freq domain
	Spectrum In  numSamples= 2048
	 Re: max= 0.316228  @ 240  min= -0.316228  @ 740
	 Im: max= 0.316228  @ 990  min= -0.316228  @ 490
	 Power: max= 0.1  @ 1214  min= 0.1  @ 15  total= 204.8  mean= 0.1
	 Power db: max= -10 db min= -10 db total= 23.1133 db mean= -10 db
	 Amplitude: max= 0.316228  min= 0.316228  total= 647.634  mean= 0.316228
	 Amplitude db: max= -5 db min= -5 db total= 28.1133 db mean= -5 db
	-
	Spectrum Out  numSamples= 32768  windowGain= 0.35875  maxBinPower= 2048
	 Re: max= 226.854  @ 15962  min= -188.98  @ 15947
	 Im: max= 226.763  @ 15954  min= -191.332  @ 15969
	 Power: max= 0.0999999  @ 15958  min= 2.31081e-24  @ 16294  total= 3.20696  mean= 9.78688e-05
	 Power db: max= -10 db min= -236.362 db total= 5.06094 db mean= -40.0936 db
	 Amplitude: max= 0.316228  min= 1.52014e-12  total= 14.1056  mean= 0.00043047
	 Amplitude db: max= -5 db min= -118.181 db total= 11.4939 db mean= -33.6606 db


*/

void FFT::calcPowerAverages(CPX* in, double *out, int numSamples)
{

	numSamples = m_fftSize; //Todo: Remove arg

    //This is called from code that is locked with fftMutex, don't re-lock or will hang
	double binPower;
	double binAmp;
	double binPowerdB;
	double binAmpdB;
	double psd;
	double asd;

    // FFT output index 0 to N/2-1
    // is frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term )
    // Buffer is already unfolded, unlike in cuteSDR implementation, so simple loop
	// FFT results contain both amplitude and phase information (re/im)
	// Without this, an inverse FFT would not be able to recreate the original signal (sine waves)

	//DB::analyzeCPX(in, numSamples, "Spectrum Out", true, windowFunction->coherentGain, maxBinPower);

	//Some texts say Accelerate fft results are in polar notation (re = mag and im = phase)
	//But analyzing results shows that re varies +/-, so it can't be magnitude

	for( int i = 0; i < numSamples; i++){
		//Amplitude Spectral Density
		//fft output is in amplitude, so calculate that first
		asd = DB::amplitude(in[i]) / m_windowFunction->coherentGain;
		//Power Spectral Density
		//calculate power based on amplitude
		psd = DB::amplitudeToPower(asd);
		//We can also correct for effective noise bandwidth, scallop loss, etc if necessary

		//Normalize to 0 to 1, see detailed explanation above
		psd /= m_maxBinPower;

		asd /= m_maxBinPower;

		//Normalize for fft size if necessary
		//Tests with -10db testbench signal, 1msps, expected maxDB -10.00
		//fftSize maxDB   binWidth
		//  2048 -10.3044 488.28
		//  4096 -10.1264 244.14
		//  8192 -10.0096 122.07
		// 16384 -10.0096  61.04
		// 32768 -10.0002  30.52
		//Smaller bins == less correction needed
		//Smaller bins have fewer frequency peaks that are not centered in the bin, so more accurate
		//Lyons ffr size correction: mag = sqrt((re^2 + im^2)) / (N * coherentGain))
		//From http://www.ap.com/kb/show/170
		//When fftSize is doubled, binWidth is halved, reducing the noise power of each bin by a factor of 2
		//Factor of 2 equates to a 3db reduction every time we double
		//Todo: correct for small error

		//Can't average db, do average on power before conversion
		if (m_isAveraged) {
			binPower = (psd + m_fftPower[i]) / 2;
			binAmp = (asd + m_fftAmplitude[i]) /2;
		} else {
			//Direct results
			binPower = psd;
			binAmp = asd;
		}
		//Keep unaveraged values for next pass
		m_fftPower[i] = psd;
		m_fftAmplitude[i] = asd;

		binPowerdB = DB::clip(DB::powerTodB(binPower));
		binAmpdB = DB::clip(DB::amplitudeTodB(binAmp));

		//Todo: UI to switch between amplitude and power spectrum?
		out[i] = binAmpdB;
		//out[i] = binPowerdB;

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

bool FFT::mapFFTToScreen(double *inBuf,

		qint32 yPixels, //Height of the plot area
		qint32 xPixels, //Width of the plot area
		double maxdB, //Fft value corresponding to output value 0
		double mindB, //FFT value corresponding to output value DB::maxDb
		qint32 startFreq, //In +/- hZ, relative to 0
		qint32 stopFreq, //In +/- hZ, relative to 0
		qint32* outBuf)
{

	qint32 binLow;
	qint32 binHigh;
	//float hzPerBin = (float)sampleRate / (float)fftSize; //hZ per fft bin
	float binsPerHz = (float)m_fftSize / (float)m_sampleRate;
	//
	//qint32 fftLowFreq = -sampleRate / 2;
	//qint32 fftHighFreq = sampleRate / 2;

	//Find the beginning and ending FFT bins that correspond to Start and StopFreq
	//
	//Ex: If no zoom, binLow will be 1/2 of fftSize or -1024
	binLow = (qint32)(startFreq * binsPerHz);
	//Ex: makes it zero relative, so will be 0
	binLow += (m_fftSize/2);
	//Ex: 1024 for full spectrum
	binHigh = (qint32)(stopFreq * binsPerHz);
	//Ex: now 2048
	binHigh += (m_fftSize/2);

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

	m_fftMutex.lock();

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
			} else if (fftBin >= m_fftSize) {
				powerdB = DB::minDb;
			} else if (lastFftBin > 0 && fftBin != lastFftBin + 1) {
				//Lowest freq is in fftBin[0]

				//We skipped some bins, average the ones we skipped - including this one
				qint32 skippedBins = fftBin - lastFftBin;
				//dB can only be averaged directly if the differences are very small
				//Otherwise we need to convert back to power, and then convert the average power back to dB
				//Brute force, there may be faster algorithms
				//Bug: Hires occasionally flickers with wierd display
				double tmpPower = 0;
				for (int j=0; j<skippedBins; j++) {
					tmpPower += DB::dBToPower(inBuf[lastFftBin + j]);
				}
				powerdB = DB::powerTodB(tmpPower/skippedBins) - maxdB;

			} else {
				powerdB = inBuf[fftBin] - maxdB;
			}
			lastFftBin = fftBin;
			// y(0) = maxdB (top of screen)  y(yPixels) = mindB (bottom of screen)
			//dbGainFactor scales powerDb to this range
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
			} else if (fftBin >= m_fftSize) {
				powerdB = DB::minDb;
			} else {
				powerdB = inBuf[fftBin] - maxdB;
			}
			outBuf[i] = (qint32)(yPixels * dBGainFactor * powerdB);
			lastFftBin = fftBin;

		} //End xPixel loop
	}

	m_fftMutex.unlock();

	return false; //True if y has to be truncated (overloaded)
}
