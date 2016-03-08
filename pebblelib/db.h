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
	static double amplitude(CPX cx);
	static double power(CPX cx);
	static double fftPower(CPX cx, quint32 fftBinWidth);

	static double dBToAmplitude(double db);
	static double amplitudeTodB(double amplitude);

    //Calculates the total power of all samples in buffer
	static double totalPower(CPX *in, int bsize);

    //db conversion functions from Steven Smith book
	static double powerRatioToDb(double measuredPower, double comparedPower);
	static double dBToPower(double dB);
	static double powerTodB(double power);

	static int dbToSUnit(double db);

    //Smallest db we'll return from FFT
	static double minDb;
	static double maxDb;

	//For reference
	static double uv(CPX cx);
	static double mw(CPX cx);
	static double uvTodBuv(double uv);
	static double dBuvTouv(double dBuv);
	static double mwTodBm(double mw);
	static double dBmTomw(double dBm);
};

#endif // DB_H
