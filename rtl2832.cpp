#include "rtl2832.h"
#include "Demod.h"

/*
  Derived From and credit to:
  - Steve Markgraf <steve@steve-m.de> for rtl-sdr
  - rtl2832_2836_2840...zip which appears to be source code for linuxTV and supports numerous tuners
  - rtl2831-r2-891128e7c333
  - osmo-sdr firmware
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

    crystalFreqHz = 28800000;

    usb = new USBUtil();
    usb->LibUSBInit();

    producerThread = new SDRProducerThread(this);
    producerThread->setRefresh(0); //Semaphores will block and wait, no extra delay needed
    //consumerThread = new SDRConsumerThread(this);
    //consumerThread->setRefresh(0);


}

RTL2832::~RTL2832(void)
{

}

//Different for each rtl2832 device, read from settings file eventually
#define DVB_PID 0x2832
#define DVB_VID 0x0bda

bool RTL2832::Connect()
{
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

}

bool RTL2832::Disconnect()
{
    return true;
}

double RTL2832::SetFrequency(double fRequested, double fCurrent)
{
    if (E4000_SetRfFreqHz(fRequested))
        return fRequested;
    else
        return fCurrent;
}

void RTL2832::ShowOptions()
{
}

#if 0
int demod::read_samples(unsigned char* buffer, uint32_t buffer_size, int* bytes_read, int timeout /*= -1*/)
{
    assert(buffer);
    assert(buffer_size > 0);
    assert(bytes_read);

    return libusb_bulk_transfer(m_devh, 0x81, buffer, buffer_size, bytes_read, ((timeout < 0) ? m_params.default_timeout : timeout));
}

#endif

bool RTL2832::RTL_WriteReg(quint16 block, quint16 address, quint16 value, quint16 length)
{
    quint16 index = (block << 8) | 0x10;
    return usb->LibUSBWriteReg(index, address, value, length);
}
bool RTL2832::DemodWriteReg(quint16 page, quint16 address, quint16 value, quint16 length)
{
    quint16 index = 0x10 | page;
    address = (address << 8) | 0x20;
    return usb->LibUSBWriteReg(index, address, value, length);
}

bool RTL2832::RTL_I2CRead(quint8 addr, quint8 *buffer, int len)
{
    return RTL_ReadArray(IICB, (quint16)addr, buffer, len);
}

int RTL2832::RTL_I2CWrite(quint8 addr, quint8 *buffer, int len)
{
    return RTL_WriteArray(IICB, (quint16)addr, buffer, len);
}

bool RTL2832::RTL_ReadArray(quint8 block, quint16 addr, quint8 *array, quint8 len)
{
    quint16 index = (block << 8);

    return usb->LibUSBReadArray(index, addr, array, len);
}

int RTL2832::RTL_WriteArray(quint8 block, quint16 addr, quint8 *array, quint8 len)
{
    quint16 index = (block << 8) | 0x10;

    return usb->LibUSBWriteArray(index, addr, array, len);
}


void RTL2832::RTL_Init()
{
    bool res;


#if 0
    /* default FIR coefficients used for DAB/FM by the Windows driver,
     * the DVB driver uses different ones */
    static uint8_t default_fir_coeff[RTL2832_FIR_COEFF_COUNT] = {
        0xca, 0xdc, 0xd7, 0xd8, 0xe0, 0xf2, 0x0e, 0x35, 0x06, 0x50,
        0x9c, 0x0d, 0x71, 0x11, 0x14, 0x71, 0x74, 0x19, 0x41, 0x00,
    };
#endif

    //USB Init
    res = RTL_WriteReg(USBB, USB_SYSCTL, 0x09, 1);
    res = RTL_WriteReg(USBB, USB_EPA_MAXPKT, 0x0002, 2);
    res = RTL_WriteReg(USBB, USB_EPA_CTL, 0x1002, 2);

    //Power On Demod
    res = RTL_WriteReg(SYSB, DEMOD_CTL_1, 0x22, 1);
    res = RTL_WriteReg(SYSB, DEMOD_CTL, 0xe8, 1);

    //reset demod (bit 3, soft_rst)
    res = DemodWriteReg(1, 0x01, 0x14, 1);
    res = DemodWriteReg(1, 0x01, 0x10, 1);

    //disable spectrum inversion and adjacent channel rejection
    res = DemodWriteReg(1, 0x15, 0x00, 1);
    res = DemodWriteReg(1, 0x16, 0x0000, 2);

    //set IF-frequency to 0 Hz
    res = DemodWriteReg(1, 0x19, 0x0000, 2);

#if 0
    //If we want to set customer FIR coefficients, goes here
    uint8_t* fir_coeff = (m_params.use_custom_fir_coefficients ? m_params.fir_coeff : default_fir_coeff);

    /* set FIR coefficients */
    for (i = 0; i < RTL2832_FIR_COEFF_COUNT; i++)
        res = DemodWriteReg(1, 0x1c + i, fir_coeff[i], 1);

    res = DemodWriteRegg(0, 0x19, 0x25, 1);
#endif

    /* init FSM state-holding register */
    res = DemodWriteReg(1, 0x93, 0xf0, 1);

    /* disable AGC (en_dagc, bit 0) */
    res = DemodWriteReg(1, 0x11, 0x00, 1);

    /* disable PID filter (enable_PID = 0) */
    res = DemodWriteReg(0, 0x61, 0x60, 1);

    /* opt_adc_iq = 0, default ADC_I/ADC_Q datapath */
    res = DemodWriteReg(0, 0x06, 0x80, 1);

    /* Enable Zero-IF mode (en_bbin bit), DC cancellation (en_dc_est),
     * IQ estimation/compensation (en_iq_comp, en_iq_est) */
    res = DemodWriteReg(1, 0xb1, 0x1b, 1);

}

