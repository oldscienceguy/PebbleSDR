//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "db.h"
#include <QDebug>
#include <float.h> //Floating point limits

const double DB::fullScale = 1.0;
const double DB::minDb = -120.0;
const double DB::maxDb = 0.0;

#if 0
	Naming confusion.  There are different usages of dB that get used interchangeably
	http://www.ap.com/kb/show/170
	See http://www.microvolt.com/table.html
	Lyons Appendix E p885
	dB is a relative power relationship between two signals (S1w / S2w)
	dbFS is a relative power relationship between S1 and full scale (ie -1 or +1)
	dBm is the absolute power level in milliwatts (W watts relative to 1mw)
	dBuV is the absolute power level in microvolts (V volts relative to 1uv)

	When we use dB in Pebble, we mean dbFS unless otherwise indicated

	u = micro = 10E-6
	m = milli = 10E-3
	w = watts = S1v^2 / Impedance(ohms)
	v = volts
	o = ohms

	Complex amplitude = sqrt(I^2 + Q^2)
	Complex power = I^2 + Q^2

	dB = 10 * log10 (S1w / S2w)

	dBFS = 10 * log10(CPXpower / 1) //compared to full scale of 1
	dBFS = 20 * log10(CPXamplitude / 1)

	//milliwats == power
	dBm = 10 * log10 (S1w / .001w)
	dBm = 10 * log10 (S1mw)

	//microvolts == amplitude
	dBuv = 20 * log10 (S1v / .000001v)
	dBuv = 20 * log10 (S1uv)

	For a 50uv signal @ 50ohm impedance (the reference for S9)
	vrms 0.000050v at 50ohm = .00005^2 / 50 = 5e-11 watts
	20 * log10(.000050  / .000001) = 33.979 dBuv
	10 * log10(.000050^2  / 50o) = -103.01 dBw
	10 * log10(5e-11w / .001w) = -73.01 dBm (aka S9)
	//Note the difference betwren dBW and dbm is always 30 for a 50ohm impedance

	So how does this relate to use in an SDR program?
	The best explanation I found came from Kevin Reid
	http://dsp.stackexchange.com/questions/19615/converting-raw-i-q-to-db

	Basics
	The amplitude of an IQ signal is just the vector magnitude, sqrt(I^2 + Q^2)

	The power of an IQ signal is the squared magnitude, I^2 + Q^2

	When you see a logarithmic (dB) meter, it is usually measuring the log of the power,
	i.e. 10 X log10(I^2+Q^2). (This can also be calculated as 20 X log10(amplitude),
	but unless you already have the amplitude that wastes a square root operation.)

	Units
	Remember, dB is a relative figure. If you just take 10 X log10(I^2+Q^2), then 0 dB corresponds
	to an amplitude of exactly 1. If your hardware driver takes the usual floating-point convention
	of the absolute extreme sample values being from −1 to +1, then you can say that your dB power
	values are dBFS — decibels relative to full scale. Any signals stronger than that level will be
	clipped, distorting the signal.

	dBm, decibels relative to one milliwatt of power, just uses a different reference level.
	You can convert to dBm just by adding or subtracting the proper calibration value from the dBFS value
	but you need to know that calibration for your hardware at the frequency of interest,
	such as by measuring it (using a signal source of known power output); it is impossible to perform
	that calibration purely in software since the digital samples are just numbers with no inherent units.

	(A mistake I've seen is to refer to the sample values, or parameters which scale according to them
	such as an amplitude threshold, as being in “volts”; this is complete nonsense unless your
	ADC (and other hardware) is actually calibrated to one volt. This is unreasonably large for a
	radio receiver.)

	Practical application
	If you are just looking to ignore signals that aren't sufficiently strong (this is known as carrier
	squelch or power squelch), it doesn't matter what units you use, or even if they're logarithmic or
	linear, because you're just doing a greater-than comparison. The only critical component is that you
	start with I^2+Q^2 (as opposed to, say, I+Q, which would be just plain wrong).

	(Accelerate FFT handles this adjustment internally)
	It may also be relevant to note that if you filter a signal, you are by definition removing some of
	the signal power, so the measurement will be smaller.

	In particular, a FFT (such as is the primary visual display in tools like SDR#) can loosely be
	thought of as a large collection of extremely sharp filters; each output “bin” collects some
	fraction of the input power. Accordingly, the power in each bin depends on the width of the bin.
	If you divide by the width of the bin in hertz (that value being sample rate/FFT length) before
	taking the logarithm, then instead of dB power, you measure dB power spectral density, which has
	the advantage of being independent of the FFT bin width if the features you care about are wider
	than one bin (e.g. a wideband modulated signal); if they are narrower (e.g. pure tones) then the
	power value is more useful.

	3/21/16: To show how inconsistent SDR programs are.  Using Red Pitaya signal generator at 10mhz with Amplitude (Vpp).
	|          SDR IQ, RF Gain 0dB, IF Gain 18dB, SR 196k (Elad SW using Elad FDM-S2)
	|          SpectraVue      SDR#            SDR-Radio        Elad
	|          Units: dB       Units: dbFS     Units: dBm       Units: dBm
	Amplitude  Peak RMS Floor  Peak SNR Floor  dBm  S  Floor     dBm    S
	.00025     -67  -73 -110   -27  58  -85    -56              -66.5  S9
	.00050     -62  -67        -22  63  -85    -52              -61.5  +10
	.00500     -41  -47        -1   83  -85    -34              -41.0  +30
	9.950mhz
	Noise

	|SDR-Play LNA off, RFGain -50db, VisualGain -10db?
	Amplitude  Peak RMS Floor  Peak SNR Floor  dBm  S  Floor     dBm    S
	.00025                                     -59     -112
	.00050
	.00500
	9.950mhz                                   -55
	Noise                                      -102

	Note that SpectraVue Peak dB and Elad dBm are approx the same.  SV has a default -5.4db FFT correction which is
	  included in the above results.  Without this correction, the peak is -63dB at .00025 vpp
	SDR# uses db Full Scale and is a consistent 40db different
	SDR-Radio is consistently 10db different

#endif

//Returns the root mean square (average power) in buffer
double DB::rms(CPX *in, quint32 numSamples)
{
	double totalSquared = 0;
	double rms = 0;
	for (quint32 i=0; i<numSamples; i++) {
		totalSquared += in[i].re*in[i].re + in[i].im*in[i].im;
	}
	rms = sqrt(totalSquared / numSamples);
	return rms;
	return DB::amplitudeTodB(rms); //20 * log10(rms);
#if 0
	//Same results, with power, but slightly faster since no sqrt
	double totalPower = 0;
	for (quint32 i=0; i<numSamples; i++) {
		totalPower += DB::power(in[i]);
	}
	return DB::powerTodB(totalPower/numSamples); //10*log10(power)
#endif
}

void DB::test()
{
	//Test
	CPX cx(0.3,0.4);
	double pwr = DB::power(cx);
	double db = DB::powerTodB(pwr);
	Q_ASSERT(DB::dBToPower(db) == pwr);
}

//Prints useful stats about a CPX buffer
//Special handling if the CPX buffer is an FFT result
void DB::analyzeCPX(CPX* in, quint32 numSamples, const char* label, bool isFftOutput, double windowGain, double maxBinPower)
{
	double maxRe = -DBL_MAX;
	quint32 maxReIndex = 0;

	double minRe = DBL_MAX;
	quint32 minReIndex = 0;

	double maxIm = -DBL_MAX;
	quint32 maxImIndex = 0;

	double minIm = DBL_MAX;
	quint32 minImIndex = 0;

	double cxPower = 0;

	double maxPower = 0;
	quint32 maxPowerIndex = 0;

	double minPower = DBL_MAX;
	quint32 minPowerIndex = 0;

	double totalPower = 0;
	double meanPower = 0;
	double medianPower; //Future use for noise floor

	double cxAmplitude = 0;
	double maxAmplitude = 0;
	double minAmplitude = DBL_MAX;
	double totalAmplitude = 0;
	double meanAmplitude = 0;

	Q_UNUSED(medianPower);

	for( quint32 i = 0; i < numSamples; i++){
		if (in[i].re > maxRe) {
			maxRe = in[i].re;
			maxReIndex = i;
		}
		if (in[i].re < minRe) {
			minRe = in[i].re;
			minReIndex = i;
		}
		if (in[i].im > maxIm) {
			maxIm = in[i].im;
			maxImIndex = i;
		}
		if (in[i].im < minIm) {
			minIm = in[i].im;
			minImIndex = i;
		}


		if (isFftOutput) {
			//Accelerate fft returns amplitude in re/im
			//Compensate for window function loss etc if used to analyze fft results
			cxAmplitude = DB::amplitude(in[i]) / windowGain;
			//Normalize to 0-1
			cxAmplitude /= maxBinPower;
			cxPower = DB::amplitudeToPower(cxAmplitude);
		} else {
			cxPower = DB::power(in[i]);
			cxAmplitude = DB::amplitude(in[i]);
		}

		if (cxPower > maxPower) {
			maxPower = cxPower;
			maxPowerIndex = i;
		}
		if (cxPower < minPower) {
			minPower = cxPower;
			minPowerIndex = i;
		}
		totalPower += cxPower;

		if (cxAmplitude > maxAmplitude) {
			maxAmplitude = cxAmplitude;
		}
		if (cxAmplitude < minAmplitude) {
			minAmplitude = cxAmplitude;
		}
		totalAmplitude += cxAmplitude;
	}

	meanPower = totalPower / numSamples;
	meanAmplitude = totalAmplitude / numSamples;
	if (isFftOutput) {
		qDebug()<<label<<" numSamples="<<numSamples<<" windowGain="<<windowGain<<" maxBinPower="<<maxBinPower;
	} else {
		qDebug()<<label<<" numSamples="<<numSamples;
	}
	qDebug()<<" Re: max="<<maxRe<<" @"<<maxReIndex<<" min="<<minRe<<" @"<<minReIndex;
	qDebug()<<" Im: max="<<maxIm<<" @"<<maxImIndex<<" min="<<minIm<<" @"<<minImIndex;
	qDebug()<<" Power: max="<<maxPower<<" @"<<maxPowerIndex<<" min="<<minPower<<" @"<<minPowerIndex
		   <<" total="<<totalPower<<" mean="<<meanPower;
	qDebug()<<" Power db: max="<<DB::powerTodB(maxPower)<<"db min="<<DB::powerTodB(minPower)<<"db total="
		<<DB::powerTodB(totalPower)<<"db mean="<<DB::powerTodB(meanPower)<<"db";
	qDebug()<<" Amplitude: max="<<maxAmplitude<<" min="<<minAmplitude<<" total="
		   <<totalAmplitude<<" mean="<<meanAmplitude;
	qDebug()<<" Amplitude db: max="<<DB::powerTodB(maxAmplitude)<<"db min="<<DB::powerTodB(minAmplitude)<<"db total="
		<<DB::powerTodB(totalAmplitude)<<"db mean="<<DB::powerTodB(meanAmplitude)<<"db";
	qDebug()<<"-";

}


