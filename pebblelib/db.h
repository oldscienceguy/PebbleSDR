#ifndef DB_H
#define DB_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "pebblelib_global.h"
#include "cpx.h"

class PEBBLELIBSHARED_EXPORT DB
{
public:
	//DB(); //All static, no constructor

	//All samples are -1 to +1.
	static const double fullScale;

	//min/max db we'll return from FFT, use in power calculations, plotting, etc
	static const double minDb;
	static const double maxDb;
	static const double minPower; //Equivalent to binDb
	static const double maxPower;

    //Useful conversion functions
	static inline double clip(double db) {
		return qBound(DB::minDb, db, DB::maxDb);
	}

	static inline double amplitude(CPX cx){
		return sqrt(cx.real()*cx.real() + cx.im*cx.im);
	}

	//Power same as cpx.mag(), use this for clarity
	static inline double power(CPX cx) {
		return cx.real()*cx.real() + cx.im*cx.im;
	}

	//Steven Smith pg 264
	static inline double dBToAmplitude(double db) {
		return pow(10, db/20.0);
	}

	//Steven Smith pg 264
	//Use powerTodB where possible to avoid extra sqrt() needed to calculate amplitude
	static inline double amplitudeTodB(double amplitude) {
		if (amplitude==0)
			return DB::minDb; //log10(0) = nan
		 return 20 * log10(amplitude);
	}

    //Calculates the total power of all samples in buffer
	static inline double totalPower(CPX *in, int bsize) {
		float tmp = 0.0;
		//sum(re^2 + im^2)
		for (int i = 0; i < bsize; i++) {
			//Total power of this sample pair
			tmp += power(in[i]);
		}
		return tmp;
	}

    //db conversion functions from Steven Smith book
	//Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
	static inline double powerRatioToDb(double comparedPower, double measuredPower) {
		if (measuredPower==0 || comparedPower==0)
			return DB::minDb;
		return 10.0 * log10(comparedPower / measuredPower);
	}

	// Positive db returns power = 1.0 and up
	// Zero db return power = 1.0 (1:1 ratio)
	// Negative db returns power = 0.0 to 1.0
	static inline double dBToPower(double dB) {
		//Note pow(10, db/10.0) is the same as antilog(db/10.0) which is shown in some texts.
		return pow(10, dB/10.0);
	}

	// d = powerToDb(x) followed by dbToPower(d) should return same x
	static inline double powerTodB(double power) {
		if (power==0)
			return DB::minDb; //log10(0) = nan
		return 10 * log10(power);
	}

	//Power has already been squared, just take the sqrt()
	static inline double powerToAmplitude(double power) {
		return sqrt(power);
	}

	//Amplitude is sqrt(power), reverse
	static double amplitudeToPower(double amplitude) {
		return amplitude * amplitude;
	}

	//Convert db to linear S-Unit approx for use in S-Meter
	//	One S unit = 6db
	//	s0 = -127, s1=-121, s2=-115, ... s9 = -73db (-93dbVHF)
	//	+10 = -63db, +20 = -53, ... +60 = -13db
	static inline int dbToSUnit(double db) {
		double aboveSZero = fabs(-127 - db);
		return aboveSZero / 6.0;
	}

	//Here for reference
	static inline double rms(CPX *in, quint32 numSamples);
	static inline double rmsToDb(double rms) {
		return 20*log10(DB::fullScale / rms);
	}

	//microvolts == amplitude
	static inline double uv(CPX cx) {
		return sqrt(cx.real()*cx.real() + cx.im*cx.im);
	}

	static inline double mw(CPX cx) {
		return cx.real()*cx.real() + cx.im*cx.im;
	}

	static inline double uvTodBuv(double uv) {
		if (uv==0)
			return DB::minDb; //log10(0) = nan
		return 20 * log10 (uv);
	}

	static inline double dBuvTouv(double dBuv) {
		return pow(10, dBuv / 20);
	}

	//milliwatts == power
	static inline double mwTodBm(double mw) {
		if (mw==0)
			return DB::minDb; //log10(0) = nan
		return 10 * log10(mw);
	}

	static inline double dBmTomw(double dBm) {
		return pow(10, dBm/10);
	}


	static void test();
	static void analyzeCPX(CPX *in, quint32 numSamples, const char *label, bool isFftOutput = false,
						   double windowGain = 1.0, double maxBinPower = 2048);
};

#endif // DB_H
