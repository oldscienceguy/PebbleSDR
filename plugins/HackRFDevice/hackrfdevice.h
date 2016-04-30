#ifndef HACKRFDEVICE_H
#define HACKRFDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_hackrfoptions.h"
#include "hackrf.h"
#include "../d2xx/libusb/libusb/libusb.h"
#include "decimator.h"

//From hackrf.c
struct hackrf_device {
	libusb_device_handle* usb_device;
	struct libusb_transfer** transfers;
	hackrf_sample_block_cb_fn callback;
	volatile bool transfer_thread_started; /* volatile shared between threads (read only) */
	pthread_t transfer_thread;
	uint32_t transfer_count;
	uint32_t buffer_size;
	volatile bool streaming; /* volatile shared between threads (read only) */
	void* rx_ctx;
	void* tx_ctx;
};
typedef enum {
	HACKRF_TRANSCEIVER_MODE_OFF = 0,
	HACKRF_TRANSCEIVER_MODE_RECEIVE = 1,
	HACKRF_TRANSCEIVER_MODE_TRANSMIT = 2,
} hackrf_transceiver_mode;
typedef enum {
	HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE = 1,
	HACKRF_VENDOR_REQUEST_MAX2837_WRITE = 2,
	HACKRF_VENDOR_REQUEST_MAX2837_READ = 3,
	HACKRF_VENDOR_REQUEST_SI5351C_WRITE = 4,
	HACKRF_VENDOR_REQUEST_SI5351C_READ = 5,
	HACKRF_VENDOR_REQUEST_SAMPLE_RATE_SET = 6,
	HACKRF_VENDOR_REQUEST_BASEBAND_FILTER_BANDWIDTH_SET = 7,
	HACKRF_VENDOR_REQUEST_RFFC5071_WRITE = 8,
	HACKRF_VENDOR_REQUEST_RFFC5071_READ = 9,
	HACKRF_VENDOR_REQUEST_SPIFLASH_ERASE = 10,
	HACKRF_VENDOR_REQUEST_SPIFLASH_WRITE = 11,
	HACKRF_VENDOR_REQUEST_SPIFLASH_READ = 12,
	HACKRF_VENDOR_REQUEST_BOARD_ID_READ = 14,
	HACKRF_VENDOR_REQUEST_VERSION_STRING_READ = 15,
	HACKRF_VENDOR_REQUEST_SET_FREQ = 16,
	HACKRF_VENDOR_REQUEST_AMP_ENABLE = 17,
	HACKRF_VENDOR_REQUEST_BOARD_PARTID_SERIALNO_READ = 18,
	HACKRF_VENDOR_REQUEST_SET_LNA_GAIN = 19,
	HACKRF_VENDOR_REQUEST_SET_VGA_GAIN = 20,
	HACKRF_VENDOR_REQUEST_SET_TXVGA_GAIN = 21,
	HACKRF_VENDOR_REQUEST_ANTENNA_ENABLE = 23,
	HACKRF_VENDOR_REQUEST_SET_FREQ_EXPLICIT = 24,
} hackrf_vendor_request;


class HackRFDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	HackRFDevice();
	~HackRFDevice();

	//Required
	bool initialize(CB_ProcessIQData _callback,
					CB_ProcessBandscopeData _callbackBandscope,
					CB_ProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool command(StandardCommands _cmd, QVariant _arg);
	QVariant get(StandardKeys _key, QVariant _option = 0);
	bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

private slots:
	void rfGainChanged(bool _value);
	void lnaGainChanged(int _value);
	void vgaGainChanged(int _value);
	void decimationChanged(int _index);
	void consumerSlot();

signals:
	void newIQData();

private:
	void readSettings();
	void writeSettings();

	void setSampleRate(quint32 _sampleRate); //Handles decimation and deviceSampleRate

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	//Work buffer for producer to convert device format data to CPX Pebble format data
	CPX8 *producerBuf;
	CPX *decimatorBuf;

	Ui::HackRFOptions *optionUi;
	hackrf_device* hackrfDevice;
	quint8 hackrfBoardId = BOARD_ID_INVALID;
	char hackrfVersion[255 + 1];

	quint16 producerIndex;
	CPX *producerFreeBufPtr; //Treat as array of CPX
	QMutex mutex;
	static int rx_callback(hackrf_transfer *transfer);
	bool apiCheck(int result, const char *api);

	bool rfGain; //off = 0db on = 14db
	quint16 lnaGain; //IF Gain 0-40db 8db steps
	quint16 vgaGain; //Baseband gain 0-62db 2db steps

	bool useSynchronousAPI; //Testing to see if more even performance than asynchronous API
	bool useSignals; //Testing to see if producer signals consumer is better than consumer process

	bool synchronousStartRx();
	int hackrf_set_transceiver_mode(hackrf_transceiver_mode value);
	bool synchronousStopRx();
	bool synchronousRead(CPX8 *_buf, int _bufLen);

	Decimator *decimator;  //Decimator with lp filter
	quint32 deviceSamplesPerBuffer;
};
#endif // HACKRFDEVICE_H
