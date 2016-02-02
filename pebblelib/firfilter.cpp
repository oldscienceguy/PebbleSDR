//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "firfilter.h"
#include "cmath" //For std::abs() that supports float.  Include last so it overrides abs(int)

/*
The best explanation I found for how FIR filters work was "Digital Filters, 3rd Edition" by R.W. Hamming
My version for reference

Sample Buffer: u[0] u[1] u[2] u[3] u[4] u[5] u[6]... u[n-1]
where u[0] is the most current sample and u[n-1] the oldest in a buffer of size n

Filter Coefficients: c[0] c[1] c[2] c[3]
for a simple filter with 4 taps

To compute output buffer o[0..n-1]
u[0] u[1] u[2] u[3] u[4] u[5] u[6]... u[n-1]
c[0] c[1] c[2] c[3]
o[0] = u[0]*c[0] + u[1]*c[1] + u[2]*c[2] + u[3]*c[3] //This is the Multiply And Accumulate operation (MAC)
//Next sample
u[0] u[1] u[2] u[3] u[4] u[5] u[6]... u[n-1]
     c[0] c[1] c[2] c[3]
     o[1] = u[1]*c[0] + u[2]*c[1] + u[3]*c[2] + u[4]*c[3] 
//Next sample
u[0] u[1] u[2] u[3] u[4] u[5] u[6]... u[n-1]
          c[0] c[1] c[2] c[3]
          o[2] = u[2]*c[0] + u[3]*c[1] + u[4]*c[2] + u[5]*c[3] 

and so on.  You can see how this can be easily coded in a loop for any size buffer and coefficients

If c[] = {1,0,0,0} then the output is the same as the input
if c[] = {0,0,0,1] then the ouput is 'delayed' by 4 samples

The size of c[] and the values chosen for c[] to achieve are very precise to achieve the desired filter characteristics
They are computed by DSP algorithms in the code, or by DSP tools (online and offline) that do the math
Remember that we are NOT just changing amplitude.  
There is an equivalence between the time domain and the frequency domain that is captured in the Fourier Transform
So by changing these values we are in fact, doing frequency filtering

Note there is a problem when we get close to the end of buffer u[]
u[0] u[1] u[2] u[3] u[4] u[5] u[6] u[n-1]
                              c[0] c[1] c[2] c[3]
We run out of values in u[] to MAC with c[]
To solve this, we create a 'Delay line' that remembers values across buffers so that the filtering
is continuous across samples without any end-of-buffer discontinuity.
The delay line is implemented as a 'ring buffer' so it never overflows and always has at least enough samples for c[]

Each sample gets added to the delay line, and then the filtered value is calculated.
Note that the delay line must be kept in time order also, so the index is usually decremeneted

So the end of buffer scenario with a delay line d[], it now looks like this;
Delay d[6] is assigned the most current sample, like u[0].
Delay d[0] has the next older sample from the previous buffer u[...]
Delay d[1] has the next oldest, etc

                                  v- Delay line index wraps to create ring  
d[0] d[1] d[2] d[3] d[4] d[5] d[6] d[0] d[1] d[2] d[3] d[4] d[5] dp[6]
                              c[0] c[1] c[2] c[3]

Other notes and 'discoveries'
Angular frequency w = 2*Pi*frequency //This lower case w appears in many formulas
H(w) is called the transfer function.
So H(w) = H(2*Pi*Fc) in forumulas

fc in forumulas is usually normalize to sample rate so
fc = freq / sample rate

FIR Filters can be used in conjuction with FFT based filters.
FFT on FIR coefficients creates a frequency mask which can then be applied to subsequent
batches of samples.  
See text on Digital Filtering by Fast Convolution (Lynn) and Chapt 18 FFT Convolution (Smith) for
details on overlap-add method of combining filter results and dealing with convolution expansion



*/

