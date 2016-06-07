#ifndef MORSEGENDEVICE_H
#define MORSEGENDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_morsegendevice.h"
#include "morsegen.h"

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

private slots:
	void resetButtonClicked(bool clicked);
	void updateAllFields();
	void updateGen1Fields();
	void updateGen2Fields();
	void updateGen3Fields();
	void updateGen4Fields();
	void updateGen5Fields();
	void updateNoiseFields();

private:
	enum SampleTextChoices {
		ST_SAMPLE1, //Sample1 text from ini file
		ST_SAMPLE2, //Sample2 text from ini file
		ST_SAMPLE3, //Sample3 text from ini file
		ST_SAMPLE4, //Sample4 text from ini file
		ST_WORDS,	//Random words from table
		ST_ABBREV,	//Random morse abbeviations
		ST_TABLE,	//All characters in morse table
		ST_END};
	void readSettings();
	void writeSettings();
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	void setupOptionUi(QWidget *parent);

	//Work buffer for consumer to convert device format data to CPX Pebble format data
	CPX *m_consumerBuffer;
	quint16 m_producerIndex;
	CPX *m_producerFreeBufPtr; //Treat as array of CPX
	CPX *m_consumerFilledBufPtr; //Treat as array of CPX

	Ui::MorseGenOptions *m_optionUi;

	QElapsedTimer m_elapsedTimer;
	qint64 m_nsPerBuffer; //How fast do we have to output a buffer of data to match recorded sample rate

	static const quint32 c_dbFadeRange = 20; //For random fade generation

	CPX *m_outBuf;

	CPX *m_outBuf1;
	CPX *m_outBuf2;
	CPX *m_outBuf3;
	CPX *m_outBuf4;
	CPX *m_outBuf5;

	MorseGen *m_morseGen1;
	MorseGen *m_morseGen2;
	MorseGen *m_morseGen3;
	MorseGen *m_morseGen4;
	MorseGen *m_morseGen5;

	QString m_sampleText[ST_END];
	CPX nextNoiseSample(double _dbGain);
	qint32 m_dbNoiseAmp;
	double m_noiseAmp;

	void generate(CPX *out);

	struct GenSettings {
		bool enabled;
		double freq;
		double amp;
		quint32 wpm;
		quint32 rise;
		quint32 text;
		bool fade;
		bool fist;

	};

	GenSettings m_gs1;
	GenSettings m_gs2;
	GenSettings m_gs3;
	GenSettings m_gs4;
	GenSettings m_gs5;

	void updateGenerators();

	QMutex m_mutex; //Locks generator changes when producer thead is calling generate()
};
#endif // MORSEGENDEVICE_H
