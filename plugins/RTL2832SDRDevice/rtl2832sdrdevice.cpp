//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rtl2832sdrdevice.h"
#include "arpa/inet.h" //For ntohl()
/*
  rtl_tcp info
*/

RTL2832SDRDevice::RTL2832SDRDevice()
{
    InitSettings("rtl2832sdr");
    inBuffer = NULL;
    connected = false;
}

RTL2832SDRDevice::~RTL2832SDRDevice()
{
    Stop();
    Disconnect();
    if (inBuffer != NULL)
        delete (inBuffer);
}

QString RTL2832SDRDevice::GetPluginName(int _devNum)
{
    switch (_devNum) {
        case RTL_USB:
            return "RTL2832 USB";
            break;
        case RTL_TCP:
            return "RTL2832 TCP";
            break;
        default:
            qDebug()<<"Error: Unknown RTL device";
            return "Error";
            break;

    }
}

QString RTL2832SDRDevice::GetPluginDescription(int _devNum)
{
    switch (_devNum) {
        case RTL_USB:
            return "RTL2832 USB Devices";
            break;
        case RTL_TCP:
            return "RTL2832 TCP Servers";
            break;
        default:
            qDebug()<<"Error: Unknown RTL device";
            return "Error";
            break;

    }
}

bool RTL2832SDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
    ProcessIQData = _callback;
    framesPerBuffer = _framesPerBuffer;
    inBuffer = new CPXBuf(framesPerBuffer);

    //crystalFreqHz = DEFAULT_CRYSTAL_FREQUENCY;
    //RTL2832 samples from 900001 to 3200000 sps (900ksps to 3.2msps)
    //1 msps seems to be optimal according to boards
    //We have to decimate from rtl rate to our rate for everything to work

    //Make rtlSample rate close to 1msps but with even decimate
    //Test with higher sps, seems to work better
    /*
      RTL2832 sample rate likes to be 2.048, 1.024 or an even quotient of 28.8mhz
      Other rates may cause intermittent sync problems
      2048000 is the default rate for DAB and FM
      3.20 (28.8 / 9)
      2.88 (28.8 / 10)
      2.40 (28.8 / 12)
      1.80 (28.8 / 16)
      1.44 (28.8 / 20) even dec at 48 and 96k
      1.20 (28.8 / 24)
      1.152 (28.8 / 25) 192k * 6 - This is our best rate convert to 192k effective rate
       .96 (28.8 / 30)
    */
    /*
    range += osmosdr::range_t( 250000 ); // known to work
      range += osmosdr::range_t( 1000000 ); // known to work
      range += osmosdr::range_t( 1024000 ); // known to work
      range += osmosdr::range_t( 1800000 ); // known to work
      range += osmosdr::range_t( 1920000 ); // known to work
      range += osmosdr::range_t( 2000000 ); // known to work
      range += osmosdr::range_t( 2048000 ); // known to work
      range += osmosdr::range_t( 2400000 ); // known to work
    //  range += osmosdr::range_t( 2600000 ); // may work
    //  range += osmosdr::range_t( 2800000 ); // may work
    //  range += osmosdr::range_t( 3000000 ); // may work
    //  range += osmosdr::range_t( 3200000 ); // max rate
    */
    //Different sampleRates for different RTL rates
    rtlSampleRate = 2.048e6; //We can keep up with Spectrum

    //rtlSampleRate = 1.024e6;

    rtlDecimate = rtlSampleRate / sampleRate; //Must be even number, convert to lookup table
    /*
    //Find whole number decimate rate less than 2048000
    rtlDecimate = 1;
    rtlSampleRate = 0;
    quint32 tempRtlSampleRate = 0;
    while (tempRtlSampleRate <= 1800000) {
        rtlSampleRate = tempRtlSampleRate;
        tempRtlSampleRate = sampleRate * ++rtlDecimate;
    }
*/
    rtlFrequency = 162400000;

    sampleGain = .005; //Matched with rtlGain

    return true;
}

bool RTL2832SDRDevice::Connect()
{
    if (deviceNumber == RTL_USB) {
        int device_count;
        int dev_index = 0; //Assume we only have 1 RTL2832 device for now
        //dev = NULL;
        device_count = rtlsdr_get_device_count();
        if (device_count == 0) {
            qDebug("No supported devices found.");
            return false;
        }

        //for (int i = 0; i < device_count; i++)
        //    qDebug("%s",rtlsdr_get_device_name(i));

        //rtlsdr_get_device_name(dev_index));

        if (rtlsdr_open(&dev, dev_index) < 0) {
            qDebug("Failed to open rtlsdr device #%d", dev_index);
            return false;
        }
        connected = true;
        return true;
    } else if (deviceNumber == RTL_TCP) {
        rtlTcpSocket = new QTcpSocket();

        //rtlServerIP.setAddress("192.168.0.300:1234");
        rtlTcpSocket->connectToHost("192.168.0.121",1234,QTcpSocket::ReadWrite);
        if (!rtlTcpSocket->waitForConnected(3000)) {
            //Socket was closed or error
            qDebug()<<"RTL Server error "<<rtlTcpSocket->errorString();
            return false;
        }
        qDebug()<<"RTL Server connected";
        //Handle signals from socket or use blocking???

        //rtlServerSocket->writeData("",0);

        connected = true;
        return true;
    }
    return false;
}