FIRFilter::FIRFilter(int sr, int ns, bool _useFFT, int _numTaps, int _delay)
{
	sampleRate = sr;
	numSamples = ns;
	numSamplesX2 = ns*2; //Frequency used
	out = CPX::memalign(numSamples);

	useFFT = _useFFT;
	lpCutoff = 0.0;
	delay = _delay;
	if (useFFT) {
		//Ignore _numtaps and size at 2X numSamples to make room for FFT Overlap-Add expansion
		numTaps = 0; //Not used for FFT
		M = numTaps; //Not used for FFT
		//FFT sizes must be sizeof fftFIR + sizeof fftSamples -1
		//to avoid circular convolution.
		//Circular convolution appears as a ghost signal that does not show up in the spectrum at the
		//mirror frequency of a signal in the lower part of the spectrum
		fftFIR = FFT::Factory("FIR");
		fftFIR->FFTParams(numSamplesX2, 0, sr, numSamples, WindowFunction::NONE);
		fftSamples = FFT::Factory("FIR samples");
		fftSamples->FFTParams(numSamplesX2, 0, sr, numSamples, WindowFunction::NONE);

		//Time domain FIR coefficients
		taps = CPX::memalign(numSamples);
		CPXBuf::clear (taps,numSamples);

		//We don't need a delayLine
		delayLine = NULL;
	} else {
		//Algorithm says M must be even, and we need 0..numTaps values
		//So add 1 to whatever we're given, M is even and numTaps is M+1
		numTaps = _numTaps + 1;
		M = _numTaps; //For sinc formula
		//Delay line has to be large enough so we have 1 sample per tap
		delayLine = new DelayLine((numTaps + delay) * 2, delay);

		taps = CPX::memalign(numTaps);
		CPXBuf::clear (taps,numTaps);

		//Unused
		fftFIR = NULL;
		fftSamples = NULL;
	}
	enabled = false;

}

FIRFilter::~FIRFilter(void)
{
	enabled = false;
	if (delayLine != NULL)
		delete delayLine;
	if (taps != NULL)
		free(taps);
	if (fftFIR != NULL)
		delete fftFIR;
	if (fftSamples != NULL)
		delete fftSamples;
	if (out != NULL)
		free(out);

}
void FIRFilter::setEnabled(bool b)
{
	enabled = b;
}
//If DoFFTForward has already been called, use this to remain in frequency domain
void FIRFilter::Convolution(FFT *fft)
{
	//Do Convolution
    CPXBuf::mult(fft->getFreqDomain(),fft->getFreqDomain(),fftFIR->getFreqDomain(), fft->getFFTSize());
}

CPX * FIRFilter::ProcessBlock(CPX *in)
{
	if (!enabled){
		return in;
	}
	if (useFFT) {
		//NOTE: FFT sizes are 2X numSamples to avoid circular convolution
		//Convert in to freq domain
		//DoFFTForward will copy in[numSamples] and pad with zeros to FFT size if necessary
		//12/31/10: Avoid extra buffer copies and let FFT manage buffers
        fftSamples->FFTForward(in, NULL, numSamples);// pass size of in buffer, NOT size of FFT
		//fftSamples->freqDomain is now in freq domain
		//Mask freq domain by our filter
		this->Convolution(fftSamples);
		//freqDomain is now filtered in freq domain
		//And return to time domain
		fftSamples->FFTInverse(NULL, NULL, fftSamples->getFFTSize());
		//tmp1 is time domain
		//Do Overlap-Add to reduce from 2X numSamples to numSamples
		fftSamples->OverlapAdd(out,numSamples);

	} else {
		if (taps==NULL || delayLine == NULL)
			return NULL;

		for (int i=0; i<numSamples; i++) {
			//Add new sample to delayLine
			delayLine->NewSample(in[i]);
			//And calc filtered output
			out[i] = delayLine->MAC(taps,numTaps);
		}
	}
	return out;
}
//From dspguide.com: Basic Windowed-Sinc function
//Fc = Frequency / Sample rate, ie 10000/48000
CPX FIRFilter::Sinc2(float Fc, int i)
{
	//float h;
	CPX h;
	if (i != 0) {
		h.re = h.im = sin(TWOPI * Fc * i) / (ONEPI * i);
	} else {
		h.re = h.im = TWOPI * Fc; //Check this
	}
	return h;

}
//M must be even number, and taps[] = numTaps + 1!  Careful here!
//Forumla from "Digital Signal Processing Technology" Doug Smith, Appendix D-2
//Also dspguide.com, chapt 16 on windowed-sinc filters!! Best explanation yet
CPX FIRFilter::Sinc(float Fc, int i)
{
	//Generates coefficients from -M/2 ... 0 ... +M/2
	int tmp = i - M/2;
	//float h;
	CPX h;
	if ( tmp != 0) {
		//Note: TWOPI * Fc = angular frequency
		//So this is basically stepping through points on generated sinewave at our cutoff freq
		//Basic sinc is sin(a)/a, this is slightly modified
		h.re = (sin(TWOPI * Fc * tmp) / tmp);
		h.im = (sin(TWOPI * Fc * tmp) / tmp);
		//h.im = (cos(TWOPI * Fc * tmp) / tmp); //Why doesn't this work?

	} else {
		//Special case to avoid divide by zero at middle
		h.re = h.im = TWOPI * Fc;
	}

	return h;
}

