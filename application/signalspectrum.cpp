//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalspectrum.h"
#include "firfilter.h"

SignalSpectrum::SignalSpectrum(quint32 _sampleRate, quint32 _hiResSampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	//FFT bin size can be greater than sample size
	numSpectrumBins = global->settings->numSpectrumBins;
	numHiResSpectrumBins = global->settings->numHiResSpectrumBins;

	hiResSampleRate = _hiResSampleRate;
	//Output buffers
	rawIQ = CPX::memalign(numSamples);

	fftUnprocessed = FFT::Factory("Unprocessed spectrum");
	unprocessedSpectrum = new double[numSpectrumBins];

	fftHiRes = FFT::Factory("HiRes spectrum");
	hiResSpectrum = new double[numHiResSpectrumBins];

	tmp_cpx = CPX::memalign(numSpectrumBins);

	//db calibration
	dbOffset  = global->settings->dbOffset;

    //Spectrum refresh rate from 1 to 50 per second
    //Init here for now and add UI element to set, save with settings data
	updatesPerSec = global->settings->updatesPerSecond; //Refresh rate per second
	//Elapsed time in ms for use with QElapsedTimer
	spectrumTimerUpdate = 1000 /updatesPerSec;
	hiResTimerUpdate = 1000/ updatesPerSec;

    displayUpdateComplete = true;
    displayUpdateOverrun = 0;

	useHiRes = false;

	isOverload = false;

	SetSampleRate(sampleRate, hiResSampleRate);

}

SignalSpectrum::~SignalSpectrum(void)
{
	if (unprocessedSpectrum != NULL) {free (unprocessedSpectrum);}
	if (hiResSpectrum != NULL) {free (hiResSpectrum);}
	if (tmp_cpx != NULL) {free(tmp_cpx);}
	if (rawIQ != NULL) {free(rawIQ);}
}

void SignalSpectrum::SetSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate)
{
    sampleRate = _sampleRate;
	hiResSampleRate = _hiResSampleRate;
	fftUnprocessed->FFTParams(numSpectrumBins, DB::maxDb, sampleRate, numSamples, WindowFunction::BLACKMANHARRIS);
	fftHiRes->FFTParams(numHiResSpectrumBins, DB::maxDb, hiResSampleRate, numSamples, WindowFunction::BLACKMANHARRIS);
    emitFftCounter = 0;
}

void SignalSpectrum::Unprocessed(CPX * in, int _numSamples)
{	
	if (!spectrumTimer.isValid()) {
		spectrumTimer.start(); //First time
		return;
	}
	if (updatesPerSec == 0 ||  spectrumTimer.elapsed() < spectrumTimerUpdate)
		return;
	spectrumTimer.start(); //Reset

    if (!displayUpdateComplete) {
        //We're not keeping up with display for some reason
		//qDebug()<<"Display update overrun counter "<<displayUpdateOverrun;
        displayUpdateOverrun++;
    }

    //Keep a copy raw I/Q to local buffer for display
	//CPX::copyCPX(rawIQ, in, numSamples);
	//global->perform.StartPerformance("MakeSpectrum");
	MakeSpectrum(fftUnprocessed, in, unprocessedSpectrum, _numSamples);
	//global->perform.StopPerformance(10);
	displayUpdateComplete = false;
	emit newFftData();
}

//http://www.arc.id.au/ZoomFFT.html
void SignalSpectrum::Zoomed(CPX *in, int _numSamples)
{
	if (!useHiRes)
		return; //Nothing to do

	if (!hiResTimer.isValid()) {
		hiResTimer.start(); //First time
		return;
	}
	if (updatesPerSec == 0 ||  hiResTimer.elapsed() < hiResTimerUpdate)
		return;
	hiResTimer.start(); //Reset

	MakeSpectrum(fftHiRes, in, hiResSpectrum, _numSamples);
	//Updated HiRes fft data won't be available to SpectrumWidget untill Unprocessed() is called in next loop
	//Should have no impact and avoids displaying spectrum 2x in each ProcessIQ loop
	//This signal is for future use in case we want to do special handling in SpectrumWidget
	emit newHiResFftData();
}

void SignalSpectrum::MakeSpectrum(FFT *fft, CPX *in, double *sOut, int _numSamples)
{
	isOverload = fft->FFTSpectrum(in, sOut, _numSamples);
    //out now has the spectrum in db, -f..0..+f
}

void SignalSpectrum::SetSpectrum(double *in)
{
	for (int i=0; i< numSpectrumBins ;i++) {
		unprocessedSpectrum[i] = in[i];
	}
	displayUpdateComplete = false;
	emit newFftData();
}

void SignalSpectrum::SetUpdatesPerSec(int updatespersec)
{
    updatesPerSec = updatespersec;
	global->settings->updatesPerSecond = updatesPerSec;
	if (updatesPerSec > 0) {
		//Elapsed time in ms for use with QElapsedTimer
		spectrumTimerUpdate = 1000 /updatesPerSec;
		hiResTimerUpdate = 1000/ updatesPerSec;
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
		return fftUnprocessed->MapFFTToScreen(unprocessedSpectrum, maxHeight,maxWidth,maxdB,mindB,startFreq,stopFreq,outBuf);
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
		return fftHiRes->MapFFTToScreen(hiResSpectrum,maxHeight,maxWidth,maxdB,mindB, -span/2 - modeOffset, span/2 - modeOffset, outBuf);
    else
        return false;
}
