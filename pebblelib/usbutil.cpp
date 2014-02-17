//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "usbutil.h"

USBUtil::USBUtil()
{
    timeout = 1000; //default timeout in ms
	dev = NULL;
	hDev = NULL;
}

bool USBUtil::LoadLibUsb()
{
	//Placeholder for future
	isLibUsbLoaded = true;
	return true;
}

bool USBUtil::LoadFtdi()
{
	return true;
}

//Returns device # for serialNumber or -1 if not found
int USBUtil::FTDI_FindDeviceBySerialNumber(QString serialNumber)
{
    FT_STATUS ftStatus;
    DWORD numDevices;
    int deviceNumber = -1;
    //Get all the FTDI USB devices that are connected
    ftStatus = FT_CreateDeviceInfoList(&numDevices);
    if (ftStatus != FT_OK)
        return -1;

    //Allocate array for numDevices
    FT_DEVICE_LIST_INFO_NODE *pDeviceInfoList = new FT_DEVICE_LIST_INFO_NODE[numDevices];
    ftStatus = FT_GetDeviceInfoList(pDeviceInfoList, &numDevices);
    if (ftStatus != FT_OK)
        return -1;
    //We've got devices, see if ours is connected
    for (unsigned i=0; i<numDevices; i++)
    {
        if (QString::compare(pDeviceInfoList[i].SerialNumber,serialNumber) == 0)
        {
            deviceNumber = i;
            break;
        }

    }
    return deviceNumber;
}

int USBUtil::FTDI_FindDeviceByName(QString deviceName)
{
    FT_STATUS ftStatus;
    DWORD numDevices;
    int deviceNumber = -1;
    //Get all the FTDI USB devices that are connected
    ftStatus = FT_CreateDeviceInfoList(&numDevices);
    if (ftStatus != FT_OK)
        return -1;

    //Allocate array for numDevices
    FT_DEVICE_LIST_INFO_NODE *pDeviceInfoList = new FT_DEVICE_LIST_INFO_NODE[numDevices];
    ftStatus = FT_GetDeviceInfoList(pDeviceInfoList, &numDevices);
    if (ftStatus != FT_OK)
        return -1;
    //We've got devices, see if ours is connected
    for (unsigned i=0; i<numDevices; i++)
    {
        if (QString(pDeviceInfoList[i].Description).contains(deviceName,Qt::CaseInsensitive))
        {
            deviceNumber = i;
            break;
        }

    }
    return deviceNumber;
}

bool USBUtil::InitUSB()
{
#ifdef LIBUSB_VERSION1
    int ret = libusb_init(NULL);
    return ret == 0 ? true: false;
#else
    usb_init(NULL);
    usb_find_busses();
    usb_find_devices();
    return true;
#endif
}
bool USBUtil::OpenDevice()
{
#ifdef LIBUSB_VERSION1
	int ret = libusb_open(dev,&hDev);
    if (ret < 0 ) {
		hDev = NULL;
        return false;
    }
    return true;
#else
    hDev = usb_open(dev);

#endif
    return true;
}
bool USBUtil::CloseDevice()
{
#ifdef LIBUSB_VERSION1
    libusb_close(hDev);
#else
    usb_close(hDev);
#endif
	hDev = NULL;
    return true;
}
bool USBUtil::SetConfiguration(int config)
{
#ifdef LIBUSB_VERSION1
    int ret = libusb_set_configuration(hDev,config);
#else
    usb_set_configuration(hDev,config);
#endif
    return ret == 0 ? true: false;
}
bool USBUtil::Claim_interface(int iface)
{
	if (hDev==NULL)
		return false;

#ifdef LIBUSB_VERSION1
    int ret = libusb_claim_interface(hDev,iface);
#else
#endif
    return ret == 0 ? true: false;
}

bool USBUtil::ReleaseInterface(int iface)
{
	if (hDev==NULL)
		return false;

#ifdef LIBUSB_VERSION1
	int ret = libusb_release_interface(hDev,iface);
#else
#endif
	return ret == 0 ? true: false;
}

/*
libusb_error {
  LIBUSB_SUCCESS = 0, LIBUSB_ERROR_IO = -1, LIBUSB_ERROR_INVALID_PARAM = -2, LIBUSB_ERROR_ACCESS = -3,
  LIBUSB_ERROR_NO_DEVICE = -4, LIBUSB_ERROR_NOT_FOUND = -5, LIBUSB_ERROR_BUSY = -6, LIBUSB_ERROR_TIMEOUT = -7,
  LIBUSB_ERROR_OVERFLOW = -8, LIBUSB_ERROR_PIPE = -9, LIBUSB_ERROR_INTERRUPTED = -10, LIBUSB_ERROR_NO_MEM = -11,
  LIBUSB_ERROR_NOT_SUPPORTED = -12, LIBUSB_ERROR_OTHER = -99
}
*/
int USBUtil::ControlMsg(uint8_t reqType, uint8_t req, uint16_t value, uint16_t index,
						 unsigned char *data, uint16_t length, unsigned int timeout)
{
#ifdef LIBUSB_VERSION1
    return libusb_control_transfer(hDev,reqType,req,value,index,data,length,timeout);
#else
    return = usb_control_msg(hDev,reqType, req, value, data, index, length, timeout);

#endif
}

int USBUtil::LibUSBControlMsg(uint8_t reqType, uint8_t req, uint16_t value, uint16_t index, unsigned char *data, uint16_t length, unsigned int timeout)
{
	return ControlMsg(reqType,req,value,index,data,length,timeout);
}

