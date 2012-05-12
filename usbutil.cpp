#include "usbutil.h"

USBUtil::USBUtil()
{
    timeout = 1000; //default timeout in ms
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

bool USBUtil::LibUSBInit()
{
    return USBUtil::InitLibUsb();
}

bool USBUtil::InitLibUsb()
{
#ifdef LIBUSB_VERSION1
    int res = libusb_init(NULL);
    return true;
#else
    usb_init(NULL);
    usb_find_busses();
    usb_find_devices();
    return true;
#endif
}
bool USBUtil::OpenDevice(libusb_device *dev, libusb_device_handle **phDev)
{
#ifdef LIBUSB_VERSION1
    int ret = libusb_open(dev,phDev);
    if (ret < 0 )
        *phDev = NULL;
    return true;
#else
    hDev = usb_open(dev);

#endif
    return true;
}
bool USBUtil::CloseDevice(libusb_device_handle *hDev)
{
#ifdef LIBUSB_VERSION1
    libusb_close(hDev);
#else
    usb_close(hDev);
#endif
    return true;
}
bool USBUtil::SetConfiguration(libusb_device_handle *hDev,int config)
{
#ifdef LIBUSB_VERSION1
    libusb_set_configuration(hDev,config);
#else
    usb_set_configuration(hDev,config);
#endif
    return true;
}
bool USBUtil::Claim_interface(libusb_device_handle *hDev, int iface)
{
#ifdef LIBUSB_VERSION1
    libusb_claim_interface(hDev,iface);
#else
#endif
    return true;
}
int USBUtil::ControlMsg(libusb_device_handle *hDev,
                         uint8_t reqType, uint8_t req, uint16_t value, uint16_t index,
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
    return USBUtil::ControlMsg(hDev,reqType,req,value,index,data,length,timeout);
}

//WIP to replace static functions with instance methods, call static for now
void USBUtil::LibUSBFindAndOpenDevice(int PID, int VID, int multiple)
{
    hDev = USBUtil::LibUSB_FindAndOpenDevice(PID,VID,multiple);

}

//Assumes InitLibUsb already called
libusb_device_handle * USBUtil::LibUSB_FindAndOpenDevice(int PID, int VID, int multiple)
{
    int numFound = 0;

#ifdef LIBUSB_VERSION1
#if 1
        libusb_device_handle *hDev;
        //Can't use this because we support multiple PIDs and may want nth one, but it works
        hDev = libusb_open_device_with_vid_pid(NULL,VID,PID);
        return hDev;
#else
    //This crashes with memAccess, use above for now
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
                USBUtil::OpenDevice(dev,&hDev);
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

//Dumps device info to debug for now
void USBUtil::ListDevices()
{
    libusb_device *dev; //Our device (hopefully)
    libusb_device **devs; //List of all the devices found
    libusb_device_handle *hDev;

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
        USBUtil::OpenDevice(dev,&hDev);
        libusb_get_string_descriptor_ascii(hDev,desc.iManufacturer,mfg,sizeof(mfg));
        libusb_get_string_descriptor_ascii(hDev,desc.iProduct,prod,sizeof(prod));
        qDebug("%s %s Class %d VID 0x%x PID 0x%x",mfg, prod, desc.bDeviceClass, desc.idVendor,desc.idProduct);
        USBUtil::CloseDevice(hDev);
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