void RTL2832::Start()
{
    RTL_Init();
    SetSampleRate(48000);
    InitTuner();
    //SetFrequency();;

    /* reset endpoint before we start reading */
    RTL_WriteReg(USBB, USB_EPA_CTL, 0x1002, 2);
    RTL_WriteReg(USBB, USB_EPA_CTL, 0x0000, 2);


    //WIP: We don't need both threads, just the producer.  TBD
    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    //consumerThread->start();
    producerThread->start();
    isThreadRunning = true;

    return;

}

void RTL2832::Stop()
{
    if (isThreadRunning) {
        producerThread->stop();
        //consumerThread->stop();
        isThreadRunning = false;
    }
    return;

}

void RTL2832::RunProducerThread()
{
    int bytesRead;
    int timeout = 3000;
    int bufferSize = 2048;
    unsigned char *buffer = new unsigned char[bufferSize];
    //What is endpoint 0x81?
    int err = libusb_bulk_transfer(usb->hDev, 0x81, buffer, bufferSize, &bytesRead, timeout);
    if (err < 0)
        qDebug("Bulk transfer error %d",err);
    else {
        for (int i=0; i<bytesRead; i++)
        {
            qDebug("0x%x",buffer[i]);
        }

    }

}

void RTL2832::StopConsumerThread()
{
}

void RTL2832::RunConsumerThread()
{
}

void RTL2832::ReadSettings()
{
}

void RTL2832::WriteSettings()
{
}

double RTL2832::GetStartupFrequency()
{
    return 107500000;
}

int RTL2832::GetStartupMode()
{
    return dmFMN;
}

double RTL2832::GetHighLimit()
{
    return 1000000000;
}

double RTL2832::GetLowLimit()
{
    return 60000000;
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
    return 48000; //Hard code for now
}

void RTL2832::StopProducerThread()
{
}

void RTL2832::SetSampleRate(quint32 sampleRate)
{
    quint16 tmp;
    quint32 rsamp_ratio;
    double real_rate;
    bool ret;

    //check for the maximum rate the resampler supports
    if (sampleRate > 3200000)
        sampleRate = 3200000;

    //Convert relative to crystalFrequency
    rsamp_ratio = (crystalFreqHz * pow(2, 22)) / sampleRate;
    rsamp_ratio &= ~3;

    real_rate = (crystalFreqHz * pow(2, 22)) / rsamp_ratio;
    //qDebug("Setting sample rate: %.3f Hz\n", real_rate);

    tmp = (rsamp_ratio >> 16);
    ret = DemodWriteReg(1, 0x9f, tmp, 2);
    tmp = rsamp_ratio & 0xffff;
    ret = DemodWriteReg(1, 0xa1, tmp, 2);
}

void RTL2832::SetI2CRepeater(bool on)
{
    DemodWriteReg(1, 0x01, on ? 0x18 : 0x10, 1);
}

void RTL2832::InitTuner()
{
    SetI2CRepeater(true);
    E4000_Init();
    E4000_SetBandwidthHz(8000000);
#if 0

    switch (tuner_type) {
    case TUNER_E4000:
        e4000_SetRfFreqHz(1, frequency);
        break;
    case TUNER_FC0013:
        FC0013_Open();
        FC0013_SetFrequency(frequency/1000, 8);
        break;
    default:
        printf("No valid tuner available!");
        break;
    }

    printf("Tuned to %i Hz\n", frequency);
#endif
    SetI2CRepeater(false);
}

//E4000 Derived multiple versions of tuner_e4000.c
#define E4K_I2C_ADDR		0xc8

bool RTL2832::E4000_Init()
{

    // Initialize tuner.
    // Note: Call E4000 source code functions.
    if(!E4000_ResetTuner())
        return false;

    if(!E4000_TunerClock())
        return false;

    if(!E4000_Qpeak())
        return false;

    if(!E4000_DCoffloop())
        return false;

    if(!E4000_GainControlinit())
        return false;

    return true;
}


bool RTL2832::E4000_I2CReadByte(quint8 addr, quint8 *byte)
{
    quint8 data = addr;

    if (RTL_I2CWrite(E4K_I2C_ADDR, &data, 1) < 0)
        return false;

    if (RTL_I2CRead(E4K_I2C_ADDR, &data, 1) < 0)
        return false;

    *byte = data;

    return true;
}

bool RTL2832::E4000_I2CWriteByte(quint8 addr, quint8 byte)
{
    quint8 data[2];

    data[0] = addr;
    data[1] = byte;
    return RTL_I2CWrite(addr,data,2);
}
bool RTL2832::E4000_I2CWriteArray(quint8 addr, quint8 *array, quint8 len)
{
    quint8 data[len+1];
    data[0] = addr;
    for (int i=0; i<len; i++)
        data[i+1] = array[i];

    return RTL_I2CWrite(E4K_I2C_ADDR,data,len+1);

}

