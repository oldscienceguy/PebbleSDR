#ifndef DECIMATOR_H
#define DECIMATOR_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
#include <Accelerate/Accelerate.h>

/*
	This class implements a decimation (anti-alias filter + downsampling) step.
	It is derived from cuteSDR downconvert

	References:
	http://www.mathworks.com/help/dsp/examples/fir-halfband-filter-design.html
	http://www.mathworks.com/help/dsp/examples/design-of-decimators-interpolators.html
	https://ccrma.stanford.edu/~jos/resample/ (resampling)
	http://www.mega-nerd.com/SRC/index.html (libsamplerate)
	http://www.dsprelated.com/freebooks/sasp/
	http://www.dsprelated.com/showarticle/903.php (Lyons, 1/4/16 important)
	http://www.dsprelated.com/showarticle/761.php (Lyons)
	Understanding DSP 3rd edition (Lyons) 10.12 - Sample rate conversion with halfband filters
	http://hamiltonkibbe.com/ 3 articles on using vDsp (Accelerate) for FIR filters
	....TBD....

	The aliasing theorem states that downsampling in time corresponds to aliasing in the frequency domain.
	Assume 2.048e6 complex (I/Q) sps in 0 centered spectrum incoming rate, downsampled by factor of 2
	Fs = 2048000 sps
	Bw = 10000hz (*2 for both sides)
	Signal = area of interest where we can't tolerate aliasing

	Original signal @2048000sps                    Signal
												   |Bw|Bw|
	| ................... samples .................|. 0 .|................. samples .................. |
	-1024                                             0                                            +1024

	Decimation x 2 with no filter                  Signal
												   |Bw|Bw|
	| ................... samples .................|. 0 .|................. samples .................. |
	|Bw|                     |                        |                        |                    |Bw|
	| .... Alias area .... -512k                      |                      +512k .... Alias area ... |

	|Bw| ----------> Aliasing -------------------> |Bw|Bw|  <--------------- Aliasing <-------------|Bw|
	Post decimation, any signals in Bw @ -1024 and +1024 alias into our zero centered signal and corrupt

	Decimation x 2 with halfband lowpass filter     Signal
												   |Bw|Bw|
	| ................... samples .................|. 0 .|................. samples .................. |
	|Bw|                     |                        |                        |                    |Bw|
						   -512k                                              +512k

	Halfband lowpass filter: nodes = 10, wPass = 0.05
	|Sw| ---------- Transition Width ------------> |Bw|Bw|  ----------- Transition width -----------|Sw|
	51.2k                921.6k                 51.2k | 51.2k               921.6k                 51.2k
	Crude magnitude plot (see Matlab code for accurate magnitude plot)
												-------------
										/                          \
							 /                                                \
				   /									                                \
	_____ /                                                                                     \______
	Aliases in the stop bands (-1024 and +1024) are filtered before decimation and will not corrupt
	signal of interest at zero center


	Pre decimation, we have freq components of +/- 1024k in the buffer
	Post decimation, any freq component outside of +/- 512k new range violates Nyquist and may be aliased.
	To avoid this, we filter the 2048sps buffer with a Halfband Lowpass filter before downsampling.

	If we decimate in the device plugin or before the spectrum is generated, we need to make sure that no
	signals above the nyquist limit are left in the signal before downsampling.  This requires a filter with
	a passband of 512k (either side), a narrow transition band, and a stop band that removes the entire
	upper and lower aliasing zone.

	However, when we are decimating post spectrum, in preparation for demodulation and other processing, all
	we need to protect is the bandwidth of the largest signal we want to work with.  For AM modes, this is
	typically 20k (10k per halfband).  For FM, its typically 200k (100k per halfband).  For CPU effective
	decimation, we need to find the smallest filter, in terms of taps, that protects this passBand.

	Halfband Lowpass filters are designed using MatLab Filter Design and Analysis Tool with the following parameters
	The transition band in halfband filters is centered on Fs/4 (real samples) or Fs/2 (complex samples), so the
	discarded portion of the band (post decimation) is effectively discarded.  Every other coefficient is also zero,
	reducing the calculation complexity.

	For our purposes, we care mostly about the stop band, where aliasing can put false signals in our primary
	area of interest.

	Response type: Halfband Lowpass
	Design method: FIR - equiripple
	Filter order: 10 (results in 11 taps)
	Density factor: 20 (doesn't seem to matter)
	Units: Normalized
	wPass: 0.050
	Magnitude specifications: Fixed at 6db by program

	This generates a set of coefficients that match cuteSDR HB11TAP_H[11]
	Note the spectrum plot in MatLab matches the lower half of Ideal filter diagram above
	When implemented, the upper half of the diagram is filtered around the stop band as well

	Same results can be generated with these MatLab scripts
		% Right click in fvtool and select Analysis Parameters to change display to -1 0 +1
		% Equiripple Halfband lowpass filter designed using the FIRHALFBAND
		% function.
		% All frequency values are normalized to 1.
		N     = 10;    % Order
		Fpass = 0.05;  % Passband Frequency
		% Calculate the coefficients using the FIRPM function.
		b  = firhalfband(N, Fpass);
		Hd = dfilt.dffir(b);
		fcfwrite(Hd,'foo'); %Saves coefficients to disk
		measure (Hd)
		hfvt = fvtool(Hd,'DesignMask','on','Color','White');
	or
		% Fs(48000) * Fpass (0.05) = 2400, 24000 - 2400 = 21600 tw (transition width)
		% Tw = (Fs / 2) - (Fs * Fpass)
		HfdDecim = fdesign.decimator(M,'halfband', 'n,tw',10,21600,48000)
		HDecim = design(HfdDecim,'equiripple', 'SystemObject', true);
		measure(HDecim)
		hfvt = fvtool(HDecim,'DesignMask','on','Color','White');

	Note that there are 3 ways of displaying filter magnitude/response graphs and they can be confusing
	1. Zero centered, -Fs 0 +Fs.  The filter passband will be centered.  This is a Matlab optional display
	2. Zero to Fs/2 (or Fs/4 for real signals).  The filter passband will be shown on the left (zero) side
		of the plot.  This is the default Matlab display.
	3. Zero to Fs.  The filter passband will be shown at the left (zero) side AND at the right (Fs) side.
		The result can be confusing because it looks like the Zero Centered plot, but with the stop band
		in the center.

	How was wPass determined for each filter order?
	Lower tap filters are designed to be used with higher sample rates, ie more samples to process.
	Fewer taps = less CPU
	As we decimate by 2, each filter gets more taps (fewer samples to process) and
	wider stop band (percentage of total)

	Filters used in cuteSDR downconvert, 4 digit precision
	Discovered by tweaking MatLab wPass until we get exact match to cuteSDR filters
	Note that cuteSDR uses 1/2 wPass for specification
	wPass is the Passband edge frequency when filter is normalized (0-1). Alias free bandwidth
	fPass is the Passband edge frequency when filter is specified in hz
	fPass = SR * (wPass / 2) = passband at given sample rate
	Normalized with wPass=0.05 sames as Fs=48000 and fPass=1200

	Higher sample rates need lower alais free bandwidth percentage (minBw / SR)
	ie SR = 2.048e6 minBw = 20k, 20k / 2.048e6 = 9.76e-03, .00976
	Lower sample rates need higher alias free bandwidth percentage
	ie SR = 1.024e6 minBw = 20k, 20k / 1.024e6 = .01953

	The lowest inbound sample rate a filter can handle for a given bandwidth = Bandwidth / wPass
	Lowest outbound sample rate is lowest inbound rate / 2
	Smallest decimated rate, as defined by cuteSDR, is 7900.  So smallest inbound rate is 2x or 15800
	This means smallest bw we can handle is 15800 * .3332 (51tap) = 5264 (5.3k)

	To build linear phase half-band FIR filters, Taps + 1 must be an integer multiple of four.

	Examples at 5.3k(lowest bw), 20k(am), 30k(fmn) bw

	Order  Taps   wPass	5.3k	20k		30k
	----------------------------------------------------------------
	6*      7    .0030	1.766m	6.666m	10.000m
	10     11    .0500	106k	400k	600k
	14     15    .0980	54.1k	204k	306k
	18     19    .1434	37.0k	139k	209k
	22     23    .1820	29.1k	110k	165k
	26     27    .2160	24.5k	93k		139k
	30     31    .2440	21.7k	82k		123k
	34     35    .2680	19.7k	75k		112k
	38     39    .2880	18.4k	69k		104k
	42     43    .3060	17.3k	65k		98k
	46     47    .3200	16.5k	63k		94k
	50     51    .3332	15.9k	60k		90k
	58**   55    .4000          50k          (100db)
	* - Implemented as CIC filter in cuteSDR
	** - Added to Pebble to support lower sample rates, still above minimum sample rate

*/

