//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalspectrum.h"
#include "firfilter.h"

SignalSpectrum::SignalSpectrum(int sr, quint32 zsr, int ns, Settings *set):
	SignalProcessing(sr,ns)
{
	settings = set;

	//FFT bin size can be greater than sample size
	//But we don't need 2x bins for FFT spectrum processing unless we are going to support
	//a zoom function, ie 500 pixels can show 500 bins, or a portion of available spectrum data
	//Less bins = faster FFT
	if (false)
        fftSize = numSamplesX2;
	else
        fftSize = numSamples;

	//Output buffers
	rawIQ = CPXBuf::malloc(numSamples);

	fftUnprocessed = FFT::Factory();
    unprocessed = new double[fftSize];

	fftHiRes = FFT::Factory();
	hiResBuffer = new double[fftSize];

    tmp_cpx = CPXBuf::malloc(fftSize);
	//Create our window coefficients 
    window = new double[numSamples];
	window_cpx = CPXBuf::malloc(numSamples);
	FIRFilter::MakeWindow(FIRFilter::BLACKMANHARRIS, numSamples, window);
	for (int i = 0; i < numSamples; i++)
	{
		window_cpx[i].re = window[i];
		window_cpx[i].im = window[i];
	}

	//db calibration
    dbOffset  = settings->dbOffset;

    //Spectrum refresh rate from 1 to 50 per second
    //Init here for now and add UI element to set, save with settings data
	updatesPerSec = global->settings->updatesPerSecond; //Refresh rate per second
    skipFfts = 0; //How many samples should we skip to sync with rate
    skipFftsCounter = 0; //Keep count of samples we've skipped
	skipFftsHiResCounter = 0; //Keep count of samples we've skipped
    displayUpdateComplete = true;
    displayUpdateOverrun = 0;

	isHiRes = false;

    SetSampleRate(sr, zsr);

}

SignalSpectrum::~SignalSpectrum(void)
{
	if (unprocessed != NULL) {free (unprocessed);}
	if (hiResBuffer != NULL) {free (hiResBuffer);}
    if (window != NULL) {free (window);}
	if (window_cpx != NULL) {CPXBuf::free(window_cpx);}
	if (tmp_cpx != NULL) {CPXBuf::free(tmp_cpx);}
	if (rawIQ != NULL) {CPXBuf::free(rawIQ);}
}
void SignalSpectrum::SetDisplayMode(DISPLAYMODE _displayMode, bool _isZoomed)
{
    displayMode = _displayMode;
	isHiRes = _isZoomed;
}

void SignalSpectrum::SetSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate)
{
    sampleRate = _sampleRate;
	hiResSampleRate = _hiResSampleRate;
	fftUnprocessed->FFTParams(fftSize, +1, DB::maxDb, sampleRate);
	fftHiRes->FFTParams(fftSize, +1, DB::maxDb, hiResSampleRate);
    //Based on sample rates
	SetUpdatesPerSec(global->settings->updatesPerSecond);
    emitFftCounter = 0;

}

void SignalSpectrum::Unprocessed(CPX * in, double inUnder, double inOver,double outUnder, double outOver)
{	
    //Only make spectrum often enough to match spectrum update rate, otherwise we just throw it away
	if (updatesPerSec == 0 ||  ++skipFftsCounter < skipFfts)
        return;
    skipFftsCounter = 0;

    if (!displayUpdateComplete) {
        //We're not keeping up with display for some reason
		//qDebug()<<"Display update overrun counter "<<displayUpdateOverrun;
        displayUpdateOverrun++;
    }

    inBufferUnderflowCount = inUnder;
	inBufferOverflowCount = inOver;
	outBufferUnderflowCount = outUnder;
	outBufferOverflowCount = outOver;

    //Keep a copy raw I/Q to local buffer for display
    //CPXBuf::copy(rawIQ, in, numSamples);
	if (displayMode != NODISPLAY) {
        MakeSpectrum(fftUnprocessed, in, unprocessed, numSamples);
		displayUpdateComplete = false;
		emit newFftData();
	}
}

//http://www.arc.id.au/ZoomFFT.html
void SignalSpectrum::Zoomed(CPX *in, int size)
{
	quint16 resampledSize;
	Q_UNUSED(resampledSize)

	if (!isHiRes)
		return; //Nothing to do

    //Only make spectrum often enough to match spectrum update rate, otherwise we just throw it away
	if (++skipFftsHiResCounter < skipFftsHiRes)
        return;
	skipFftsHiResCounter = 0;

#if 0
	//Experiment: This currently gets called with demodSampleRate - 62k for non WFM modes
	//62,000 / 2048 buckets = 30hz per bucket
	//If we want to get even finer resolution, we can further resample to a lower rate
	//ie 24k sample rate = 12hz per bucket
	//
	//We need to fill with zeros because Resample won't fill entire buffer
	CPXBuf::clear(zoomedResampled,fftSize);
	//LP Filter needed to get rid of aliasing unless .Resample does it for us
	resampledSize = zoomedResampler.Resample(size,sampleRateIn/sampleRateOut,in,zoomedResampled);
	//qDebug()<<"ResampledSize: "<<resampledSize<<" sampleRateIn: "<<sampleRateIn<<" sampleRateOut: "<<sampleRateOut;
	fftZoomed->FFTParams(fftSize, +1, DB::maxDb, sampleRateOut);
#endif
	MakeSpectrum(fftHiRes, in, hiResBuffer, size);
	//Updated HiRes fft data won't be available to SpectrumWidget untill Unprocessed() is called in next loop
	//Should have no impact and avoids displaying spectrum 2x in each ProcessIQ loop
	//This signal is for future use in case we want to do special handling in SpectrumWidget
	emit newHiResFftData();
}

