//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SDR_IQ.h"
#include "settings.h"
#include "receiver.h"
#include "qmessagebox.h"

SDR_IQ::SDR_IQ(Receiver *_receiver, SDRDEVICE dev,Settings *_settings):SDR(_receiver, dev,_settings)
{
    settings = _settings;
    if (!settings)
        return; //Init only

    InitSettings("sdr_iq");
	ReadSettings();

	ftHandle = NULL;
	sdrIQOptions = NULL;
	inBuffer = NULL;
	outBuffer = NULL;
    readBuf = readBufProducer = NULL;
	isThreadRunning = false;

	if (!isFtdiLoaded) {
        if (!USBUtil::LoadFtdi()) {
			QMessageBox::information(NULL,"Pebble","ftd2xx.dll could not be loaded.  SDR-IQ communication is disabled.");
			return;
		}
        isFtdiLoaded = true;
    }

	//Todo: SDR-IQ has fixed block size 2048, are we going to support variable size or just force
	inBufferSize = 2048; //settings->framesPerBuffer;
	inBuffer = CPXBuf::malloc(inBufferSize);
	outBuffer = CPXBuf::malloc(inBufferSize);
	//Max data block we will ever read is dataBlockSize
	readBuf = new BYTE[dataBlockSize];
    readBufProducer = new BYTE[dataBlockSize];

	producerThread = new SDRProducerThread(this);
	producerThread->setRefresh(0); //Semaphores will block and wait, no extra delay needed
	consumerThread = new SDRConsumerThread(this);
	consumerThread->setRefresh(0);

    //Instead of getting BW from SDR_IQ options dialog, we now get it directly from settings dialog
    //because we're passing GetSampleRates() during dialog setup
    //So sampleRate is actually bandwidth
    if (sampleRate==190000)
        sBandwidth = BW190K;
    else if (sampleRate==150000)
        sBandwidth = BW150K;
    else if (sampleRate==100000)
        sBandwidth = BW100K;
    else
        sBandwidth = BW50K;

}

SDR_IQ::~SDR_IQ(void)
{
    if (!settings)
        return;

	WriteSettings();

	if (sdrIQOptions != NULL && sdrIQOptions->isVisible())
		sdrIQOptions->hide();

	//Let FTDI DLL know we're done, otherwise we won't be able to re-open same device in this session
	if (ftHandle != NULL)
		FT_Close(ftHandle);

	if (inBuffer != NULL)
		CPXBuf::free(inBuffer);
	if (outBuffer != NULL)
		CPXBuf::free(outBuffer);
	if (readBuf != NULL)
		free (readBuf);
    if (readBufProducer != NULL)
        free (readBufProducer);
}
void SDR_IQ::ReadSettings()
{
    SDR::ReadSettings();
	sStartup = qSettings->value("Startup",10000000).toDouble();
	sLow = qSettings->value("Low",150000).toDouble();
	sHigh = qSettings->value("High",33000000).toDouble();
		sStartupMode = qSettings->value("StartupMode",dmAM).toInt();
	sGain = qSettings->value("Gain",1.0).toDouble();
	sBandwidth = qSettings->value("Bandwidth",3).toInt(); //Enum
	sRFGain = qSettings->value("RFGain",0).toInt();
	sIFGain = qSettings->value("IFGain",18).toInt();

}
void SDR_IQ::WriteSettings()
{
    SDR::WriteSettings();
	qSettings->setValue("Startup",sStartup);
	qSettings->setValue("Low",sLow);
	qSettings->setValue("High",sHigh);
	qSettings->setValue("StartupMode",sStartupMode);
	qSettings->setValue("Gain",sGain);
	qSettings->setValue("Bandwidth",sBandwidth); //Enum
	qSettings->setValue("RFGain",sRFGain);
	qSettings->setValue("IFGain",sIFGain);

	qSettings->sync();
}

