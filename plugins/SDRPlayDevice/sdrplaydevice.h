#ifndef SDRPLAYDEVICE_H
#define SDRPLAYDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_sdrplayoptions.h"
#include "MiricsAPI/mir_sdr.h"
#include <QLibrary>


class SDRPlayDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	SDRPlayDevice();
	~SDRPlayDevice();

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
	Ui::SDRPlayOptions *optionUi;

	QLibrary *api;


};
#endif // SDRPLAYDEVICE_H
