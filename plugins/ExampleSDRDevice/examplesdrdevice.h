#ifndef EXAMPLESDRDEVICE_H
#define EXAMPLESDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_examplesdroptions.h"

class ExampleSDRDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	ExampleSDRDevice();
	~ExampleSDRDevice();

	//Required
	bool Initialize(cbProcessIQData _callback,
					cbProcessBandscopeData _callbackBandscope,
					cbProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool Command(STANDARD_COMMANDS _cmd, QVariant _arg);
	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);

private:
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	//Work buffer for consumer to convert device format data to CPX Pebble format data
	CPXBuf *consumerBuffer;

	Ui::ExampleSdrOptions *optionUi;


};
#endif // EXAMPLESDRDEVICE_H
