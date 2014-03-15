//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rtl2832sdrdevice.h"
#include "arpa/inet.h" //For ntohl()
/*
  rtl_tcp info
*/

RTL2832SDRDevice::RTL2832SDRDevice():DeviceInterfaceBase()
{
    InitSettings("rtl2832sdr");
    inBuffer = NULL;
    running = false;
    optionUi = NULL;
    rtlTcpSocket = NULL;
    tcpThreadSocket = NULL;
    producerFreeBufPtr = NULL;
    readBufferIndex = 0;
    rtlTunerGainCount = 0;
}

RTL2832SDRDevice::~RTL2832SDRDevice()
{
    Stop();
    Disconnect();
    if (inBuffer != NULL)
        delete (inBuffer);
}

bool RTL2832SDRDevice::Initialize(cbProcessIQData _callback,
								   cbProcessBandscopeData _callbackSpectrum,
								   cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callbackSpectrum);
	Q_UNUSED(_callbackAudio);
	DeviceInterfaceBase::Initialize(_callback, _callbackSpectrum, _callbackAudio, _framesPerBuffer);
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
      2.048 (works)
      1.92 (28.8 / 15)
      1.80 (28.8 / 16)
      1.60 (28.8 / 18)
      1.44 (28.8 / 20) even dec at 48 and 96k
      1.20 (28.8 / 24)
      1.152 (28.8 / 25) 192k * 6 - This is our best rate convert to 192k effective rate
      1.024 (works)
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
    //rtlSampleRate = 2.048e6; //We can keep up with Spectrum
    //rtlSampleRate = 1.024e6;

    //sampleRate must <= to rtlSampleRate, find closest /2 match
    sampleRate = (sampleRate <= rtlSampleRate) ? sampleRate : sampleRate/2;


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
	//1 byte per I + 1 byte per Q
	//This is set so we always get framesPerBuffer samples after decimating to lower sampleRate
	readBufferSize = framesPerBuffer * rtlDecimate * 2;

    //sampleGain = .005; //Matched with rtlGain
    sampleGain = 1/128.0;

    haveDongleInfo = false; //Look for first bytes
    tcpDongleInfo.magic[0] = 0x0;
    tcpDongleInfo.magic[1] = 0x0;
    tcpDongleInfo.magic[2] = 0x0;
    tcpDongleInfo.magic[3] = 0x0;

    tcpDongleInfo.tunerType = RTLSDR_TUNER_UNKNOWN;
    tcpDongleInfo.tunerGainCount = 0;

    running = false;
    connected = false;

    numProducerBuffers = 50;
    producerConsumer.Initialize(std::bind(&RTL2832SDRDevice::producerWorker, this, std::placeholders::_1),
        std::bind(&RTL2832SDRDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	producerConsumer.SetProducerInterval(rtlSampleRate,readBufferSize);
	producerConsumer.SetConsumerInterval(rtlSampleRate,readBufferSize);

    readBufferIndex = 0;

    if (deviceNumber == RTL_TCP) {
        //Start consumer thread so it's ready to get data which starts as soon as we are connected
        producerConsumer.Start(false,true);
        if (rtlTcpSocket == NULL) {
            rtlTcpSocket = new QTcpSocket();

            //QTcpSocket can have strange behavior if shared between threads
            //We use the signals emitted by QTcpSocket to get new data, instead of polling in a separate thread
            //As a result, there is no producer thread in the TCP case.
            //Set up state change connections
            connect(rtlTcpSocket,&QTcpSocket::connected, this, &RTL2832SDRDevice::TCPSocketConnected);
            connect(rtlTcpSocket,&QTcpSocket::disconnected, this, &RTL2832SDRDevice::TCPSocketDisconnected);
            connect(rtlTcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(TCPSocketError(QAbstractSocket::SocketError)));
            //QT TCP classes are implemented as non blocking using signals/slots
            //Recommended usage is to use these instead of separate threads whereever possible because QTcpSockets have
            //to be managed carefully across threads
            //We'll support both modes so we have an example of socket use across threads, but default to signal/slot

            connect(rtlTcpSocket,&QTcpSocket::readyRead, this, &RTL2832SDRDevice::TCPSocketNewData);
            //test
            connect(this,&RTL2832SDRDevice::reset,this,&RTL2832SDRDevice::Reset);
        }
    } else if (deviceNumber == RTL_USB) {
        //Start this immediately, before connect, so we don't miss any data
        producerConsumer.Start(true,true);

    }

    producerFreeBufPtr = NULL;

    return true;
}

//Restart everything with current values, same as power off then power on
//Should be part of DeviceInterface?
void RTL2832SDRDevice::Reset()
{
    qDebug()<<"RTL reset signal received";
    Stop();
    Disconnect();
	Initialize(ProcessIQData, NULL, NULL, framesPerBuffer);
    Connect();
    Start();
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

        //The socket buffer is infinite by default, it will keep growing if we're not able to fetch data fast enough
        //So we set it to a fixed size, which is the maximum amount of data that can be fetched in one read call
        //We can try differnt buffer sizes here to keep up with higher sample rates
        //If too large, we may have problems processing all the data before the next batch comes in.
        //Setting to 1 framesPerBuffer seems to work best
        //If problems, try setting to some multiple that less than numProducerBuffers
        rtlTcpSocket->setReadBufferSize(readBufferSize);

        //rtl_tcp server starts to dump data as soon as there is a connection, there is no start/stop command
        rtlTcpSocket->connectToHost(rtlServerIP,rtlServerPort,QTcpSocket::ReadWrite);
        if (!rtlTcpSocket->waitForConnected(3000)) {
            //Socket was closed or error
            qDebug()<<"RTL Server error "<<rtlTcpSocket->errorString();
            return false;
        }
        qDebug()<<"RTL Server connected";
        connected = true;

        return true;
    }
    return false;
}

