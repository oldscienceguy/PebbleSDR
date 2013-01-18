#ifndef GLOBAL_H
#define GLOBAL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "QDebug"
#include "QFile"

class Receiver;
class Settings;

class Global
{
public:
    Global();
	~Global();

	QDebug *pLogfile;
    Receiver *receiver;
    char *revision;
    Settings *settings;
    const static double MIN_DB = -150; //Smallest db we'll return from FFT

private:
	QFile *file;
};

extern Global *global;
#define logfile (*global->pLogfile)
#endif // GLOBAL_H
