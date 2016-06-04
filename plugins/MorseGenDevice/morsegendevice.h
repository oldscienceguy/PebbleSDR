#ifndef EXAMPLESDRDEVICE_H
#define EXAMPLESDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_morsegendevice.h"

class MorseGenDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	MorseGenDevice();
	~MorseGenDevice();

	//Required
	bool initialize(CB_ProcessIQData _callback,
					CB_ProcessBandscopeData _callbackBandscope,
					CB_ProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool command(StandardCommands _cmd, QVariant _arg);
	QVariant get(StandardKeys _key, QVariant _option = 0);
	bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

private:
	void readSettings();
	void writeSettings();
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	//Work buffer for consumer to convert device format data to CPX Pebble format data
	CPX *consumerBuffer;
	quint16 producerIndex;
	CPX *producerFreeBufPtr; //Treat as array of CPX

	Ui::MorseGenOptions *optionUi;


};
#endif // EXAMPLESDRDEVICE_H
