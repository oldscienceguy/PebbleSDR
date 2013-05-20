#include "rtl2832.h"
#include "Demod.h"
#include "Settings.h"
#include "Receiver.h"

/*
  Derived From and credit to:
  - Steve Markgraf <steve@steve-m.de> for rtl-sdr
  - rtl2832_2836_2840...zip which appears to be source code for linuxTV and supports numerous tuners
  - rtl2831-r2-891128e7c333
  - osmo-sdr firmware tuner_e4k.c is considered latest and best (not using yet)
  Latest rtl-sdr source: git clone git://git.osmocom.org/rtl-sdr.git
  http://sdr.osmocom.org/trac/wiki/rtl-sdr
*/

enum usb_reg {
    USB_SYSCTL          = 0x2000,
    USB_CTRL            = 0x2010,
    USB_STAT            = 0x2014,
    USB_EPA_CFG         = 0x2144,
    USB_EPA_CTL         = 0x2148,
    USB_EPA_MAXPKT		= 0x2158,
    USB_EPA_MAXPKT_2	= 0x215a,
    USB_EPA_FIFO_CFG	= 0x2160
};

enum sys_reg {
    DEMOD_CTL		= 0x3000,
    GPO             = 0x3001,
    GPI             = 0x3002,
    GPOE			= 0x3003,
    GPD             = 0x3004,
    SYSINTE			= 0x3005,
    SYSINTS			= 0x3006,
    GP_CFG0			= 0x3007,
    GP_CFG1			= 0x3008,
    SYSINTE_1		= 0x3009,
    SYSINTS_1		= 0x300a,
    DEMOD_CTL_1		= 0x300b,
    IR_SUSPEND		= 0x300c
};

enum blocks {
    DEMODB			= 0,
    USBB			= 1,
    SYSB			= 2,
    TUNB			= 3,
    ROMB			= 4,
    IRB             = 5,
    IICB			= 6
};

#define DEFAULT_CRYSTAL_FREQUENCY 28800000

//Should be 48000?
#define DEFAULT_SAMPLE_RATE 2048000
//#define DEFAULT_SAMPLE_RATE 48000

//Driver file?? 20101202_linux_install_package.rar
//See osmo_sdr tuner_e4k.c
//Build table of possible values and check all - see rtl2832.cc
#define HAMA_VID		0x0bda	// Same as ezcap
#define HAMA_PID		0x2832

RTL2832::RTL2832 (Receiver *_receiver,SDRDEVICE dev,Settings *_settings): SDR(_receiver, dev,_settings)
{
    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = new QSettings(path+"/PebbleData/rtl2832.ini",QSettings::IniFormat);
    ReadSettings();

    //crystalFreqHz = DEFAULT_CRYSTAL_FREQUENCY;
    //RTL2832 samples from 900001 to 3200000 sps (900ksps to 3.2msps)
    //1 msps seems to be optimal according to boards
    //We have to decimate from rtl rate to our rate for everything to work
    sampleRate = settings->sampleRate;
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

    //Not using this anymore?  Remove
    usb = new USBUtil();
    usb->LibUSBInit();

    framesPerBuffer = settings->framesPerBuffer;

    inBuffer = new CPXBuf(framesPerBuffer);
    outBuffer = new CPXBuf(framesPerBuffer);

    numDataBufs = 50;
    //2 bytes per sample, framesPerBuffer samples after decimate
    producerBufferSize = framesPerBuffer * rtlDecimate * 2;
    producerBuffer = new unsigned char *[numDataBufs];
    for (int i=0; i<numDataBufs; i++)
        producerBuffer[i] = new unsigned char [producerBufferSize];

    semNumFreeBuffers = NULL;
    semNumFilledBuffers = NULL;

    producerThread = new SDRProducerThread(this);
    producerThread->setRefresh(0);
    consumerThread = new SDRConsumerThread(this);
    consumerThread->setRefresh(0);


}

RTL2832::~RTL2832(void)
{
    WriteSettings();

    if (inBuffer != NULL)
        delete (inBuffer);
    if (outBuffer != NULL)
        delete (outBuffer);
    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            free (producerBuffer[i]);
        free (producerBuffer);
    }
}

//Different for each rtl2832 device, read from settings file eventually
#define DVB_PID 0x2832
#define DVB_VID 0x0bda

