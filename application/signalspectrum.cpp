//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalspectrum.h"
#include "firfilter.h"

SignalSpectrum::SignalSpectrum(int _sampleRate, quint32 _hiResSampleRate, int _numSamples):
	SignalProcessing(_sampleRate,_numSamples)
{
	//FFT bin size can be greater than sample size
	numSpectrumBins = global->settings->numSpectrumBins;
	numHiResSpectrumBins = global->settings->numHiResSpectrumBins;

	hiResSampleRate = _hiResSampleRate;
	//Output buffers
	rawIQ = CPXBuf::malloc(numSamples);

	fftUnprocessed = FFT::Factory("Unprocessed spectrum");
	unprocessed = new double[numSpectrumBins];

	fftHiRes = FFT::Factory("HiRes spectrum");
	hiResBuffer = new double[numHiResSpectrumBins];

	tmp_cpx = CPXBuf::malloc(numSpectrumBins);
	//Create our window coefficients 
	window = new double[numSpectrumBins];
	window_cpx = CPXBuf::malloc(numSpectrumBins);
	FIRFilter::MakeWindow(FIRFilter::BLACKMANHARRIS, numSpectrumBins, window);
	for (int i = 0; i < numSpectrumBins; i++)
	{
		window_cpx[i].re = window[i];
		window_cpx[i].im = window[i];
	}
	hiResWindow = new double[numHiResSpectrumBins];
	hiResWindow_cpx = CPXBuf::malloc(numHiResSpectrumBins);
	FIRFilter::MakeWindow(FIRFilter::BLACKMANHARRIS, numHiResSpectrumBins, hiResWindow);
	for (int i = 0; i < numHiResSpectrumBins; i++)
	{
		hiResWindow_cpx[i].re = hiResWindow[i];
		hiResWindow_cpx[i].im = hiResWindow[i];
	}

	//db calibration
	dbOffset  = global->settings->dbOffset;

    //Spectrum refresh rate from 1 to 50 per second
    //Init here for now and add UI element to set, save with settings data
	updatesPerSec = global->settings->updatesPerSecond; //Refresh rate per second
    skipFfts = 0; //How many samples should we skip to sync with rate
    skipFftsCounter = 0; //Keep count of samples we've skipped
	skipFftsHiResCounter = 0; //Keep count of samples we've skipped
    displayUpdateComplete = true;
    displayUpdateOverrun = 0;

	useHiRes = false;

	SetSampleRate(sampleRate, hiResSampleRate);

}

SignalSpectrum::~SignalSpectrum(void)
{
	if (unprocessed != NULL) {free (unprocessed);}
	if (hiResBuffer != NULL) {free (hiResBuffer);}
    if (window != NULL) {free (window);}
	if (window_cpx != NULL) {CPXBuf::free(window_cpx);}
	if (hiResWindow != NULL) {free (hiResWindow);}
	if (hiResWindow_cpx != NULL) {CPXBuf::free(hiResWindow_cpx);}
	if (tmp_cpx != NULL) {CPXBuf::free(tmp_cpx);}
	if (rawIQ != NULL) {CPXBuf::free(rawIQ);}
}

void SignalSpectrum::SetSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate)
{
    sampleRate = _sampleRate;
	hiResSampleRate = _hiResSampleRate;
	fftUnprocessed->FFTParams(numSpectrumBins, DB::maxDb, sampleRate);
	fftHiRes->FFTParams(numHiResSpectrumBins, DB::maxDb, hiResSampleRate);
    //Based on sample rates
	SetUpdatesPerSec(global->settings->updatesPerSecond);
    emitFftCounter = 0;

}

void SignalSpectrum::Unprocessed(CPX * in, double inUnder, double inOver,double outUnder, double outOver, int _numSamples)
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
	MakeSpectrum(fftUnprocessed, in, unprocessed, _numSamples, window_cpx);
	displayUpdateComplete = false;
	emit newFftData();
}

//http://www.arc.id.au/ZoomFFT.html
void SignalSpectrum::Zoomed(CPX *in, int _numSamples)
{
	if (!useHiRes)
		return; //Nothing to do

    //Only make spectrum often enough to match spectrum update rate, otherwise we just throw it away
	if (++skipFftsHiResCounter < skipFftsHiRes)
        return;
	skipFftsHiResCounter = 0;

	MakeSpectrum(fftHiRes, in, hiResBuffer, _numSamples, hiResWindow_cpx);
	//Updated HiRes fft data won't be available to SpectrumWidget untill Unprocessed() is called in next loop
	//Should have no impact and avoids displaying spectrum 2x in each ProcessIQ loop
	//This signal is for future use in case we want to do special handling in SpectrumWidget
	emit newHiResFftData();
}

void SignalSpectrum::MakeSpectrum(FFT *fft, CPX *in, double *sOut, int _numSamples, CPX *_window)
{
	//Must work with unprocessed or zoomed fft, so get size directly from fft
	int fftSize = fft->getFFTSize();
    //Smooth the data with our window
	CPXBuf::clear(tmp_cpx, fftSize);
	CPXBuf::copy(tmp_cpx,in, _numSamples);

	//Zero padded
	//Apply window filter so we get better FFT results
	CPXBuf::mult(tmp_cpx, tmp_cpx, _window, fftSize);

	fft->FFTSpectrum(tmp_cpx, sOut, fftSize);

    //out now has the spectrum in db, -f..0..+f
}

//Obsolete and needs work if we use in future
void SignalSpectrum::MakeSpectrum(FFT *fft, double *sOut)
{
    //Only make spectrum often enough to match spectrum update rate, otherwise we just throw it away
	if (updatesPerSec != 0 ||  ++skipFftsCounter >= skipFfts) {
        skipFftsCounter = 0;

        if (displayUpdateComplete) {
			CPXBuf::clear(tmp_cpx, numSpectrumBins);

			if (numSpectrumBins < fft->getFFTSize()) {
                //Decimate to fit spectrum binCount
				int decimate = fft->getFFTSize() / numSpectrumBins;
				for (int i=0; i<numSpectrumBins; i++)
                    tmp_cpx[i] = fft->getFreqDomain()[i*decimate];
            } else {
				CPXBuf::copy(tmp_cpx,fft->getFreqDomain(),numSpectrumBins);
            }

			fft->FreqDomainToMagnitude(tmp_cpx, 0, dbOffset, sOut);
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
	for (int i=0; i< numSpectrumBins ;i++) {
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