void SDR_IQ::Start()
{
	if (!isFtdiLoaded)
		return;

    InitProducerConsumer(50,4096 * sizeof(short));

    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    consumerThread->start();
    producerThread->start();
    isThreadRunning = true;

	//Set bandwidth first, takes a while and returns lots of garbage ACKs
    SetBandwidth((SDR_IQ::BANDWIDTH)sBandwidth);
	//We're going to get back over 256 ACKs, eat em before continuing 
	//Otherwise we fill up data buffers while processing acks
    FlushDataBlocks();

	//Get basic Device Information for later use, name, version, etc
	bool result = RequestTargetName();
	result = RequestTargetSerialNumber();
	result = RequestFirmwareVersion();
	result = RequestBootcodeVersion();
	result = RequestInterfaceVersion();

	//Set IF Gain, 0,+6, +12,+18,+24
	SetIFGain(sIFGain);

	//RFGain MUST be set to something when we power up or we get no data in buffer
	//RFGain is actually an attenuator
	SetRFGain(sRFGain);

	//Finally ready to start getting data samples
    StartCapture();

}
void SDR_IQ::Stop()
{
	if (!isFtdiLoaded)
		return;

	if (isThreadRunning) {
		StopCapture();
		producerThread->stop();
		consumerThread->stop();
		isThreadRunning = false;
	}
	FlushDataBlocks();
}
bool SDR_IQ::Connect()
{
	if (!isFtdiLoaded)
		return false;

	FT_STATUS ftStatus;

    int deviceNumber = USBUtil::FTDI_FindDeviceByName("SDR-IQ");
	if (deviceNumber == -1)
		return false;

	//Open device
	if (ftHandle == NULL) 
	{
		ftStatus = FT_Open(deviceNumber,&ftHandle);
		if (ftStatus != FT_OK) {
			//commError(ftStatus);
			return false; // FT_Open failed
		}
	}

    ftStatus = FT_ResetDevice(ftHandle);
	//Make sure driver buffers are empty
    ftStatus = FT_Purge(ftHandle,FT_PURGE_RX | FT_PURGE_TX);
    ftStatus = FT_SetTimeouts(ftHandle,500,500);
    //Testing: Increase size of internal buffers from default 4096
    //ftStatus = FT_SetUSBParameters(ftHandle,8192,8192);


    //SDR sends a 0 on startup, clear it out of buffer
	DWORD bytesReturned;
	ftStatus = FT_Read(ftHandle,readBuf,1,&bytesReturned);
    if (ftStatus != FT_OK) //Don't care how many bytes actually got read
        return false;

	return true;
}
bool SDR_IQ::Disconnect()
{
	return true;
}

double SDR_IQ::GetStartupFrequency()
{
	return sStartup;
}

int SDR_IQ::GetStartupMode()
{
	return sStartupMode;
}

double SDR_IQ::GetHighLimit()
{
	return sHigh;
}

double SDR_IQ::GetLowLimit()
{
	return sLow;
}

double SDR_IQ::GetGain()
{
	return sGain;
}

QString SDR_IQ::GetDeviceName()
{
	return "RFSpace SDR-IQ";
}



//Control Item Code 0x0001
bool SDR_IQ::RequestTargetName()
{
	if (!isFtdiLoaded)
		return false;

	targetName = "Pending";
	FT_STATUS ftStatus;
	//0x04, 0x20 is the request command
	//0x01, 0x00 is the Control Item Code (command)
	BYTE writeBuf[4] = { 0x04, 0x20, 0x01, 0x00 };
	DWORD bytesWritten;
	//Ask for data
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return false;
	return true;
}
//Same pattern as TargetName
bool SDR_IQ::RequestTargetSerialNumber()
{
	if (!isFtdiLoaded)
		return false;

	serialNumber = "Pending";
	FT_STATUS ftStatus;
	BYTE writeBuf[4] = { 0x04, 0x20, 0x02, 0x00 };
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return false;
	return true;
}
bool SDR_IQ::RequestInterfaceVersion()
{
	if (!isFtdiLoaded)
		return false;

	interfaceVersion = 0;
	FT_STATUS ftStatus;
	BYTE writeBuf[4] = { 0x04, 0x20, 0x03, 0x00 };
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return false;
	else
		return true;
}
bool SDR_IQ::RequestFirmwareVersion()
{
	if (!isFtdiLoaded)
		return false;

	firmwareVersion = 0;
	FT_STATUS ftStatus;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x04, 0x00, 0x01 };
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return false;

	return true;
}
bool SDR_IQ::RequestBootcodeVersion()
{
	if (!isFtdiLoaded)
		return false;

	bootcodeVersion = 0;
	FT_STATUS ftStatus;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x04, 0x00, 0x00 };
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return false;
	return true;
}
unsigned SDR_IQ::StatusCode()
{
	if (!isFtdiLoaded)
		return false;

	FT_STATUS ftStatus;
	BYTE writeBuf[4] = { 0x04, 0x20, 0x05, 0x00};
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return -1;

	return 0;
}
//Call may not be working right in SDR-IQ
QString SDR_IQ::StatusString(BYTE code)
{
	if (!isFtdiLoaded)
		return "Error";

	FT_STATUS ftStatus;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x06, 0x00, code};
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return "";
	return "TBD";
}
double SDR_IQ::GetFrequency()
{
	if (!isFtdiLoaded)
		return -1;

	FT_STATUS ftStatus;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x20, 0x00, 0x00};
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return -1;
	return 0;
}

