#ifndef SDRSERVER_H
#define SDRSERVER_H
#include <QCoreApplication>
#include <QtCore>

#include <QTcpServer>
#include <QTcpSocket>
#include "deviceplugins.h"

class SdrServer : public QCoreApplication

{
    Q_OBJECT
public:
    SdrServer(int & argc, char ** argv);

public slots:
    void newConnection();
    void serverError();

private:
    QTcpServer *server;
    DevicePlugins *plugins;
    void getCommandLineArguments();

    QHostAddress hostAddress;
    quint16 hostPort;
};

#endif // SDRSERVER_H