void RTL2832SDRDevice::TCPSocketError(QAbstractSocket::SocketError socketError)
{
    qDebug()<<"TCP Socket Error "<<socketError;
}

void RTL2832SDRDevice::TCPSocketConnected()
{
    //Nothing to do, we waiting for connection in Connect()
    qDebug()<<"TCP connected signal received";
}

void RTL2832SDRDevice::TCPSocketDisconnected()
{
    //Get get this signal in 2 cases
    // -When we turn off device
    // -When device is disconnected due to network or other failure
    //If device is running, try to reset
    if (running) {
        emit reset();
    }
    qDebug()<<"TCP disconnected signal received";
}

//This can be called by connecting to readyRead signal, or from Producer thread
//When this is called, we know we have data
void RTL2832SDRDevice::TCPSocketNewData()
{
    //Do we really need to protect the socket?
    rtlTcpSocketMutex.lock();

    qint64 bytesRead;
    //We have to process all the data we have because we limit buffer size
    //We only get one readRead() signal for each block of data that's ready

    qint64 bytesAvailable = rtlTcpSocket->bytesAvailable();
    if (bytesAvailable == 0) {
        rtlTcpSocketMutex.unlock();
        return;
    }

    //qDebug()<<"Bytes available "<<bytesAvailable;

    //  Even though we keep up with incoming at 2msps in USB mode, and rtl-tcp server says we're keeping up in TCP mode
    //  FreeBufs goes right to zero and mostly stays there.
    //  Could be a problem with QT network threads interfering with ProducerConsumer consumer thread?
    //  Tried all sorts of yeilds, etc, but no joy as of 1/19/14
	//qDebug()<<producerConsumer.GetNumFreeBufs();


    //The first bytes we get from rtl_tcp are dongle information
    //This comes immediately after connect, before we start running
    if (!haveDongleInfo && bytesAvailable >= (qint64)sizeof(DongleInfo)) {
        rtlTunerType = RTLSDR_TUNER_UNKNOWN; //Default if we don't get valid data
        bytesRead = rtlTcpSocket->peek((char *)&tcpDongleInfo,sizeof(DongleInfo));
        haveDongleInfo = true; //We only try once for this after we read some bytes

        if (bytesRead == sizeof(DongleInfo)) {
            //If we got valid data, then use internally
            if (tcpDongleInfo.magic[0] == 'R' &&
                    tcpDongleInfo.magic[1] == 'T' &&
                    tcpDongleInfo.magic[2] == 'L' &&
                    tcpDongleInfo.magic[3] == '0') {
                //We have valid data
                rtlTunerType = (RTLSDR_TUNERS)ntohl(tcpDongleInfo.tunerType);
                rtlTunerGainCount = ntohl(tcpDongleInfo.tunerGainCount);
                qDebug()<<"Dongle type "<<rtlTunerType;
                qDebug()<<"Dongle gain count "<< rtlTunerGainCount;
            }
        }

    }

    quint16 bytesToFillBuffer;
    while (bytesAvailable > 0) {
        if (producerFreeBufPtr == NULL) {
            //Starting a new buffer
            //We wait for a free buffer for 100ms before giving up.  May need to adjust this
            if ((producerFreeBufPtr = (char*)producerConsumer.AcquireFreeBuffer(1000)) == NULL) {
                rtlTcpSocketMutex.unlock();
                //We're sol in this situation because we won't get another readReady() signal
                //emit reset(); //Start over
                return;
            }
            //qDebug()<<"Acquired free buffer";
            //qDebug()<<producerConsumer.IsFreeBufferOverflow();
        }
        bytesToFillBuffer = readBufferSize - readBufferIndex;
        bytesToFillBuffer = (bytesToFillBuffer <= bytesAvailable) ? bytesToFillBuffer : bytesAvailable;
        bytesRead = rtlTcpSocket->read(producerFreeBufPtr + readBufferIndex, bytesToFillBuffer);
        bytesAvailable -= bytesRead;
        readBufferIndex += bytesRead;
        readBufferIndex %= readBufferSize;
        if (readBufferIndex == 0) {
            producerConsumer.ReleaseFilledBuffer();
            producerFreeBufPtr = NULL; //Trigger new Acquire next loop
		}

    }

    rtlTcpSocketMutex.unlock();

}