double SDR_IQ::SetFrequency(double freq, double fCurrent)
{
    if (!isFtdiLoaded)
		return freq;

	if (freq==0)
		return freq; //ignore

	FT_STATUS ftStatus;
	ULONG f = (ULONG)freq; 
	BYTE writeBuf[0x0a] = { 0x0a, 0x00, 0x20, 0x00, 0x00,0xff,0xff,0xff,0xff,0x01};
	//Byte order is LSB/MSB
	//Ex: 14010000 -> 0x00D5C690 -> 0x90C6D500
	//We just reverse the byte order by masking and shifting
	writeBuf[5] = (BYTE)((f) & 0xff);
	writeBuf[6] = (BYTE)((f >> 8) & 0xff);
	writeBuf[7] = (BYTE)((f >> 16) & 0xff);
	writeBuf[8] = (BYTE)((f >> 24) & 0xff);;

	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return 0;
	return freq;
}

unsigned SDR_IQ::GetRFGain()
{
	if (!isFtdiLoaded)
		return -1;

	FT_STATUS ftStatus;
	BYTE writeBuf[5] = { 0x05, 0x20, 0x38, 0x00, 0x00};
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return -1;

	return 0;
}
//0,-10,-20,-30
bool SDR_IQ::SetRFGain(qint8 gain)
{
	if (!isFtdiLoaded)
		return false;

	FT_STATUS ftStatus;
	BYTE writeBuf[6] = { 0x06, 0x00, 0x38, 0x00, 0xff, 0xff};
    writeBuf[4] = 0x00;
	writeBuf[5] = gain ;

	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return false;
	return true;
}
//This is not documented in Interface spec, but used in activeX examples
bool SDR_IQ::SetIFGain(qint8 gain)
{
	if (!isFtdiLoaded)
		return false;

	FT_STATUS ftStatus;
	BYTE writeBuf[6] = { 0x06, 0x00, 0x40, 0x00, 0xff, 0xff};
	//Bits 7,6,5 are Factory test bits and are masked
	writeBuf[4] = 0; //gain & 0xE0;
    writeBuf[5] = gain;

	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	return (ftStatus == FT_OK);
}
//AD6620 registers
bool SDR_IQ::SetAD6620Raw(unsigned address, short d0, short d1, short d2, short d3, short d4)
{
	if (!isFtdiLoaded)
		return false;

	qint8 writeBuf[9];
	writeBuf[0] = 0x09;
	writeBuf[1] = 0xa0;
	writeBuf[2] = address & 0x00ff;
	writeBuf[3] = (address >> 8) & 0x00ff;
	writeBuf[4] = d0 & 0x00ff;
	writeBuf[5] = d1 & 0x00ff;
	writeBuf[6] = d2 & 0x00ff;
	writeBuf[7] = d3 & 0x00ff;
	writeBuf[8] = d4 & 0x00ff;
	DWORD bytesWritten;
	return (FT_Write(ftHandle,(LPVOID)&writeBuf,9,&bytesWritten) == FT_OK);
}
//2byte register followed by 5 data bytes
bool SDR_IQ::SetRegisters(unsigned adrs, qint32 data)
{
	if (!isFtdiLoaded)
		return false;

	AD6620_Register writeBuf;

	writeBuf.header[0] = 9;
	writeBuf.header[1] = 0xa0;
	writeBuf.address = adrs;
	writeBuf.data1 = data;
	writeBuf.data2 = 0;

	DWORD bytesWritten;
	return (FT_Write(ftHandle,(LPVOID)&writeBuf,9,&bytesWritten) == FT_OK);

}