void SignalSpectrum::MakeSpectrum(FFT *fft, CPX *in, double *sOut, int size)
{
    //Smooth the data with our window
    CPXBuf::clear(tmp_cpx, fftSize);
    //Since tmp_cpx is 2x size of in, we're left with empty entries at the end of tmp_cpx
    //The extra zeros don't change the frequency component of the signal and give us smaller bins
    //But why do we need more bins for spectrum, we're just going to decimate to fit # pixels in graph

    //Zero padded
    if (size < fftSize)
        CPXBuf::scale(tmp_cpx, in, 1.0, size);
    else
        //Apply window filter so we get better FFT results
        //If size != numSamples, then window_cpx will not be right and we skip
        CPXBuf::mult(tmp_cpx, in, window_cpx, fftSize);

    //I don't think this is critical section
    //mutex.lock();

#if 0
    //Change #if here in sync with SpectrumWidget to switch between old and new paint
    fft->FFTMagnForward (tmp_cpx, size, 0, dbOffset, sOut);
#else
    fft->FFTSpectrum(tmp_cpx, sOut, fftSize);
#endif

    //out now has the spectrum in db, -f..0..+f
    //mutex.unlock();
}

void SignalSpectrum::MakeSpectrum(FFT *fft, double *sOut)
{
    //Only make spectrum often enough to match spectrum update rate, otherwise we just throw it away
	if (updatesPerSec != 0 ||  ++skipFftsCounter >= skipFfts) {
        skipFftsCounter = 0;

        if (displayUpdateComplete) {
            CPXBuf::clear(tmp_cpx, fftSize);

            if (fftSize < fft->getFFTSize()) {
                //Decimate to fit spectrum binCount
                int decimate = fft->getFFTSize() / fftSize;
                for (int i=0; i<fftSize; i++)
                    tmp_cpx[i] = fft->getFreqDomain()[i*decimate];
            } else {
                CPXBuf::copy(tmp_cpx,fft->getFreqDomain(),fftSize);
            }

            fft->FreqDomainToMagnitude(tmp_cpx, fftSize, 0, dbOffset, sOut);
            displayUpdateComplete = false;
            emit newFftData();
        } else {
            //We're not able to keep up with selected display rate, display is taking longer than refresh rate
            //We could auto-adjust display rate when we get here
            displayUpdateOverrun++;
        }
	}
}

void SignalSpectrum::SetSpectrum(double *in)
{
	for (int i=0; i< fftSize ;i++) {
		unprocessed[i] = in[i];
	}
	displayUpdateComplete = false;
	emit newFftData();
}

//UpdatesPerSecond 1(min) to 50(max)
void SignalSpectrum::SetUpdatesPerSec(int updatespersec)
{
    // updateInterval = 1 / UpdatesPerSecond = updateInterval
    // updateInterval = 1 / 1 = 1 update ever 1.000 sec
    // updateInterval = 1 / 10 = 1 update every 0.100sec (100 ms)
    // updateInterval = 1/ 50 = 1 update every 0.020 sec (20ms)
    // So we only need to proccess 1 FFT Spectrum block every updateInterval seconds
    // If we're sampling at 192,000 sps and each Spectrum fft is 4096 (fftSize)
    // then there are 192000/4096 or 46.875 possible FFTs every second
    // fftsToSkip = FFTs per second * updateInterval
    // fftsToSkip = (192000 / 4096) * 1.000 sec = 46.875 = skip 46 and process every 47th
    // fftsToSkip = (192000 / 4096) * 0.100 sec = 4.6875 = skip 4 and process every 5th FFT
    // fftsToSkip = (192000 / 4096) * 0.020 sec = 0.920 = skip 0 and process every FFT
    updatesPerSec = updatespersec;
	global->settings->updatesPerSecond = updatesPerSec;
	skipFfts = skipFftsCounter = 0;
	skipFftsHiRes = skipFftsHiResCounter = 0;
	if (updatesPerSec > 0) {
		skipFfts = sampleRate / (numSamples * updatesPerSec);
		skipFftsHiRes = hiResSampleRate / (numSamples * updatesPerSec);
	}
}


//See fft.cpp for details, this is here as a convenience so we don't have to expose FFT everywhere
bool SignalSpectrum::MapFFTToScreen(qint32 maxHeight,
                                qint32 maxWidth,
                                double maxdB,
                                double mindB,
                                qint32 startFreq,
                                qint32 stopFreq,
                                qint32* outBuf )
{
    if (fftUnprocessed!=NULL)
		return fftUnprocessed->MapFFTToScreen(unprocessed, maxHeight,maxWidth,maxdB,mindB,startFreq,stopFreq,outBuf);
    else
        return false;
}
//See fft.cpp for details, this is here as a convenience so we don't have to expose FFT everywhere
bool SignalSpectrum::MapFFTZoomedToScreen(qint32 maxHeight,
                                qint32 maxWidth,
                                double maxdB,
                                double mindB,
                                double zoom,
                                int modeOffset, //For CWL and CWU
                                qint32* outBuf )
{
    //Zoomed spectrum is created AFTER mixer and downconvert and has modeOffset applied
    //So if unprocessed spectrum is centered on 10k, zoomed spectrum will be centered on 10k +/- mode offset
    //We correct for this by adjusting starting,ending frequency by mode offset
	quint16 span = hiResSampleRate * zoom;

	if (fftHiRes!=NULL)
		return fftHiRes->MapFFTToScreen(hiResBuffer,maxHeight,maxWidth,maxdB,mindB, -span/2 - modeOffset, span/2 - modeOffset, outBuf);
    else
        return false;
}