class HalfbandFilterDesign {
public:
	HalfbandFilterDesign(quint16 _numTaps, double _wPass, double * _coeff);
	quint32 numTaps;
	double wPass;
	double *coeff;
};

class HalfbandFilter {
public:
	HalfbandFilter(HalfbandFilterDesign *_design);
	HalfbandFilter(quint16 _numTaps, double _wPass, double * _coeff);
	~HalfbandFilter();
	quint32 process(CPX *_in, CPX *_out, quint32 _numInSamples);

	quint32 processCIC3(const CPX *_in, CPX *_out, quint32 _numInSamples);
	quint32 processCIC3(const DSPDoubleSplitComplex *_in, DSPDoubleSplitComplex *_out, quint32 _numInSamples);
	quint32 processVDsp(const DSPDoubleSplitComplex *_in, DSPDoubleSplitComplex *_out, quint32 _numInSamples);

	const quint32 maxResultLen = 32768;

	quint32 numTaps;

	double wPass; //For reference
	quint32 sampleRateIn;

	//CIC3 implemenation for early stages
	bool useCIC3;
	CPX xOdd;
	CPX xEven;
	//double has 15 decimal digit precision, so we truncate Matlab results
	const double *coeff;
	quint32 convolve(const double *x, quint32 xLen, const double *h, quint32 hLen, double *y);
	//Last Sample version
	quint32 convolveOS(const CPX *x, quint32 xLen, const double *h, quint32 hLen, CPX *y,
		quint32 ySize, quint32 decimate = 1);
	//Overlap/Add version
	quint32 convolveOA(const CPX *x, quint32 xLen, const double *h, quint32 hLen, CPX *y,
		quint32 ySize,	quint32 decimate = 1);
	quint32 convolveVDsp1(const DSPDoubleSplitComplex *x, quint32 xLen, const double *h, quint32 hLen, DSPDoubleSplitComplex *y,
		quint32 ySize,	quint32 decimate = 1);
	quint32 convolveVDsp2(const DSPDoubleSplitComplex *x, quint32 xLen, const double *h, quint32 hLen, DSPDoubleSplitComplex *y,
		quint32 ySize,	quint32 decimate = 1);
	//Testing decimate by more than 2
	quint32 decimate;
private:
	DSPDoubleSplitComplex lastXVDsp; //Working buffer if we need it

