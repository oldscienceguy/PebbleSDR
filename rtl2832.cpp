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
    qSettings = new QSettings(path+"/dvb_t.ini",QSettings::IniFormat);
    ReadSettings();

    //crystalFreqHz = DEFAULT_CRYSTAL_FREQUENCY;
    //RTL2832 samples from 900001 to 3200000 sps (900ksps to 3.2msps)
    //1 msps seems to be optimal according to boards
    //We have to decimate from rtl rate to our rate for everything to work
    sampleRate = 48000; //settings->sampleRate;
    //Make rtlSample rate close to 1msps but with even decimate
    rtlDecimate = 1000000 / sampleRate;
    rtlSampleRate = sampleRate * rtlDecimate;

    rtlFrequency = 162400000;
    rtlGain = -10; // tenths of a dB

    //Not using this anymore?  Remove
    usb = new USBUtil();
    usb->LibUSBInit();

    framesPerBuffer = settings->framesPerBuffer;

    inBuffer = CPXBuf::malloc(framesPerBuffer);
    outBuffer = CPXBuf::malloc(framesPerBuffer);

    numDataBufs = 100;
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
    if (inBuffer != NULL)
        free (inBuffer);
    if (outBuffer != NULL)
        free (outBuffer);
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

    for (int i = 0; i < device_count; i++)
        qDebug("%s",rtlsdr_get_device_name(i));

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
    if (rtlsdr_set_center_freq(dev, rtlFrequency) < 0) {
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
    if (rtlsdr_set_tuner_gain(dev, rtlGain) < 0) {
        qDebug("WARNING: Failed to set tuner gain.");
        return;
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

    int available = semNumFreeBuffers->available();
    if ( available < (numDataBufs -5))
        qDebug("Producer %d",available);

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
}

void RTL2832::RunConsumerThread()
{
    double fpSampleRe;
    double fpSampleIm;

    //Wait for data to be available from producer
    semNumFilledBuffers->acquire();

    //RTL I/Q samples are 8bit unsigned 0-256
    int bufInc = rtlDecimate * 2;
    for (int i=0,j=0; i<framesPerBuffer; i++, j+=bufInc) {
        //Every nth sample from producer buffer
        fpSampleRe = (double)producerBuffer[nextConsumerDataBuf][j];
        fpSampleRe -= 127.0;
        fpSampleRe /= 127.0;
        fpSampleRe *= 0.001;

        fpSampleIm = (double)producerBuffer[nextConsumerDataBuf][j+1];
        fpSampleIm -= 127.0;
        fpSampleIm /= 127.0;
        fpSampleIm *= 0.001;
        inBuffer[i].re = fpSampleRe;
        inBuffer[i].im = fpSampleIm;
    }

    //We're done with databuf, so we can release before we call ProcessBlock
    //Update lastDataBuf & release dataBuf
    nextConsumerDataBuf = (nextConsumerDataBuf +1 ) % numDataBufs;
    semNumFreeBuffers->release();

    if (receiver != NULL)
        receiver->ProcessBlock(inBuffer,outBuffer,framesPerBuffer);


}

void RTL2832::ReadSettings()
{
}

void RTL2832::WriteSettings()
{
}

double RTL2832::GetStartupFrequency()
{
    return 162450000;
}

int RTL2832::GetStartupMode()
{
    return dmFMN;
}

double RTL2832::GetHighLimit()
{
    return 1700000000;  //1.7ghz
}

double RTL2832::GetLowLimit()
{
    return 64000000; //64mhz
}

double RTL2832::GetGain()
{
    return 1.0;
}

QString RTL2832::GetDeviceName()
{
    return "DVB-T";
}

int RTL2832::GetSampleRate()
{
    return sampleRate;
}

void RTL2832::StopProducerThread()
{
}