bool RTL2832::E4000_ResetTuner()
{
    quint8 byte;

    byte = 64;
    // For dummy I2C command, don't check executing status.
    E4000_I2CWriteByte (0x02,byte);
    if (!E4000_I2CWriteByte (0x02,byte))
        return false;

    byte = 0;
    if (!E4000_I2CWriteByte (0x09,byte))
        return false;

    byte = 0;
    if (!E4000_I2CWriteByte (0x05,byte))
        return false;

    byte = 7;
    if (!E4000_I2CWriteByte (0x00,byte))
        return false;

    return true;
}
/****************************************************************************\
*  Function: Tunerclock
*
*  Detailed Description:
*  The function configures the E4000 clock. (Register 0x06, 0x7a).
*  Function disables the clock - values can be modified to enable if required.
\****************************************************************************/

bool RTL2832::E4000_TunerClock()
{
    quint8 byte;

    byte = 0;
    if (!E4000_I2CWriteByte(0x06,byte))
        return false;

    byte = 150;
    if (!E4000_I2CWriteByte(0x7a,byte))
    //**Modify commands above with value required if output clock is required,
        return false;

    return true;
}
/****************************************************************************\
*  Function: Qpeak()
*
*  Detailed Description:
*  The function configures the E4000 gains.
*  Also sigma delta controller. (Register 0x82).
*
\****************************************************************************/

bool RTL2832::E4000_Qpeak()
{
    unsigned char writearray[5];

    writearray[0] = 1;
    writearray[1] = 254;
    if (!E4000_I2CWriteArray(0x7e,writearray,2))
        return false;

    if (!E4000_I2CWriteByte (0x82,0))
        return false;

    if (!E4000_I2CWriteByte (0x24,5))
        return false;

    writearray[0] = 32;
    writearray[1] = 1;
    if (!E4000_I2CWriteArray(0x87,writearray,2))
        return false;

    return true;
}

/****************************************************************************\
*  Function: DCoffloop
*
*  Detailed Description:
*  Populates DC offset LUT. (Registers 0x2d, 0x70, 0x71).
*  Turns on DC offset LUT and time varying DC offset.
\****************************************************************************/
bool RTL2832::E4000_DCoffloop()
{
    unsigned char writearray[5];

    //writearray[0]=0;
    //I2CWriteByte(pTuner, 200,0x73,writearray[0]);
    writearray[0] = 31;
    if (!E4000_I2CWriteByte(0x2d,writearray[0]))
        return false;

    writearray[0] = 1;
    writearray[1] = 1;
    if (!E4000_I2CWriteArray(0x70,writearray,2))
        return false;

    return true;
}

/****************************************************************************\
*  Function: GainControlinit
*
*  Detailed Description:
*  Configures gain control mode. (Registers 0x1d, 0x1e, 0x1f, 0x20, 0x21,
*  0x1a, 0x74h, 0x75h).
*  User may wish to modify values depending on usage scenario.
*  Routine configures LNA: autonomous gain control
*  IF PWM gain control.
*  PWM thresholds = default
*  Mixer: switches when LNA gain =7.5dB
*  Sensitivity / Linearity mode: manual switch
*
\****************************************************************************/
bool RTL2832::E4000_GainControlinit()
{
    unsigned char writearray[5];
    unsigned char read1[1];

    unsigned char sum=255;

    writearray[0] = 23;
    if (!E4000_I2CWriteByte(0x1a,writearray[0]))
        return false;

    if (!E4000_I2CReadByte(0x1b,read1))
        return false;

    writearray[0] = 16;
    writearray[1] = 4;
    writearray[2] = 26;
    writearray[3] = 15;
    writearray[4] = 167;
    if (!E4000_I2CWriteArray(0x1d,writearray,5))
        return false;

    writearray[0] = 81;
    if (!E4000_I2CWriteByte(0x86,writearray[0]))
        return false;

    //For Realtek - gain control logic
    if (!E4000_I2CReadByte(0x1b,read1))
        return false;

    if(read1[0]<=sum)
    {
        sum=read1[0];
    }

    if (!E4000_I2CWriteByte(0x1f,writearray[2]))
        return false;

    if (!E4000_I2CReadByte(0x1b,read1))
        return false;

    if(read1[0] <= sum)
    {
        sum=read1[0];
    }

    if (!E4000_I2CWriteByte(0x1f,writearray[2]))
        return false;

    if (!E4000_I2CReadByte(0x1b,read1))
        return false;

    if(read1[0] <= sum)
    {
        sum=read1[0];
    }

    if (!E4000_I2CWriteByte(0x1f,writearray[2]))
        return false;

    if (!E4000_I2CReadByte(0x1b,read1))
        return false;

    if(read1[0] <= sum)
    {
        sum=read1[0];
    }

    if (!E4000_I2CWriteByte(0x1f,writearray[2]))
        return false;

    if (!E4000_I2CReadByte(0x1b,read1))
        return false;

    if (read1[0]<=sum)
    {
        sum=read1[0];
    }

    writearray[0]=sum;
    if (!E4000_I2CWriteByte(0x1b,writearray[0]))
        return false;

    return true;
}