#if(0)
//Another algorithm that kinda works, here for reference
void FIRFilter::SetBandPass(float lo, float hi)
{
	float fl = lo / sampleRate;
    float fh = hi / sampleRate;
    float fc = (fh - fl) / 2.0;
    float ff = (fl + fh) * ONEPI;

	float bw = fh-fl;
	float a,ys,yg,yw,yf;
	float g = 1;
	for (int i=0; i<numTaps; i++)
	{
		a = (i-M/2) * TWOPI*bw;
		ys = a==0? 1 : sin(a)/a;
		yg = g * (4.0 * bw);
		yw = 0.54 - 0.46 * cos(i * TWOPI/M);
		yf = cos((i-M/2) * TWOPI*fc);
		taps[i] = yf*yw*yg*ys;
	}
}
#endif

/*
Per Paul Lynn, practical FIR filters are between 10-150 taps
Lynn algorithm 5.12 and 5.14, example program 13
Semi working but also doesn't handle neg frequencies
*/
void FIRFilter::SetBandPass2(float lo, float hi, WindowFunction::WINDOWTYPE w)
{
	WindowFunction wf(w,numTaps);
	//Negative frequencies not supported, use FIR-DFT methods
	float tmp;
	if (lo < 0 && hi < 0) {
		// -3000 to -50 becomes 50 to 3000
		tmp = std::abs(hi);
		hi = std::abs(lo);
		lo = tmp;
	} else if (lo < 0 && hi >= 0) {
		// -3000 to + 3000 becomes 0 to 3000
		lo = 0.0;
	}

	float fl = lo / sampleRate; //Normalize
	float fh = hi / sampleRate;
	//float fc = ((hi + lo) / 2.0) / sampleRate; 
	float fc = (fl + fh) / 2.0;
	float fb = (hi - lo) / sampleRate; //Bandwidth
	float fbDeg = fAng(fb/2);
	// 0 = LP
	// 1.57 = HP (90deg * ONEPI / 180
	float fcDeg = fAng(fc); //Center freq in degrees 0=LP 180=HP else BP
	//int mid = M/2; //M is mid point in Lynn
	taps[0].re = taps[0].im = fbDeg / ONEPI;
	float coeff;
	for (int i = 1; i <= M; i++) {
		coeff = (1.0 / (((float)i) * ONEPI)) * 
			sin(((float)i) * fbDeg) * 
			cos(((float)i) * fcDeg); 
		taps[i].re = taps[i].im = coeff * wf.window[i];
	}
}
void FIRFilter::SetBandStop(float lo, float hi, WindowFunction::WINDOWTYPE w)
{
	useFFT = true; //No MAC version yet
	FFTSetBandStop(lo, hi, w);
	MakeFFTTaps();
}
void FIRFilter::SetBandPass(float lo, float hi, WindowFunction::WINDOWTYPE w)
{
	if (useFFT) {
		FFTSetBandPass(lo, hi, w);
		MakeFFTTaps();
		return;
	}
	WindowFunction wf(w,numTaps);

	//Negative frequencies not supported, use FIR-DFT methods
	float tmp;
	if (lo < 0 && hi < 0) {
		// -3000 to -50 becomes 50 to 3000
		tmp = std::abs(hi);
		hi = std::abs(lo);
		lo = tmp;
	} else if (lo < 0 && hi >= 0) {
		// -3000 to + 3000 becomes 0 to 3000
		lo = 50.0;
	}
	lpCutoff = lo;
	hpCutoff = hi;
	FIRFilter *lp = new FIRFilter(sampleRate,numSamples,numTaps);
	//Only apply window once at end
	lp->SetLowPass(lo,WindowFunction::RECTANGULAR);
	FIRFilter *hp = new FIRFilter(sampleRate,numSamples,numTaps);
	hp->SetHighPass(hi,WindowFunction::RECTANGULAR);
	//Add the two filters to get a band reject
	float reSumTaps = 0.0;
	float imSumTaps = 0.0;
	for(int i = 0; i < numTaps; i++) {
		taps[i].re = (lp->taps[i].re + hp->taps[i].re);// * window[i];
		taps[i].im = (lp->taps[i].im + hp->taps[i].im);// * window[i];
		reSumTaps += taps[i].re;
		imSumTaps += taps[i].im;
	}
	SpectralInversion();
	for (int i = 0; i < numTaps; i++){
		taps[i].re = taps[i].re/reSumTaps;
		taps[i].im = taps[i].im/imSumTaps;
	}
	delete lp;
	delete hp;
}