	//For generic convolve
	CPX *lastX;
	CPX *tmpX;
};

class Decimator
{
public:
	Decimator(quint32 _sampleRate, quint32 _bufferSize);
	~Decimator();

	//Builds a decimation chain to get from incoming sample rate to lowest sample rate that protects
	//_protectBw
	//_protectBw is the full bandwith (wPass) ie not 1/2 bw as in cuteSDR.  Compared to wPass
	float buildDecimationChain(quint32 _sampleRateIn, quint32 _protectBw, quint32 _sampleRateOut = 0);

	quint32 process(CPX *_in, CPX* _out, quint32 _numSamples);

private:
	quint32 sampleRate;
	quint32 bufferSize;

	const quint32 minDecimatedSampleRate = 15000; //Review
	bool useVdsp;
	bool combineStages;

	float decimatedSampleRate;
	double protectBandWidth; //Protected bandwidth of signal of interest

	void initFilters();
	void deleteFilters();

	//Halfband filters (order n) designed with Matlab using parameters in comments

	HalfbandFilterDesign *cic3; //Faster than hb7 for early stages of decimation
	HalfbandFilterDesign *hb7;
	HalfbandFilterDesign *hb11;
	HalfbandFilterDesign *hb15;
	HalfbandFilterDesign *hb19;
	HalfbandFilterDesign *hb23;
	HalfbandFilterDesign *hb27;
	HalfbandFilterDesign *hb31;
	HalfbandFilterDesign *hb35;
	HalfbandFilterDesign *hb39;
	HalfbandFilterDesign *hb43;
	HalfbandFilterDesign *hb47;
	HalfbandFilterDesign *hb51;
	HalfbandFilterDesign *hb59; //Testing

	QVector<HalfbandFilter*> decimationChain;

	CPX* workingBuf1;
	CPX* workingBuf2;

	//Accelerate fw doesn't work with interleaved CPX (re,im,re,im,...).
	//Instead is uses Split Complex which is an array of reals and an array of imag
	DSPDoubleSplitComplex splitComplexIn;
	DSPDoubleSplitComplex splitComplexOut;

	QMutex mutex;
};

#endif // DECIMATOR_H
