#include "sdrserver.h"

/*
    Protocol notes
    What Devices are availabale? Return enum list
    What Protocol to use? rtl-tcp, hpsdr, sdr-ip, ghpsdr
    What SDR to use? Enum from list

*/

SdrServer::SdrServer(int &argc, char **argv) : QCoreApplication(argc,argv)
{
    setApplicationName("SDRGarage");
    setApplicationVersion("0.0.1");

    //Command line parsing
    getCommandLineArguments();


    //Here or in init?
    //Load plugins
    plugins = new DevicePlugins();

    server = new QTcpServer(this);
    connect(server,SIGNAL(newConnection()), this, SLOT(newConnection()));
    connect(server,SIGNAL(serverError()),this,SLOT(serverError()));
    //One connection at a time for now, eventually we'll handle multiple connections
    server->setMaxPendingConnections(1);
    server->listen(hostAddress, hostPort);
    qDebug()<<"Listening on "<<hostAddress.toString()<<" port "<<hostPort;
}

void SdrServer::getCommandLineArguments
()
{
    QCommandLineParser parser;
    parser.setApplicationDescription("SDR Garage");
    parser.addHelpOption();
    parser.addVersionOption();

    //parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
    //parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));

    QCommandLineOption serverIPArg(
        QStringList() << "a" << "address",                                  //Option names: -a or -address
        QCoreApplication::translate("main", "Specify server IP address"),   //Option description
        QCoreApplication::translate("main", "serverIPAddress"),             //Value name
        QCoreApplication::translate("main", "0.0.0.0"));                    //Default
    parser.addOption(serverIPArg);

    QCommandLineOption serverPortArg(QStringList() << "p" << "port",
        QCoreApplication::translate("main", "Specify server port"),
        QCoreApplication::translate("main", "serverPort"),
        QCoreApplication::translate("main", "1234"));
    parser.addOption(serverPortArg);

    // A boolean option with multiple names (-f, --force)
    //QCommandLineOption forceOption(QStringList() << "f" << "force", "Overwrite existing files.");
    //parser.addOption(forceOption);

    // Process the actual command line arguments given by the user
    parser.process(*this);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

    //bool showProgress = parser.isSet(showProgressOption);
    //bool force = parser.isSet(forceOption);
    hostAddress.setAddress(parser.value(serverIPArg));
    hostPort = parser.value(serverPortArg).toUInt();
}

void SdrServer::newConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();
    qDebug()<<"New connection from "<<socket->peerAddress().toString();

}

void SdrServer::serverError()
{
    qDebug()<<"Server error"<<server->errorString();
    server->close();
}