//dspguide using sinc
//Works well, but doesn't differentiate negative freq or cutoff
void FIRFilter::SetLowPass(float cutoff, WindowFunction::WINDOWTYPE w)
{
	//For some TBD reason, taps for MAC FIR are not the same as for FFT FIR
	//We should be able to use the same algorithm for both
	//Excercise for the future
	if (useFFT) {
		FFTSetLowPass(cutoff, w );
		MakeFFTTaps();
		return;
	}

	int length = numTaps;

	WindowFunction wf(w, numTaps);

	cutoff = std::abs(cutoff); //No negative freq in this algorithm

	lpCutoff = 0;
	hpCutoff = cutoff;

	float fc = cutoff / sampleRate;
	//if (fc > 0.5) filter won't work, nyquist etc
	float reSumTaps = 0.0;
	float imSumTaps = 0.0;
	
	for (int i = 0; i < length; i++)
    {
		taps[i] = Sinc(fc,i) * wf.window[i];
		reSumTaps += taps[i].re;
		imSumTaps += taps[i].im;
    }

	//Normalize gain for unity gain at DC
	//This is critical or else Spectral inversion and HP filters will not work
	for (int i = 0; i < length; i++){
		taps[i].re = taps[i].re / reSumTaps;
		taps[i].im = taps[i].im / imSumTaps;
	}
}

//Inverts filter, makes hiPass from loPass, bandPass to bandReject, etc
void FIRFilter::SpectralInversion()
{
	//Spectral inversion
	//dspguide chapt 14
	int mid = M/2;
	for (int i = 0; i < numTaps; i++) {
		taps[i].re = -taps[i].re;
		taps[i].im = -taps[i].im;
	}
	taps[mid].re = 1.0 + taps[mid].re;
	taps[mid].im = 1.0 + taps[mid].im;
}
//Spectral Reversal flips the freq response
//So what was a 3k LP becomes a 21K HP (assuming 48K sample rate, 48/2 = 24 - 3 = 21
//So we have to adjust the passed in cutoff accordingly
void FIRFilter::SpectralReversal()
{
	//Invert sign for every other sample
	for (int i = 0; i < numTaps; i+=2)
		taps[i] = taps[i] * -1;
}
//HighPass taps are same as LowPass, but with neg coefficients
//"Digital Filters", Hamming pg124
void FIRFilter::SetHighPass(float cutoff, WindowFunction::WINDOWTYPE w)
{
	if (useFFT) {
		FFTSetHighPass(cutoff, w);
		MakeFFTTaps();
		return;		
	}

	WindowFunction wf(w,numTaps);

	cutoff = std::abs(cutoff); //Negative freq not supported in this algorithm
	//Create low pass taps
	SetLowPass(cutoff,w);
	lpCutoff = cutoff;
	hpCutoff = 0;
	//Spectral Inversion doesn't work, not sure why, reversal works ok
	SpectralInversion();
	//Now we have hi pass
}
#if (1)
#else
void FIRFilter::SetBandPass(float lo, float hi)
{
	float fl = lo / sampleRate; //Normalize
	float fh = hi / sampleRate;
	float fc = (fh - fl) / 2.0; //Bandwidth +/- (fh-fl)
    float ff = (fl + fh) * ONEPI;
	lpCutoff = lo;
	hpCutoff = hi;
	FIRFilter *lp = new FIRFilter(sampleRate,numSamples,numTaps);
	lp->SetLowPass(fc);
	for (int i=0; i<numTaps; i++)
	{
		float phase = (i-M/2) * ff * -1;
		taps[i] = 2 * taps[i] * cos(phase);
	}
}
#endif

