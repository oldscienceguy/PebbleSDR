#ifndef USBUTIL_H
#define USBUTIL_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QLibrary>
#include "../d2xx/bin/ftd2xx.h"
#include "../d2xx/libusb/libusb/libusb.h"

/*
  FTDI Devices
    - Elektor SDR
    - SDR-IQ

  LibUSB 1.0 Devices (libusb.org also in FTDI cross platform libries)
    - Softrock
	- HPSDR (Libusb0 for Windows)

  HID Devices
    - FunCube Dongle

  Options
  - Use d2xx libraries ftd2xx.h and libusb.h
  - Use d2xx libraries darwin_usb.h and windows_usb.h
  - Use mac usb.h <IOKit>
*/

/*
  Mac & Win USB utilities
*/

//LibUSB 1.0
#define REQUEST_TYPE_VENDOR LIBUSB_REQUEST_TYPE_VENDOR
#define RECIP_DEVICE LIBUSB_RECIPIENT_DEVICE
#define ENDPOINT_IN LIBUSB_ENDPOINT_IN
#define ENDPOINT_OUT LIBUSB_ENDPOINT_OUT

class USBUtil
{
public:
	//Attempt to abstract across libraries so each device doesn't have to know details
	enum USB_LIB_TYPE {LIB_USB, FTDI_D2XX, USB_H};

	USBUtil(USB_LIB_TYPE _libType = LIB_USB);

	bool LoadUsb();
	bool InitUSB();
	bool OpenDevice();
	bool CloseDevice();
	//FTDI Specific, review all these and rename to indicate FT specific
	bool ResetDevice();
	bool SetBaudRate(quint16 _baudRate);
	bool SetBitMode(unsigned char _ucMask, unsigned char _ucEnable);
	bool Write(void *_buffer, quint32 _length);


	bool SetConfiguration(int config);
	bool Claim_interface(int iface);
	bool ReleaseInterface(int iface);
	int ControlMsg(uint8_t reqType, uint8_t req, uint16_t value, uint16_t index,
            unsigned char *data, uint16_t length, unsigned int timeout);

	bool FindDevice(int PID, int VID, int multiple); //LibUSB search terms
	bool FindAndOpenDevice(int PID, int VID, int multiple); //LibUSB search terms

	bool FindDevice(QString query, bool matchSerialNumber); //FTDI

	void ListDevices();

	bool ReadArray(quint16 index, quint16 address, unsigned char *data, quint16 length);
	bool WriteArray(quint16 index, quint16 address, unsigned char*data, quint16 length);
	bool WriteReg(quint16 index, quint16 address, quint16 value, quint16 length);
    //LibUSBReadReg();

	bool BulkTransfer(unsigned char endpoint, unsigned char *data, int length,
							int *actual_length, unsigned int timeout);

	bool IsUSBLoaded();
	bool Exit();

	bool Read(void *_buffer, quint32 _bytesToRead);
	bool Purge();
	bool SetTimeouts(quint32 _readTimeout, quint32 _writeTimeout);
	quint32 GetQueueStatus();
	bool SetUSBParameters(quint32 inboundBufferSize, quint32 outboundBufferSize);
private:
	USB_LIB_TYPE libType;
	quint16 timeout;

	bool isLibUsbLoaded;
	libusb_device *libusbDev;
	libusb_device_handle* libusbDevHandle;

	bool isFtdiLoaded;
	int ftdiDevice;
	FT_HANDLE ftHandle;

};

#endif // USBUTIL_H
