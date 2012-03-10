//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QLibrary>

#include "usb.h"
#include "ftd2xx.h"
#include "settings.h"
#include "SoftRock.h"
#include "ElektorSDR.h"
#include "sdr_iq.h"
#include "hpsdr.h"
#include "funcube.h"
#include "SoundCard.h"
#include "Receiver.h"

SDR::SDR(Receiver *_receiver, SDRDEVICE dev,Settings *_settings)
{
	sdrDevice = dev;
	settings = _settings;
	receiver = _receiver;
	startupFrequency = 0;
	//DLL's are loaded explicitly when we connect to SDR.  Not everyone will have DLLs for every SDR installed
	isLibUsbLoaded = false;
	isFtdiLoaded = false;
	isThreadRunning = false;
	audioInput = new SoundCard(receiver,GetSampleRate(),settings->framesPerBuffer,settings);
	producerThread = NULL;
	consumerThread = NULL;

	//Will be set when we read device.ini file, here for safety
	sIQBalanceGain=1.0;
	sIQBalancePhase=0;
	sIQBalanceEnable=false;
	sIQOrder = SDR::IQ;
	iqBalanceOptions = NULL;
	iqDialog = NULL;
}

SDR::~SDR(void)
{
	if (audioInput != NULL) {
		delete audioInput;
	}
	if (iqDialog != NULL && iqDialog->isVisible())
		iqDialog->hide();

}

//Settings common to all devices
void SDR::ReadSettings(QSettings *settings)
{
	sIQBalanceGain = settings->value("IQBalanceGain",1.0).toDouble();
	sIQBalancePhase = settings->value("IQBalancePhase",0.0).toDouble();
	sIQBalanceEnable = settings->value("IQBalanceEnable",false).toBool();
	sIQOrder = (IQORDER)settings->value("IQOrder", SDR::IQ).toInt();


}
void SDR::WriteSettings(QSettings *settings)
{
	settings->setValue("IQBalanceGain",sIQBalanceGain);
	settings->setValue("IQBalancePhase",sIQBalancePhase);
	settings->setValue("IQBalanceEnable",sIQBalanceEnable);
	settings->setValue("IQOrder", sIQOrder);

}

//Static
SDR *SDR::Factory(Receiver *receiver, Settings *settings)
{
	SDR *sdr=NULL;

	switch (settings->sdrDevice)
	{
	case SDR::SR_LITE:
		sdr = new SoftRock(receiver, SDR::SR_LITE,settings);
		break;
	case SDR::SR_V9:
		sdr = new SoftRock(receiver, SDR::SR_V9,settings);
		break;
	case SDR::SR_ENSEMBLE:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE,settings);
		break;
	case SDR::SR_ENSEMBLE_2M:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_2M,settings);
		break;
	case SDR::SR_ENSEMBLE_4M:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_4M,settings);
		break;
	case SDR::SR_ENSEMBLE_6M:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_6M,settings);
		break;
	case SDR::SR_ENSEMBLE_LF:
		sdr = new SoftRock(receiver, SDR::SR_ENSEMBLE_LF,settings);
		break;
	case SDR::ELEKTOR:
		sdr = new ElektorSDR(receiver, SDR::ELEKTOR,settings);
		break;
	case SDR::ELEKTOR_PA:
		sdr = new ElektorSDR(receiver, SDR::ELEKTOR_PA,settings);
		break;
	case SDR::SDR_IQ:
		//Todo: Enum is conflicting with class, rename one or the other
		sdr = new SDR_IQ::SDR_IQ(receiver, SDR::SDR_IQ,settings);
		break;
	case SDR::HPSDR_USB:
		sdr = new HPSDR(receiver, SDR::HPSDR_USB,settings);
		break;
	case SDR::SDRWIDGET:
		sdr = new HPSDR(receiver, SDR::SDRWIDGET,settings);
		break;
	case SDR::FUNCUBE:
		sdr = new FunCube(receiver, SDR::FUNCUBE,settings);
		break;

	case SDR::HPSDR_TCP:
	case SDR::NOSDR:
		sdr = NULL;
		break;
	}
	return sdr;
}


//Devices may override this and return a rate based on other settings
int SDR::GetSampleRate()
{
	return settings->sampleRate;
}