//Allows us to load predefined coefficients for FFT FIR
void FIRFilter::SetLoadable(float * coeff)
{
	useFFT = true;  
    for (int i = 0; i < numSamples; i++)
    {
        taps[i].re = coeff[i];
        taps[i].im = coeff[i];
    }
	MakeFFTTaps();
}

void FIRFilter::MakeFFTTaps()
{
	// size x 4 adjusts for no gain
    float one_over_norm = 1.0 / (numSamples * 4);

	//We now have a filter impulse response in taps[numSamples] that represents our desired filter
	//Convert the time domain filter coefficients (taps[]) to frequency domain (fftTaps[])
	//We then use the frequency domain (fftTaps[]) against the FFT we get from actual samples to apply the filter
	//DoFFTForward will handle numSamples < fftSize and automatically pad with zeros
    fftFIR->FFTForward(taps, NULL, numSamples); //

    // Do compensation here instead of in inverse FFT
    CPXBuf::scale(fftFIR->getFreqDomain(), fftFIR->getFreqDomain(), one_over_norm, fftFIR->getFFTSize());

	//firTaps now has filter in frequency-domain and can be used for FFT convolution

}

void FIRFilter::FFTSetBandPass(float lo, float hi, WindowFunction::WINDOWTYPE wtype)
{
	int size = numSamples; //not numTaps!
	WindowFunction wf(wtype,numSamples);

	float fl = lo / sampleRate;
	float fh = hi / sampleRate;
    float fc = (fh - fl) / 2.0;
    float ff = (fl + fh) * ONEPI;

    int midpoint = size >> 1;

    for (int i = 1; i <= size; i++)
    {
        int j = i - 1;
        int k = i - midpoint;
        float temp = 0.0;
        float phase = k * ff * -1;
        if (i != midpoint)
			temp = ((sin(TWOPI * k * fc) / (ONEPI * k))) * wf.window[j];
        else
            temp = 2.0 * fc;
        temp *= 2.0;
        taps[j].re = temp * (cos(phase));
        taps[j].im = temp * (sin(phase));
    }
}

void FIRFilter::FFTSetLowPass(float cutoff, WindowFunction::WINDOWTYPE wtype)
{
	int length = numSamples; //NOT numTaps
    float fc = cutoff / sampleRate;

    if (fc > 0.5)
    {
        return;
    }

    int midpoint = length >> 1;
	
	WindowFunction wf(wtype,length);
    for (int i = 1; i <= length; i++)
    {
        int j = i - 1;
        if (i != midpoint)
        {
			taps[j].re = (sin(TWOPI * (i - midpoint) * fc) / (ONEPI * (i - midpoint))) * wf.window[j];
            //Setting im to cos(...) doesn't work, so just like normal FIR, we'll set em both to sin for now
			//taps[j].im = (cos(TWOPI * (i - midpoint) * fc) / (ONEPI * (i - midpoint))) * window[j];
			taps[j].im = (sin(TWOPI * (i - midpoint) * fc) / (ONEPI * (i - midpoint))) * wf.window[j];
        }
        else
        {
            taps[midpoint - 1].re = 2.0 * fc;
            taps[midpoint - 1].im = 2.0 * fc;
        }
    }
}
//Broken, Spectral Inversion doesn't work with FFT, need different algorithm
void FIRFilter::FFTSetHighPass(float cutoff, WindowFunction::WINDOWTYPE wtype)
{
	FFTSetLowPass(cutoff,wtype);
	SpectralInversion();
}

