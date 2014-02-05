#ifndef SDRSERVER_H
#define SDRSERVER_H
#include <QCoreApplication>
#include <QtCore>

#include <QTcpServer>
#include <QTcpSocket>
#include "deviceplugins.h"
#include "rtltcpprotocol.h"

class SdrServer : public QCoreApplication

{
    Q_OBJECT
public:
    SdrServer(int & argc, char ** argv);
    ~SdrServer();


    qintptr GetSocketDescriptor() {return socket->socketDescriptor();}

public slots:
    void newConnection();
    void closeConnection();
    void serverError(QAbstractSocket::SocketError error);
    void newData();

private:

    QTcpServer *server;
    DevicePlugins *plugins;
    void getCommandLineArguments();

    QTcpSocket *socket;
    quint16 dataBufLen;
    char *dataBuf; //For incoming data

    DeviceInterface *sdr;
    RtlTcpProtocol *protocol;
};

#endif // SDRSERVER_H
