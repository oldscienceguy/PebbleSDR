//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include <QDebug>
#include "qcoreapplication.h"
#include "testbench.h"

Global::Global()
{
	appDirPath = QCoreApplication::applicationDirPath();
	QDir appDir = QDir(appDirPath);
#if defined(Q_OS_MAC)
	if (appDir.dirName() == "MacOS") {
		//We're in the mac package and need to get back to the package to access relative directories
		appDir.cdUp();
		appDir.cdUp();
		appDir.cdUp(); //Root dir where app is located
		appDirPath = appDir.absolutePath();
	}
#endif

	pebbleDataPath = appDirPath + "/PebbleData/";

	file = new QFile(pebbleDataPath+"pebblelog.txt");
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

	beep.setSource(QUrl::fromLocalFile(pebbleDataPath + "beep-07.wav"));
	beep.setLoopCount(1);
	beep.setVolume(0.25f);

	primaryScreen = QGuiApplication::primaryScreen();

}

Global::~Global()
{
	if (pLogfile){
		file->close();
		delete pLogfile;
		delete file;
	}
}