bool USBUtil::LibUSBBulkTransfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
{
	return libusb_bulk_transfer(hDev, endpoint, data, length, actual_length, timeout);
}

bool USBUtil::IsUSBLoaded()
{
	return isLibUsbLoaded;
	//return isFtdiLoaded;
}

bool USBUtil::Exit()
{
	libusb_exit(NULL);
	return true;
}

//Assumes InitLibUsb already called
bool USBUtil::LibUSB_FindAndOpenDevice(int PID, int VID, int multiple)
{
	Q_UNUSED(multiple);

#ifdef LIBUSB_VERSION1
#if 1
        //Can't use this because we support multiple PIDs and may want nth one, but it works
        hDev = libusb_open_device_with_vid_pid(NULL,VID,PID);
		return hDev != NULL;
#else
    //This crashes with memAccess, use above for now
	int numFound = 0;
	libusb_device *dev; //Our device (hopefully)
    libusb_device **devs; //List of all the devices found
    libusb_device_handle hDev;

    hDev=NULL;

    int cnt = libusb_get_device_list(NULL,&devs);
    if (cnt < 0)
        return NULL;
    int i=0;
    while ((dev = devs[i++]) != NULL) {
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            libusb_free_device_list(devs,1);
            return NULL; //Failed to get descriptor
        }

        if ( VID == desc.idVendor && PID == desc.idProduct) {
            //Got our device
            //There may be multiple devices and caller wants Nth device
            if (numFound++ == multiple) {
                libusb_free_device_list(devs,1);
				OpenDevice(dev,&hDev);
                return hDev;
            }
            //libusb_get_bus_number(dev);
            //libusb_get_device_address(dev);
        }
    }

    libusb_free_device_list(devs,1);
    return NULL;
#endif
#else
    struct usb_device *dev;
    struct usb_bus *busses;
    busses = usb_get_busses();

    //Iterate through busses and devices looking for a PID VID match to SoftRock
    while (busses)
    {
        //For each bus, check each device
        dev = busses->devices;
        while (dev)
        {
            //Todo: We can build a list of all usb devices here to help user troubleshoot
            if (dev->descriptor.idProduct==PID && dev->descriptor.idVendor==VID)
            {
                //Got our device
                //There may be multiple devices and caller wants Nth device
                if (numFound++ == multiple)
                    return dev;
            }
            dev = dev->next;
        }
        busses = busses->next;
    }

    return NULL; //Nothing found
#endif
}

libusb_device * USBUtil::FindDevice(int PID, int VID, int multiple)
{
    int numFound = 0;

    libusb_device **devs; //List of all the devices found

    int cnt = libusb_get_device_list(NULL,&devs);
    if (cnt < 0) {
        qDebug("No Devices Found");
        return NULL;
    }
    int i=0;
    while ((dev = devs[i++]) != NULL) {
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            libusb_free_device_list(devs,1);
            qDebug("Failed to get descriptor");
            return NULL;
        }
        if ( VID == desc.idVendor && PID == desc.idProduct) {
            //Got our device
            //There may be multiple devices and caller wants Nth device
            if (numFound++ == multiple) {
                libusb_free_device_list(devs,1);
                return dev;
            }
        }

    }

    libusb_free_device_list(devs,1);
    return NULL;

}

//Dumps device info to debug for now
void USBUtil::ListDevices()
{
    libusb_device **devs; //List of all the devices found

    unsigned char mfg[256]; //string buffer
    unsigned char prod[256]; //string buffer

    int cnt = libusb_get_device_list(NULL,&devs);
    if (cnt < 0) {
        qDebug("No Devices Found");
        return;
    }
    int i=0;
    while ((dev = devs[i++]) != NULL) {
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            libusb_free_device_list(devs,1);
            qDebug("Failed to get descriptor");
            return;
        }
        //Get ascii string for mfg descriptor
		OpenDevice();
        libusb_get_string_descriptor_ascii(hDev,desc.iManufacturer,mfg,sizeof(mfg));
        libusb_get_string_descriptor_ascii(hDev,desc.iProduct,prod,sizeof(prod));
        qDebug("%s %s Class %d VID 0x%x PID 0x%x",mfg, prod, desc.bDeviceClass, desc.idVendor,desc.idProduct);
		CloseDevice();
    }

    libusb_free_device_list(devs,1);
    return;

}

//Utility function to read array
bool USBUtil::LibUSBReadArray(quint16 index, quint16 address, unsigned char *data, quint16 length)
{
    quint8 reqType = LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN;
    quint8 req = 0;
    return LibUSBControlMsg(reqType,req,address,index,data,length,timeout);
}

//Writes array
bool USBUtil::LibUSBWriteArray(quint16 index, quint16 address, unsigned char*data, quint16 length)
{
    quint8 reqType = LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT;
    quint8 req = 0;
    return LibUSBControlMsg(reqType,req,address,index,data,length,timeout);
}

//Writes a 1 or 2 byte register
bool USBUtil::LibUSBWriteReg(quint16 index, quint16 address, quint16 value, quint16 length)
{
    unsigned char data[2];
    quint8 reqType = LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT;
    quint8 req = 0;

    //Transfer value into data buffer in right order
    if (length == 1) {
        //Single byte in data[0], data[1] doesn't matter
        data[0] = value & 0xff;
        data[1] = value & 0xff;
    } else {
        //2 bytes:  HOB in data[0], LOB in data[1]
        data[0] = value >> 8; //Shift hob to lob
        data[1] = value & 0xff; //mask hob so lob is all that's left
    }

	return LibUSBControlMsg(reqType,req,address,index,data,length,timeout);
}