bool RTL2832::E4000_SetBandwidthHz(unsigned long BandwidthHz)
{
    int BandwidthKhz;
    int CrystalFreqKhz;


    // Set tuner bandwidth Hz.
    // Note: 1. BandwidthKhz = round(BandwidthHz / 1000)
    //          CrystalFreqKhz = round(CrystalFreqHz / 1000)
    //       2. Call E4000 source code functions.
    BandwidthKhz   = (int)((BandwidthHz + 500) / 1000);
    CrystalFreqKhz = (int)((crystalFreqHz + 500) / 1000);

    if(!E4000_IFFilter(BandwidthKhz))
        return false;


    return true;
}

/****************************************************************************\
*  Function: IFfilter
*
*  Detailed Description:
*  The function configures the E4000 IF filter. (Register 0x11,0x12).
*
\****************************************************************************/
bool RTL2832::E4000_IFFilter(int bandwidth)
{
    unsigned char writearray[5];

    int IF_BW;

    IF_BW = bandwidth / 2;
    if(IF_BW<=2150)
    {
        writearray[0]=253;
        writearray[1]=31;
    }
    else if(IF_BW<=2200)
    {
        writearray[0]=253;
        writearray[1]=30;
    }
    else if(IF_BW<=2240)
    {
        writearray[0]=252;
        writearray[1]=29;
    }
    else if(IF_BW<=2280)
    {
        writearray[0]=252;
        writearray[1]=28;
    }
    else if(IF_BW<=2300)
    {
        writearray[0]=252;
        writearray[1]=27;
    }
    else if(IF_BW<=2400)
    {
        writearray[0]=252;
        writearray[1]=26;
    }
    else if(IF_BW<=2450)
    {
        writearray[0]=252;
        writearray[1]=25;
    }
    else if(IF_BW<=2500)
    {
        writearray[0]=252;
        writearray[1]=24;
    }
    else if(IF_BW<=2550)
    {
        writearray[0]=252;
        writearray[1]=23;
    }
    else if(IF_BW<=2600)
    {
        writearray[0]=252;
        writearray[1]=22;
    }
    else if(IF_BW<=2700)
    {
        writearray[0]=252;
        writearray[1]=21;
    }
    else if(IF_BW<=2750)
    {
        writearray[0]=252;
        writearray[1]=20;
    }
    else if(IF_BW<=2800)
    {
        writearray[0]=252;
        writearray[1]=19;
    }
    else if(IF_BW<=2900)
    {
        writearray[0]=251;
        writearray[1]=18;
    }
    else if(IF_BW<=2950)
    {
        writearray[0]=251;
        writearray[1]=17;
    }
    else if(IF_BW<=3000)
    {
        writearray[0]=251;
        writearray[1]=16;
    }
    else if(IF_BW<=3100)
    {
        writearray[0]=251;
        writearray[1]=15;
    }
    else if(IF_BW<=3200)
    {
        writearray[0]=250;
        writearray[1]=14;
    }
    else if(IF_BW<=3300)
    {
        writearray[0]=250;
        writearray[1]=13;
    }
    else if(IF_BW<=3400)
    {
        writearray[0]=249;
        writearray[1]=12;
    }
    else if(IF_BW<=3600)
    {
        writearray[0]=249;
        writearray[1]=11;
    }
    else if(IF_BW<=3700)
    {
        writearray[0]=249;
        writearray[1]=10;
    }
    else if(IF_BW<=3800)
    {
        writearray[0]=248;
        writearray[1]=9;
    }
    else if(IF_BW<=3900)
    {
        writearray[0]=248;
        writearray[1]=8;
    }
    else if(IF_BW<=4100)
    {
        writearray[0]=248;
        writearray[1]=7;
    }
    else if(IF_BW<=4300)
    {
        writearray[0]=247;
        writearray[1]=6;
    }
    else if(IF_BW<=4400)
    {
        writearray[0]=247;
        writearray[1]=5;
    }
    else if(IF_BW<=4600)
    {
        writearray[0]=247;
        writearray[1]=4;
    }
    else if(IF_BW<=4800)
    {
        writearray[0]=246;
        writearray[1]=3;
    }
    else if(IF_BW<=5000)
    {
        writearray[0]=246;
        writearray[1]=2;
    }
    else if(IF_BW<=5300)
    {
        writearray[0]=245;
        writearray[1]=1;
    }
    else if(IF_BW<=5500)
    {
        writearray[0]=245;
        writearray[1]=0;
    }
    else
    {
        writearray[0]=0;
        writearray[1]=32;
    }
    if (!E4000_I2CWriteArray(0x11,writearray,2))
        return false;

    return true;
}


bool RTL2832::E4000_SetRfFreqHz(unsigned long RfFreqHz)
{
    int RfFreqKhz;
    int CrystalFreqKhz;

    // Set tuner RF frequency in KHz.
    // Note: 1. RfFreqKhz = round(RfFreqHz / 1000)
    //          CrystalFreqKhz = round(CrystalFreqHz / 1000)
    //       2. Call E4000 source code functions.
    RfFreqKhz      = (int)((RfFreqHz + 500) / 1000);
    CrystalFreqKhz = (int)((crystalFreqHz + 500) / 1000);

    if(!E4000_GainManual())
        return false;

    if(!E4000_GainFreq(RfFreqKhz))
        return false;

    if(!E4000_PLL(CrystalFreqKhz, RfFreqKhz))
        return false;

    if(!E4000_LNAFilter(RfFreqKhz))
        return false;

    if(!E4000_FreqBand(RfFreqKhz))
        return false;

    if(!E4000_DCoffLUT())
        return false;

    if(!E4000_GainControlAuto())
        return false;

    return true;
}

