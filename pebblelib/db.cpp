//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "db.h"
#include <QDebug>

/*
 * We need to match dB calculations to the output of an FFT to be precise
 * If our dB range is -120dB to 0dB, then that must match to smallest FFT output to largest FFT output
 * Smallest FFT output = -120dB and largest FFT output = 0dB
 *
 * We also have to consider the FFT input range. -32767 to +32767 or -1.0 to +1.0
 * Finally, the output of a Complex FFT vs a Real FFT are different
 *
 * For Pebble, we use double values (-1.0 to +1.0) and a Complex FFT
 *
 * A pure input sin wave (asin(wt)) will produce (Complex) FFT values of (FFT_SIZE * Amplitude / 2 )^2
 * A pure input sin wave Asin(wt) will produce (Real) FFT values of (FFT_SIZE * Amplitude / 4 )^2
 * Where Amplitude varies from min to max sample range
 *
 * If FFT_SIZE = 2048, and Amplitude varies from -1.0 to +1.0
 * Then the peak FFT power value will be (2048 * 1.0 / 2.0)^2 or 1024^2 or 1048576
 *
 * Using db = 10 * log10(power) and substituting formulas back
 * maxDB (0dB) = 10 * log10(FFT_SIZE * MAX_AMPLITUDE / 2) ^2
 * We need some adjustments to make this formula balance, so we add a power adjustment (pwrOffset) and a dB adjustment (dbOffset)
 * maxDB (0dB) = 10 * log10((FFT_SIZE * MAX_AMPLITUDE / 2) + pwrOffset) ^2 + dbOffset
 * Solving forumla for pwrOffset = 0 we get
 * dbOffset = maxDB - 20 * log10(FFT_SIZE * MAX_AMPLITUDE / 2) or -60.205 for Pebble
 * Then solving for pwrOffset using dbOffset we get
 * pwrOffset = 10^((minDB - dbOffset) / 10)
 *
 * To calculate FFT normalized dB
 * dB = 10 * log10(samplePower + pwrOffset) + dbOffset
 *
*/

//Init static class variables
double DB::maxDb = 0;
//Same as SpectraVue & CuteSDR;
double DB::minDb = -120.0;
//Hard wired for FFT 2048 for testing
double DB::dbOffset = DB::maxDb - 20 * log10(2048 * 1.0 / 2.0);
double DB::pwrOffset = pow(10, (DB::minDb - DB::dbOffset) / 10.0);

DB::DB()
{
}


//Returns the total power in the sample buffer, using Lynn formula
double DB::totalPower(CPX *in, int bsize)
{
    float tmp = 0.0;
    //sum(re^2 + im^2)
    for (int i = 0; i < bsize; i++) {
        //Total power of this sample pair
        tmp += cpxToWatts(in[i]);
    }
    return tmp;
}

//8/13 verified that powerToDb(cpxToWatts(in[i])) yields correct results compared to other SDR products
double DB::cpxToWatts(CPX cx)
{
    //Power, same as cpx.sqrMax()
    return (cx.re * cx.re + cx.im * cx.im);
}

/*
db tutorial from Steven Smith
'bel' means power has changed by factor of 10
'decibel' is 0.1 of a bell, so 10db means power changed by factor of 10
60db = power changed by 10^3 = 1000
40db = power changed by 10^2 = 100
20db = power changed by 10^1 = 10
0db = power changed by 10^0 = 1
*/
//Power same as cpx.mag()

//Convert Power to Db
double DB::powerToDb(double p)
{
    //For our purposes -127db is the lowest we'll ever see.  Handle special case of 0 directly
    if (p==0)
		return DB::minDb;

    //Voltage = 20 * log10(V2/V1)
    //  + ALMOSTZERO avoid problem if p==0 but does not impact result
	return  qBound(DB::minDb, 10.0 * log10(p + DB::pwrOffset + ALMOSTZERO) + DB::dbOffset, DB::maxDb);
}

//Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
double DB::powerRatioToDb(double measuredPower, double comparedPower)
{
	return 10.0 * log10(comparedPower / measuredPower);
}

// Positive db returns power = 1.0 and up
// Zero db return power = 1.0 (1:1 ratio)
// Negative db returns power = 0.0 to 1.0
double DB::dbToPower(double db)
{
	//Note pow(10, db/10.0) is the same as antilog(db/10.0) which is shown in some texts.
    return pow(10, db/10.0);
}

//Steven Smith pg 264
double DB::amplitudeToDb(double a)
{
	return qBound(DB::minDb, 20.0 * log10(a + ALMOSTZERO), DB::maxDb);
}
//Steven Smith pg 264
double DB::dbToAmplitude(double db)
{
    return pow(10, db/20.0);
}
/*
    One S unit = 6db
    s0 = -127, s1=-121, s2=-115, ... s9 = -73db (-93dbVHF)
    +10 = -63db, +20 = -53, ... +60 = -13db
*/
int DB::dbToSUnit(double db)
{
    int unit = db/6;
    return unit;
}
double DB::dBm_2_Watts(double dBm)
{
    return pow(10.0, (dBm - 30.0) / 10.0);
}

double DB::watts_2_dBm(double watts)
{
    if (watts < 10.0e-32)
        watts = 10.0e-32;
	return qBound(DB::minDb, (10.0 * log10(watts)) + 30.0, DB::maxDb);
}

double DB::dBm_2_RMSVolts(double dBm, double impedance)
{
    return sqrt(dBm_2_Watts(dBm) * impedance);
}

double DB::rmsVolts_2_dBm(double volts, double impedance)
{
    return watts_2_dBm((volts * volts) / impedance);
}

