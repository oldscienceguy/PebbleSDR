//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebbleii.h"
#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile qssFile( ":/pebble.qss" );  // assuming stylesheet is part of the resources
    qssFile.open( QFile::ReadOnly );
    app.setStyleSheet( qssFile.readAll() );
    qssFile.close();

	PebbleII w;
	w.show();
    return app.exec();
}