void RTL2832SDRDevice::TCPSocketError()
{

}

void RTL2832SDRDevice::TCPSocketConnected()
{

}

bool RTL2832SDRDevice::Disconnect()
{
    if (!connected)
        return false;

    if (deviceNumber == RTL_USB) {
        rtlsdr_close(dev);
    } else if (deviceNumber == RTL_TCP) {
        rtlTcpSocket->disconnectFromHost();
        //Wait for disconnect?
    }
    connected = false;
    dev = NULL;
    return true;
}

void RTL2832SDRDevice::Start()
{
    int r;

    if (deviceNumber == RTL_USB) {
        /* Set the sample rate */
        if (rtlsdr_set_sample_rate(dev, rtlSampleRate) < 0) {
            qDebug("WARNING: Failed to set sample rate.");
            return;
        }

        //Center freq is set in SetFrequency()

        /* Set the tuner gain */
        //Added support for automatic gain from rtl-sdr.c
        if (rtlGain == 0) {
             /* Enable automatic gain */
            r = rtlsdr_set_tuner_gain_mode(dev, GAIN_MODE_AUTO);
            if (r < 0) {
                qDebug("WARNING: Failed to enable automatic gain.");
                return;
            } else {
                qDebug("Automatic gain set");
            }
        } else {
            /* Enable manual gain */
            r = rtlsdr_set_tuner_gain_mode(dev, GAIN_MODE_MANUAL);
            if (r < 0) {
                qDebug("WARNING: Failed to enable manual gain.");
                return;
            }

            /* Set the tuner gain */
            r = rtlsdr_set_tuner_gain(dev, rtlGain);
            if (r < 0) {
                qDebug("WARNING: Failed to set tuner gain.");
                return;
            }else {
                qDebug("Tuner gain set to %f dB.", rtlGain/10.0);
            }
        }

        /* Reset endpoint before we start reading from it (mandatory) */
        if (rtlsdr_reset_buffer(dev) < 0) {
            qDebug("WARNING: Failed to reset buffers.");
            return;
        }        
    } else if (deviceNumber == RTL_TCP) {
        SendTcpCmd(TCP_SET_SAMPLERATE,rtlSampleRate);
        if (rtlGain == 0) {
             /* Enable automatic gain */
            SendTcpCmd(TCP_SET_GAIN_MODE, GAIN_MODE_AUTO);
        } else {
            SendTcpCmd(TCP_SET_GAIN_MODE, GAIN_MODE_MANUAL); //Manual
            SendTcpCmd(TCP_SET_IF_GAIN,rtlGain);
        }
    }
    //1 byte per I + 1 byte per Q
    producerConsumer.Initialize(this, 50, framesPerBuffer * rtlDecimate * 2, 0);
    producerConsumer.Start();

    return;
}

void RTL2832SDRDevice::Stop()
{
    if (deviceNumber == RTL_USB) {

    } else if (deviceNumber == RTL_TCP) {

    }
    producerConsumer.Stop();
}

double RTL2832SDRDevice::SetFrequency(double fRequested,double fCurrent)
{
    if (deviceNumber == RTL_USB) {
        /* Set the frequency */
        if (rtlsdr_set_center_freq(dev, fRequested) < 0) {
            qDebug("WARNING: Failed to set center freq.");
            rtlFrequency = fCurrent;
        } else {
            rtlFrequency = fRequested;
        }

        return rtlFrequency;
    } else if (deviceNumber == RTL_TCP) {
        if (SendTcpCmd(TCP_SET_FREQ,fRequested))
            return fRequested;
        else
            return fCurrent;
    }
}

void RTL2832SDRDevice::ShowOptions()
{

}

void RTL2832SDRDevice::ReadSettings()
{
    //Valid gain values (in tenths of a dB) for the E4000 tuner:
    //-10, 15, 40, 65, 90, 115, 140, 165, 190,
    //215, 240, 290, 340, 420, 430, 450, 470, 490
    //0 for automatic gain
    rtlGain = qSettings->value("RtlGain",0).toInt(); //0=automatic

}

void RTL2832SDRDevice::WriteSettings()
{
    qSettings->setValue("RtlGain",rtlGain);
    qSettings->sync();

}

