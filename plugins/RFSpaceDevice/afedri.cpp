#include "afedri.h"
#include <QDebug>
#include <QThread>

AFEDRI::AFEDRI()
{
	hidDev = NULL;
	hidDevInfo = NULL;

}

AFEDRI::~AFEDRI()
{
	hid_exit();
}

void AFEDRI::Initialize()
{
	hid_init();
}

bool AFEDRI::Connect()
{
	//Don't know if we can get this from DDK calls
	maxPacketSize = 64;

	//If this doesn't work, try ATEECS_PID instead of SDR_NET.  Not sure which is used where
	hidDevInfo = hid_enumerate(ATEECS_VID, SDR_NET_PID);
	if (hidDevInfo == NULL) {
		hidDevInfo = hid_enumerate(ATEECS_VID, ATEECS_PID);
		if (hidDevInfo == NULL)
			return false;
	} else {
		//Command size = 5 or 7 depending on device?
		//HID_COMMAND_SIZE = 7;
	}

	char *hidPath = strdup(hidDevInfo->path); //Get local copy
	if (hidPath == NULL)
		return false;
	hid_free_enumeration(hidDevInfo);
	hidDevInfo = NULL;

	if ((hidDev = hid_open_path(hidPath)) == NULL){
		free(hidPath);
		hidPath = NULL;
		return false;
	}

	free(hidPath);
	hidPath = NULL;
	connected = true;
	return true;
}

bool AFEDRI::Disconnect()
{
	//Mac sometimes crashes when hid_close is called, but we need it
	if (hidDev != NULL)
		hid_close(hidDev);
	hidDev = NULL;
	connected = false;
	return true;
}

int AFEDRI::WriteReport(unsigned char * report)
{
	if (!connected)
		return HID_TIMEOUT;

	return hid_write(hidDev, report, HID_COMMAND_SIZE);
}

//Not used
int AFEDRI::HIDWrite(unsigned char *data, int length)
{
	if (!connected)
		return HID_TIMEOUT;

	return hid_write(hidDev,data,length);
}

/*
 usb error codes
 -116	Timeout
 -22	Invalid arg
 -5
 */
int AFEDRI::HIDRead(unsigned char *data, int length)
{
	if (!connected)
		return HID_TIMEOUT;

	memset(data,0x00,length); // Clear out the response buffer
	int bytesRead = hid_read(hidDev,data,length);
	//Caller does not expect or handle report# returned from windows
	if (bytesRead > 0) {
		bytesRead--;
		memcpy(data,&data[1],bytesRead);
	}
	return bytesRead;
}

//Not used
bool AFEDRI::HIDSet(char cmd, unsigned char value)
{
	if (!connected)
		return false;

	unsigned char outBuf[maxPacketSize+1];
	unsigned char inBuf[maxPacketSize];

	outBuf[0] = 0; // Report ID, ignored
	outBuf[1] = cmd;
	outBuf[2] = value;

	int numBytes = HIDWrite(outBuf,sizeof(outBuf));
	if (numBytes < 0) {
		qDebug()<<"FCD: HIDSet-HIDWrite error\n";
		return false; //Write error
	}
	numBytes = HIDRead(inBuf,sizeof(inBuf));
	if (numBytes < 0) {
		qDebug()<<"FCD: HIDSet-HIDRead after HIDWrite error \n";
		return false; //Read error
	}

	return true;
}

//Not used
bool AFEDRI::HIDGet(char cmd, void *data, int len)
{
	if (!connected)
		return false;

	unsigned char outBuf[maxPacketSize+1]; //HID Write skips report id 0 and adj buffer, so we need +1 to send 64
	unsigned char inBuf[maxPacketSize];

	outBuf[0] = 0; // Report ID, ignored
	outBuf[1] = cmd;

	int numBytes = HIDWrite(outBuf,sizeof(outBuf));
	if (numBytes < 0) {
		qDebug()<<"FCD: HIDGet-Write error\n";
		return false; //Write error
	}
	numBytes = HIDRead(inBuf,sizeof(inBuf));
	if (numBytes < 0) {
		qDebug()<<"FCD: HIDGet-Read after Write error\n";
		return false; //Read error
	}
#if 0
	// FCD Plus fails this test
	//inBuf[0] = cmd we're getting data for
	//inBuf[1] = First data byte
	if (inBuf[0] != cmd){
		logfile<<"FCD: Invalid read response for cmd: "<<cmd<<"\n";
		return false;
	}
#endif
	//Don't return cmd, strip from buffer
	int retLen = numBytes-1 < len ? numBytes-1 : len;
	memcpy(data,&inBuf[1],retLen);
	return true;
}

