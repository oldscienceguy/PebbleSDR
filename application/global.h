#ifndef GLOBAL_H
#define GLOBAL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include "QDebug"
#include "QFile"
#include <QSize>
#include <QMainWindow>
#include "perform.h"

class Receiver;
class Settings;
class SDR;
class CTestBench;

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
    QSize defaultWindowSize;
    QMainWindow *mainWindow;


private:
	QFile *file;
};

extern Global *global;
#define logfile (*global->pLogfile)
#endif // GLOBAL_H