int SDR_IQ::GetSampleRate()
{
	return GetSampleRate((BANDWIDTH)sBandwidth); //Sample rate for current BW
}

int *SDR_IQ::GetSampleRates(int &len)
{
    len = 4;
    //Ugly, but couldn't find easy way to init with {1,2,3} array initializer
    sampleRates[0] = 50000;
    sampleRates[1] = 100000;
    sampleRates[2] = 150000;
    sampleRates[3] = 190000;
    return sampleRates;
}

//Receiver needs to get sample rate early in setup process, before we've actually started SDR
int SDR_IQ::GetSampleRate(BANDWIDTH bw)
{
	switch (bw)
	{
	case BW5K: return 8138;
	case BW10K: return 16276;
	case BW25K: return 37792;
	case BW50K: return 55555;
	case BW100K: return 111111;
	case BW150K: return 158730;
	case BW190K: return 196078;
	}
	return 48000;
}
/*
Bandwidth options, these also change the effective sampleRate we use throughout the program
From AD6620 DataSheet "Programming AD6620"
Some registers can only be set in 'reset' mode, others can be changed dynamically
BW (IF Filter) can not be changed on the fly, SDR-IQ has to be stopped, then restarted
*/

int SDR_IQ::SetBandwidth(BANDWIDTH bw)
{
	if (!isFtdiLoaded)
		return false;

	qint8 CIC2Rate=0;
	qint8 CIC5Rate=0;
	qint8 RCFRate=0;
	qint16 RCFTaps=0;
	qint32 *RCFCoef=NULL;
	int totalDecimationRate;
	int usableBandwidth;

	int newSampleRate=0;

	//Rate, Taps, and SampleRate are one less than Spectrum shows in UI.  Do redundant math here so it's obvious
	switch (bw)
	{
	case BW5K:
		CIC2Rate = FIL5KHZ_CIC2RATE;
		CIC5Rate = FIL5KHZ_CIC5RATE;
		RCFRate = FIL5KHZ_RCFRATE;
		RCFTaps = FIL5KHZ_TAPS;
		RCFCoef = (qint32 *)FIL5KHZ_COEF;
		usableBandwidth = FIL5KHZ_USEABLEBW;
		newSampleRate = 8138;
		break;
	case BW10K:
		CIC2Rate = FIL10KHZ_CIC2RATE;
		CIC5Rate = FIL10KHZ_CIC5RATE;
		RCFRate = FIL10KHZ_RCFRATE;
		RCFTaps = FIL10KHZ_TAPS;
		RCFCoef = (qint32 *)FIL10KHZ_COEF;
		usableBandwidth = FIL10KHZ_USEABLEBW;
		newSampleRate = 16276;
		break;
	case BW25K:
		CIC2Rate = FIL25KHZ_CIC2RATE;
		CIC5Rate = FIL25KHZ_CIC5RATE;
		RCFRate = FIL25KHZ_RCFRATE;
		RCFTaps = FIL25KHZ_TAPS;
		RCFCoef = (qint32 *)FIL25KHZ_COEF;
		usableBandwidth = FIL25KHZ_USEABLEBW;
		newSampleRate = 37792-1;
		break;
	case BW50K:
		CIC2Rate = FIL50KHZ_CIC2RATE;
		CIC5Rate = FIL50KHZ_CIC5RATE;
		RCFRate = FIL50KHZ_RCFRATE;
		RCFTaps = FIL50KHZ_TAPS;
		RCFCoef = (qint32 *)FIL50KHZ_COEF;
		usableBandwidth = FIL50KHZ_USEABLEBW;
		newSampleRate = 55555-1;
		break;
	case BW100K:
		CIC2Rate = FIL100KHZ_CIC2RATE;
		CIC5Rate = FIL100KHZ_CIC5RATE;
		RCFRate = FIL100KHZ_RCFRATE;
		RCFTaps = FIL100KHZ_TAPS;
		RCFCoef = (qint32 *)FIL100KHZ_COEF;
		usableBandwidth = FIL100KHZ_USEABLEBW;
		newSampleRate = 111111-1;
		break;
	case BW150K:
		CIC2Rate = FIL150KHZ_CIC2RATE;
		CIC5Rate = FIL150KHZ_CIC5RATE;
		RCFRate = FIL150KHZ_RCFRATE;
		RCFTaps = FIL150KHZ_TAPS;
		RCFCoef = (qint32 *)FIL150KHZ_COEF;
		usableBandwidth = FIL150KHZ_USEABLEBW;
		newSampleRate = 158730-1;
		break;
	case BW190K:
		CIC2Rate = FIL190KHZ_CIC2RATE;
		CIC5Rate = FIL190KHZ_CIC5RATE;
		RCFRate = FIL190KHZ_RCFRATE;
		RCFTaps = FIL190KHZ_TAPS;
		RCFCoef = (qint32 *)FIL190KHZ_COEF;
		usableBandwidth = FIL190KHZ_USEABLEBW;
		newSampleRate = 196078-1;
		break;
	}
	bool res;
	//SDR-IQ has to be off to change rates and filters
	//Scales can be changed at any time
	//You would think mode should be SCOMPLEX (single 2input Complex) but SREAL is the only thing that works
	res = SetRegisters(MODECTRL, MODECTRL_SREAL | MODECTRL_SYNCMASTER | MODECTRL_RESET);
	//Not related to Bandwidth, but set here while we're in reset mode
	res = SetRegisters(ADR_NCOCTRL,NCOCTRL_PHZDITHER | NCOCTRL_AMPDITHER); //Dither options

	//-1 values indicate decimation registers which are defined as value-1 in AD6620 data sheet
	res = SetRegisters(CIC2SCALE,CIC2_SCALE_TBL[CIC2Rate]);
	res = SetRegisters(CIC2RATE, CIC2Rate-1);
	res = SetRegisters(CIC5SCALE, CIC5_SCALE_TBL[CIC5Rate]);
	res = SetRegisters(CIC5RATE, CIC5Rate-1);
	res = SetRegisters(RCFRATE, RCFRate-1);
	//res = SetRegisters(ADR_RCFOFFSET,0); //Don't know if we need this
	res = SetRegisters(RCFTAPS, RCFTaps-1);
	//We need to load filter coeffients for each range, they are not in SDR-IQ by default
	for (int i=0; i<RCFTaps; i++) {
		res = SetRegisters(i,RCFCoef[i]);
	}
	//Turn off RESET mode
	res = SetRegisters(MODECTRL, MODECTRL_SREAL | MODECTRL_SYNCMASTER);

	totalDecimationRate = CIC2Rate * CIC5Rate * RCFRate;

	//Todo: compare this to fixed rates above and determine which to use
	//int calculatedSampleRate = 66666667 / totalDecimationRate;

	return newSampleRate;
}