void SDR::phaseChanged(int v)
{
	//v is an integer, convert to fraction -.500 to +.500
	double newValue = v/1000.0;
	iqBalanceOptions->phaseLabel->setText("Phase: " + QString::number(newValue));
	receiver->GetIQBalance()->setPhaseFactor(newValue);
}
void SDR::gainChanged(int v)
{
	//v is an integer, convert to fraction -.750 to +1.250
	double newValue = v/1000.0;
	iqBalanceOptions->gainLabel->setText("Gain: " + QString::number(newValue));
	receiver->GetIQBalance()->setGainFactor(newValue);
}
void SDR::enabledChanged(bool b)
{
	receiver->GetIQBalance()->setEnabled(b);
}
void SDR::automaticChanged(bool b)
{
	receiver->GetIQBalance()->setAutomatic(b);
}
void SDR::resetClicked()
{
	receiver->GetIQBalance()->setGainFactor(1);
	receiver->GetIQBalance()->setPhaseFactor(0);
	iqBalanceOptions->phaseSlider->setValue(0);
	iqBalanceOptions->gainSlider->setValue(1000);
}
void SDR::saveClicked()
{
	sIQBalanceGain=receiver->GetIQBalance()->getGainFactor();
	sIQBalancePhase=receiver->GetIQBalance()->getPhaseFactor();
	sIQBalanceEnable=receiver->GetIQBalance()->getEnabled();
}
//IQ order can be changed in real time, without saving
void SDR::IQOrderChanged(int i)
{
	sIQOrder = (IQORDER)iqBalanceOptions->IQSettings->itemData(i).toInt();
	//Settings are read in real time by soundcard loop, no need to notify
}

void SDR::ShowIQOptions()
{

	if (iqDialog == NULL) {
		iqBalanceOptions = new Ui::IQBalanceOptions();
		iqDialog = new QDialog();
		iqBalanceOptions->setupUi(iqDialog);

		iqBalanceOptions->IQSettings->addItem("I/Q (normal)",IQ);
		iqBalanceOptions->IQSettings->addItem("Q/I (swap)",QI);
		iqBalanceOptions->IQSettings->addItem("I Only",IONLY);
		iqBalanceOptions->IQSettings->addItem("Q Only",QONLY);
		connect(iqBalanceOptions->IQSettings,SIGNAL(currentIndexChanged(int)),this,SLOT(IQOrderChanged(int)));

		iqBalanceOptions->phaseSlider->setMaximum(500);
		iqBalanceOptions->phaseSlider->setMinimum(-500);

		connect(iqBalanceOptions->phaseSlider,SIGNAL(valueChanged(int)),this,SLOT(phaseChanged(int)));

		iqBalanceOptions->gainSlider->setMaximum(1250);
		iqBalanceOptions->gainSlider->setMinimum(750);

		connect(iqBalanceOptions->gainSlider,SIGNAL(valueChanged(int)),this,SLOT(gainChanged(int)));

		connect(iqBalanceOptions->enableBalanceBox,SIGNAL(toggled(bool)),this,SLOT(enabledChanged(bool)));
		iqBalanceOptions->autoBalanceBox->setEnabled(false); //Not ready yet
		connect(iqBalanceOptions->autoBalanceBox,SIGNAL(toggled(bool)),this,SLOT(automaticChanged(bool)));
		connect(iqBalanceOptions->resetButton,SIGNAL(clicked()),this,SLOT(resetClicked()));

		connect(iqBalanceOptions->saveButton,SIGNAL(clicked()),this,SLOT(saveClicked()));
	}

	iqBalanceOptions->IQSettings->setCurrentIndex(sIQOrder);
	iqBalanceOptions->phaseSlider->setValue(sIQBalancePhase*1000);
	iqBalanceOptions->phaseLabel->setText("Phase: " + QString::number(sIQBalancePhase));
	iqBalanceOptions->gainSlider->setValue(sIQBalanceGain*1000);
	iqBalanceOptions->gainLabel->setText("Gain: " + QString::number(sIQBalanceGain));
	iqBalanceOptions->enableBalanceBox->setChecked(sIQBalanceEnable);

	iqDialog->show();

}





//Explicit DLL Load
//LibUSB
typedef int (*LPUSB_FIND_BUSSES)(void);
LPUSB_FIND_BUSSES usb_find_busses;
typedef void (*LPUSB_INIT)(void);
LPUSB_INIT usb_init;
typedef int (*LP_USB_FIND_DEVICES)(void);
LP_USB_FIND_DEVICES usb_find_devices;
typedef int (*LPUSB_CLAIM_INTERFACE)(usb_dev_handle *dev, int interface);
LPUSB_CLAIM_INTERFACE usb_claim_interface;
typedef int (*LPUSB_RELEASE_INTERFACE)(usb_dev_handle *dev, int interface);
LPUSB_RELEASE_INTERFACE usb_release_interface;
typedef int (*LPUSB_CLOSE)(usb_dev_handle *dev);
LPUSB_CLOSE usb_close;
typedef usb_dev_handle *(*LPUSB_OPEN)(struct usb_device *dev);
LPUSB_OPEN usb_open;
typedef int (*LPUSB_SET_CONFIGURATION)(usb_dev_handle *dev, int configuration);
LPUSB_SET_CONFIGURATION usb_set_configuration;
typedef int (*LPUSB_CONTROL_MSG)(usb_dev_handle *dev, int requesttype, int request,
	int value, int index, char *bytes, int size, 
    int timeout);
