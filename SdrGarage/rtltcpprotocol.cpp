/*
 * Adopted from rtl-tcp for Qt and use with any Pebble device plugin, not just rtlsdr's
*/

#include "rtltcpprotocol.h"

#include <QCoreApplication>

RtlTcpProtocol::RtlTcpProtocol(DeviceInterface *_sdr)
{
    sdr = _sdr;
    cmdIndex = 0;
}

//Called from server to get any additional commandline arguments
void RtlTcpProtocol::getCommandLineArguments(QCommandLineParser *parser)
{
    //How do we display additional overview text?
    //rtl_tcp, an I/Q spectrum server for RTL2832 based DVB-T receivers

    //parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
    //parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));

    // A boolean option with multiple names (-f, --force)
    //QCommandLineOption forceOption(QStringList() << "f" << "force", "Overwrite existing files.");
    //parser.addOption(forceOption);


    //bool showProgress = parser.isSet(showProgressOption);
    //bool force = parser.isSet(forceOption);

    parser->setApplicationDescription("SDR Garage - an I/Q spectrum server for RTL2832 based DVB-T receivers using rtl-tcp protocol");

    QCommandLineOption serverIPArg(
        QStringList() << "a" << "address",                                  //Option names: -a or -address
        QCoreApplication::translate("main", "Specify server IP address (Required)"),   //Option description
        QCoreApplication::translate("main", "serverIPAddress"),             //Value name
        QCoreApplication::translate("main", "0.0.0.0"));                    //Default
    parser->addOption(serverIPArg);

    QCommandLineOption serverPortArg(QStringList() << "p" << "port",
        QCoreApplication::translate("main", "Specify server port"),
        QCoreApplication::translate("main", "serverPort"),
        QCoreApplication::translate("main", "1234"));
    parser->addOption(serverPortArg);

    QCommandLineOption frequencyArg(QStringList() << "f" << "frequency",
        QCoreApplication::translate("main", "Set device frequency in Hz"),
        QCoreApplication::translate("main", "frequency"),
        QCoreApplication::translate("main", "10000000"));
    parser->addOption(frequencyArg);

    QCommandLineOption gainArg(QStringList() << "g" << "gain",
        QCoreApplication::translate("main", "Set device gain (default 0 for auto)"),
        QCoreApplication::translate("main", "deviceGain"),
        QCoreApplication::translate("main", "0"));
    parser->addOption(gainArg);

    QCommandLineOption samplerateArg(QStringList() << "s" << "samplerate",
        QCoreApplication::translate("main", "Set device sample rate (default 2048000)"),
        QCoreApplication::translate("main", "sampleRate"),
        QCoreApplication::translate("main", "2048000"));
    parser->addOption(samplerateArg);

    QCommandLineOption bufferArg(QStringList() << "b" << "buffers",
        QCoreApplication::translate("main", "Set number of buffers (default 32)"),
        QCoreApplication::translate("main", "numBuffers"),
        QCoreApplication::translate("main", "32"));
    parser->addOption(bufferArg);

    QCommandLineOption linkedBufferArg(QStringList() << "n" << "linkedbuffers",
        QCoreApplication::translate("main", "Set number of linked list buffers (default 500)"),
        QCoreApplication::translate("main", "numLinkedListBuffers"),
        QCoreApplication::translate("main", "500"));
    parser->addOption(linkedBufferArg);

    QCommandLineOption deviceIndexArg(QStringList() << "d" << "deviceindex",
        QCoreApplication::translate("main", "Set device index (default 0)"),
        QCoreApplication::translate("main", "deviceIndex"),
        QCoreApplication::translate("main", "0"));
    parser->addOption(deviceIndexArg);

    // Process the actual command line arguments given by the user
    parser->process(*qApp);

    //Required
    if (!parser->isSet(serverIPArg))
        parser->showHelp(0);

    //Post process
    hostAddress.setAddress(parser->value(serverIPArg));
    hostPort = parser->value(serverPortArg).toUInt();

}

//Called when we have new command data from server
void RtlTcpProtocol::commandWorker(char *data, qint64 numBytes)
{
    //Loop till we have full command and parameters
    quint16 bufIndex = 0;
    while (bufIndex < numBytes) {
        cmd.buf[cmdIndex++] = data[bufIndex++];
        if (cmdIndex > 4) {
            //Proces command
            //qDebug()<<"Command "<<cmd.cmd<<" Param "<<cmd.param;
            tcpCommands(cmd);
            cmdIndex = 0;
        }
    }
    //If we don't have a valid command byte, then keep sliding data until we get one
}

void RtlTcpProtocol::tcpCommands(RTL_CMD cmd)
{
    quint32 tmp;
    switch(cmd.cmd) {
    case CMD_FREQ:
        frequency = ntohl(cmd.param);
        printf("set freq %d\n", frequency);
        sdr->SetFrequency(frequency,frequency);
        break;

    case CMD_SAMPLERATE:
        sampleRate = ntohl(cmd.param);
        printf("set sample rate %d\n", sampleRate);
        sdr->SetKeyValue("KeyDeviceSampleRate",sampleRate);
        break;
    case CMD_GAIN_MODE:
        tunerGainMode = ntohl(cmd.param);
        printf("set gain mode %d\n", tunerGainMode);
        sdr->SetKeyValue("KeyTunerGainMode",tunerGainMode);
        break;
    case CMD_TUNER_GAIN:
        tunerGain = ntohl(cmd.param);
        printf("set gain %d\n", tunerGain);
        sdr->SetKeyValue("KeyTunerGain", tunerGain);
        break;
    case CMD_FREQ_CORRECTION:
        frequencyCorrection = ntohl(cmd.param);
        printf("set freq correction %d\n", frequencyCorrection);
        sdr->SetFrequencyCorrection(frequencyCorrection);
        break;
    case CMD_IF_GAIN:
        tmp = ntohl(cmd.param);
        printf("set if stage %d gain %d\n", tmp >> 16, (short)(tmp & 0xffff));
        //rtlsdr_set_tuner_if_gain(dev, tmp >> 16, (short)(tmp & 0xffff));
        break;
    case CMD_TEST_MODE:
        printf("set test mode %d\n", ntohl(cmd.param));
        //rtlsdr_set_testmode(dev, ntohl(cmd.param));
        break;
    case CMD_AGC_MODE:
        agcMode = ntohl(cmd.param);
        printf("set agc mode %d\n",agcMode );
        sdr->SetKeyValue("KeyAgcMode",agcMode);
        break;
    case CMD_DIRECT_SAMPLING:
        directSampling = ntohl(cmd.param);
        printf("set direct sampling %d\n", directSampling);
        sdr->SetKeyValue("KeySampleMode",directSampling);
        break;
    case CMD_OFFSET_TUNING:
        offsetTuning = ntohl(cmd.param);
        printf("set offset tuning %d\n", offsetTuning);
        sdr->SetKeyValue("KeyOffsetMode",offsetTuning);
        break;
    case CMD_XTAL_FREQ:
        printf("set rtl xtal %d\n", ntohl(cmd.param));
        //rtlsdr_set_xtal_freq(dev, ntohl(cmd.param), 0);
        break;
    case CMD_TUNER_XTAL:
        printf("set tuner xtal %d\n", ntohl(cmd.param));
        //rtlsdr_set_xtal_freq(dev, 0, ntohl(cmd.param));
        break;
    case CMD_TUNER_GAIN_BY_INDEX:
        printf("set tuner gain by index %d\n", ntohl(cmd.param));
        //set_gain_by_index(dev, ntohl(cmd.param));
        break;
    default:
        break;
    }
}
