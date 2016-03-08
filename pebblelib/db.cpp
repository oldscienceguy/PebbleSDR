//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "db.h"
#include <QDebug>


#if 0
	Naming confusion.  There are different usages of dB that get used interchangeably
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

	Note on bandwidth
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

#endif

//Init static class variables
double DB::maxDb = 0;
//Same as SpectraVue & CuteSDR;
double DB::minDb = -120.0;

DB::DB()
{
}

//Move to inline where possible
//See notes above for detailed explanation
double DB::amplitude(CPX cx)
{
	return sqrt(cx.re*cx.re + cx.im*cx.im);
}

//Power same as cpx.mag(), use this for clarity
double DB::power(CPX cx)
{
	return cx.re*cx.re + cx.im+cx.im;
}

//Returns power adjusted for loss due to filters in FFT process (see above)
//Use this before calculating db from fft to get dB power spectral density
//fftBinWidth = sampleRate / fftSize
double DB::fftPower(CPX cx, quint32 fftBinWidth)
{
	return DB::power(cx) / fftBinWidth;
}

//Returns the total power in the sample buffer, using Lynn formula
double DB::totalPower(CPX *in, int bsize)
{
    float tmp = 0.0;
    //sum(re^2 + im^2)
    for (int i = 0; i < bsize; i++) {
        //Total power of this sample pair
		tmp += power(in[i]);
    }
    return tmp;
}

//Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
double DB::powerRatioToDb(double measuredPower, double comparedPower)
{
	double db = 10.0 * log10(comparedPower / measuredPower);
	return qBound(DB::minDb, db, DB::maxDb);
}

// Positive db returns power = 1.0 and up
// Zero db return power = 1.0 (1:1 ratio)
// Negative db returns power = 0.0 to 1.0
double DB::dBToPower(double dB)
{
	//Note pow(10, db/10.0) is the same as antilog(db/10.0) which is shown in some texts.
	return pow(10, dB/10.0);
}

// d = powerToDb(x) followed by dbToPower(d) should return same x
// Returns dBm, ratio of watts to 1 milliwatt
double DB::powerTodB(double power)
{
	double db = 10 * log10(power);
	return qBound(DB::minDb, db, DB::maxDb);
}

//Steven Smith pg 264
double DB::dBToAmplitude(double db)
{
	return pow(10, db/20.0);
}

//Steven Smith pg 264
//Use powerTodB where possible to avoid extra sqrt() needed to calculate amplitude
double DB::amplitudeTodB(double amplitude)
{
	double db = 20 * log10(amplitude);
	return qBound(DB::minDb, db, DB::maxDb);
}

/*
   Convert db to linear S-Unit approx for use in S-Meter
    One S unit = 6db
    s0 = -127, s1=-121, s2=-115, ... s9 = -73db (-93dbVHF)
	+10 = -63db, +20 = -53, ... +60 = -13db
*/
int DB::dbToSUnit(double db)
{
	double aboveSZero = fabs(-127 - db);
	return aboveSZero / 6.0;
}

//Here for reference
//microvolts == amplitude
double DB::uv(CPX cx)
{
	return sqrt(cx.re*cx.re + cx.im*cx.im);
}
double DB::mw(CPX cx)
{
	return cx.re*cx.re + cx.im*cx.im;
}
double DB::uvTodBuv(double uv)
{
	return 20 * log10 (uv);
}
double DB::dBuvTouv(double dBuv)
{
	return pow(10, dBuv / 20);
}
//milliwatts == power
double DB::mwTodBm (double mw)
{
	return 10 * log10(mw);
}
double DB::dBmTomw (double dBm)
{
	return pow(10, dBm/10);
}


