#ifndef DECIMATOR_H
#define DECIMATOR_H

#include "processstep.h"

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

	Low pass filters are designed using MatLab Filter Design and Analysis Tool with the following parameters

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



	How is the wPass determined for each set of coefficients?
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

	The lowest sample rate a filter can handle for a given bandwidth = Bandwidth / wPass
	Examples at 10k, 20k, 30k bw

	Order  Taps   wPass	10k		20k		30k
	----------------------------------------------------------------
	*6      7    .0300	333k	666k	1.000m
	10     11    .0500	200k	400k	600k
	14     15    .0980	102k	204k	306k
	18     19    .1434	69k		139k	209k
	22     23    .1820	55k		110k	165k
	26     27    .2160	49k		93k		139k
	30     31    .2440	41k		82k		123k
	34     35    .2680	37k		75k		112k
	38     39    .2880	35k		69k		104k
	42     43    .3060	33k		65k		98k
	46     47    .3200	31k		63k		94k
	50     51    .3332	30k		60k		90k
	* - Implemented as CIC filter in cuteSDR



*/
class Decimator : public ProcessStep
{
public:
	Decimator(quint32 _sampleRate, quint32 _bufferSize);

	CPX *process(CPX *in, quint32 _numSamples);

};

#endif // DECIMATOR_H