bool RTL2832SDRDevice::Disconnect()
{
    if (!connected)
        return false;

    if (deviceNumber == RTL_USB) {
        rtlsdr_close(dev);
    } else if (deviceNumber == RTL_TCP) {
        rtlTcpSocket->disconnectFromHost();
        //Wait for disconnect
        //if (!rtlTcpSocket->waitForDisconnected(500)) {
        //    qDebug()<<"rtlTcpSocket didn't disconnect within timeout";
        //}
    }
    connected = false;
    dev = NULL;
    return true;
}

void RTL2832SDRDevice::Start()
{
    //producerConsumer.
    if (deviceNumber == RTL_USB) {
        rtlTunerType = (RTLSDR_TUNERS) rtlsdr_get_tuner_type(dev);

        /* Reset endpoint before we start reading from it (mandatory) */
        if (rtlsdr_reset_buffer(dev) < 0) {
            qDebug("WARNING: Failed to reset buffers.");
            return;
        }
    } else if (deviceNumber == RTL_TCP) {
        //Nothing to do here
    }

    //These have to be executed after we've done USB or TCP specific setup
    //Handles both USB and TCP
    SetRtlSampleMode(rtlSampleMode);
    SetRtlSampleRate(rtlSampleRate);
    SetRtlAgcMode(rtlAgcMode);
    GetRtlValidTunerGains();
    SetRtlTunerMode(rtlTunerGainMode);
    SetRtlTunerGain(rtlTunerGain);
    //SetRtlIfGain(1,rtlIfGain);
    SetRtlFrequencyCorrection(rtlFreqencyCorrection);

    //Setting running=true is what tells the producer thread to start collecting data
    running = true; //Must be after reset

    return;
}

//Stop is called by Reciever first, then Disconnect
//Stop incoming samples
void RTL2832SDRDevice::Stop()
{
    running = false;

    if (deviceNumber == RTL_USB) {

    } else if (deviceNumber == RTL_TCP) {

    }
    producerConsumer.Stop();
}

bool RTL2832SDRDevice::SetRtlFrequencyCorrection(qint16 _correction)
{
    if (!connected)
        return false;

    //Trying to set a correction of zero fails, so ignore
    if (_correction == 0)
        return true;

    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_freq_correction(dev,_correction);
        if (r<0) {
            qDebug()<<"Frequency correction failed";
            return false;
        }

    } else if (deviceNumber == RTL_TCP) {
        SendTcpCmd(TCP_SET_FREQ_CORRECTION,_correction);
    }
    return false;
}

bool RTL2832SDRDevice::SetRtlSampleMode(RTL2832SDRDevice::SAMPLING_MODES _sampleMode)
{
    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_direct_sampling(dev,_sampleMode);
        if (r<0) {
            qDebug()<<"SampleMode failed";
            return false;
        }

    } else {
        return SendTcpCmd(TCP_SET_DIRECT_SAMPLING,_sampleMode);
    }
    return false;
}

bool RTL2832SDRDevice::SetRtlAgcMode(bool _on)
{
    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_agc_mode(dev, _on);
        if (r<0) {
            qDebug()<<"AGC mode failed";
            return false;
        }

    } else {
        return SendTcpCmd(TCP_SET_AGC_MODE, _on);
    }
    return false;
}

bool RTL2832SDRDevice::SetRtlOffsetMode(bool _on)
{
    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_offset_tuning(dev, _on);
        if (r<0) {
            qDebug()<<"Offset mode not supported for this device";
            return false;
        }

    } else {
        return SendTcpCmd(TCP_SET_OFFSET_TUNING, _on);
    }
    return false;

}