//OneShot - Requests SDR to send numBlocks of data
//Note: Blocks are fixed size, 8192 bytes
unsigned SDR_IQ::CaptureBlocks(unsigned numBlocks)
{
	if (!isFtdiLoaded)
		return -1;

	FT_STATUS ftStatus;
	BYTE writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x02, numBlocks};
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return -1;

	return 0;
}

unsigned SDR_IQ::StartCapture()
{
	if (!isFtdiLoaded)
		return -1;

	FT_STATUS ftStatus;
	BYTE writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x02, 0x00, 0x00};
	DWORD bytesWritten;
    ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return -1;
	return 0;
}

unsigned SDR_IQ::StopCapture()
{
	if (!isFtdiLoaded)
		return -1;

	FT_STATUS ftStatus;
	BYTE writeBuf[8] = { 0x08, 0x00, 0x18, 0x00, 0x81, 0x01, 0x00, 0x00};
	DWORD bytesWritten;
	ftStatus = FT_Write(ftHandle,(LPVOID)writeBuf,sizeof(writeBuf),&bytesWritten);
	if (ftStatus != FT_OK)
		return -1;

	return 0;
}

//Called from Producer thread
//Returns bytesReturned or -1 if error
int SDR_IQ::ReadResponse(int expectedType)
{
	if (!isFtdiLoaded)
		return -1;

	//mutex.lock();
    FT_STATUS ftStatus;

	DWORD bytesReturned;

	//FT_Read blocks until bytes are ready or timeout, which wastes time
	//Check to make sure we've got bytes ready before proceeding
	ftStatus = FT_GetQueueStatus(ftHandle,&bytesReturned);
	if (ftStatus != FT_OK || bytesReturned < 2)
        //We get a lot of this initially as there's no data to return until rcv gets set up
		return 0;

	//Note, this assumes that we're in sync with stream
	//Read 16 bit Header which contains message length and type
    ftStatus = FT_Read(ftHandle, readBufProducer, 2, &bytesReturned);
	if (ftStatus != FT_OK || bytesReturned != 2){
		//mutex.unlock();
		return -1;
	}

	//Get 13bit message length, mask 3 high order bits of 2nd byte
    //Example from problem response rb = 0x01 0x03
    //rb[0] = 0000 0001 rb[1] = 000 0011
    //type is 0 and length is 769
    int type = readBufProducer[1]>>5; //Get 3 high order bits
    int length = readBufProducer[0] + (readBufProducer[1] & 0x1f) * 0x100;

	if (type == 0x04 && length==0) {

		//Add DataBuf to circular buffer
		//Special case to allow 2048 samples per block
        //When length==0 it means to read 8192 bytes
        //Largest value in a 13bit field is 8191 and we need length of 8192
		length = 8192; 
		//int tmp = semDataBuf.available();
        //This will block if we don't have any free data buffers to use, pending consumer thread releasing
        AcquireFreeBuffer();

        ftStatus = FT_Read(ftHandle,producerBuffer[nextProducerDataBuf],length,&bytesReturned);

        if (ftStatus != FT_OK || bytesReturned != 8192) {
            //Lost data
            ReleaseFreeBuffer(); //Put back what we acquired
            return -1;
        }

		//Circular buffer of dataBufs
        IncrementProducerBuffer();

        //Increment the number of data buffers that are filled so consumer thread can access
        ReleaseFilledBuffer();
		return 0;
	}

    if (length <= 2 || length > 8192)
		//Receive a 2 byte NAK nothing else to read
		return -1;
    else
        //Length can never be zero
        length -= 2; //length includes 2 byte header which we've already read

    //BUG: We always get a type 0 with length 769(-2) early in the startup sequence
    //readBuf is filled with 0x01 0x03 (repeated)
    //FT_Read fails in this case because there are actually no more bytes to read
    //See if we this is ack, nak, or something else that needs special casing
    //Trace back and see what last FT_Write was?
    ftStatus = FT_Read(ftHandle, readBufProducer, length, &bytesReturned);

	if (ftStatus != FT_OK || bytesReturned != (unsigned)length){
		//mutex.unlock();
        return -1;
	}

    int itemCode = readBufProducer[0] | readBufProducer[1]<<8;
    //qDebug()<<"Type ="<<type<<" ItemCode = "<<itemCode;
	switch (type)
	{
	//Response to Set or Request Current Control Item
	case 0x00:
        switch (itemCode)
		{
		case 0x0001: //Target Name
            targetName = QString((char*)&readBufProducer[2]);
			break;
		case 0x0002: //Serial Number
            serialNumber = QString((char*)&readBufProducer[2]);
			break;
		case 0x0003: //Interface Version
            interfaceVersion = (unsigned)(readBufProducer[2] + readBufProducer[3] * 256);
			break;
		case 0x0004: //Hardware/Firmware Version
            if (readBufProducer[2] == 0x00)
                bootcodeVersion = (unsigned)(readBufProducer[3] + readBufProducer[4] * 256);
            else if (readBufProducer[2] == 0x01)
                firmwareVersion = (unsigned)(readBufProducer[3] + readBufProducer[4] * 256);
			break;
		case 0x0005: //Status/Error Code
            statusCode = (unsigned)(readBufProducer[2]);
			break;
		case 0x0006: //Status String
            statusString = QString((char*)&readBufProducer[4]);
			break;
		case 0x0018: //Receiver Control
            lastReceiverCommand = readBufProducer[3];
			//More to get if we really need it
			break;
		case 0x0020: //Receiver Frequency
            receiverFrequency = (double) readBufProducer[3] + readBufProducer[4] * 0x100 + readBufProducer[5] * 0x10000 + readBufProducer[6] * 0x1000000;
			break;
		case 0x0038: //RF Gain - values are 0 to -30
            rfGain = (qint8)readBufProducer[3];
			break;
		case 0x0040: //IF Gain - values are all 8bit
            ifGain = (qint8)readBufProducer[3];
			break;
		case 0x00B0: //A/D Input Sample Rate
			break;
		default:
			//bool brk = true;
			break;
		}
		break;
	//Unsolicited Control Item
	case 0x01:
		break;
	//Response to Request Control Item Range
	case 0x02:
		break;
	case 0x03:
		//ACK: ReadBuf[0] has data item being ack'e (0-3)
		break;
	//Target Data Item 0
	case 0x04:
		//Should never get here, data blocks handled in separate thread
        qDebug()<<"Should never get to case 0x04 in ReadResponse()";
		break;
	//Target Data Item 1 (not used in SDR-IQ)
	case 0x05:
		break;
	//Target Data Item 2 (not used in SDR-IQ)
	case 0x06:
		break;
	//Target Data Item 3 (not used in SDR-IQ)
	case 0x07:
		break;
	default:
		break;
	}
	//mutex.unlock();
	return bytesReturned;

}
//This is called from Consumer thread
void SDR_IQ::ProcessDataBlocks()
{
	//BYTE ackBuf[] = {0x03,0x60,0x00}; //Ack data block 0
	//DWORD bytesWritten;
	//FT_STATUS ftStatus;
	float I,Q;
    //Wait for data to be available from producer
    AcquireFilledBuffer();

	for (int i=0,j=0; i<inBufferSize; i++, j+=2)
	{
		/*
		After a couple of days of banging my head trying to figure why I couldn't get anything but static,
		I figured out how to interpret the data coming back from the SDR-IQ.
		I tried converting LSB/MSB to an integer (0-65535) and normalizing to -32767 to +32767
			I = (SHORT)(readBuf[j] + (readBuf[j + 1] * 0x100));
			I -= 32767.0;
		I tried bit shifting and cast to signed
			I = (short)(readBuf[j] | (readBuf[j + 1] <<8))
		And several other crazy (with hindsight) interpretations
		Finally this worked: csst address in buffer as (short *)
		Update: Reading the AD6620 data sheet, I see that the data is a 16bit integer in 2's compliment form
		Better late than never!
		*/
		//I = *(short *)(&readBuf[j]);
        //producerBuffer is an array of bytes that we need to treat as array of shorts
        I = ((short **)producerBuffer)[nextConsumerDataBuf][j];
		//Convert to float: div by 32767 for -1 to 1, 
		I /= (32767.0); 
		//SoundCard levels approx 0.02 at 50% mic

		//maxI = I>maxI ? I : maxI;
		//minI = I<minI ? I : minI;


		//Q = *(short *)(&readBuf[j+2]);
        Q = ((short **)producerBuffer)[nextConsumerDataBuf][j+1];
		Q /= (32767.0);

		//IQ appear to be reversed
		inBuffer[i].re =  Q;
		inBuffer[i].im =  I;
		//qDebug() << QString::number(I) << ":" << QString::number(Q);
		//qDebug() << minI << "-" << maxI;

	}

	//Not sure if this is required for SDR-IQ, but it is for SDR-14
	//Send Ack
	//FT_Write(sdr_iq->ftHandle,ackBuf,sizeof(ackBuf),&bytesWritten);
	
	//We're done with databuf, so we can release before we call ProcessBlock
	//Update lastDataBuf & release dataBuf
    IncrementConsumerBuffer();
    ReleaseFreeBuffer();

	if (receiver != NULL)
        receiver->ProcessBlock(inBuffer,outBuffer,inBufferSize);
}

