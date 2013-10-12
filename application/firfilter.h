#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "gpl.h"
#include "signalprocessing.h"
#include <QMutex>
#include "fftw.h"

class FIRFilter :
	public SignalProcessing
{
public:
	enum FILTERTYPE {LOWPASS, HIGHPASS, BANDPASS, BANDSTOP};
	enum WINDOWTYPE {RECTANGULAR=1,HANNING,WELCH,PARZEN,BARTLETT,HAMMING,BLACKMAN2,BLACKMAN3,
		BLACKMAN4,EXPONENTIAL,RIEMANN,BLACKMANHARRIS,BLACKMAN};
	FIRFilter(int sr, int ns, bool useFFT=false, int taps=64, int delay=0);
	~FIRFilter(void);
	//Apply the filter, results in out
	CPX * ProcessBlock(CPX *in);
    void Convolution(FFTfftw *fft);

	//Create coefficients for filter
	void setEnabled(bool b);
	//Mutually exclusive, filter can only be one of the below types
	void SetLowPass(float cutoff, WINDOWTYPE = HAMMING);
	void SetHighPass(float cutoff, WINDOWTYPE = HAMMING);
	void SetBandPass(float lo, float hi, WINDOWTYPE = BLACKMANHARRIS);
	void SetBandPass2(float lo, float hi, WINDOWTYPE = BLACKMAN); //Alternate algorithm
	void SetBandStop(float lo, float hi, WINDOWTYPE w = BLACKMANHARRIS); //FFT version only
	//Allows us to load predefined coefficients for FFT FIR
	void SetLoadable(float * coeff);

	//Utility functions
    static void MakeWindow(WINDOWTYPE wtype, int size, double *window);

	//Angular frequency in radians/sec w = 2*Pi*frequency //This lower case w appears in many formulas
	//fHz is normalized for sample rate, ie fHz = f/fs = 10000/48000
	static inline float fAng(float fHz) {return TWOPI * fHz;}
	//Angular frequency in degrees/sec
	static inline float fDeg(float fHz) {return 360 * fHz;}

	CPX Sinc(float Fc, int i);
	CPX Sinc2(float Fc, int i); //Alternate algorithm
	void SpectralInversion();
	void SpectralReversal();



private:
	QMutex mutex;
	bool useFFT;
	int numTaps; //Size of filter coefficient array
	int M; //numTaps -1
	int delay;
	float lpCutoff;
	float hpCutoff;
	bool enabled;
	DelayLine * delayLine;
	//Filter coefficients
	CPX *taps; 
    double *window;

	//FFT versions
    FFTfftw *fftFIR;
    FFTfftw *fftSamples;

	void FFTSetBandPass(float lo, float hi,WINDOWTYPE wtype);
	void FFTSetLowPass(float cutoff, WINDOWTYPE wtype);
	void FFTSetHighPass(float cutoff, WINDOWTYPE wtype); //NOT WORKING
	void FFTSetBandStop(float lo, float hi, WINDOWTYPE wtype); //NOT TESTED

	//Converts taps[] to fftTaps[]
	void MakeFFTTaps();


};
