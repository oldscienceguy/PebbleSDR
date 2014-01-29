#include "sdrserver.h"

/*
    Startup args
    IP address
    Port
*/
int main(int argc, char *argv[])
{
    //Our QCoreApplication
    SdrServer server(argc, argv);

    return server.exec(); //Start the event loop and continue until quit()
}
