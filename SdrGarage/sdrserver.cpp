#include "sdrserver.h"

/*
    Protocol notes
    What Devices are availabale? Return enum list
    What Protocol to use? rtl-tcp, hpsdr, sdr-ip, ghpsdr
    What SDR to use? Enum from list

*/

using namespace std::placeholders; //For _1, _2 arguments

SdrServer::SdrServer(int &argc, char **argv) : QCoreApplication(argc,argv)
{
    setApplicationName("SDRGarage");
    setApplicationVersion("0.0.1");

    //Here or in init?
    //Load plugins
    plugins = new DevicePlugins();
    sdr = plugins->GetDeviceInterface("RTL2832 USB");

    protocol = new RtlTcpProtocol(this, sdr);

    //Command line parsing
    getCommandLineArguments();

    dataBufLen = 256;
    dataBuf = new char(dataBufLen);


    server = new QTcpServer(this);
    connect(server,SIGNAL(newConnection()), this, SLOT(newConnection()));
    connect(server,SIGNAL(acceptError(QAbstractSocket::SocketError)),this,SLOT(serverError(QAbstractSocket::SocketError)));
    //One connection at a time for now, eventually we'll handle multiple connections
    server->setMaxPendingConnections(1);
    server->listen(protocol->getHostAddress(), protocol->getPort());
    qDebug()<<"Listening on "<<protocol->getHostAddress().toString()<<" port "<<protocol->getPort();
}

SdrServer::~SdrServer()
{
    if (sdr != NULL) {
        sdr->Stop();
        sdr->Disconnect();
    }
}

void SdrServer::getCommandLineArguments()
{
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    //Eventually display protocol options first, then options for each protocol

    protocol->getCommandLineArguments(&parser);

}

void SdrServer::newConnection()
{
    socket = server->nextPendingConnection();
    qDebug()<<"New connection from "<<socket->peerAddress().toString();
    connect(socket,SIGNAL(readyRead()),this,SLOT(newData()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(closeConnection()));

    sdr->ReadSettings(); //Gets current settings or defaults if first use

    sdr->Initialize(std::bind(&RtlTcpProtocol::ProcessIQData, protocol, _1, _2),2048);

    if (!sdr->Connect()) {
        qDebug()<<"Could not connect to device";
        return;
    }
    sdr->Start();

}

void SdrServer::closeConnection()
{
    qDebug()<<"Connection closed";
    sdr->Stop();
    sdr->Disconnect();
    protocol->Reset();

}

void SdrServer::serverError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);

    qDebug()<<"Server error"<<server->errorString();
    server->close();
}

//Connected to TcpSocket readyRead
void SdrServer::newData()
{
    //qint64 bytesAvailable = socket->bytesAvailable();
    qint64 bytesRead = socket->read(dataBuf,dataBufLen);
    //qDebug()<<bytesRead;
    if (bytesRead > 0) {
        protocol->commandWorker(dataBuf, bytesRead);
    }

}
