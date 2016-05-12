//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebbleii.h"
#include <QtGui>
#include <QtPlugin>

//Use this if we want to import a static plugin, ie one that is always installed
//Q_IMPORT_PLUGIN(...)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile qssFile( ":/pebble.qss" );  // assuming stylesheet is part of the resources
    qssFile.open( QFile::ReadOnly );
	QString qss = QTextStream(&qssFile).readAll();
	//Find variables
	int begin = 0;
	int length = 0;
	QString name;
	QString value;
	//@mycolor: #455090;
	QRegularExpression varRegex = QRegularExpression("^(?<name>@.*):(?<value>.*);.*",QRegularExpression::MultilineOption);
	QRegularExpressionMatch match;
	//Loop until there are no more definitions
	do {
		match = varRegex.match(qss);
		if (match.hasMatch()) {
			//qDebug()<<match.captured();
			name = match.captured("name");
			value = match.captured("value");
			//qDebug()<<name<<" "<<value;
			begin = match.capturedStart();
			length = match.capturedLength();
			//Remove definition
			qss.remove(match.capturedStart(), match.capturedLength());
			//Replace all instances of var
			qss.replace(name,value);
		}
	}while (match.hasMatch());

	app.setStyleSheet(qss);
    qssFile.close();

    // Qt::FramelessWindowHint No title, size, nothing
    // Qt::WindowTitleHint
    // Qt::Tool thin tile, no full screen default, close button hides not quits
    PebbleII w(0, 0);
	w.show();
    //Layout should now be done, save default window size for later restore
    global->defaultWindowSize = w.size();

    return app.exec();
}