bool RTL2832SDRDevice::SetRtlTunerGain(quint16 _gain)
{
    if (!connected)
        return false;

    qint16 gain = _gain;
    if (rtlTunerGainMode == GAIN_MODE_MANUAL) {
        //Find closed supported gain
        qint16 delta;
        qint16 lastDelta = 10000;
        qint16 lastGain = 0;
        //Gains in UI are generic, we have to find closest gain that device actually supports
        //Loop through valid gains until we get the smallest delta
        for (int i=0; i<rtlTunerGainCount; i++) {
            gain = rtlTunerGains[i];
            delta = abs(gain - _gain); //Abs delta from requested gain
            //qDebug()<<"Requested gain "<<_gain<<" Checking tunerGains[i] "<<gain<<" Delta "<<delta;
            if ( delta >= lastDelta) {
                //Done
                gain = lastGain;
                break;
            }
            lastGain = rtlTunerGains[i];
            lastDelta = delta;
        }
    }
    if (deviceNumber == RTL_USB) {

        if (rtlTunerGainMode == GAIN_MODE_MANUAL){
            /* Set the tuner gain */
            int r = rtlsdr_set_tuner_gain(dev, gain);
            if (r < 0) {
                qDebug("WARNING: Failed to set tuner gain.");
                return false;
            }
            int actual = rtlsdr_get_tuner_gain(dev);
            qDebug()<<"Requested tuner gain "<<_gain<<" Actual tuner gain "<<actual;
        }
        return true;
    } else if (deviceNumber == RTL_TCP) {
        if (rtlTunerGainMode == GAIN_MODE_MANUAL) {
            SendTcpCmd(TCP_SET_TUNER_GAIN, gain);
            //When do we need to set IF Gain?
            //SendTcpCmd(TCP_SET_IF_GAIN, gain);
        }
    }
    return false;
}

bool RTL2832SDRDevice::SetRtlTunerMode(quint16 _mode)
{
    if (!connected)
        return false;
    if (rtlSampleMode != NORMAL) {
        //Tuner AGC mode is disabled in Direct, have to set manually
        _mode = GAIN_MODE_MANUAL;
    }
    if (deviceNumber == RTL_USB) {

        int r = rtlsdr_set_tuner_gain_mode(dev, _mode);
        if (r < 0) {
            qDebug("WARNING: Failed to set gain mode.");
            return false;
        }
    } else {
        SendTcpCmd(TCP_SET_GAIN_MODE, _mode);
    }
    return true;
}

bool RTL2832SDRDevice::SetRtlIfGain(quint16 _stage, quint16 _gain)
{
    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_tuner_if_gain(dev, _stage, _gain);
        if (r < 0) {
            qDebug("WARNING: Failed to set IF gain.");
            return false;
        }

    } else {
        //Check command args
        //return SendTcpCmd(TCP_SET_IF_GAIN, _gain);

    }
    return false;
}

void RTL2832SDRDevice::ReadSettings()
{
	if (deviceNumber == RTL_USB)
		qSettings = usbSettings;
	else if (deviceNumber == RTL_TCP)
		qSettings = tcpSettings;

	//Defaults for initial ini file
	sampleRate = 2048000;
	startupDemodMode = dmFMN;
	DeviceInterfaceBase::ReadSettings();

    //Valid gain values (in tenths of a dB) for the E4000 tuner:
    //-10, 15, 40, 65, 90, 115, 140, 165, 190,
    //215, 240, 290, 340, 420, 430, 450, 470, 490
    //0 for automatic gain
	rtlTunerGain = qSettings->value("RtlGain",15).toInt();
	rtlServerIP = QHostAddress(qSettings->value("IPAddr","127.0.0.1").toString());
	rtlServerPort = qSettings->value("Port","1234").toInt();
	rtlSampleRate = qSettings->value("RtlSampleRate",2048000).toUInt();
	rtlTunerGainMode = qSettings->value("RtlGainMode",GAIN_MODE_AUTO).toUInt();
	rtlFreqencyCorrection = qSettings->value("RtlFrequencyCorrection",0).toInt();
	rtlSampleMode = (SAMPLING_MODES)qSettings->value("RtlSampleMode",NORMAL).toInt();
	rtlAgcMode = qSettings->value("RtlAgcMode",false).toBool();
	rtlOffsetMode = qSettings->value("RtlOffsetMode",false).toBool();
}

void RTL2832SDRDevice::WriteSettings()
{
	if (deviceNumber == RTL_USB)
		qSettings = usbSettings;
	else if (deviceNumber == RTL_TCP)
		qSettings = tcpSettings;

	DeviceInterfaceBase::WriteSettings();

	qSettings->setValue("RtlGain",rtlTunerGain);
	qSettings->setValue("IPAddr",rtlServerIP.toString());
	qSettings->setValue("Port",rtlServerPort);
	qSettings->setValue("RtlSampleRate",rtlSampleRate);
	qSettings->setValue("RtlGainMode",rtlTunerGainMode);
	qSettings->setValue("RtlFrequencyCorrection",rtlFreqencyCorrection);
	qSettings->setValue("RtlSampleMode",rtlSampleMode);
	qSettings->setValue("RtlAgcMode",rtlAgcMode);
	qSettings->setValue("RtlOffsetMode",rtlOffsetMode);
	qSettings->sync();

}

double RTL2832SDRDevice::GetStartupFrequency()
{
    if (rtlSampleMode == NORMAL)
        return 162450000;
    else
        return 10000000; //Direct mode, WWV
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
    //Special case with direct sampling from I or Q ADC pin on 2832 is enabled
    if (rtlSampleMode == DIRECT_I || rtlSampleMode == DIRECT_Q)
        return 28800000;

    switch (rtlTunerType) {
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
        case RTLSDR_TUNER_R828D:
            return 1766000000;
        case RTLSDR_TUNER_UNKNOWN:
        default:
            return 1700000000;
    }
}

