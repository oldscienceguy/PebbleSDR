#include <QCoreApplication>
#include <QtCore>

/*
    Startup args
    IP address
    Port
*/
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList argList = a.arguments();

    //Load device plugins

    return a.exec();
}
