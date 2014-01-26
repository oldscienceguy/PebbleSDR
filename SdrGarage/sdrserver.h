#ifndef SDRSERVER_H
#define SDRSERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include "deviceplugins.h"

class SdrServer : public QObject

{
    Q_OBJECT
public:
    SdrServer();

public slots:
    void newConnection();

private:
    QTcpServer *server;
    DevicePlugins *plugins;
};

#endif // SDRSERVER_H
