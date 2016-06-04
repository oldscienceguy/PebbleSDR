#ifndef FILEDEVICE_H
#define FILEDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "wavfile.h"

class FileSDRDevice : public QObject, public DeviceInterfaceBase
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DeviceInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DeviceInterface)

public:
    FileSDRDevice();
    ~FileSDRDevice();

	bool initialize(CB_ProcessIQData _callback,
					CB_ProcessBandscopeData _callbackBandscope,
					CB_ProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	QVariant get(StandardKeys _key, QVariant _option = 0);
	bool set(StandardKeys _key, QVariant _value, QVariant _option = 0);

public slots:
	void producerSlot();

private:
	bool connectDevice();
	bool disconnectDevice();
	void startDevice();
	void stopDevice();
	void readSettings();
	void writeSettings();
	//Display device option widget in settings dialog
	void setupOptionUi(QWidget *parent);
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	QString m_fileName;
	QString m_recordingPath;

	WavFile m_wavFileRead;
	WavFile m_wavFileWrite;
	bool m_copyTest; //True if we're reading from one file and writing to another file for testing

	QElapsedTimer m_elapsedTimer;
	qint64 m_nsPerBuffer; //How fast do we have to output a buffer of data to match recorded sample rate
	CPX *m_producerBuf;
};



#endif // FILEDEVICE_H
