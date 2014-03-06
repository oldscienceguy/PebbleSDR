#ifndef AFEDRI_H
#define AFEDRI_H

#include <QObject>
#include "hidapi.h"

#define HID_TIMEOUT 1000 //ms
#define HID_COMMAND_MAX_SIZE 7

#define HID_COMMAND_SIZE 7

//Reports and commands from fe_interface.h AFEDRI control source
#define HID_MEMORY_READ_WRITE_REPORT 1
#define HID_FREQUENCY_REPORT 3
#define HID_GENERIC_REPORT 2
#define HID_GENERIC_GAIN_COMMAND 2
#define HID_GENERIC_INIT_FE_COMMAND 0xE1
#define HID_GENERIC_GET_FREQ_COMMAND 1
#define HID_GENERIC_GET_INIT_STATUS_COMMAND 4
#define HID_GENERIC_DAC_COMMAND 8
#define HID_GENERIC_VER_COMMAND 9
#define HID_GENERIC_CONSOLE_IO_COMMAND 10
#define HID_GENERIC_HID_CONSOLE_ON 11
#define HID_READ_EEPROM_COMMAND 0x55
#define HID_WRITE_EEPROM_COMMAND 0x56
#define HID_GENERIC_SAVE_DEFAULT_PARAMS_COMMAND 12
#define HID_GENERIC_GET_SR_COMMAND 14
#define HID_GENERIC_GET_DIVERSITY_MODE 15

#define HID_GENERIC_GET_SDR_IP_ADDRESS 16
#define HID_GENERIC_GET_SDR_IP_MASK 17
#define HID_GENERIC_GET_GATEWAY_IP_ADDRESS 18
#define HID_GENERIC_GET_DST_IP_ADDRESS 19
#define HID_GENERIC_GET_DST_PORT 20

#define HID_GENERIC_SAVE_SDR_IP_ADDRESS 21
#define HID_GENERIC_SAVE_SDR_IP_MASK 22
#define HID_GENERIC_SAVE_GATEWAY_IP_ADDRESS 23
#define HID_GENERIC_SAVE_DST_IP_ADDRESS 24
#define HID_GENERIC_SAVE_DST_PORT 25
#define HID_GENERIC_START_UDP_STREAM_COMMAND 26
#define HID_GENERIC_OVERLAOD_STATE 27
#define HID_GENERIC_ACK 28
#define HID_GENERIC_GET_SERIAL_NUMBER_COMMAND 29
#define HID_GENERIC_SET_SAMPLE_RATE_COMMAND 30
#define HID_GENERIC_SET_VCO_COMMAND 31
#define HID_GENERIC_GET_VCO_COMMAND 32
#define HID_GENERIC_SAVE_GAIN_TABLE_COMMAND 33
#define HID_GENERIC_GET_GAIN_TABLE_COMMAND 34
#define HID_GENERIC_SET_BPF_COMMAND 35
#define HID_GENERIC_SYNC_WORD_COMMAND 36
#define HID_GENERIC_HW_CPLD_SN_COMMAND 37
#define HID_GENERIC_IAP_COMMAND 38
#define HID_GENERIC_WRITE_HWORD_TO_FLASH 39
#define HID_GENERIC_BROADCAST_COMMAND 40
#define HID_GENERIC_GET_BROADCAST_STATE_COMMAND 41
#define HID_GENERIC_BYPASS_LPF_COMMAND 42
#define HID_GENERIC_GET_BYPASS_LPF_COMMAND 43
#define HID_GENERIC_SET_MULTICHANNEL_COMMAND 48
#define HID_GENERIC_MULTICAST_COMMAND 49
#define HID_GENERIC_SET_AFEDRI_ID_COMMAND 50
#define HID_GENERIC_GET_AFEDRI_ID_COMMAND 51
#define HID_GENERIC_SAVE_AFEDRI_ID_COMMAND 52
#define HID_GENERIC_SOFT_RESET_COMMAND 53
#define HID_GENERIC_GET_DHCP_STATE_COMMAND 54
#define HID_GENERIC_SET_DHCP_STATE_COMMAND 55

#define VADDRESS_MAIN_CLOCK_FREQ_LOW_HALFWORD   0x0000
#define VADDRESS_MAIN_CLOCK_FREQ_HIGH_HALFWORD  0x0001
#define VADDRESS_CICDECRATE                     0x0002
#define VADDRESS_CICPARAMS                      0x0003
#define VADDRESS_SAMPLE_RATE_LO                 0x0006
#define VADDRESS_SAMPLE_RATE_HI                 0x0007
#define VADDRESS_DIVERSITY_MODE                 0x0008
#define VADDRESS_DIVERSITY_SAMPLE_RATE_LO       0x0009
#define VADDRESS_DIVERSITY_SAMPLE_RATE_HI       0x000A
#define VADDRESS_SDR_IP_ADDRESSS_BYTE_4_3       0x000B
#define VADDRESS_SDR_IP_ADDRESSS_BYTE_2_1       0x000C
#define VADDRESS_SDR_IP_MASK_BYTE_4_3           0x000F
#define VADDRESS_SDR_IP_MASK_BYTE_2_1           0x0010
#define VADDRESS_GATEWAY_IP_BYTE_4_3            0x0013
#define VADDRESS_GATEWAY_IP_BYTE_2_1            0x0014
#define VADDRESS_DST_IP_ADDRESSS_BYTE_4_3       0x0017
#define VADDRESS_DST_IP_ADDRESSS_BYTE_2_1       0x0018
#define VADDRESS_DST_UDP_PORT                   0x001A
#define VADDRESS_VCO_VOLTAGE                    0x001B

#define VADDRESS_BASE                           0x5500


//See Funcube plugin for another HID interface example
class AFEDRI
{
public:
	AFEDRI();
	~AFEDRI();
	void Initialize();
	bool Connect();
	bool Disconnect();
	int HIDWrite(unsigned char *data, int length);
	int HIDRead(unsigned char *data, int length);

	bool HIDGet(char cmd, void *data, int len);
	bool HIDSet(char cmd, unsigned char value);
	int SetFrequency(quint32 frequency);
	int WriteReport(unsigned char *report);
	QString Get_Serial_Number();
	int Send_Generic_Command_No_Param(unsigned char command);
	QString Get_HW_CPLD_SN();
	int Set_New_Sample_Rate(unsigned long SampleRate);
	int Send_Generic_Long_Data(unsigned char command, unsigned long eeprom_data);
	int Send_EEPROM_Data(unsigned char VirtAddress, unsigned short eeprom_data);
	int Send_Sample_Rate(unsigned long SampleRate);
private:
	hid_device *hidDev;
	hid_device_info *hidDevInfo;

	//Device constants
	static const unsigned short ATEECS_VID = 0x255D;
	static const unsigned short ATEECS_PID = 0x0001;
	static const unsigned short SDR_NET_PID = 0x0002;
	int maxPacketSize;

	bool connected;
};

#endif // AFEDRI_H
