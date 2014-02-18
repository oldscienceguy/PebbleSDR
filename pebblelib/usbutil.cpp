//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "usbutil.h"

USBUtil::USBUtil(USB_LIB_TYPE _libType)
{
	libType = _libType;
    timeout = 1000; //default timeout in ms
	libusbDev = NULL;
	libusbDevHandle = NULL;
	ftdiDevice = -1;
}

bool USBUtil::LoadUsb()
{
	//Placeholder for future
	if (libType == LIB_USB) {
		isLibUsbLoaded = true;
	} else if (libType == FTDI_D2XX) {
		isFtdiLoaded = true;
	}
	return true;
}

bool USBUtil::FindDevice(QString query, bool matchSerialNumber = true)
{
	if (libType == LIB_USB) {
		return false; //FTDI only for now
	}
    FT_STATUS ftStatus;
    DWORD numDevices;
    //Get all the FTDI USB devices that are connected
    ftStatus = FT_CreateDeviceInfoList(&numDevices);
	if (ftStatus != FT_OK) {
		ftdiDevice = -1;
		return false;
	}

    //Allocate array for numDevices
    FT_DEVICE_LIST_INFO_NODE *pDeviceInfoList = new FT_DEVICE_LIST_INFO_NODE[numDevices];
    ftStatus = FT_GetDeviceInfoList(pDeviceInfoList, &numDevices);
	if (ftStatus != FT_OK) {
		ftdiDevice = -1;
		return false;
	}
    //We've got devices, see if ours is connected
    for (unsigned i=0; i<numDevices; i++)
    {
		if ((matchSerialNumber && QString::compare(pDeviceInfoList[i].SerialNumber,query)) == 0 ||
			(!matchSerialNumber && QString(pDeviceInfoList[i].Description).contains(query,Qt::CaseInsensitive)))
        {
			ftdiDevice = i;
			return true;
        }

    }
	ftdiDevice = -1;
	return false;
}

bool USBUtil::InitUSB()
{
	if (libType == LIB_USB) {
		int ret = libusb_init(NULL);
		return ret == 0 ? true: false;
	}
#if 0
    usb_init(NULL);
    usb_find_busses();
    usb_find_devices();
    return true;
#endif
	return false;
}
bool USBUtil::OpenDevice()
{
	if (libType == LIB_USB) {
		int ret = libusb_open(libusbDev,&libusbDevHandle);
		if (ret < 0 ) {
			libusbDevHandle = NULL;
			return false;
		}
		return true;
	} else if (libType == FTDI_D2XX) {
		return false;
	}
#if 0
    hDev = usb_open(dev);

#endif
    return true;
}
bool USBUtil::CloseDevice()
{
	if (libType == LIB_USB) {
		libusb_close(libusbDevHandle);
	}
#if 0
    usb_close(hDev);
#endif
	libusbDevHandle = NULL;
    return true;
}
bool USBUtil::SetConfiguration(int config)
{
	if (libType == LIB_USB) {
		int ret = libusb_set_configuration(libusbDevHandle,config);
		return ret == 0 ? true: false;
	}
#if 0
    usb_set_configuration(hDev,config);
#endif
	return false;
}
bool USBUtil::Claim_interface(int iface)
{
	if (libusbDevHandle==NULL)
		return false;

	if (libType == LIB_USB) {
		int ret = libusb_claim_interface(libusbDevHandle,iface);
		return ret == 0 ? true: false;
	}
	return false;
}

bool USBUtil::ReleaseInterface(int iface)
{
	if (libusbDevHandle==NULL)
		return false;

	if (libType == LIB_USB) {
		int ret = libusb_release_interface(libusbDevHandle,iface);
		return ret == 0 ? true: false;
	}
	return false;
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
	if (libType == LIB_USB) {
		return libusb_control_transfer(libusbDevHandle,reqType,req,value,index,data,length,timeout);
	}
#if 0
    return = usb_control_msg(hDev,reqType, req, value, data, index, length, timeout);

#endif
	return 0;
}

