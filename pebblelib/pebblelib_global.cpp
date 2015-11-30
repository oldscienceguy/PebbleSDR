#include "pebblelib_global.h"
#include "qcoreapplication.h"
#include <QDir>

//See application/global.cpp
PebbleLibGlobal::PebbleLibGlobal()
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

	perform.InitPerformance();

}

PebbleLibGlobal::~PebbleLibGlobal()
{

}