double RTL2832SDRDevice::GetStartupFrequency()
{
    return 162450000;
}

int RTL2832SDRDevice::GetStartupMode()
{
    return dmFMN;
}

/*
Elonics E4000	 52 - 2200 MHz with a gap from 1100 MHz to 1250 MHz (varies)
Rafael Micro R820T	24 - 1766 MHz
Fitipower FC0013	22 - 1100 MHz (FC0013B/C, FC0013G has a separate L-band input, which is unconnected on most sticks)
Fitipower FC0012	22 - 948.6 MHz
FCI FC2580	146 - 308 MHz and 438 - 924 MHz (gap in between)
*/
double RTL2832SDRDevice::GetHighLimit()
{
    if (dev==NULL)
        return 1700000000;

    if (deviceNumber == RTL_TCP) {
        return 1700000000;
    }
    switch (rtlsdr_get_tuner_type(dev)) {
        case RTLSDR_TUNER_E4000:
            return 2200000000;
        case RTLSDR_TUNER_FC0012:
            return 948600000;
        case RTLSDR_TUNER_FC0013:
            return 1100000000;
        case RTLSDR_TUNER_FC2580:
            return 146000000;
        case RTLSDR_TUNER_R820T:
            return 1766000000;
        default:
            return 1700000000;
    }
}

double RTL2832SDRDevice::GetLowLimit()
{
    if (dev==NULL)
        return 64000000;
    if (deviceNumber == RTL_TCP) {
        return 64000000;
    }
    switch (rtlsdr_get_tuner_type(dev)) {
        case RTLSDR_TUNER_E4000:
            return 52000000;
        case RTLSDR_TUNER_FC0012:
            return 22000000;
        case RTLSDR_TUNER_FC0013:
            return 22000000;
        case RTLSDR_TUNER_FC2580:
            return 146000000;
        case RTLSDR_TUNER_R820T:
            return 24000000;
        default:
            return 64000000;
    }
}

double RTL2832SDRDevice::GetGain()
{
    return 1;
}

QString RTL2832SDRDevice::GetDeviceName()
{
    if (dev==NULL)
        return "No Device Found";

    if (deviceNumber == RTL_USB) {
        switch (rtlsdr_get_tuner_type(dev)) {
            case RTLSDR_TUNER_E4000:
                return "RTL2832-E4000";
            case RTLSDR_TUNER_FC0012:
                return "RTL2832-FC0012";
            case RTLSDR_TUNER_FC0013:
                return "RTL2832-FC0013";
            case RTLSDR_TUNER_FC2580:
                return "RTL2832-FC2580";
            case RTLSDR_TUNER_R820T:
                return "RTL2832-R820T";
            default:
                return "RTL2832-Unknown";
        }
    } else if (deviceNumber == RTL_TCP) {
        return "TCP";
    }
}

int RTL2832SDRDevice::GetSampleRate()
{
    return sampleRate;
}

int *RTL2832SDRDevice::GetSampleRates(int &len)
{
    len = 6;
    //Ugly, but couldn't find easy way to init with {1,2,3} array initializer
    sampleRates[0] = 64000;
    sampleRates[1] = 128000;
    sampleRates[2] = 256000;
    sampleRates[3] = 512000;
    sampleRates[4] = 1024000;
    sampleRates[5] = 2048000;
    return sampleRates;

}

bool RTL2832SDRDevice::UsesAudioInput()
{
    return false;
}

bool RTL2832SDRDevice::GetTestBenchChecked()
{
    return isTestBenchChecked;
}

void RTL2832SDRDevice::SetupOptionUi(QWidget *parent)
{
    //Nothing to do
}

bool RTL2832SDRDevice::SendTcpCmd(quint8 _cmd, quint32 _data)
{
    RTL_CMD cmd;
    cmd.cmd = _cmd;
    //ntohl() is what rtl-tcp uses to convert to network byte order
    //So we use inverse htonl (hardware to network long)
    cmd.data = htonl(_data);
    quint16 bytesWritten = rtlTcpSocket->write((const char *)&cmd, sizeof(cmd));
    //QTcpSocket is always buffered, no choice.
    //So we need to either call an explicit flush() or waitForBytesWritten() to make sure data gets sent
    //Not sure which one is better or if it makes a difference
    //rtlTcpSocket->flush();
    rtlTcpSocket->waitForBytesWritten(1000);
    if (bytesWritten != sizeof(cmd)) {
        //qDebug()<<"Error in TcpSocket write";
        return false;
    } else {
        //qDebug()<<"TcpSocket write ok";
        return true;
    }
}

