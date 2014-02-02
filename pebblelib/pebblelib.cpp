//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebblelib.h"


PebbleLib::PebbleLib()
{
}


/*
    Redirect qDebug() etc
#include <QApplication>
#include <QtDebug>
#include <QFile>
#include <QTextStream>

//in this function, you can write the message to any stream!
void myMessageHandler(QtMsgType type, const char *msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        //fprintf(stderr, "Debug: %s\n", msg);
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        //fprintf(stderr, "Warning: %s\n", msg);
        txt = QString("Warning: %1").arg(msg);
    break;
    case QtCriticalMsg:
        //fprintf(stderr, "Critical: %s\n", msg);
        txt = QString("Critical: %1").arg(msg);
    break;
    case QtFatalMsg:
        //fprintf(stderr, "Fatal: %s\n", msg);
        txt = QString("Fatal: %1").arg(msg);
    break;
    }
    QFile outFile("log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
}

//Add qInstallMsgHandler to main() in every program to use it
int main( int argc, char * argv[] )
{
    QApplication app( argc, argv );
    qInstallMsgHandler(myMessageHandler);
    ...
    return app.exec();
}


*/