//NOT TESTED
void FIRFilter::FFTSetBandStop(float lo, float hi, WindowFunction::WINDOWTYPE wtype)
{
	int length = numSamples;

    float fl = lo / sampleRate;
    float fh = hi / sampleRate;
    float fc = (fh - fl) / 2.0;
    float ff = (fl + fh) * ONEPI;

    int midpoint = (length >> 1) | 1;

	WindowFunction wf(wtype,length);
    for (int i = 1; i <= length; i++)
    {
        int j = i - 1;
        int k = i - midpoint;
        float temp = 0.0;
        float phase = k * ff * -1.0;
        if (i != midpoint)
        {
			temp = ((sin(TWOPI * k * fc) / (ONEPI * k))) * wf.window[j];
            taps[j].re = -2.0 * temp * (cos(phase));
            taps[j].im = -2.0 * temp * (sin(phase));
        }
        else
        {
            temp = 4.0 * fc;
            taps[midpoint - 1].re = 1.0 - taps[midpoint - 1].re;
            taps[midpoint - 1].im = 0.0 - taps[midpoint - 1].im;
        }

    }
}

#if(0)
//Inverse SINC coefficients? From SDRMax, not sure how to use
const float FILTCOEFF[512] = {
  -7.298647688e-005,-0.0006591212004,-0.002619047416,-0.006309854332,  -0.0100497622,
   -0.01030933112, -0.00512559386, 0.002354965545, 0.005713210441, 0.002275705803,
  -0.003084241645,-0.003601335222,0.0007034620503, 0.003363352269,0.0008431510651,
  -0.002575940453,-0.001681111869, 0.001701403758, 0.002036997117,-0.0009210249991,
  -0.002096840879,0.0002871879842, 0.001987287775,0.0001987546857,-0.001792274415,
  -0.0005589024513, 0.001559824217,0.0008162951563,-0.001320442185,-0.000995823415,
   0.001087572658, 0.001113920705,-0.0008722782368,-0.001188867842,0.0006758925156,
   0.001231985865,-0.000498666428,-0.001251817564,0.0003399479028, 0.001254849718,
  -0.0001991765457,-0.001247569569,7.392977568e-005, 0.001235375414,4.123276085e-005,
  -0.001217678422,-0.0001465942914, 0.001196535421, 0.000243384231, -0.00117429113,
  -0.0003348062455, 0.001149932388,0.0004205152218, -0.00112563686,-0.0005032685003,
   0.001100820256,0.0005843482213,-0.001074886066,-0.0006646135589, 0.001047056401,
  0.0007442943752, -0.00101693254,-0.0008235950954,0.0009842554573,0.0009030248038,
  -0.0009477832355,-0.0009810557822,0.0009091657703, 0.001059165923,-0.0008679664461,
  -0.001138553023,  0.00082209229, 0.001217685989,-0.0007719836431,-0.001296614995,
  0.0007174044149, 0.001375256572,-0.0006578231696,-0.001452584052, 0.000594522804,
   0.001530058333,-0.0005264937645,-0.001608328195,0.0004511743609, 0.001684416085,
  -0.0003703148686,-0.001758773113,0.0002839826047, 0.001831884147,-0.0001910350693,
  -0.001902386313,9.27066576e-005, 0.001972250175,1.451703884e-005,-0.002037639031,
  -0.0001286926563, 0.002098251833,0.0002489787876,-0.002154306741,-0.0003754671779,
   0.002205130411,0.0005068014725,-0.002253053477,-0.0006465745391, 0.002294297796,
  0.0007927893312, -0.00232853787,-0.000944050902, 0.002357372548, 0.001102598035,
  -0.002377682133,-0.001264899038, 0.002393632894, 0.001436556573,-0.002398973331,
  -0.001613004017, 0.002395873656, 0.001796123339,-0.002380831633,-0.001981404843,
   0.002357972786, 0.002172444016, -0.00232419977,-0.002367483685, 0.002279908862,
   0.002567417221,-0.002222327981,-0.002768594073,  0.00215465948, 0.002974577248,
  -0.002071162919,-0.003176601371, 0.001984296367, 0.003392344108,-0.001870410633,
  -0.003593309782, 0.001762269414, 0.003818378551,-0.001613235101,-0.004014817532,
   0.001478523714, 0.004237284884,-0.001307581435,-0.004444869235, 0.001126970164,
   0.004649222363,-0.0009381354903,-0.004864084534,0.0007162823458, 0.005058682058,
  -0.0004906012327,-0.005254743621,0.0002504401491, 0.005455542356,2.14788397e-005,
  -0.005635460373,-0.0003002338926, 0.005811699666,0.0005916802911,-0.005991247483,
  -0.0009132201085, 0.006154606584, 0.001253017108,-0.006301850546,-0.001601491706,
   0.006448314991, 0.001975012943,-0.006583563052,-0.002374651376,  0.00669672247,
   0.002787304111,-0.006796817295,-0.003215527395, 0.006888513919, 0.003669659607,
  -0.006961984094,-0.004147280473, 0.007011558395, 0.004639521241,-0.007045097649,
  -0.005150847603, 0.007064725272, 0.005690811202,-0.007059961092,-0.006256772671,
   0.007024813443, 0.006840964779, -0.00696356874,-0.007443124428, 0.006882129703,
   0.008074145764, -0.00677188905, -0.00873577781, 0.006622628309, 0.009418459609,
  -0.006438742857, -0.01012247056, 0.006224870216,  0.01085730083,-0.005974722095,
   -0.01162762847, 0.005678492598,  0.01243014261,-0.005333583802, -0.01326503698,
   0.004937817343,  0.01413488016,-0.004487683997, -0.01504271384, 0.003980736714,
    0.01599739864,-0.003406297183,  -0.0170029439, 0.002752586966,  0.01805772632,
  -0.002016746672, -0.01917018928,  0.00119104574,  0.02035386302,-0.0002581478329,
   -0.02161752619,-0.0007987120189,  0.02297377214,  0.00200156169, -0.02443705499,
  -0.003379397793,  0.02601802349, 0.004954055417, -0.02774950489,  -0.0067701675,
    0.02966706641,  0.00888942834, -0.03181019425, -0.01138752978,  0.03424045816,
    0.01438213047, -0.03702627495, -0.01802601293,  0.04027441144,  0.02255047485,
   -0.04414317012, -0.02832753956,  0.04885149002,  0.03595076501,  -0.0547539629,
   -0.04649358243,  0.06237275898,  0.06197695807, -0.07256446034, -0.08679046482,
     0.0865688622,   0.1321066916,  -0.1050333083,  -0.2351729274,   0.1105839834,
     0.6013018489,   0.6013018489,   0.1105839834,  -0.2351729274,  -0.1050333083,
     0.1321066916,   0.0865688622, -0.08679046482, -0.07256446034,  0.06197695807,
    0.06237275898, -0.04649358243,  -0.0547539629,  0.03595076501,  0.04885149002,
   -0.02832753956, -0.04414317012,  0.02255047485,  0.04027441144, -0.01802601293,
   -0.03702627495,  0.01438213047,  0.03424045816, -0.01138752978, -0.03181019425,
    0.00888942834,  0.02966706641,  -0.0067701675, -0.02774950489, 0.004954055417,
    0.02601802349,-0.003379397793, -0.02443705499,  0.00200156169,  0.02297377214,
  -0.0007987120189, -0.02161752619,-0.0002581478329,  0.02035386302,  0.00119104574,
   -0.01917018928,-0.002016746672,  0.01805772632, 0.002752586966,  -0.0170029439,
  -0.003406297183,  0.01599739864, 0.003980736714, -0.01504271384,-0.004487683997,
    0.01413488016, 0.004937817343, -0.01326503698,-0.005333583802,  0.01243014261,
   0.005678492598, -0.01162762847,-0.005974722095,  0.01085730083, 0.006224870216,
   -0.01012247056,-0.006438742857, 0.009418459609, 0.006622628309, -0.00873577781,
   -0.00677188905, 0.008074145764, 0.006882129703,-0.007443124428, -0.00696356874,
   0.006840964779, 0.007024813443,-0.006256772671,-0.007059961092, 0.005690811202,
   0.007064725272,-0.005150847603,-0.007045097649, 0.004639521241, 0.007011558395,
  -0.004147280473,-0.006961984094, 0.003669659607, 0.006888513919,-0.003215527395,
  -0.006796817295, 0.002787304111,  0.00669672247,-0.002374651376,-0.006583563052,
   0.001975012943, 0.006448314991,-0.001601491706,-0.006301850546, 0.001253017108,
   0.006154606584,-0.0009132201085,-0.005991247483,0.0005916802911, 0.005811699666,
  -0.0003002338926,-0.005635460373,2.14788397e-005, 0.005455542356,0.0002504401491,
  -0.005254743621,-0.0004906012327, 0.005058682058,0.0007162823458,-0.004864084534,
  -0.0009381354903, 0.004649222363, 0.001126970164,-0.004444869235,-0.001307581435,
   0.004237284884, 0.001478523714,-0.004014817532,-0.001613235101, 0.003818378551,
   0.001762269414,-0.003593309782,-0.001870410633, 0.003392344108, 0.001984296367,
  -0.003176601371,-0.002071162919, 0.002974577248,  0.00215465948,-0.002768594073,
  -0.002222327981, 0.002567417221, 0.002279908862,-0.002367483685, -0.00232419977,
   0.002172444016, 0.002357972786,-0.001981404843,-0.002380831633, 0.001796123339,
   0.002395873656,-0.001613004017,-0.002398973331, 0.001436556573, 0.002393632894,
  -0.001264899038,-0.002377682133, 0.001102598035, 0.002357372548,-0.000944050902,
   -0.00232853787,0.0007927893312, 0.002294297796,-0.0006465745391,-0.002253053477,
  0.0005068014725, 0.002205130411,-0.0003754671779,-0.002154306741,0.0002489787876,
   0.002098251833,-0.0001286926563,-0.002037639031,1.451703884e-005, 0.001972250175,
  9.27066576e-005,-0.001902386313,-0.0001910350693, 0.001831884147,0.0002839826047,
  -0.001758773113,-0.0003703148686, 0.001684416085,0.0004511743609,-0.001608328195,
  -0.0005264937645, 0.001530058333, 0.000594522804,-0.001452584052,-0.0006578231696,
   0.001375256572,0.0007174044149,-0.001296614995,-0.0007719836431, 0.001217685989,
    0.00082209229,-0.001138553023,-0.0008679664461, 0.001059165923,0.0009091657703,
  -0.0009810557822,-0.0009477832355,0.0009030248038,0.0009842554573,-0.0008235950954,
   -0.00101693254,0.0007442943752, 0.001047056401,-0.0006646135589,-0.001074886066,
  0.0005843482213, 0.001100820256,-0.0005032685003, -0.00112563686,0.0004205152218,
   0.001149932388,-0.0003348062455, -0.00117429113, 0.000243384231, 0.001196535421,
  -0.0001465942914,-0.001217678422,4.123276085e-005, 0.001235375414,7.392977568e-005,
  -0.001247569569,-0.0001991765457, 0.001254849718,0.0003399479028,-0.001251817564,
  -0.000498666428, 0.001231985865,0.0006758925156,-0.001188867842,-0.0008722782368,
   0.001113920705, 0.001087572658,-0.000995823415,-0.001320442185,0.0008162951563,
   0.001559824217,-0.0005589024513,-0.001792274415,0.0001987546857, 0.001987287775,
  0.0002871879842,-0.002096840879,-0.0009210249991, 0.002036997117, 0.001701403758,
  -0.001681111869,-0.002575940453,0.0008431510651, 0.003363352269,0.0007034620503,
  -0.003601335222,-0.003084241645, 0.002275705803, 0.005713210441, 0.002354965545,
   -0.00512559386, -0.01030933112,  -0.0100497622,-0.006309854332,-0.002619047416,
  -0.0006591212004,-7.298647688e-005
};
#endif