void RTL2832SDRDevice::StopProducerThread(){}
void RTL2832SDRDevice::RunProducerThread()
{
    int bytesRead;
    producerConsumer.AcquireFreeBuffer();

    if (deviceNumber == RTL_TCP) {
        //This assumes we get a full buffer every time
        //Todo: Build up buffer like we do for AFEDRI
        bytesRead = rtlTcpSocket->read(producerConsumer.GetProducerBufferAsChar(),producerConsumer.GetBufferSize());
        if (bytesRead != producerConsumer.GetBufferSize()) {
            //No data, or not enough data
            producerConsumer.ReleaseFreeBuffer(); //Put back buffer for next try
            return;
        }
    } else if (deviceNumber == RTL_USB) {
        //Insert code to put data from device in buffer
        if (rtlsdr_read_sync(dev, producerConsumer.GetProducerBuffer(), producerConsumer.GetBufferSize(), &bytesRead) < 0) {
            qDebug("Sync transfer error");
            producerConsumer.ReleaseFreeBuffer(); //Put back buffer for next try
            return;
        }

        if (bytesRead < producerConsumer.GetBufferSize()) {
            qDebug("Under read");
            producerConsumer.ReleaseFreeBuffer(); //Put back buffer for next try
            return;
        }
    }
    producerConsumer.SupplyProducerBuffer();
    producerConsumer.ReleaseFilledBuffer();    
}

void RTL2832SDRDevice::StopConsumerThread()
{
    //Problem - RunConsumerThread may be in process when we're asked to stopp
    //We have to wait for it to complete, then return.  Bad dependency - should not have tight connection like this
}

void RTL2832SDRDevice::RunConsumerThread()
{
    //Wait for data to be available from producer
    producerConsumer.AcquireFilledBuffer();
    double fpSampleRe;
    double fpSampleIm;


    //RTL I/Q samples are 8bit unsigned 0-256
    int bufInc = rtlDecimate * 2;
    int decMult = 1; //1=8bit, 2=9bit, 3=10bit for rtlDecimate = 6 and sampleRate = 192k
    int decNorm = 128 * decMult;
    double decAvg = rtlDecimate / decMult; //Double the range = 1 bit of sample size
    for (int i=0,j=0; i<framesPerBuffer; i++, j+=bufInc) {
#if 1
        //We are significantly oversampling, so we can use decimation to increase dynamic range
        //See http://www.actel.com/documents/Improve_ADC_WP.pdf as one example
        //We take N (rtlDecimate) samples and create one result
        fpSampleRe = fpSampleIm = 0.0;
        for (int k = 0; k < bufInc; k+=2) {
            //I/Q reversed from normal devices, correct here
            fpSampleIm += producerConsumer.GetConsumerBufferDataAsDouble(j + k);
            fpSampleRe += producerConsumer.GetConsumerBufferDataAsDouble(j + k + 1);
        }
        //If we average, we get a better sample
        //But if we average with a smaller number, we increase range of samples
        //Instead of 8bit 0-255, we get 9bit 0-511
        //Testing assuming rtlDecmiate = 6
        fpSampleRe = fpSampleRe / decAvg; //Effectively increasing dynamic range
        fpSampleRe -= decNorm;
        fpSampleRe /= decNorm;
        fpSampleRe *= sampleGain;
        fpSampleIm = fpSampleIm / decAvg;
        fpSampleIm -= decNorm;
        fpSampleIm /= decNorm;
        fpSampleIm *= sampleGain;
#else
        //Oringal simple decimation - no increase in dynamic range
        //Every nth sample from producer buffer
        fpSampleRe = (double)producerBuffer[nextConsumerDataBuf][j];
        fpSampleRe -= 127.0;
        fpSampleRe /= 127.0;
        fpSampleRe *= sampleGain;

        fpSampleIm = (double)producerBuffer[nextConsumerDataBuf][j+1];
        fpSampleIm -= 127.0;
        fpSampleIm /= 127.0;
        fpSampleIm *= sampleGain;
#endif
        inBuffer->Re(i) = fpSampleRe;
        inBuffer->Im(i) = fpSampleIm;
    }

    //We're done with databuf, so we can release before we call ProcessBlock
    //Update lastDataBuf & release dataBuf
    producerConsumer.SupplyConsumerBuffer();
    producerConsumer.ReleaseFreeBuffer();

    ProcessIQData(inBuffer->Ptr(),framesPerBuffer);



}

//These need to be moved to a DeviceInterface implementation class, equivalent to SDR
//Must be called from derived class constructor to work correctly
void RTL2832SDRDevice::InitSettings(QString fname)
{
    //Use ini files to avoid any registry problems or install/uninstall
    //Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
    //Scope::SystemScope puts file c:\ProgramData\n1ddy

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = new QSettings(path + "/PebbleData/" + fname +".ini",QSettings::IniFormat);

}
