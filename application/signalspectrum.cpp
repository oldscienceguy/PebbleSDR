//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalspectrum.h"
#include "firfilter.h"

SignalSpectrum::SignalSpectrum(quint32 _sampleRate, quint32 _hiResSampleRate, quint32 _bufferSize):
	ProcessStep(_sampleRate,_bufferSize)
{
	//FFT bin size can be greater than sample size
	m_numSpectrumBins = global->settings->m_numSpectrumBins;
	m_numHiResSpectrumBins = global->settings->m_numHiResSpectrumBins;

	m_hiResSampleRate = _hiResSampleRate;
	//Output buffers
	m_rawIQ = memalign(numSamples);

	m_fftUnprocessed = FFT::factory("Unprocessed spectrum");
	m_unprocessedSpectrum = new double[m_numSpectrumBins];

	m_fftHiRes = FFT::factory("HiRes spectrum");
	m_hiResSpectrum = new double[m_numHiResSpectrumBins];

	m_tmp_cpx = memalign(m_numSpectrumBins);

	//db calibration
	m_dbOffset  = global->settings->m_dbOffset;

    //Spectrum refresh rate from 1 to 50 per second
    //Init here for now and add UI element to set, save with settings data
	m_updatesPerSec = global->settings->m_updatesPerSecond; //Refresh rate per second
	//Elapsed time in ms for use with QElapsedTimer
	m_spectrumTimerUpdate = 1000 /m_updatesPerSec;
	m_hiResTimerUpdate = 1000/ m_updatesPerSec;

	m_displayUpdateComplete = true;
	m_displayUpdateOverrun = 0;

	m_useHiRes = false;

	m_isOverload = false;

	setSampleRate(sampleRate, m_hiResSampleRate);

}

SignalSpectrum::~SignalSpectrum(void)
{
	if (m_unprocessedSpectrum != NULL) {free (m_unprocessedSpectrum);}
	if (m_hiResSpectrum != NULL) {free (m_hiResSpectrum);}
	if (m_tmp_cpx != NULL) {free(m_tmp_cpx);}
	if (m_rawIQ != NULL) {free(m_rawIQ);}
}

void SignalSpectrum::setSampleRate(quint32 _sampleRate, quint32 _hiResSampleRate)
{
    sampleRate = _sampleRate;
	m_hiResSampleRate = _hiResSampleRate;
	m_fftUnprocessed->fftParams(m_numSpectrumBins, DB::maxDb, sampleRate, numSamples, WindowFunction::BLACKMANHARRIS);
	m_fftHiRes->fftParams(m_numHiResSpectrumBins, DB::maxDb, m_hiResSampleRate, numSamples, WindowFunction::BLACKMANHARRIS);
	m_emitFftCounter = 0;
}

void SignalSpectrum::unprocessed(CPX * in, int _numSamples)
{	
	if (!m_spectrumTimer.isValid()) {
		m_spectrumTimer.start(); //First time
		return;
	}
	if (m_updatesPerSec == 0 ||  m_spectrumTimer.elapsed() < m_spectrumTimerUpdate)
		return;
	m_spectrumTimer.start(); //Reset

	if (!m_displayUpdateComplete) {
        //We're not keeping up with display for some reason
		//qDebug()<<"Display update overrun counter "<<displayUpdateOverrun;
		m_displayUpdateOverrun++;
    }

    //Keep a copy raw I/Q to local buffer for display
	//copyCPX(rawIQ, in, numSamples);
	//global->perform.StartPerformance("MakeSpectrum");
	makeSpectrum(m_fftUnprocessed, in, m_unprocessedSpectrum, _numSamples);
	//global->perform.StopPerformance(10);
	m_displayUpdateComplete = false;
	emit newFftData();
}

//http://www.arc.id.au/ZoomFFT.html
void SignalSpectrum::zoomed(CPX *in, int _numSamples)
{
	if (!m_useHiRes)
		return; //Nothing to do

	if (!m_hiResTimer.isValid()) {
		m_hiResTimer.start(); //First time
		return;
	}
	if (m_updatesPerSec == 0 ||  m_hiResTimer.elapsed() < m_hiResTimerUpdate)
		return;
	m_hiResTimer.start(); //Reset

	makeSpectrum(m_fftHiRes, in, m_hiResSpectrum, _numSamples);
	//Updated HiRes fft data won't be available to SpectrumWidget untill Unprocessed() is called in next loop
	//Should have no impact and avoids displaying spectrum 2x in each ProcessIQ loop
	//This signal is for future use in case we want to do special handling in SpectrumWidget
	emit newHiResFftData();
}

void SignalSpectrum::makeSpectrum(FFT *fft, CPX *in, double *sOut, int _numSamples)
{
	m_isOverload = fft->fftSpectrum(in, sOut, _numSamples);
    //out now has the spectrum in db, -f..0..+f
}

void SignalSpectrum::setSpectrum(double *in)
{
	for (int i=0; i< m_numSpectrumBins ;i++) {
		m_unprocessedSpectrum[i] = in[i];
	}
	m_displayUpdateComplete = false;
	emit newFftData();
}

void SignalSpectrum::setUpdatesPerSec(int updatespersec)
{
	m_updatesPerSec = updatespersec;
	global->settings->m_updatesPerSecond = m_updatesPerSec;
	if (m_updatesPerSec > 0) {
		//Elapsed time in ms for use with QElapsedTimer
		m_spectrumTimerUpdate = 1000 /m_updatesPerSec;
		m_hiResTimerUpdate = 1000/ m_updatesPerSec;
	}
}


//See fft.cpp for details, this is here as a convenience so we don't have to expose FFT everywhere
bool SignalSpectrum::mapFFTToScreen(qint32 maxHeight,
                                qint32 maxWidth,
                                double maxdB,
                                double mindB,
                                qint32 startFreq,
                                qint32 stopFreq,
                                qint32* outBuf )
{
	if (m_fftUnprocessed!=NULL)
		return m_fftUnprocessed->mapFFTToScreen(m_unprocessedSpectrum, maxHeight,maxWidth,maxdB,mindB,startFreq,stopFreq,outBuf);
    else
        return false;
}
//See fft.cpp for details, this is here as a convenience so we don't have to expose FFT everywhere
bool SignalSpectrum::mapFFTZoomedToScreen(qint32 maxHeight,
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
	quint16 span = m_hiResSampleRate * zoom;

	if (m_fftHiRes!=NULL)
		return m_fftHiRes->mapFFTToScreen(m_hiResSpectrum,maxHeight,maxWidth,maxdB,mindB, -span/2 - modeOffset, span/2 - modeOffset, outBuf);
    else
        return false;
}
