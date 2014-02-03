#ifndef RTLTCPPROTOCOL_H
#define RTLTCPPROTOCOL_H

#include <QCommandLineParser>
#include <QHostAddress>
#include "device_interfaces.h"

class RtlTcpProtocol
{
public:
    //Some of thes arguments should be common to all server protocols, here to get started
    enum COMMANDS {CMD_FREQ=0x01, CMD_SAMPLERATE=0x02, CMD_GAIN_MODE=0x03, CMD_TUNER_GAIN=0x04, CMD_FREQ_CORRECTION=0x05,
                CMD_IF_GAIN=0x06, CMD_TEST_MODE=0x07, CMD_AGC_MODE=0x08, CMD_DIRECT_SAMPLING = 0x09,
                CMD_OFFSET_TUNING = 0x0a, CMD_XTAL_FREQ=0x0b, CMD_TUNER_XTAL=0x0c, CMD_TUNER_GAIN_BY_INDEX = 0x0d};

//This should go in global.h or somewhere we can use everywhere
#ifdef _WIN32
    //gcc and clang use __attribute, define it for windows as noop
    #define __attribute__(x)
    #define packStart pack(push, 1)
    #define packEnd pragma pack(pop)
#else
    #define packStart
    #define packEnd
#endif

    union RTL_CMD {
        //This needs to be packed for network transfer, no padded bytes
        packStart
        struct{
            quint8 cmd;
            quint32 param;
        }__attribute__((packed));
        packEnd
        char buf[5]; //Must be same size as packed structure
    };

    RtlTcpProtocol(DeviceInterface *_sdr);
    void getCommandLineArguments(QCommandLineParser *parser);

    QHostAddress getHostAddress() {return hostAddress;}
    quint16 getPort() {return hostPort;}

    void tcpCommands(RTL_CMD cmd);
    void commandWorker(char *data, qint64 numBytes);
private:
    QHostAddress hostAddress;
    quint16 hostPort;
    RTL_CMD cmd; //Incoming command
    quint16 cmdIndex; //Used if we dont get all the command bytes at once
    DeviceInterface *sdr;

    //Last command param values
    quint32 frequency;
    quint32 sampleRate;
    quint32 frequencyCorrection;
    quint32 tunerGainMode;
    quint32 tunerGain;
    quint32 agcMode;
    quint32 directSampling;
    quint32 offsetTuning;

};

#endif // RTLTCPPROTOCOL_H
