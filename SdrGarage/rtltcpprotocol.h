#ifndef RTLTCPPROTOCOL_H
#define RTLTCPPROTOCOL_H

#include <QCommandLineParser>
#include <QHostAddress>
#include "deviceinterfacebase.h"
#include "cpx.h"
#include <QTcpSocket>

class SdrServer;

class RtlTcpProtocol
{
public:
    //Some of thes arguments should be common to all server protocols, here to get started
    enum COMMANDS {CMD_FREQ=0x01, CMD_SAMPLERATE=0x02, CMD_GAIN_MODE=0x03, CMD_TUNER_GAIN=0x04, CMD_FREQ_CORRECTION=0x05,
                CMD_IF_GAIN=0x06, CMD_TEST_MODE=0x07, CMD_AGC_MODE=0x08, CMD_DIRECT_SAMPLING = 0x09,
                CMD_OFFSET_TUNING = 0x0a, CMD_XTAL_FREQ=0x0b, CMD_TUNER_XTAL=0x0c, CMD_TUNER_GAIN_BY_INDEX = 0x0d};

#define K_RTLSampleRate DeviceInterface::Key_CustomKey1
#define K_RTLTunerGainMode DeviceInterface::Key_CustomKey2
#define K_RTLTunerGain DeviceInterface::Key_CustomKey3
#define K_RTLAgcMode DeviceInterface::Key_CustomKey4
#define K_RTLSampleMode DeviceInterface::Key_CustomKey5
#define K_RTLOffsetMode DeviceInterface::Key_CustomKey6

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

    RtlTcpProtocol(SdrServer *_server, DeviceInterface *_sdr);
    void getCommandLineArguments(QCommandLineParser *parser);

    QHostAddress getHostAddress() {return hostAddress;}
    quint16 getPort() {return hostPort;}

    void tcpCommands(RTL_CMD cmd);
    void commandWorker(char *data, qint64 numBytes);
    void ProcessIQData(CPX *in, quint16 numSamples); //Must be public so we can bind to it
    void Reset();
private:

    QHostAddress hostAddress;
    quint16 hostPort;
    RTL_CMD cmd; //Incoming command
    quint16 cmdIndex; //Used if we dont get all the command bytes at once
    DeviceInterface *sdr;
    SdrServer *sdrServer;
    QTcpSocket *threadSocket;

    char out[4096]; //Make dynamic

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