double RTL2832SDRDevice::GetLowLimit()
{
    //Special case with direct sampling from I or Q ADC pin on 2832 is enabled
    if (rtlSampleMode == DIRECT_I || rtlSampleMode == DIRECT_Q)
        return 1; //Technically possible, but maybe practical limit is better

    switch (rtlTunerType) {
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
        case RTLSDR_TUNER_R828D:
            return 24000000;
        case RTLSDR_TUNER_UNKNOWN:
        default:
            return 64000000;
    }
}

QVariant RTL2832SDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
{
	switch (_key) {
		case PluginName:
			return "RTL2832 Family";
		case PluginDescription:
			return "RTL2832 Family";
		case PluginNumDevices:
			return 2;
			break;
		case DeviceName:
			switch (_option) {
				case 0:
					return "RTL2832 USB";
				case 1:
					return "RTL2832 TCP";
				default:
					return "RTL2832 Unknown";
			}

#if 0
			switch (rtlTunerType) {
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
				case RTLSDR_TUNER_R828D:
					return "RTL2832-R820D";
				case RTLSDR_TUNER_UNKNOWN:
				default:
					return "RTL2832-Unknown";
			}
#endif
		case DeviceDescription:
			break;
		case DeviceType:
			return DeviceInterface::INTERNAL_IQ;
			break;
		case DeviceSampleRates:
			return QStringList()<<"64000"<<"128000"<<"256000"<<"512000"<<"1024000"<<"2048000";
			break;
		case HighFrequency:
			return GetHighLimit();
			break;
		case LowFrequency:
			return GetLowLimit();
			break;
		case FrequencyCorrection:
			return rtlFreqencyCorrection; //int, may not be right format for all devices
			break;
		case StartupDemodMode:
			if (rtlSampleMode == NORMAL)
				return dmFMN;
			else
				//No place to store two lastModes for normal and direct, so always start direct in AM for now
				return dmAM; //Direct mode
			break;
		case StartupFrequency:
			return GetStartupFrequency();
			break;
		case UserFrequency:
			//If freq is outside of mode we are in return default
			if (userFrequency > GetHighLimit() || userFrequency < GetLowLimit())
				return GetStartupFrequency();
			else
				return userFrequency;
			break;
		//Custom keys
		case K_RTLSampleRate:
			return rtlSampleRate;
		case K_RTLTunerGainMode:
			return rtlTunerGainMode;
		case K_RTLTunerGain:
			return rtlTunerGain;
		case K_RTLAgcMode:
			return rtlAgcMode;
		case K_RTLSampleMode:
			return rtlSampleMode;
		case K_RTLOffsetMode:
			return rtlOffsetMode;
		default:
			//If we don't handle it, let default grab it
			return DeviceInterfaceBase::Get(_key, _option);
			break;
	}
	return QVariant();
}