bool USBUtil::BulkTransfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
{
	if (libType == LIB_USB) {
		return libusb_bulk_transfer(libusbDevHandle, endpoint, data, length, actual_length, timeout);
	}
	return false;
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
bool USBUtil::FindAndOpenDevice(int PID, int VID, int multiple)
{
	Q_UNUSED(multiple);


	if (libType == LIB_USB) {
        //Can't use this because we support multiple PIDs and may want nth one, but it works
		libusbDevHandle = libusb_open_device_with_vid_pid(NULL,VID,PID);
		return libusbDevHandle != NULL;
	}
#if 0
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
#if 0
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
	return 0;
}

bool USBUtil::FindDevice(int PID, int VID, int multiple)
{
    int numFound = 0;

	if (libType == LIB_USB) {
		libusb_device **devs; //List of all the devices found

		int cnt = libusb_get_device_list(NULL,&devs);
		if (cnt < 0) {
			qDebug("No Devices Found");
			libusbDev = NULL;
			return false;
		}
		int i=0;
		while ((libusbDev = devs[i++]) != NULL) {
			libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(libusbDev, &desc);
			if (r < 0) {
				libusb_free_device_list(devs,1);
				qDebug("Failed to get descriptor");
				libusbDev = NULL;
				return false;
			}
			if ( VID == desc.idVendor && PID == desc.idProduct) {
				//Got our device
				//There may be multiple devices and caller wants Nth device
				if (numFound++ == multiple) {
					libusb_free_device_list(devs,1);
					return true;
				}
			}
		}
		libusb_free_device_list(devs,1);
	}
	libusbDev = NULL;
	return false;

}

//Dumps device info to debug for now
void USBUtil::ListDevices()
{
	if (libType == LIB_USB) {
		libusb_device **devs; //List of all the devices found

		unsigned char mfg[256]; //string buffer
		unsigned char prod[256]; //string buffer

		int cnt = libusb_get_device_list(NULL,&devs);
		if (cnt < 0) {
			qDebug("No Devices Found");
			return;
		}
		int i=0;
		while ((libusbDev = devs[i++]) != NULL) {
			libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(libusbDev, &desc);
			if (r < 0) {
				libusb_free_device_list(devs,1);
				qDebug("Failed to get descriptor");
				return;
			}
			//Get ascii string for mfg descriptor
			OpenDevice();
			libusb_get_string_descriptor_ascii(libusbDevHandle,desc.iManufacturer,mfg,sizeof(mfg));
			libusb_get_string_descriptor_ascii(libusbDevHandle,desc.iProduct,prod,sizeof(prod));
			qDebug("%s %s Class %d VID 0x%x PID 0x%x",mfg, prod, desc.bDeviceClass, desc.idVendor,desc.idProduct);
			CloseDevice();
		}

		libusb_free_device_list(devs,1);
	}
    return;

}

//Utility function to read array
bool USBUtil::ReadArray(quint16 index, quint16 address, unsigned char *data, quint16 length)
{
	if (libType == LIB_USB) {
		quint8 reqType = LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN;
		quint8 req = 0;
		return ControlMsg(reqType,req,address,index,data,length,timeout);
	}
	return false;
}

//Writes array
bool USBUtil::WriteArray(quint16 index, quint16 address, unsigned char*data, quint16 length)
{
	if (libType == LIB_USB) {
		quint8 reqType = LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT;
		quint8 req = 0;
		return ControlMsg(reqType,req,address,index,data,length,timeout);
	}
	return false;
}

//Writes a 1 or 2 byte register
bool USBUtil::WriteReg(quint16 index, quint16 address, quint16 value, quint16 length)
{
		if (libType == LIB_USB) {
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

		return ControlMsg(reqType,req,address,index,data,length,timeout);
	}
	return false;
}