/****************************************************************************\
*  Function: Gainmanual
*
*  Detailed Description:
*  Sets Gain control to serial interface control.
*
\****************************************************************************/
bool RTL2832::E4000_GainManual()
{
    quint8 byte;

    byte=0;
    if (!E4000_I2CWriteByte(0x1a,byte))
        return false;

    byte = 0;
    if (!E4000_I2CWriteByte (0x09,byte))
        return false;

    byte = 0;
    if (!E4000_I2CWriteByte (0x05,byte))
        return false;

    return true;
}

/****************************************************************************\
*  Function: E4000_gain_freq()
*
*  Detailed Description:
*  The function configures the E4000 gains vs. freq
*  0xa3 to 0xa7. Also 0x24.
*
\****************************************************************************/
bool RTL2832::E4000_GainFreq(int Freq)
{
    unsigned char writearray[5];

    if (Freq<=350000)
    {
        writearray[0] = 0x10;
        writearray[1] = 0x42;
        writearray[2] = 0x09;
        writearray[3] = 0x21;
        writearray[4] = 0x94;
    }
    else if(Freq>=1000000)
    {
        writearray[0] = 0x10;
        writearray[1] = 0x42;
        writearray[2] = 0x09;
        writearray[3] = 0x21;
        writearray[4] = 0x94;
    }
    else
    {
        writearray[0] = 0x10;
        writearray[1] = 0x42;
        writearray[2] = 0x09;
        writearray[3] = 0x21;
        writearray[4] = 0x94;
    }
    if (!E4000_I2CWriteArray(0xa3,writearray,5))
        return false;

    if (Freq<=350000)
    {
        writearray[0] = 94;
        writearray[1] = 6;
        if (!E4000_I2CWriteArray(0x9f,writearray,2))
            return false;

        writearray[0] = 0;
        if (!E4000_I2CWriteArray(0x88,writearray,1))
            return false;
    }
    else
    {
        writearray[0] = 127;
        writearray[1] = 7;
        if (!E4000_I2CWriteArray(0x9f,writearray,2))
            return false;

        writearray[0] = 1;
        if (!E4000_I2CWriteArray(0x88,writearray,1))
            return false;
    }

    return true;
}

/****************************************************************************\
*  Function: PLL
*
*  Detailed Description:
*  Configures E4000 PLL divider & sigma delta. 0x0d,0x09, 0x0a, 0x0b).
*
\****************************************************************************/
bool RTL2832::E4000_PLL(int Ref_clk, int Freq)
{
    int VCO_freq;
    unsigned char writearray[5];

    unsigned char divider;
    int intVCOfreq;
    int SigDel;
    int SigDel2;
    int SigDel3;
//	int harmonic_freq;
//	int offset;

    if (Freq<=72400)
    {
        writearray[4] = 15;
        VCO_freq=Freq*48;
    }
    else if (Freq<=81200)
    {
        writearray[4] = 14;
        VCO_freq=Freq*40;
    }
    else if (Freq<=108300)
    {
        writearray[4]=13;
        VCO_freq=Freq*32;
    }
    else if (Freq<=162500)
    {
        writearray[4]=12;
        VCO_freq=Freq*24;
    }
    else if (Freq<=216600)
    {
        writearray[4]=11;
        VCO_freq=Freq*16;
    }
    else if (Freq<=325000)
    {
        writearray[4]=10;
        VCO_freq=Freq*12;
    }
    else if (Freq<=350000)
    {
        writearray[4]=9;
        VCO_freq=Freq*8;
    }
    else if (Freq<=432000)
    {
        writearray[4]=3;
        VCO_freq=Freq*8;
    }
    else if (Freq<=667000)
    {
        writearray[4]=2;
        VCO_freq=Freq*6;
    }
    else if (Freq<=1200000)
    {
        writearray[4]=1;
        VCO_freq=Freq*4;
    }
    else
    {
        writearray[4]=0;
        VCO_freq=Freq*2;
    }

//	divider =  VCO_freq * 1000 / Ref_clk;
    divider =  VCO_freq / Ref_clk;
    writearray[0]= divider;
//	intVCOfreq = divider * Ref_clk /1000;
    intVCOfreq = divider * Ref_clk;
//	SigDel=65536 * 1000 * (VCO_freq - intVCOfreq) / Ref_clk;
    SigDel=65536 * (VCO_freq - intVCOfreq) / Ref_clk;
    if (SigDel<=1024)
    {
        SigDel = 1024;
    }
    else if (SigDel>=64512)
    {
        SigDel=64512;
    }
    SigDel2 = SigDel / 256;
    writearray[2] = (unsigned char)SigDel2;
    SigDel3 = SigDel - (256 * SigDel2);
    writearray[1]= (unsigned char)SigDel3;
    writearray[3]=(unsigned char)0;
    if (!E4000_I2CWriteArray(0x09,writearray,5))
        return false;

    if (Freq<=82900)
    {
        writearray[0]=0;
        writearray[2]=1;
    }
    else if (Freq<=89900)
    {
        writearray[0]=3;
        writearray[2]=9;
    }
    else if (Freq<=111700)
    {
        writearray[0]=0;
        writearray[2]=1;
    }
    else if (Freq<=118700)
    {
        writearray[0]=3;
        writearray[2]=1;
    }
    else if (Freq<=140500)
    {
        writearray[0]=0;
        writearray[2]=3;
    }
    else if (Freq<=147500)
    {
        writearray[0]=3;
        writearray[2]=11;
    }
    else if (Freq<=169300)
    {
        writearray[0]=0;
        writearray[2]=3;
    }
    else if (Freq<=176300)
    {
        writearray[0]=3;
        writearray[2]=11;
    }
    else if (Freq<=198100)
    {
        writearray[0]=0;
        writearray[2]=3;
    }
    else if (Freq<=205100)
    {
        writearray[0]=3;
        writearray[2]=19;
    }
    else if (Freq<=226900)
    {
        writearray[0]=0;
        writearray[2]=3;
    }
    else if (Freq<=233900)
    {
        writearray[0]=3;
        writearray[2]=3;
    }
    else if (Freq<=350000)
    {
        writearray[0]=0;
        writearray[2]=3;
    }
    else if (Freq<=485600)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=493600)
    {
        writearray[0]=3;
        writearray[2]=5;
    }
    else if (Freq<=514400)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=522400)
    {
        writearray[0]=3;
        writearray[2]=5;
    }
    else if (Freq<=543200)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=551200)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=572000)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=580000)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=600800)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=608800)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=629600)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=637600)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=658400)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=666400)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=687200)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=695200)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=716000)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=724000)
    {
        writearray[0]=3;
        writearray[2]=13;
    }
    else if (Freq<=744800)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=752800)
    {
        writearray[0]=3;
        writearray[2]=21;
    }
    else if (Freq<=773600)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=781600)
    {
        writearray[0]=3;
        writearray[2]=21;
    }
    else if (Freq<=802400)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=810400)
    {
        writearray[0]=3;
        writearray[2]=21;
    }
    else if (Freq<=831200)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=839200)
    {
        writearray[0]=3;
        writearray[2]=21;
    }
    else if (Freq<=860000)
    {
        writearray[0]=0;
        writearray[2]=5;
    }
    else if (Freq<=868000)
    {
        writearray[0]=3;
        writearray[2]=21;
    }
    else
    {
        writearray[0]=0;
        writearray[2]=7;
    }

    if (!E4000_I2CWriteByte (0x07,writearray[2]))
        return false;

    if (!E4000_I2CWriteByte (0x05,writearray[0]))
        return false;

    return true;
}