bool RTL2832SDRDevice::Set(STANDARD_KEYS _key, QVariant _value, quint16 _option) {
	Q_UNUSED(_option);
	switch (_key) {
		case DeviceFrequency: {
			double fRequested = _value.toDouble();
			if (!connected || fRequested == 0)
				return false;

			if (fRequested > GetHighLimit() || fRequested < GetLowLimit())
				return false;

			if (deviceNumber == RTL_USB) {
				/* Set the frequency */
				if (rtlsdr_set_center_freq(dev, fRequested) < 0) {
					qDebug("WARNING: Failed to set center freq.");
					return false;
				} else {
					deviceFrequency = fRequested;
					lastFreq = deviceFrequency;
					return true;
				}
			} else if (deviceNumber == RTL_TCP) {
				if (SendTcpCmd(TCP_SET_FREQ,fRequested)) {
					deviceFrequency = fRequested;
					lastFreq = deviceFrequency;
					return true;
				} else {
					qDebug("WARNING: Failed to set center freq.");
					return false;
				}
			}
			break;
		}
		case FrequencyCorrection:
			rtlFreqencyCorrection = _value.toInt();
			break;
		//Custom keys
		case K_RTLSampleRate:
			return SetRtlSampleRate(_value.toULongLong());
		case K_RTLTunerGainMode:
			return SetRtlTunerMode(_value.toInt());
		case K_RTLTunerGain:
			return SetRtlTunerGain(_value.toInt());
		case K_RTLAgcMode:
			return SetRtlAgcMode(_value.toBool());
		case K_RTLSampleMode:
			return SetRtlSampleMode((SAMPLING_MODES)_value.toInt());
		case K_RTLOffsetMode:
			return SetRtlOffsetMode(_value.toInt());

		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
	return true;
}

void RTL2832SDRDevice::SetupOptionUi(QWidget *parent)
{
    ReadSettings();

    //This sets up the sdr specific widget
    if (optionUi != NULL)
        delete optionUi;
    optionUi = new Ui::RTL2832UI();
    optionUi->setupUi(parent);
    parent->setVisible(true);

    if (deviceNumber == RTL_TCP) {
        optionUi->tcpFrame->setVisible(true);
        //Read options file and display
        optionUi->ipAddress->setText(rtlServerIP.toString());
        //Set mask after we set text, otherwise won't parse correctly
        //optionUi->ipAddress->setInputMask("000.000.000.000;0");
        optionUi->port->setText(QString::number(rtlServerPort));
        //optionUi->port->setInputMask("00000;_");

        //connect(o->pushButton,SIGNAL(clicked(bool)),this,SLOT(Test2(bool)));
        //Debugging tip using qApp as known slot, this works
        //connect(o->pushButton,SIGNAL(clicked(bool)),qApp,SLOT(aboutQt()));
        //New QT5 syntax using pointer to member functions
        //connect(o->pushButton,&QPushButton::clicked, this, &SDR_IP::Test2);
        //Lambda function example: needs compiler flag -std=c++0x
        //connect(o->pushButton,&QPushButton::clicked,[=](bool b){qDebug()<<"test";});
        connect(optionUi->ipAddress,&QLineEdit::editingFinished,this,&RTL2832SDRDevice::IPAddressChanged);
        connect(optionUi->port,&QLineEdit::editingFinished,this,&RTL2832SDRDevice::IPPortChanged);
    } else {
        optionUi->tcpFrame->setVisible(false);
    }
    //Common UI
    //Even though we can support additional rates, need to keep in sync with Pebble sampleRate options
    //So we keep decimate by 2 logic intact
    //optionUi->sampleRateSelector->addItem("250 ksps",(quint32)250000);
    optionUi->sampleRateSelector->addItem("1024 msps",(quint32)1024000);
    //optionUi->sampleRateSelector->addItem("1920 msps",(quint32)1920000);
    optionUi->sampleRateSelector->addItem("2048 msps",(quint32)2048000);
    //optionUi->sampleRateSelector->addItem("2400 msps",(quint32)2400000);
    int cur = optionUi->sampleRateSelector->findData(rtlSampleRate);
    optionUi->sampleRateSelector->setCurrentIndex(cur);
    connect(optionUi->sampleRateSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

    //Tuner gain is typically 0 to +50db (500)
    //These are generic gains, actual gains are set by finding closest to what device actually supports
    optionUi->tunerGainSelector->addItem("Tuner AGC",-1000); //A value we'll never set so findData works
    optionUi->tunerGainSelector->addItem("0db",0);
    optionUi->tunerGainSelector->addItem("5db",50);
    optionUi->tunerGainSelector->addItem("10db",100);
    optionUi->tunerGainSelector->addItem("15db",150);
    optionUi->tunerGainSelector->addItem("20db",200);
    optionUi->tunerGainSelector->addItem("25db",250);
    optionUi->tunerGainSelector->addItem("30db",300);
    optionUi->tunerGainSelector->addItem("35db",350);
    optionUi->tunerGainSelector->addItem("40db",400);
    optionUi->tunerGainSelector->addItem("45db",450);
    optionUi->tunerGainSelector->addItem("50db",500);

    if (rtlTunerGainMode == GAIN_MODE_AUTO) {
        optionUi->tunerGainSelector->setCurrentIndex(0);
    } else {
        cur = optionUi->tunerGainSelector->findData(rtlTunerGain);
        optionUi->tunerGainSelector->setCurrentIndex(cur);
    }
    connect(optionUi->tunerGainSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(TunerGainChanged(int)));

    optionUi->frequencyCorrectionEdit->setValue(rtlFreqencyCorrection);
    connect(optionUi->frequencyCorrectionEdit,SIGNAL(valueChanged(int)),this,SLOT(FreqCorrectionChanged(int)));

    optionUi->samplingModeSelector->addItem("Normal", NORMAL);
    optionUi->samplingModeSelector->addItem("Direct I", DIRECT_I);
    optionUi->samplingModeSelector->addItem("Direct Q", DIRECT_Q);
    optionUi->samplingModeSelector->setCurrentIndex(rtlSampleMode);
    connect(optionUi->samplingModeSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(SamplingModeChanged(int)));

    optionUi->agcModeBox->setChecked(rtlAgcMode);
    connect(optionUi->agcModeBox,SIGNAL(clicked(bool)),this,SLOT(AgcModeChanged(bool)));

    optionUi->offsetModeBox->setChecked(rtlOffsetMode);
    connect(optionUi->offsetModeBox,SIGNAL(clicked(bool)),this,SLOT(OffsetModeChanged(bool)));
}

void RTL2832SDRDevice::IPAddressChanged()
{
    //qDebug()<<optionUi->ipAddress->text();
    rtlServerIP = QHostAddress(optionUi->ipAddress->text());
    WriteSettings();
}

void RTL2832SDRDevice::IPPortChanged()
{
    rtlServerPort = optionUi->port->text().toInt();
    WriteSettings();
}

void RTL2832SDRDevice::SampleRateChanged(int _index)
{
    Q_UNUSED(_index);
    int cur = optionUi->sampleRateSelector->currentIndex();
    rtlSampleRate = optionUi->sampleRateSelector->itemData(cur).toUInt();
    WriteSettings();
}

void RTL2832SDRDevice::TunerGainChanged(int _selection)
{
    if (_selection == 0) {
        //Auto mode selected
        rtlTunerGainMode = GAIN_MODE_AUTO;
        rtlTunerGain = 0;
    } else {
        rtlTunerGainMode = GAIN_MODE_MANUAL;
        rtlTunerGain = optionUi->tunerGainSelector->currentData().toInt();
    }
    //Make real time changes
    SetRtlTunerMode(rtlTunerGainMode);
    SetRtlTunerGain(rtlTunerGain);
    WriteSettings();
}

void RTL2832SDRDevice::FreqCorrectionChanged(int _correction)
{
    rtlFreqencyCorrection = _correction;
    SetRtlFrequencyCorrection(rtlFreqencyCorrection);
    WriteSettings();
}

void RTL2832SDRDevice::SamplingModeChanged(int _samplingMode)
{
    Q_UNUSED(_samplingMode);
    int cur = optionUi->samplingModeSelector->currentIndex();
    rtlSampleMode = (SAMPLING_MODES)optionUi->samplingModeSelector->itemData(cur).toUInt();
    WriteSettings();
}

void RTL2832SDRDevice::AgcModeChanged(bool _selected)
{
    Q_UNUSED(_selected);
    rtlAgcMode = optionUi->agcModeBox->isChecked();
    SetRtlAgcMode(rtlAgcMode);
    WriteSettings();
}

void RTL2832SDRDevice::OffsetModeChanged(bool _selected)
{
    Q_UNUSED(_selected);
    rtlOffsetMode = optionUi->offsetModeBox->isChecked();
    if (!SetRtlOffsetMode(rtlOffsetMode))
        rtlOffsetMode = false;
    WriteSettings();
}

bool RTL2832SDRDevice::SendTcpCmd(quint8 _cmd, quint32 _data)
{
    if (!connected)
        return false;

    rtlTcpSocketMutex.lock();
    RTL_CMD cmd;
    cmd.cmd = _cmd;
    //ntohl() is what rtl-tcp uses to convert to network byte order
    //So we use inverse htonl (hardware to network long)
    cmd.data = htonl(_data);
    quint16 bytesWritten = rtlTcpSocket->write((const char *)&cmd, sizeof(cmd));
    //QTcpSocket is always buffered, no choice.
    //So we need to either call an explicit flush() or waitForBytesWritten() to make sure data gets sent
    //Not sure which one is better or if it makes a difference
    rtlTcpSocket->flush();

    //Can't use waitForBytesWritten because it runs event loop while waiting which can trigger signals
    //The readyRead() signal handler also locks the mutex, so we get in a deadlock state
    //rtlTcpSocket->waitForBytesWritten(1000);

    bool res = (bytesWritten == sizeof(cmd));
    rtlTcpSocketMutex.unlock();
    return res;
}

bool RTL2832SDRDevice::SetRtlSampleRate(quint64 _sampleRate)
{
    if (deviceNumber == RTL_USB) {
        /* Set the sample rate */
        if (rtlsdr_set_sample_rate(dev, _sampleRate) < 0) {
            qDebug("WARNING: Failed to set sample rate.");
            return true;
        }
    } else {
        return SendTcpCmd(TCP_SET_SAMPLERATE,_sampleRate);
    }
    return false;

}

bool RTL2832SDRDevice::GetRtlValidTunerGains()
{
    if (deviceNumber == RTL_USB) {
        rtlTunerGainCount = rtlsdr_get_tuner_gains(dev,rtlTunerGains);
        if (rtlTunerGainCount < 0) {
            rtlTunerGainCount = 0;
            qDebug()<<"No valid tuner gains were returned";
            return false;
        }
    } else {
        //This is set in the TCP inbound processing
        return false;
    }
    return false;
}

void RTL2832SDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
    switch (_event) {
        case cbProducerConsumerEvents::Start:
            if (deviceNumber == RTL_TCP) {
                if (tcpThreadSocket == NULL) {
                    //First time, construct any thread local objects like QTcpSockets
                    tcpThreadSocket = new QTcpSocket();
                    tcpThreadSocket->setSocketDescriptor(rtlTcpSocket->socketDescriptor()); //Has same connection as rtlTpSocket
                    //Local socket will only get thread specific event, not UI events
                    tcpThreadSocket->setReadBufferSize(readBufferSize); //Not sure if this is necessary
                }

            }
            break;

        case cbProducerConsumerEvents::Run:
            if (!connected)
                return;

            int bytesRead;

            if (deviceNumber == RTL_USB) {
                if (!running)
                    return;

                if ((producerFreeBufPtr = (char *)producerConsumer.AcquireFreeBuffer()) == NULL)
                    return;

                //Insert code to put data from device in buffer
                if (rtlsdr_read_sync(dev, producerFreeBufPtr, readBufferSize, &bytesRead) < 0) {
                    qDebug("Sync transfer error");
                    producerConsumer.PutbackFreeBuffer(); //Put back buffer for next try
                    return;
                }

                if (bytesRead < readBufferSize) {
                    qDebug("RTL2832 Under read");
                    producerConsumer.PutbackFreeBuffer(); //Put back buffer for next try
                    return;
                }
                producerConsumer.ReleaseFilledBuffer();
                return;

            } else if (deviceNumber == RTL_TCP) {
                //Moved to TcpNewData
            }
            break;

        case cbProducerConsumerEvents::Stop:
            if (deviceNumber == RTL_TCP) {
                rtlTcpSocketMutex.unlock();
                if (tcpThreadSocket != NULL)
                    tcpThreadSocket->disconnectFromHost();
                tcpThreadSocket = NULL;
            }

            break;
    }
}
//Now called via signal from producerConsumer that filled buffer is ready
//Still running in consumerThread
void RTL2832SDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
    double fpSampleRe;
    double fpSampleIm;
    int bufInc;
    int decMult;
    int decNorm;;
    double decAvg;

    switch (_event) {
        case cbProducerConsumerEvents::Start:
            break;

        case cbProducerConsumerEvents::Run:
            if (!connected)
                return;

            if (!running)
                return;


            unsigned char *bufPtr; //rtl data is 0-255, we need to normalize to -1 to +1
			//qDebug()<<"Free buf "<<producerConsumer.GetNumFreeBufs();
			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((bufPtr = producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}


				//RTL I/Q samples are 8bit unsigned 0-256
				bufInc = rtlDecimate * 2;
				decMult = 1; //1=8bit, 2=9bit, 3=10bit for rtlDecimate = 6 and sampleRate = 192k
				decNorm = 128 * decMult;
				decAvg = rtlDecimate / decMult; //Double the range = 1 bit of sample size
				// i is index to CPX buffer, j is index to producer buffer (byte)
				for (int i=0,j=0; i<framesPerBuffer; i++, j+=bufInc) {
			#if 0
					//We are significantly oversampling, so we can use decimation to increase dynamic range
					//http://www.atmel.com/Images/doc8003.pdf Each bit of resolution requires 4 bits of oversampling
					//With atmel method, we add the oversampled data, and right shift by N
					//THis is NOT the same as just averaging the samples, which is a LP filter function
					//                      OverSampled     Right Shift (sames as /2)
					// 8 bit resolution     N/A             N/A
					// 9 bit resolution     4x              1   /2
					//10 bit resolution     16x             2   /4
					//
					//Then scale by 2^n where n is # extra bits of resolution to get back to original signal level


					//See http://www.actel.com/documents/Improve_ADC_WP.pdf as one example
					//We take N (rtlDecimate) samples and create one result
					fpSampleRe = fpSampleIm = 0.0;
					//When i is at end (2047), j is 4094, j+k+1 can't be more then readBufferSize
					//Sum samples
					for (int k = 0; k < bufInc; k+=2) {
						//I/Q reversed from normal devices, correct here
						fpSampleIm += bufPtr[j + k];
						fpSampleRe += bufPtr[j + k + 1];
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
					fpSampleRe = bufPtr[j];
					fpSampleRe -= 127.0;
					fpSampleRe /= 128.0;
					fpSampleRe *= sampleGain;

					fpSampleIm = bufPtr[j+1];
					fpSampleIm -= 127.0;
					fpSampleIm /= 128.0;
					fpSampleIm *= sampleGain;
			#endif
					inBuffer->Re(i) = fpSampleRe;
					inBuffer->Im(i) = fpSampleIm;
				}
				//perform.StartPerformance("ProcessIQ");
				ProcessIQData(inBuffer->Ptr(),framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				producerConsumer.ReleaseFreeBuffer();
			}
            break;

        case cbProducerConsumerEvents::Stop:
            break;
    }

}

//These need to be moved to a DeviceInterface implementation class, equivalent to SDR
//Must be called from derived class constructor to work correctly
void RTL2832SDRDevice::InitSettings(QString fname)
{
	DeviceInterfaceBase::InitSettings(fname + "_usb");
	usbSettings = qSettings;
	DeviceInterfaceBase::InitSettings(fname + "_tcp");
	tcpSettings = qSettings;

}
