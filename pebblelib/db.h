#ifndef DB_H
#define DB_H

#include "pebblelib_global.h"
#include "cpx.h"

class PEBBLELIBSHARED_EXPORT DB
{
public:
    DB();

    //Useful conversion functions
    static double cpxToWatts(CPX cx); //Returns power (watts) for sample c

    //Calculates the total power of all samples in buffer
    static double totalPower(CPX *in, int bsize);

    static double dBm_2_Watts(double dBm);
    static double watts_2_dBm(double watts);
    static double dBm_2_RMSVolts(double dBm, double impedance);
    static double rmsVolts_2_dBm(double volts, double impedance);

    //db conversion functions from Steven Smith book
    static double powerToDb(double p);
    static double dbToPower(double db);

    static double amplitudeToDb(double a);
    static double dbToAmplitude(double db);

    static int dbToSUnit(double db);

    //Smallest db we'll return from FFT
    static const double minDb;
    static const double maxDb;

};

#endif // DB_H