/****************************************************************************\
*  Function: LNAfilter
*
*  Detailed Description:
*  The function configures the E4000 LNA filter. (Register 0x10).
*
\****************************************************************************/

bool RTL2832::E4000_LNAFilter(int Freq)
{
    quint8 byte;

    if(Freq<=370000)
    {
        byte=0;
    }
    else if(Freq<=392500)
    {
        byte=1;
    }
    else if(Freq<=415000)
    {
        byte =2;
    }
    else if(Freq<=437500)
    {
        byte=3;
    }
    else if(Freq<=462500)
    {
        byte=4;
    }
    else if(Freq<=490000)
    {
        byte=5;
    }
    else if(Freq<=522500)
    {
        byte=6;
    }
    else if(Freq<=557500)
    {
        byte=7;
    }
    else if(Freq<=595000)
    {
        byte=8;
    }
    else if(Freq<=642500)
    {
        byte=9;
    }
    else if(Freq<=695000)
    {
        byte=10;
    }
    else if(Freq<=740000)
    {
        byte=11;
    }
    else if(Freq<=800000)
    {
        byte=12;
    }
    else if(Freq<=865000)
    {
        byte =13;
    }
    else if(Freq<=930000)
    {
        byte=14;
    }
    else if(Freq<=1000000)
    {
        byte=15;
    }
    else if(Freq<=1310000)
    {
        byte=0;
    }
    else if(Freq<=1340000)
    {
        byte=1;
    }
    else if(Freq<=1385000)
    {
        byte=2;
    }
    else if(Freq<=1427500)
    {
        byte=3;
    }
    else if(Freq<=1452500)
    {
        byte=4;
    }
    else if(Freq<=1475000)
    {
        byte=5;
    }
    else if(Freq<=1510000)
    {
        byte=6;
    }
    else if(Freq<=1545000)
    {
        byte=7;
    }
    else if(Freq<=1575000)
    {
        byte =8;
    }
    else if(Freq<=1615000)
    {
        byte=9;
    }
    else if(Freq<=1650000)
    {
        byte =10;
    }
    else if(Freq<=1670000)
    {
        byte=11;
    }
    else if(Freq<=1690000)
    {
        byte=12;
    }
    else if(Freq<=1710000)
    {
        byte=13;
    }
    else if(Freq<=1735000)
    {
        byte=14;
    }
    else
    {
        byte=15;
    }
    if (!E4000_I2CWriteByte (0x10,byte))
        return false;

    return true;
}

