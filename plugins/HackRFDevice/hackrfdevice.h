#ifndef HACKRFDEVICE_H
#define HACKRFDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_hackrfoptions.h"
#include "hackrf.h"

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
	bool Initialize(cbProcessIQData _callback,
					cbProcessBandscopeData _callbackBandscope,
					cbProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool Command(STANDARD_COMMANDS _cmd, QVariant _arg);
	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);

private slots:
	void lnaChanged(int _value);
	void vgaChanged(int _value);

private:
	void ReadSettings();
	void WriteSettings();

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	//Work buffer for consumer to convert device format data to CPX Pebble format data
	CPX *consumerBuffer;

	Ui::HackRFOptions *optionUi;
	hackrf_device* hackrfDevice;
	quint8 hackrfBoardId = BOARD_ID_INVALID;
	char hackrfVersion[255 + 1];

	quint16 producerIndex;
	CPX *producerFreeBufPtr; //Treat as array of CPX
	QMutex mutex;
	static int rx_callback(hackrf_transfer *transfer);
	bool apiCheck(int result, const char *api);

	quint16 lnaGain;
	quint16 vgaGain;

};
#endif // HACKRFDEVICE_H
