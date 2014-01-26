#include "sdrserver.h"

/*
    Protocol notes
    What Devices are availabale? Return enum list
    What Protocol to use? rtl-tcp, hpsdr, sdr-ip, ghpsdr
    What SDR to use? Enum from list

*/

SdrServer::SdrServer() : QObject()
{
    //Here or in init?
    server = new QTcpServer(this);
    QHostAddress hostAddress("192.168.0.0"); //Change or pass as arg
    quint16 hostPort = 1234;
    connect(server,SIGNAL(newConnection()), this, SLOT(newConnection()));
    //One connection at a time for now, eventually we'll handle multiple connections
    server->setMaxPendingConnections(1);
    server->listen(hostAddress, hostPort);
}

void SdrServer::newConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();

}
