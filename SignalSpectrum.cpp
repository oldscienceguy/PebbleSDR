//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SignalSpectrum.h"
#include "firfilter.h"

SignalSpectrum::SignalSpectrum(int sr, int ns, Settings *set):
	SignalProcessing(sr,ns)
{
	settings = set;
	//FFT bin size can be greater than sample size
	//But we don't need 2x bins for FFT spectrum processing unless we are going to support
	//a zoom function, ie 500 pixels can show 500 bins, or a portion of available spectrum data
	//Less bins = faster FFT
	if (false)
		binCount = numSamplesX2;
	else
		binCount = numSamples;

	//Output buffers
	rawIQ = CPXBuf::malloc(numSamples);
	unprocessed = new float[binCount];
	postMixer = new float[binCount];
	postBandPass = new float[binCount];
	fft = new FFT(binCount);

    tmp_cpx = CPXBuf::malloc(binCount);
	//Create our window coefficients 
	window = new float[numSamples];
	window_cpx = CPXBuf::malloc(numSamples);
	FIRFilter::MakeWindow(FIRFilter::BLACKMANHARRIS, numSamples, window);
	for (int i = 0; i < numSamples; i++)
	{
		window_cpx[i].re = window[i];
		window_cpx[i].im = window[i];
	}

	//db calibration
    dbOffset  = settings->dbOffset;
}

SignalSpectrum::~SignalSpectrum(void)
{
	if (unprocessed != NULL) {free (unprocessed);}
	if (postMixer != NULL) {free (postMixer);}
	if (postBandPass != NULL) {free (postBandPass);}
	if (window != NULL) {free (window);}
	if (window_cpx != NULL) {CPXBuf::free(window_cpx);}
	if (tmp_cpx != NULL) {CPXBuf::free(tmp_cpx);}
	if (rawIQ != NULL) {CPXBuf::free(rawIQ);}
}
void SignalSpectrum::SetDisplayMode(DISPLAYMODE m)
{
	displayMode = m;
}

void SignalSpectrum::PostMixer(CPX *in)
{
	if (displayMode == POSTMIXER)
		MakeSpectrum(in, postMixer, numSamples);
}

//Bandpass can occur post decimation, in which case we won't have numSamples samples
//So we pass in a size from the receive chain
void SignalSpectrum::PostBandPass(CPX *in, int size)
{
	if (displayMode == POSTBANDPASS)
		MakeSpectrum(in, postBandPass, size);
}
void SignalSpectrum::Unprocessed(CPX * in, double inUnder, double inOver,double outUnder, double outOver)
{	
	inBufferUnderflowCount = inUnder;
	inBufferOverflowCount = inOver;
	outBufferUnderflowCount = outUnder;
	outBufferOverflowCount = outOver;

	if (displayMode == IQ || displayMode == PHASE)
		//Keep a copy raw I/Q to local buffer for display
		CPXBuf::copy(rawIQ, in, numSamples);
	else if (displayMode == SPECTRUM || displayMode == WATERFALL)
		MakeSpectrum(in, unprocessed, numSamples);
}

void SignalSpectrum::MakeSpectrum(CPX *in, float *sOut, int size)
{
	//Smooth the data with our window
	CPXBuf::clear(tmp_cpx, binCount);
	//Since tmp_cpx is 2x size of in, we're left with empty entries at the end of tmp_cpx
	//The extra zeros don't change the frequency component of the signal and give us smaller bins
	//But why do we need more bins for spectrum, we're just going to decimate to fit # pixels in graph
	
	//Apply window filter so we get better FFT results
	//If size != numSamples, then window_cpx will not be right and we skip
	if (true && size==numSamples)
		CPXBuf::mult(tmp_cpx, in, window_cpx, numSamples);
	else
		//Test without FFT window
		CPXBuf::copy(tmp_cpx,in,size);

	//I don't think this is critical section
    //mutex.lock();

	fft->DoFFTWMagnForward (tmp_cpx, size, 0, dbOffset, sOut);

	//out now has the spectrum in db, -f..0..+f
    //mutex.unlock();
}
void SignalSpectrum::MakeSpectrum(FFT *fft, float *sOut)
{
	CPXBuf::clear(tmp_cpx, binCount);

	if (binCount < fft->fftSize) {
		//Decimate to fit spectrum binCount
		int decimate = fft->fftSize / binCount;
		for (int i=0; i<binCount; i++)
			tmp_cpx[i] = fft->freqDomain[i*decimate];
	} else {
		CPXBuf::copy(tmp_cpx,fft->freqDomain,binCount);
	}

	fft->FreqDomainToMagnitude(tmp_cpx, binCount, 0, dbOffset, sOut);
}
