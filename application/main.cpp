//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebbleii.h"
#include <QtGui>
#include "../pebblelib/digital_modem_interfaces.h"
#include <QtPlugin>

//Use this if we want to import a static plugin, ie one that is always installed
//Q_IMPORT_PLUGIN(...)

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile qssFile( ":/pebble.qss" );  // assuming stylesheet is part of the resources
    qssFile.open( QFile::ReadOnly );
    app.setStyleSheet( qssFile.readAll() );
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