bool RTL2832::Connect()
{
    int device_count;
    int dev_index = 0; //Assume we only have 1 RTL2832 device for now
    dev = NULL;
    device_count = rtlsdr_get_device_count();
    if (!device_count) {
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

    return true;
#if 0
    //For testing
    //USBUtil::ListDevices();

    int numFound = 0;
    int ret;
    while(true){
        usb->LibUSBFindAndOpenDevice(DVB_PID,DVB_VID,numFound);
        if (usb->hDev == NULL)
            return false; //No devices match and/or serial number not found

        // Default is config #0, Select config #1 for SoftRock
        ret = libusb_set_configuration(usb->hDev,1);//Not sure what config 1 is yet

        // Claim interface #0.
        ret = libusb_claim_interface(usb->hDev,0);

#if 0
        //Is it the right serial number?
        //unsigned serial = dev->descriptor.iSerialNumber; //This is NOT the SoftRock serial number suffix
        int serial = GetSerialNumber();
        if (settings->sdrNumber == -1 || serial == settings->sdrNumber)
            return true; //We've got it
        //Not ours, close and keep looking
        ret = libusb_release_interface(hDev,0);
        USBUtil::CloseDevice(hDev);
        numFound++;
#endif
        return true;
    }

    return false;
#endif
}

bool RTL2832::Disconnect()
{
    rtlsdr_close(dev);
    dev = NULL;
    return true;
}

double RTL2832::SetFrequency(double fRequested, double fCurrent)
{
    /* Set the frequency */
    if (rtlsdr_set_center_freq(dev, fRequested) < 0) {
        qDebug("WARNING: Failed to set center freq.");
        rtlFrequency = fCurrent;
    } else {
        rtlFrequency = fRequested;
    }

    return rtlFrequency;
}

void RTL2832::ShowOptions()
{
}


void RTL2832::Start()
{
    int r;

    /* Set the sample rate */
    if (rtlsdr_set_sample_rate(dev, rtlSampleRate) < 0) {
        qDebug("WARNING: Failed to set sample rate.");
        return;
    }

#if 0
    /* Set the frequency */
    if (rtlsdr_set_center_freq(dev, rtlFrequency) < 0) {
        qDebug("WARNING: Failed to set center freq.");
        return;
    }
#endif

    /* Set the tuner gain */
    //Added support for automatic gain from rtl-sdr.c
    if (rtlGain == 0) {
         /* Enable automatic gain */
        r = rtlsdr_set_tuner_gain_mode(dev, 0);
        if (r < 0) {
            qDebug("WARNING: Failed to enable automatic gain.");
            return;
        } else {
            qDebug("Automatic gain set");
        }
    } else {
        /* Enable manual gain */
        r = rtlsdr_set_tuner_gain_mode(dev, 1);
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

    //Start out with all producer buffers available
    if (semNumFreeBuffers != NULL)
        delete semNumFreeBuffers;
    semNumFreeBuffers = new QSemaphore(numDataBufs);

    if (semNumFilledBuffers != NULL)
        delete semNumFilledBuffers;
    //Init with zero available
    semNumFilledBuffers = new QSemaphore(0);

    nextProducerDataBuf = nextConsumerDataBuf = 0;

    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    consumerThread->start();
    producerThread->start();
    isThreadRunning = true;

    return;

}

void RTL2832::Stop()
{
    if (isThreadRunning) {
        producerThread->stop();
        consumerThread->stop();
        isThreadRunning = false;
    }
    return;

}

void RTL2832::RunProducerThread()
{
    int bytesRead;

    //This will block if we don't have any free data buffers to use, pending consumer thread releasing
    semNumFreeBuffers->acquire();

#if 0
    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFreeBuffers->available();
    if ( available < (numDataBufs -5))
        qDebug("Producer %d",available);
#endif

    if (rtlsdr_read_sync(dev, producerBuffer[nextProducerDataBuf], producerBufferSize, &bytesRead) < 0) {
        qDebug("Sync transfer error");
        semNumFreeBuffers->release(); //Put back buffer for next try
        return;
    }

    if (bytesRead < producerBufferSize) {
        qDebug("Under read");
        semNumFreeBuffers->release(); //Put back buffer for next try
        return;
    }
    //Circular buffer of dataBufs
    nextProducerDataBuf = (nextProducerDataBuf +1 ) % numDataBufs;

    //Increment the number of data buffers that are filled so consumer thread can access
    semNumFilledBuffers->release();

}

void RTL2832::StopConsumerThread()
{
    //Problem - RunConsumerThread may be in process when we're asked to stopp
    //We have to wait for it to complete, then return.  Bad dependency - should not have tight connection like this
}

void RTL2832::RunConsumerThread()
{
    double fpSampleRe;
    double fpSampleIm;

    //Wait for data to be available from producer
    semNumFilledBuffers->acquire();

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
            fpSampleRe += (double)producerBuffer[nextConsumerDataBuf][j + k];
            fpSampleIm += (double)producerBuffer[nextConsumerDataBuf][j + k + 1];
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
    nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;
    semNumFreeBuffers->release();

    if (receiver != NULL)
        receiver->ProcessBlock(inBuffer->Ptr(),outBuffer->Ptr(),framesPerBuffer);


}

void RTL2832::ReadSettings()
{
    SDR::ReadSettings(qSettings);
    //Valid gain values (in tenths of a dB) for the E4000 tuner:
    //-10, 15, 40, 65, 90, 115, 140, 165, 190,
    //215, 240, 290, 340, 420, 430, 450, 470, 490
    //0 for automatic gain
    rtlGain = qSettings->value("RtlGain",0).toInt(); //0=automatic

}

void RTL2832::WriteSettings()
{
    SDR::WriteSettings(qSettings);
    qSettings->setValue("RtlGain",rtlGain);
    qSettings->sync();

}

double RTL2832::GetStartupFrequency()
{
    return 162450000;
}

int RTL2832::GetStartupMode()
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
double RTL2832::GetHighLimit()
{
    if (dev==NULL)
        return 1700000000;

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

double RTL2832::GetLowLimit()
{
    if (dev==NULL)
        return 64000000;

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

double RTL2832::GetGain()
{
    return 1.0;
}

QString RTL2832::GetDeviceName()
{
    if (dev==NULL)
        return "No Device Found";

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
}

int RTL2832::GetSampleRate()
{
    return sampleRate;
}

void RTL2832::StopProducerThread()
{
}


