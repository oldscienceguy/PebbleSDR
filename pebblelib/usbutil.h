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
    //Placeholders to make sure we have libraries available
	bool LoadLibUsb();
	bool LoadFtdi();

	int FTDI_FindDeviceByName(QString deviceName);
	int FTDI_FindDeviceBySerialNumber(QString serialNumber);
    //LibUSB
	bool InitUSB();

	bool OpenDevice();
	bool CloseDevice();
	bool SetConfiguration(int config);
	bool Claim_interface(int iface);
	bool ReleaseInterface(int iface);
	int ControlMsg(uint8_t reqType, uint8_t req, uint16_t value, uint16_t index,
            unsigned char *data, uint16_t length, unsigned int timeout);
    int LibUSBControlMsg(uint8_t reqType, uint8_t req, uint16_t value, uint16_t index,
            unsigned char *data, uint16_t length, unsigned int timeout);
	libusb_device *FindDevice(int PID, int VID, int multiple);


	bool LibUSB_FindAndOpenDevice(int PID, int VID, int multiple);

	void ListDevices();

    bool LibUSBReadArray(quint16 index, quint16 address, unsigned char *data, quint16 length);
    bool LibUSBWriteArray(quint16 index, quint16 address, unsigned char*data, quint16 length);
    bool LibUSBWriteReg(quint16 index, quint16 address, quint16 value, quint16 length);
    //LibUSBReadReg();

	bool LibUSBBulkTransfer(unsigned char endpoint, unsigned char *data, int length,
							int *actual_length, unsigned int timeout);

	bool IsUSBLoaded();
	bool Exit();

private:
	USB_LIB_TYPE libType;

	libusb_device *dev;
    libusb_device_handle* hDev;
    quint16 timeout;

	bool isLibUsbLoaded;
	bool isFtdiLoaded;

};

#endif // USBUTIL_H
