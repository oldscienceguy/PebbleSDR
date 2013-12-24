//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "db.h"

DB::DB()
{
}

//Same as SpectraVue & CuteSDR;
const double DB::minDb = -120.0;
const double DB::maxDb = 0.0;

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
        return minDb;

    //Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
    //Voltage = 20 * log10(V2/V1)
    //  + ALMOSTZERO avoid problem if p==0 but does not impact result
    return  qBound(DB::minDb, 10.0 * log10(p + ALMOSTZERO), DB::maxDb);
}
double DB::dbToPower(double db)
{
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