LPUSB_CONTROL_MSG usb_control_msg;
typedef struct usb_bus *(*USB_GET_BUSSES)(void);
USB_GET_BUSSES usb_get_busses;
typedef int (*USB_BULK_READ)(usb_dev_handle *dev, int ep, char *bytes, int size,
				  int timeout);
USB_BULK_READ usb_bulk_read;
typedef int (*USB_BULK_WRITE)(usb_dev_handle *dev, int ep, char *bytes, int size,
				   int timeout);
USB_BULK_WRITE usb_bulk_write;

typedef int (*USB_INTERRUPT_WRITE)(usb_dev_handle *dev, int ep, char *bytes, int size,
						int timeout);
USB_INTERRUPT_WRITE usb_interrupt_write;
typedef int (*USB_INTERRUPT_READ)(usb_dev_handle *dev, int ep, char *bytes, int size,
					   int timeout);
USB_INTERRUPT_READ usb_interrupt_read;

typedef int (*USB_SET_ALTINTERFACE)(usb_dev_handle *dev, int alternate);
USB_SET_ALTINTERFACE usb_set_altinterface;
typedef int (*USB_GET_STRING_SIMPLE)(usb_dev_handle *dev, int index, char *buf,size_t buflen);
USB_GET_STRING_SIMPLE usb_get_string_simple;

bool SDR::LoadLibUsb()
{
	//Explicit loading of libraries with Qt
	//If necessary, we can move DLL name to pebble.ini so it can be changed
	QLibrary lib("libusb0"); //Qt will apply the proper platform specific suffic
	//lib.setLoadHints(QLibrary::ResolveAllSymbolsHint); //Not supported on Windows? We have to call resolve() for each function
	if (!lib.load()) {
		return false;
	}
	usb_find_busses=(LPUSB_FIND_BUSSES)lib.resolve("usb_find_busses");
	usb_init=(LPUSB_INIT)lib.resolve("usb_init");
	usb_find_devices=(LP_USB_FIND_DEVICES)lib.resolve("usb_find_devices");
	usb_claim_interface=(LPUSB_CLAIM_INTERFACE)lib.resolve("usb_claim_interface");
	usb_release_interface=(LPUSB_RELEASE_INTERFACE)lib.resolve("usb_release_interface");
	usb_close=(LPUSB_CLOSE)lib.resolve("usb_close");
	usb_open=(LPUSB_OPEN)lib.resolve("usb_open");
	usb_set_configuration=(LPUSB_SET_CONFIGURATION)lib.resolve("usb_set_configuration");
	usb_control_msg=(LPUSB_CONTROL_MSG)lib.resolve("usb_control_msg");
	usb_get_busses=(USB_GET_BUSSES)lib.resolve("usb_get_busses");
	usb_bulk_read=(USB_BULK_READ)lib.resolve("usb_bulk_read");
	usb_bulk_write=(USB_BULK_WRITE)lib.resolve("usb_bulk_write");
	usb_interrupt_write=(USB_INTERRUPT_WRITE)lib.resolve("usb_interrupt_write");
	usb_interrupt_read=(USB_INTERRUPT_READ)lib.resolve("usb_interrupt_read");
	usb_set_altinterface=(USB_SET_ALTINTERFACE)lib.resolve("usb_set_altinterface");
	usb_get_string_simple = (USB_GET_STRING_SIMPLE)lib.resolve("usb_get_string_simple");
	isLibUsbLoaded = true;
	return true;
}

typedef FT_STATUS (WINAPI *FT_SETBAUDRATE)(FT_HANDLE ftHandle,ULONG BaudRate);
typedef FT_STATUS (WINAPI *FT_SETBITMODE)(FT_HANDLE ftHandle,UCHAR ucMask,UCHAR ucEnable);
typedef FT_STATUS (WINAPI *FT_CLOSE)(FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI *FT_RESETDEVICE)(FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI *FT_OPEN)(int deviceNumber,FT_HANDLE *pHandle);
typedef FT_STATUS (WINAPI *FT_WRITE)(FT_HANDLE ftHandle,LPVOID lpBuffer,DWORD dwBytesToWrite,LPDWORD lpBytesWritten);
typedef FT_STATUS (WINAPI *FT_GETDEVICEINFOLIST)(FT_DEVICE_LIST_INFO_NODE *pDest,LPDWORD lpdwNumDevs);
typedef FT_STATUS (WINAPI *FT_CREATEDEVICEINFOLIST)(LPDWORD lpdwNumDevs);
typedef FT_STATUS (WINAPI *FT_READ)(FT_HANDLE ftHandle,LPVOID lpBuffer,DWORD dwBytesToRead,LPDWORD lpBytesReturned);
typedef FT_STATUS (WINAPI *FT_SETTIMEOUTS)(FT_HANDLE ftHandle,ULONG ReadTimeout,ULONG WriteTimeout);
typedef FT_STATUS (WINAPI *FT_PURGE)(FT_HANDLE ftHandle,ULONG Mask);
typedef FT_STATUS (WINAPI *FT_GETQUEUESTATUS)(FT_HANDLE ftHandle,DWORD *dwRxBytes);