/****************************************************************************\
*  Function: freqband
*
*  Detailed Description:
*  Configures the E4000 frequency band. (Registers 0x07, 0x78).
*
\****************************************************************************/
bool RTL2832::E4000_FreqBand(int Freq)
{
    quint8 byte;

    if (Freq<=140000)
    {
        byte = 3;
        if (!E4000_I2CWriteByte(0x78,byte))
            return false;
    }
    else if (Freq<=350000)
    {
        byte = 3;
        if (!E4000_I2CWriteByte(0x78,byte))
            return false;
    }
    else if (Freq<=1000000)
    {
        byte = 3;
        if (!E4000_I2CWriteByte(0x78,byte))
            return false;
    }
    else
    {
        byte = 7;
        if (!E4000_I2CWriteByte(0x07,byte))
            return false;

        byte = 0;
        if (!E4000_I2CWriteByte(0x78,byte))
            return false;
    }

    return true;
}

/****************************************************************************\
*  Function: DCoffLUT
*
*  Detailed Description:
*  Populates DC offset LUT. (Registers 0x50 - 0x53, 0x60 - 0x63).
*
\****************************************************************************/
bool RTL2832::E4000_DCoffLUT()
{
    unsigned char writearray[5];

    unsigned char read1[1];
    unsigned char IOFF;
    unsigned char QOFF;
    unsigned char RANGE1;
//	unsigned char RANGE2;
    unsigned char QRANGE;
    unsigned char IRANGE;
    writearray[0] = 0;
    writearray[1] = 126;
    writearray[2] = 36;
    if (!E4000_I2CWriteArray(0x15,writearray,3))
        return false;

    // Sets mixer & IF stage 1 gain = 00 and IF stg 2+ to max gain.
    writearray[0] = 1;
    if (!E4000_I2CWriteByte(0x29,writearray[0]))
        return false;

    // Instructs a DC offset calibration.
    if (!E4000_I2CReadByte(0x2a,read1))
        return false;

    IOFF=read1[0];
    if (!E4000_I2CReadByte(0x2b,read1))
        return false;

    QOFF=read1[0];
    if (!E4000_I2CReadByte(0x2c,read1))
        return false;

    RANGE1=read1[0];
    //reads DC offset values back
    if(RANGE1>=32)
    {
        RANGE1 = RANGE1 -32;
    }
    if(RANGE1>=16)
    {
        RANGE1 = RANGE1 - 16;
    }
    IRANGE=RANGE1;
    QRANGE = (read1[0] - RANGE1) / 16;

    writearray[0] = (IRANGE * 64) + IOFF;
    if (!E4000_I2CWriteByte(0x60,writearray[0]))
        return false;

    writearray[0] = (QRANGE * 64) + QOFF;
    if (!E4000_I2CWriteByte(0x50,writearray[0]))
        return false;

    // Populate DC offset LUT
    writearray[0] = 0;
    writearray[1] = 127;
    if (!E4000_I2CWriteArray(0x15,writearray,2))
        return false;

    // Sets mixer & IF stage 1 gain = 01 leaving IF stg 2+ at max gain.
    writearray[0]= 1;
    if (!E4000_I2CWriteByte(0x29,writearray[0]))
        return false;

    // Instructs a DC offset calibration.
    if (!E4000_I2CReadByte(0x2a,read1))
        return false;

    IOFF=read1[0];
    if (!E4000_I2CReadByte(0x2b,read1))
        return false;

    QOFF=read1[0];
    if (!E4000_I2CReadByte(0x2c,read1))
        return false;

    RANGE1=read1[0];
    // Read DC offset values
    if(RANGE1>=32)
    {
        RANGE1 = RANGE1 -32;
    }
    if(RANGE1>=16)
    {
        RANGE1 = RANGE1 - 16;
    }
    IRANGE = RANGE1;
    QRANGE = (read1[0] - RANGE1) / 16;

    writearray[0] = (IRANGE * 64) + IOFF;
    if (!E4000_I2CWriteByte(0x61,writearray[0]))
        return false;

    writearray[0] = (QRANGE * 64) + QOFF;
    if (!E4000_I2CWriteByte(0x51,writearray[0]))
        return false;

    // Populate DC offset LUT
    writearray[0] = 1;
    if (!E4000_I2CWriteByte(0x15,writearray[0]))
        return false;

    // Sets mixer & IF stage 1 gain = 11 leaving IF stg 2+ at max gain.
    writearray[0] = 1;
    if (!E4000_I2CWriteByte(0x29,writearray[0]))
        return false;

    // Instructs a DC offset calibration.
    if (!E4000_I2CReadByte(0x2a,read1))
        return false;

    IOFF=read1[0];
    if (!E4000_I2CReadByte(0x2b,read1))
        return false;

    QOFF=read1[0];
    if (!E4000_I2CReadByte(0x2c,read1))
        return false;

    RANGE1 = read1[0];
    // Read DC offset values
    if(RANGE1>=32)
    {
        RANGE1 = RANGE1 -32;
    }
    if(RANGE1>=16)
    {
        RANGE1 = RANGE1 - 16;
    }
    IRANGE = RANGE1;
    QRANGE = (read1[0] - RANGE1) / 16;
    writearray[0] = (IRANGE * 64) + IOFF;
    if (!E4000_I2CWriteByte(0x63,writearray[0]))
        return false;

    writearray[0] = (QRANGE * 64) + QOFF;
    if (!E4000_I2CWriteByte(0x53,writearray[0]))
        return false;

    // Populate DC offset LUT
    writearray[0] = 126;
    if (!E4000_I2CWriteByte(0x16,writearray[0]))
        return false;

    // Sets mixer & IF stage 1 gain = 11 leaving IF stg 2+ at max gain.
    writearray[0] = 1;
    if (!E4000_I2CWriteByte(0x29,writearray[0]))
        return false;

    // Instructs a DC offset calibration.
    if (!E4000_I2CReadByte(0x2a,read1))
        return false;
    IOFF=read1[0];

    if (!E4000_I2CReadByte(0x2b,read1))
        return false;
    QOFF=read1[0];

    if (!E4000_I2CReadByte(0x2c,read1))
        return false;
    RANGE1=read1[0];

    // Read DC offset values
    if(RANGE1>=32)
    {
        RANGE1 = RANGE1 -32;
    }
    if(RANGE1>=16)
    {
        RANGE1 = RANGE1 - 16;
    }
    IRANGE = RANGE1;
    QRANGE = (read1[0] - RANGE1) / 16;

    writearray[0]=(IRANGE * 64) + IOFF;
    if (!E4000_I2CWriteByte(0x62,writearray[0]))
        return false;

    writearray[0] = (QRANGE * 64) + QOFF;
    if (!E4000_I2CWriteByte(0x52,writearray[0]))
        return false;
    // Populate DC offset LUT

    return true;
}

