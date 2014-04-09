#ifndef DB_H
#define DB_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "pebblelib_global.h"
#include "cpx.h"

class PEBBLELIBSHARED_EXPORT DB
{
public:
    DB();

    //Useful conversion functions
	double cpxToWatts(CPX cx); //Returns power (watts) for sample c

    //Calculates the total power of all samples in buffer
	double totalPower(CPX *in, int bsize);

	double dBm_2_Watts(double dBm);
	double watts_2_dBm(double watts);
	double dBm_2_RMSVolts(double dBm, double impedance);
	double rmsVolts_2_dBm(double volts, double impedance);

    //db conversion functions from Steven Smith book
	double powerToDb(double p);
	double dbToPower(double db);

	double amplitudeToDb(double a);
	double dbToAmplitude(double db);

	int dbToSUnit(double db);

    //Smallest db we'll return from FFT
	double minDb;
	double maxDb;

	//Adjustments to normalize db with fft
	double pwrOffset;
	double dbOffset;

};

#endif // DB_H