FT_SETBAUDRATE FT_SetBaudRate;
FT_SETBITMODE FT_SetBitMode;
FT_CLOSE FT_Close;
FT_RESETDEVICE FT_ResetDevice;
FT_OPEN FT_Open;
FT_WRITE FT_Write;
FT_GETDEVICEINFOLIST FT_GetDeviceInfoList;
FT_CREATEDEVICEINFOLIST FT_CreateDeviceInfoList;
FT_READ FT_Read;
FT_SETTIMEOUTS FT_SetTimeouts;
FT_PURGE FT_Purge;
FT_GETQUEUESTATUS FT_GetQueueStatus;

bool SDR::LoadFtdi()
{
	QLibrary lib("ftd2xx");
	if (!lib.load()) {
		return false;
	}
	FT_SetBaudRate=(FT_SETBAUDRATE)lib.resolve("FT_SetBaudRate");
	FT_SetBitMode=(FT_SETBITMODE)lib.resolve("FT_SetBitMode");
	FT_Close=(FT_CLOSE)lib.resolve("FT_Close");
	FT_ResetDevice=(FT_RESETDEVICE)lib.resolve("FT_ResetDevice");
	FT_Open=(FT_OPEN)lib.resolve("FT_Open");
	FT_Write=(FT_WRITE)lib.resolve("FT_Write");
	FT_GetDeviceInfoList=(FT_GETDEVICEINFOLIST)lib.resolve("FT_GetDeviceInfoList");
	FT_CreateDeviceInfoList=(FT_CREATEDEVICEINFOLIST)lib.resolve("FT_CreateDeviceInfoList");
	FT_Read=(FT_READ)lib.resolve("FT_Read");
	FT_SetTimeouts=(FT_SETTIMEOUTS)lib.resolve("FT_SetTimeouts");
	FT_Purge=(FT_PURGE)lib.resolve("FT_Purge");
	FT_GetQueueStatus=(FT_GETQUEUESTATUS)lib.resolve("FT_GetQueueStatus");
	isFtdiLoaded = true;
	return true;
}

SDR::SDRDEVICE SDR::GetDevice()
{
	return sdrDevice;
}
void SDR::SetDevice(SDRDEVICE m)
{
	sdrDevice = m;
}

//Returns device # for serialNumber or -1 if not found
int SDR::FTDI_FindDeviceBySerialNumber(QString serialNumber)
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


int SDR::FTDI_FindDeviceByName(QString deviceName)
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

struct usb_device * SDR::LibUSB_FindDevice(int PID, int VID, int multiple)
{
	struct usb_device *dev;
	struct usb_bus *busses;
	busses = usb_get_busses();
	int numFound = 0;
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
}
void SDR::StopProducerThread(){}
void SDR::RunProducerThread(){}
void SDR::StopConsumerThread(){}
void SDR::RunConsumerThread(){}

//SDRThreads
SDRProducerThread::SDRProducerThread(SDR *_sdr)
{
	sdr = _sdr;
	msSleep=5;
	doRun = false;
}
void SDRProducerThread::stop()
{
	sdr->StopProducerThread();
	doRun=false;
}
//Refresh rate in me
void SDRProducerThread::setRefresh(int ms)
{
	msSleep = ms;
}
void SDRProducerThread::run()
{
	doRun = true;
	while(doRun) {
		sdr->RunProducerThread();
		msleep(msSleep);
	}
}
SDRConsumerThread::SDRConsumerThread(SDR *_sdr)
{
	sdr = _sdr;
	msSleep=5;
	doRun = false;
}
void SDRConsumerThread::stop()
{
	sdr->StopConsumerThread();
	doRun=false;
}
//Refresh rate in me
void SDRConsumerThread::setRefresh(int ms)
{
	msSleep = ms;
}
void SDRConsumerThread::run()
{
	doRun = true;
	while(doRun) {
		sdr->RunConsumerThread();
		msleep(msSleep);
	}
}
