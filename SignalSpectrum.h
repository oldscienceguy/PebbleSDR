#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include "demod.h"
#include <QMutex>
#include "settings.h"
#include "goertzel.h"

class SignalSpectrum :
	public SignalProcessing
{
public:
	//Moved from spectrumWidget to avoid .h circular ref.  We need to know mode so we can skip non-visible displays
    enum DISPLAYMODE {SPECTRUM = 0,WATERFALL,IQ,PHASE,NODISPLAY,POSTMIXER,POSTBANDPASS};

	SignalSpectrum(int sr, int ns, Settings *set);
	~SignalSpectrum(void);
	void SetDisplayMode(DISPLAYMODE m);
	//Pass in soundcard buffer under/overflow counts for display
	void Unprocessed(CPX * in, double inUnder, double inOver,double outUnder, double outOver);
	void PostMixer(CPX *in);
	void PostBandPass(CPX *in, int size);
    void MakeSpectrum(CPX *in, double *out, int size); //Use if we just have CPX samples
    void MakeSpectrum(FFT *fft, double *out); //Used if we already have FFT

	int BinCount() {return binCount;}
    double *Unprocessed() {return unprocessed;}
    double *PostMixer() {return postMixer;}
    double *PostBandPass() {return postBandPass;}
	CPX *RawIQ() {return rawIQ;}

	double inBufferUnderflowCount;
	double inBufferOverflowCount;
	double outBufferUnderflowCount;
	double outBufferOverflowCount;

	Settings *settings;


private:
	QMutex mutex;
	DISPLAYMODE displayMode; //Don't FFT spectrum if that display is not active
	int binCount;

	//Spectrum data at different steps in receive chaing
	CPX *rawIQ;
    double *unprocessed;
    double *postMixer;
    double *postBandPass;
    double * window;

    CPX *tmp_cpx;
	CPX *window_cpx;
    FFT *fft;

    float dbOffset; //Used to calibrate power to db calculations

};