int AFEDRI::SetFrequency(quint32 frequency)
{
	int i;
	quint8 report[HID_COMMAND_MAX_SIZE]={0};
	report[0] = HID_FREQUENCY_REPORT;
	for(i=0; i < 4;  i++) {
		report[i+1]= (frequency >> (i*8)) & 0xFF;
	}
	for(i=5; i < HID_COMMAND_MAX_SIZE;  i++)
		report[i]=0;

	int ret = WriteReport((unsigned char *)report);
	QThread::msleep(100); //Short delay to settle
	return ret;
}

int AFEDRI::Send_Sample_Rate(unsigned long SampleRate)
{
	int  res;
	unsigned long tmp=0;
	tmp = SampleRate & 0xFFFF;
	res = Send_EEPROM_Data(VADDRESS_SAMPLE_RATE_LO, tmp);
//            sleep(100);
	tmp = (SampleRate >> 16) & 0xFFFF;
	//HID_wait_device_answer();
	res = Send_EEPROM_Data(VADDRESS_SAMPLE_RATE_HI, tmp);
	return res;
}

int AFEDRI::Set_New_Sample_Rate(unsigned long SampleRate)
{
	int  res;
	res =  Send_Generic_Long_Data(HID_GENERIC_SET_SAMPLE_RATE_COMMAND, SampleRate);
	return res;
}

QString AFEDRI::Get_Serial_Number()
{
	//Clear previous stuff
	quint8 buf[256];
	while (hid_read_timeout(hidDev,buf,HID_COMMAND_SIZE,1000))
		;
	memset(buf,0x00,256);
	if (Send_Generic_Command_No_Param(HID_GENERIC_GET_SERIAL_NUMBER_COMMAND) > 0){
		if (hid_read(hidDev, (unsigned char *)buf, HID_COMMAND_SIZE) == HID_COMMAND_SIZE){
			//This is NOT the same format we get back from SDR-IQ and SDR-IP!
			return QString("%1%2%3%4")
			.arg(buf[5],2,16,QChar('0'))
			.arg(buf[4],2,16,QChar('0'))
			.arg(buf[3],2,16,QChar('0'))
			.arg(buf[2],2,16,QChar('0'));
		}
	}
	return "Error";
}

QString AFEDRI::Get_HW_CPLD_SN()
{
	//Clear previous stuff
	quint8 buf[256];
	while (hid_read_timeout(hidDev,buf,HID_COMMAND_SIZE,1000))
		;
	memset(buf,0x00,256);
	if (Send_Generic_Command_No_Param(HID_GENERIC_HW_CPLD_SN_COMMAND) > 0){
		if (hid_read(hidDev, (unsigned char *)buf, HID_COMMAND_SIZE) == HID_COMMAND_SIZE){
			//This is NOT the same format we get back from SDR-IQ and SDR-IP!
			return QString("%1%2%3%4")
			.arg(buf[5],2,16,QChar('0'))
			.arg(buf[4],2,16,QChar('0'))
			.arg(buf[3],2,16,QChar('0'))
			.arg(buf[2],2,16,QChar('0'));
		}
	}
	return "Error";
}

int AFEDRI::Send_EEPROM_Data(unsigned char VirtAddress, unsigned short eeprom_data)
{
	char report[HID_COMMAND_MAX_SIZE]={0};
	report[0] = HID_MEMORY_READ_WRITE_REPORT;
	report[1]=  HID_WRITE_EEPROM_COMMAND & 0xFF;
	report[2]= VirtAddress;
	report[3]= eeprom_data & 0xFF;
	report[4]= (eeprom_data >> 8)& 0xFF;
	for(int i=5; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
	return WriteReport((unsigned char *)report);
}

int AFEDRI::Send_Generic_Command_No_Param(unsigned char command)
{
	char report[HID_COMMAND_MAX_SIZE]={0};
	report[0] = HID_GENERIC_REPORT;
	report[1] = command;
	report[2] = 0;
	for(int i=3; i < HID_COMMAND_MAX_SIZE;  i++)
		report[i]=0;
	return WriteReport((unsigned char *)report);
}

int AFEDRI::Send_Generic_Long_Data(unsigned char command, unsigned long eeprom_data)
{
	char report[HID_COMMAND_MAX_SIZE]={0};
	report[0] = HID_GENERIC_REPORT;
	report[1]=  command & 0xFF;
	report[2]=  eeprom_data & 0xFF;
	report[3]= (eeprom_data >> 8)& 0xFF;
	report[4]= (eeprom_data >> 16)& 0xFF;
	report[5]= (eeprom_data >> 24)& 0xFF;
	for(int i=6; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
	return WriteReport((unsigned char *)report);
}

