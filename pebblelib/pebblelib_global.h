#ifndef PEBBLELIB_GLOBAL_H
#define PEBBLELIB_GLOBAL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include <QtCore/qglobal.h>
#include "QDebug"
#include "QFile"
#include "perform.h"
#include <QSoundEffect>

#if defined(PEBBLELIB_LIBRARY)
#  define PEBBLELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define PEBBLELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

class PebbleLibGlobal
{
public:
	PebbleLibGlobal();
	~PebbleLibGlobal();

	Perform perform;
	QString appDirPath; //Location of executable, used to access pebbledata, plugins, etc

private:
};

extern PebbleLibGlobal *pebbleLibGobal;
#define pebbleLibLogfile (*pebbleLibGobal->pLogfile)

#endif // PEBBLELIB_GLOBAL_H
