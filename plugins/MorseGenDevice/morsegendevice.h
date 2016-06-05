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

	QString m_sampleText[5];
	CPX nextNoiseSample(double _dbGain);
	qint32 m_dbNoiseAmp;
	double m_noiseAmp;

	void generate(CPX *out);

	//Settings values for all options
	bool m_gen1Enabled;
	double m_gen1Freq;
	double m_gen1Amp;
	quint32 m_gen1Wpm;
	quint32 m_gen1Rise;
	quint32 m_gen1Text;
	bool m_gen1Fade;
	bool m_gen1Fist;

	bool m_gen2Enabled;
	double m_gen2Freq;
	double m_gen2Amp;
	quint32 m_gen2Wpm;
	quint32 m_gen2Rise;
	quint32 m_gen2Text;
	bool m_gen2Fade;
	bool m_gen2Fist;

	bool m_gen3Enabled;
	double m_gen3Freq;
	double m_gen3Amp;
	quint32 m_gen3Wpm;
	quint32 m_gen3Rise;
	quint32 m_gen3Text;
	bool m_gen3Fade;
	bool m_gen3Fist;

	bool m_gen4Enabled;
	double m_gen4Freq;
	double m_gen4Amp;
	quint32 m_gen4Wpm;
	quint32 m_gen4Rise;
	quint32 m_gen4Text;
	bool m_gen4Fade;
	bool m_gen4Fist;

	bool m_gen5Enabled;
	double m_gen5Freq;
	double m_gen5Amp;
	quint32 m_gen5Wpm;
	quint32 m_gen5Rise;
	quint32 m_gen5Text;
	bool m_gen5Fade;
	bool m_gen5Fist;

	void updateGenerators();

	QMutex m_mutex; //Locks generator changes when producer thead is calling generate()
};
#endif // MORSEGENDEVICE_H
