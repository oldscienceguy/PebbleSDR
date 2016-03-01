/*
 * Adopted from rtl-tcp for Qt and use with any Pebble device plugin, not just rtlsdr's
*/

#include "rtltcpprotocol.h"
#include "sdrserver.h"

#include <QCoreApplication>

RtlTcpProtocol::RtlTcpProtocol(SdrServer *_server, DeviceInterface *_sdr)
{
    sdrServer = _server;
    sdr = _sdr;
    cmdIndex = 0;
    threadSocket = NULL;
}

//Called when connection is lost or terminated
void RtlTcpProtocol::Reset()
{
    if (threadSocket != NULL) {
        threadSocket->close();
        delete threadSocket;
        threadSocket = NULL;
    }
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
}

void RtlTcpProtocol::tcpCommands(RTL_CMD cmd)
{
    quint32 tmp;
    switch(cmd.cmd) {
    case CMD_FREQ:
        frequency = ntohl(cmd.param);
        qDebug()<<"set freq"<< frequency;
		sdr->Set(DeviceInterface::DeviceFrequency,frequency);
        break;

    case CMD_SAMPLERATE:
        sampleRate = ntohl(cmd.param);
        qDebug()<<"set sample rate"<<sampleRate;
		sdr->Set(K_RTLSampleRate,sampleRate);
        break;
    case CMD_GAIN_MODE:
        tunerGainMode = ntohl(cmd.param);
        qDebug()<<"set gain mode"<<tunerGainMode;
		sdr->Set(K_RTLTunerGainMode,tunerGainMode);
        break;
    case CMD_TUNER_GAIN:
        tunerGain = ntohl(cmd.param);
        qDebug()<<"set gain"<<tunerGain;
		sdr->Set(K_RTLTunerGain, tunerGain);
        break;
    case CMD_FREQ_CORRECTION:
        frequencyCorrection = ntohl(cmd.param);
        qDebug()<<"set freq correction"<<frequencyCorrection;
		sdr->Set(DeviceInterface::DeviceFreqCorrectionPpm,frequencyCorrection);
        break;
    case CMD_IF_GAIN:
        tmp = ntohl(cmd.param);
        qDebug()<<"set if stage "<< (tmp >> 16) <<"gain "<< (short)(tmp & 0xffff);
        //rtlsdr_set_tuner_if_gain(dev, tmp >> 16, (short)(tmp & 0xffff));
        break;
    case CMD_TEST_MODE:
        qDebug()<<"set test mode " <<ntohl(cmd.param);
        //rtlsdr_set_testmode(dev, ntohl(cmd.param));
        break;
    case CMD_AGC_MODE:
        agcMode = ntohl(cmd.param);
        qDebug()<<"set agc mode "<<agcMode;
		sdr->Set(K_RTLAgcMode,agcMode);
        break;
    case CMD_DIRECT_SAMPLING:
        directSampling = ntohl(cmd.param);
        qDebug()<<"set direct sampling "<<directSampling;
		sdr->Set(K_RTLSampleMode,directSampling);
        break;
    case CMD_OFFSET_TUNING:
        offsetTuning = ntohl(cmd.param);
        qDebug()<<"set offset tuning "<<offsetTuning;
		sdr->Set(K_RTLOffsetMode,offsetTuning);
        break;
    case CMD_XTAL_FREQ:
        qDebug()<<"set rtl xtal "<<ntohl(cmd.param);
        //rtlsdr_set_xtal_freq(dev, ntohl(cmd.param), 0);
        break;
    case CMD_TUNER_XTAL:
        qDebug()<<"set tuner xtal "<<ntohl(cmd.param);
        //rtlsdr_set_xtal_freq(dev, 0, ntohl(cmd.param));
        break;
    case CMD_TUNER_GAIN_BY_INDEX:
        qDebug()<<"set tuner gain by index "<<ntohl(cmd.param);
        //set_gain_by_index(dev, ntohl(cmd.param));
        break;
    default:
        break;
    }
}

//This is protocol specific, but we don't want server knowledge in protocol
void RtlTcpProtocol::ProcessIQData(CPX *in, quint16 numSamples)
{
    //This is being called by DeviceInterface consumer thread and has problems with cross thread socket usage
    if (threadSocket == NULL) {
        //First time and we need to create local thread
        threadSocket = new QTcpSocket();
        threadSocket->setSocketDescriptor(sdrServer->GetSocketDescriptor());
    }
    //Testing - reversing exactly what we do in consumer for device
    double sampleGain = 1/128.0;
    //Stupid, but we need to convert to 0 to 255 1 byte samples rtl_tcp generates
    for (int i=0, j=0; i<numSamples; i++, j+=2) {
        out[j] = (in[i].re / sampleGain) * 128.0 + 127.0;
        out[j+1] = (in[i].im /sampleGain) * 128.0 + 127.0;
    }

    qint64 bytesWritten = threadSocket->write(out, numSamples*2);
	Q_UNUSED(bytesWritten);
}