/****************************************************************************\
*  Function: GainControlinit
*
*  Detailed Description:
*  Configures gain control mode. (Registers 0x1a)
*
\****************************************************************************/
bool RTL2832::E4000_GainControlAuto()
{
    quint8 byte;

    byte = 23;
    if (!E4000_I2CWriteByte(0x1a,byte))
        return false;

    return true;
}

//Following now used, here for reference
/****************************************************************************\
*  Function: E4000_sensitivity
*
*  Detailed Description:
*  The function configures the E4000 for sensitivity mode.
*
\****************************************************************************/

bool RTL2832::E4000_Sensitivity(int Freq, int bandwidth)
{
    unsigned char writearray[2];
    int IF_BW;

    writearray[1]=0x00;

    if(Freq<=700000)
    {
        writearray[0] = 0x07;
    }
    else
    {
        writearray[0] = 0x05;
    }
    if (!E4000_I2CWriteArray(0x24,writearray,1))
        return false;

    IF_BW = bandwidth / 2;
    if(IF_BW<=2500)
    {
        writearray[0]=0xfc;
        writearray[1]=0x17;
    }
    else if(IF_BW<=3000)
    {
        writearray[0]=0xfb;
        writearray[1]=0x0f;
    }
    else if(IF_BW<=3500)
    {
        writearray[0]=0xf9;
        writearray[1]=0x0b;
    }
    else if(IF_BW<=4000)
    {
        writearray[0]=0xf8;
        writearray[1]=0x07;
    }
    if (!E4000_I2CWriteArray(0x11,writearray,2))
        return false;

    return true;
}
/****************************************************************************\
*  Function: E4000_linearity
*
*  Detailed Description:
*  The function configures the E4000 for linearity mode.
*
\****************************************************************************/
bool RTL2832::E4000_Linearity(int Freq, int bandwidth)
{

    unsigned char writearray[2];
    int IF_BW;

    writearray[1]=0x00;

    if(Freq<=700000)
    {
        writearray[0] = 0x03;
    }
    else
    {
        writearray[0] = 0x01;
    }
    if (!E4000_I2CWriteArray(0x24,writearray,1))
        return false;

    IF_BW = bandwidth / 2;
    if(IF_BW<=2500)
    {
        writearray[0]=0xfe;
        writearray[1]=0x19;
    }
    else if(IF_BW<=3000)
    {
        writearray[0]=0xfd;
        writearray[1]=0x11;
    }
    else if(IF_BW<=3500)
    {
        writearray[0]=0xfb;
        writearray[1]=0x0d;
    }
    else if(IF_BW<=4000)
    {
        writearray[0]=0xfa;
        writearray[1]=0x0a;
    }
    if (!E4000_I2CWriteArray(0x11,writearray,2))
        return false;

    return true;
}
/****************************************************************************\
*  Function: E4000_nominal
*
*  Detailed Description:
*  The function configures the E4000 for nominal
*
\****************************************************************************/
bool RTL2832::E4000_Nominal(int Freq, int bandwidth)
{
    unsigned char writearray[2];
    int IF_BW;

    writearray[1]=0x00;

    if(Freq<=700000)
    {
        writearray[0] = 0x03;
    }
    else
    {
        writearray[0] = 0x01;
    }
    if (!E4000_I2CWriteArray(0x24,writearray,1))
        return false;

    IF_BW = bandwidth / 2;
    if(IF_BW<=2500)
    {
        writearray[0]=0xfc;
        writearray[1]=0x17;
    }
    else if(IF_BW<=3000)
    {
        writearray[0]=0xfb;
        writearray[1]=0x0f;
    }
    else if(IF_BW<=3500)
    {
        writearray[0]=0xf9;
        writearray[1]=0x0b;
    }
    else if(IF_BW<=4000)
    {
        writearray[0]=0xf8;
        writearray[1]=0x07;
    }
    if (!E4000_I2CWriteArray(0x11,writearray,2))
        return false;

    return true;
}