void SDR_IQ::FlushDataBlocks()
{
	if (!isFtdiLoaded)
		return;

    FT_STATUS ftStatus;
    DWORD bytesReturned = 0;

    do
        ftStatus = FT_Read(ftHandle, readBuf, 1, &bytesReturned);
    while (ftStatus == FT_OK && bytesReturned > 0);
}

void SDR_IQ::StopProducerThread()
{
	StopCapture();
}

void SDR_IQ::RunProducerThread()
{
    if (ReadResponse(0x04) == -1)
        qDebug()<<"Error in SDR-IQ ReadResponse"; //Error
}

void SDR_IQ::StopConsumerThread()
{

}

void SDR_IQ::RunConsumerThread()
{
    ProcessDataBlocks();
}

//Dialog Stuff
void SDR_IQ::rfGainChanged(int i)
{
	rfGain = i * -10;
	sRFGain = rfGain;
	SetRFGain(rfGain);
}
void SDR_IQ::ifGainChanged(int i)
{
	ifGain = i * 6;
	sIFGain = ifGain;
	SetIFGain(ifGain);
}
void SDR_IQ::bandwidthChanged(int i)
{
	switch (i)
	{
	case 0: //5k
		bandwidth = SDR_IQ::BW5K;
		break;
	case 1: //10k
		bandwidth = SDR_IQ::BW10K;
		break;
	case 2: //25k
		bandwidth = SDR_IQ::BW25K;
		break;
	case 3: //50k
		bandwidth = SDR_IQ::BW50K;
		break;
	case 4: //100k
		bandwidth = SDR_IQ::BW100K;
		break;
	case 5: //150k
		bandwidth = SDR_IQ::BW150K;
		break;
	case 6: //190k
		bandwidth = SDR_IQ::BW190K;
		break;
	}
	sBandwidth = bandwidth; //If settings are saved, this is saved
}
void SDR_IQ::ShowOptions()
{
    QFont smFont = settings->smFont;
    QFont medFont = settings->medFont;
    QFont lgFont = settings->lgFont;

	if (sdrIQOptions == NULL)
	{
		sdrIQOptions = new QDialog();
		iqo = new Ui::SDRIQOptions();
		iqo->setupUi(sdrIQOptions);

        iqo->bandwidthBox->setFont(medFont);
        iqo->bootLabel->setFont(medFont);
        iqo->firmwareLabel->setFont(medFont);
        iqo->ifGainBox->setFont(medFont);
        iqo->interfaceVersionLabel->setFont(medFont);
        iqo->label->setFont(medFont);
        iqo->label_2->setFont(medFont);
        iqo->label_3->setFont(medFont);
        iqo->nameLabel->setFont(medFont);
        iqo->rfGainBox->setFont(medFont);
        iqo->serialNumberLabel->setFont(medFont);

		iqo->rfGainBox->addItem("  0db");
		iqo->rfGainBox->addItem("-10db");
		iqo->rfGainBox->addItem("-20db");
		iqo->rfGainBox->addItem("-30db");
		connect(iqo->rfGainBox,SIGNAL(currentIndexChanged(int)),this,SLOT(rfGainChanged(int)));

		iqo->ifGainBox->addItem("  0db");
		iqo->ifGainBox->addItem(" +6db");
		iqo->ifGainBox->addItem("+12db");
		iqo->ifGainBox->addItem("+18db");
		iqo->ifGainBox->addItem("+24db");
		connect(iqo->ifGainBox,SIGNAL(currentIndexChanged(int)),this,SLOT(ifGainChanged(int)));

		iqo->bandwidthBox->addItem(" 5KHz");
		iqo->bandwidthBox->addItem(" 10KHz");
		iqo->bandwidthBox->addItem(" 25KHz");
		iqo->bandwidthBox->addItem(" 50KHz");
		iqo->bandwidthBox->addItem("100KHz");
		iqo->bandwidthBox->addItem("150KHz");
		iqo->bandwidthBox->addItem("190KHz");
		connect(iqo->bandwidthBox,SIGNAL(currentIndexChanged(int)),this,SLOT(bandwidthChanged(int)));

	}
	iqo->nameLabel->setText("Name: " + targetName);
	iqo->serialNumberLabel->setText("Serial #: " + serialNumber);
	iqo->firmwareLabel->setText("Firmware Version: " + QString::number(firmwareVersion));
	iqo->bootLabel->setText("Bootcode Version: " + QString::number(bootcodeVersion));
	iqo->interfaceVersionLabel->setText("Interface Version: " + QString::number(interfaceVersion));

	if (rfGain != 0)
		iqo->rfGainBox->setCurrentIndex(rfGain / -10);
	if (ifGain != 0)
		iqo->ifGainBox->setCurrentIndex(ifGain / 6);

	iqo->bandwidthBox->setCurrentIndex(sBandwidth);

	sdrIQOptions->show();
}
