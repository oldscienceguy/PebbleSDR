//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include <QDebug>
#include "qcoreapplication.h"
#include "testbench.h"

Global::Global()
{
	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif

    file = new QFile(path+"/PebbleData/pebblelog.txt");
	bool res = file->open(QIODevice::Unbuffered | QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text);
	pLogfile = NULL;
	if (res) {
		pLogfile = new QDebug(file);
		*pLogfile << "Pebble log file\n";
	}
    revision = new char[80];
    //See PebbleQt.pro for DEFINES statement that creates PEBBLE_VERSION
    sprintf(revision,"Build: %s %s",PEBBLE_VERSION, PEBBLE_DATE);
    //qDebug(revision);

    sdr = NULL;
    perform.InitPerformance();
}

Global::~Global()
{
	if (pLogfile){
		file->close();
		delete pLogfile;
		delete file;
	}
}

