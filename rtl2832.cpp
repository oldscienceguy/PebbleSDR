#include "rtl2832.h"
#include "Demod.h"

/*
  Derived from work by Steve Markgraf <steve@steve-m.de> for rtl-sdr
*/

enum usb_reg {
    USB_SYSCTL		= 0x2000,
    USB_CTRL		= 0x2010,
    USB_STAT		= 0x2014,
    USB_EPA_CFG		= 0x2144,
    USB_EPA_CTL		= 0x2148,
    USB_EPA_MAXPKT		= 0x2158,
    USB_EPA_MAXPKT_2	= 0x215a,
    USB_EPA_FIFO_CFG	= 0x2160
};

enum sys_reg {
    DEMOD_CTL		= 0x3000,
    GPO			= 0x3001,
    GPI			= 0x3002,
    GPOE			= 0x3003,
    GPD			= 0x3004,
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
    IRB			= 5,
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

    crystalFrequency = 28800000;

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
    return fRequested;
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


void RTL2832::InitRTL()
{
    quint8 reqType;
    quint8 req;
    quint16 value;
    quint16 index;
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
    InitRTL();
    SetSampleRate(48000);
    E4000_Init();
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
    rsamp_ratio = (crystalFrequency * pow(2, 22)) / sampleRate;
    rsamp_ratio &= ~3;

    real_rate = (crystalFrequency * pow(2, 22)) / rsamp_ratio;
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

void RTL2832::InitTuner(int frequency)
{
    SetI2CRepeater(true);
#if 0

    switch (tuner_type) {
    case TUNER_E4000:
        e4000_Initialize(1);
        e4000_SetBandwidthHz(1, 8000000);
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

//E4000
#define E4000_1_SUCCESS			1
#define E4000_1_FAIL			0
#define E4000_I2C_SUCCESS		1
#define E4000_I2C_FAIL			0
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
    unsigned char writearray[5];
    int status;

    writearray[0] = 64;
    // For dummy I2C command, don't check executing status.
    status=E4000_I2CWriteByte (2,writearray[0]);
    status=E4000_I2CWriteByte (2,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
        return false;

    writearray[0] = 0;
    status=E4000_I2CWriteByte (9,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
        return false;

    writearray[0] = 0;
    status=E4000_I2CWriteByte (5,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
        return false;

    writearray[0] = 7;
    status=E4000_I2CWriteByte (0,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
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
    unsigned char writearray[5];
    int status;

    writearray[0] = 0;
    status=E4000_I2CWriteByte(6,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
        return false;

    writearray[0] = 150;
    status=E4000_I2CWriteByte(122,writearray[0]);
    //**Modify commands above with value required if output clock is required,
    if(status != E4000_I2C_SUCCESS)
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
    int status;

    writearray[0] = 1;
    writearray[1] = 254;
    status=E4000_I2CWriteArray(126,writearray,2);
    if(status != E4000_I2C_SUCCESS)
        return false;

    status=E4000_I2CWriteByte (130,0);
    if(status != E4000_I2C_SUCCESS)
        return false;

    status=E4000_I2CWriteByte (36,5);
    if(status != E4000_I2C_SUCCESS)
        return false;

    writearray[0] = 32;
    writearray[1] = 1;
    status=E4000_I2CWriteArray(135,writearray,2);
    if(status != E4000_I2C_SUCCESS)
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
    int status;

    //writearray[0]=0;
    //I2CWriteByte(pTuner, 200,115,writearray[0]);
    writearray[0] = 31;
    status=E4000_I2CWriteByte(45,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
        return false;

    writearray[0] = 1;
    writearray[1] = 1;
    status=E4000_I2CWriteArray(112,writearray,2);
    if(status != E4000_I2C_SUCCESS)
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
    int status;

    unsigned char sum=255;

    writearray[0] = 23;
    status=E4000_I2CWriteByte(26,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }
    //printf("\nRegister 1a=%d", writearray[0]);

    status=E4000_I2CReadByte(27,read1);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    writearray[0] = 16;
    writearray[1] = 4;
    writearray[2] = 26;
    writearray[3] = 15;
    writearray[4] = 167;
    status=E4000_I2CWriteArray(29,writearray,5);
    //printf("\nRegister 1d=%d", writearray[0]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    writearray[0] = 81;
    status=E4000_I2CWriteByte(134,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }
    //printf("\nRegister 86=%d", writearray[0]);

    //For Realtek - gain control logic
    status=E4000_I2CReadByte(27,read1);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    if(read1[0]<=sum)
    {
        sum=read1[0];
    }

    status=E4000_I2CWriteByte(31,writearray[2]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }
    status=E4000_I2CReadByte(27,read1);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    if(read1[0] <= sum)
    {
        sum=read1[0];
    }

    status=E4000_I2CWriteByte(31,writearray[2]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    status=E4000_I2CReadByte(27,read1);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    if(read1[0] <= sum)
    {
        sum=read1[0];
    }

    status=E4000_I2CWriteByte(31,writearray[2]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    status=E4000_I2CReadByte(27,read1);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    if(read1[0] <= sum)
    {
        sum=read1[0];
    }

    status=E4000_I2CWriteByte(31,writearray[2]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    status=E4000_I2CReadByte(27,read1);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }

    if (read1[0]<=sum)
    {
        sum=read1[0];
    }

    writearray[0]=sum;
    status=E4000_I2CWriteByte(27,writearray[0]);
    if(status != E4000_I2C_SUCCESS)
    {
        return E4000_1_FAIL;
    }
    //printf("\nRegister 1b=%d", writearray[0]);
    //printf("\nRegister 1e=%d", writearray[1]);
    //printf("\nRegister 1f=%d", writearray[2]);
    //printf("\nRegister 20=%d", writearray[3]);
    //printf("\nRegister 21=%d", writearray[4]);
    //writearray[0] = 3;
    //writearray[1] = 252;
    //writearray[2] = 3;
    //writearray[3] = 252;
    //I2CWriteArray(pTuner, 200,116,4,writearray);
    //printf("\nRegister 74=%d", writearray[0]);
    //printf("\nRegister 75=%d", writearray[1]);
    //printf("\nRegister 76=%d", writearray[2]);
    //printf("\nRegister 77=%d", writearray[3]);

    return E4000_1_SUCCESS;
}

