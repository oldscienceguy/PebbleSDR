#ifndef GLOBAL_H
#define GLOBAL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "QDebug"
#include "QFile"
#include "perform.h"
#include "testbench.h"

class Receiver;
class Settings;
class SDR;

class Global
{
public:
    Global();
	~Global();

	QDebug *pLogfile;
    Receiver *receiver;
    SDR *sdr;
    char *revision;
    Settings *settings;
    double minDb; //Smallest db we'll return from FFT
    double maxDb;
    Perform perform;
    CTestBench* testBench;


private:
	QFile *file;
};

extern Global *global;
#define logfile (*global->pLogfile)
#endif // GLOBAL_H
