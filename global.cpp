//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "qcoreapplication.h"

Global::Global()
{
	QString path = QCoreApplication::applicationDirPath();
	file = new QFile(path+"/pebblelog.txt");
	bool res = file->open(QIODevice::Unbuffered | QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text);
	pLogfile = NULL;
	if (res) {
		pLogfile = new QDebug(file);
		*pLogfile << "Pebble log file\n";
	}
}
Global::~Global()
{
	if (pLogfile){
		file->close();
		delete pLogfile;
		delete file;
	}
}

