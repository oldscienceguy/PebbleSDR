#ifndef EXAMPLESDRDEVICE_H
#define EXAMPLESDRDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "producerconsumer.h"
#include "hidapi.h"
#include "ui_funcubeoptions.h"

class FunCubeSDRDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	enum SDRDEVICE {FUNCUBE_PRO, FUNCUBE_PRO_PLUS};

	FunCubeSDRDevice();
	~FunCubeSDRDevice();

	//Required
	bool Initialize(cbProcessIQData _callback,
					cbProcessBandscopeData _callbackSpectrum,
					cbProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool Connect();
	bool Disconnect();
	void Start();
	void Stop();
	void ReadSettings();
	void WriteSettings();
	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);
	//Display device option widget in settings dialog
	void SetupOptionUi(QWidget *parent);

private slots:
	void BiasTeeChanged(int s);
	void LNAGainChanged(int s);
	void LNAEnhanceChanged(int s);
	void BandChanged(int s);
	void RFFilterChanged(int s);
	void MixerGainChanged(int s);
	void BiasCurrentChanged(int s);
	void MixerFilterChanged(int s);
	void IFGain1Changed(int s);
	void IFGain2Changed(int s);
	void IFGain3Changed(int s);
	void IFGain4Changed(int s);
	void IFGain5Changed(int s);
	void IFGain6Changed(int s);
	void IFGainModeChanged(int s);
	void IFRCFilterChanged(int s);
	void IFFilterChanged(int s);

private:
	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	ProducerConsumer producerConsumer;
	QSettings *funCubeProSettings;
	QSettings *funCubeProPlusSettings;

	//FCD HW
	double SetFrequency(double fRequested);
	void SetDCCorrection(qint16 cI, qint16 cQ);
	//Phase is signed, gain is unsigned
	void SetIQCorrection(qint16 phase, quint16 gain);
	void SetLNAGain(int v);
	void SetLNAEnhance(int v);
	void SetBand(int v);
	void SetRFFilter(int v);
	void SetMixerGain(int v);
	void SetBiasCurrent(int v);
	void SetMixerFilter(int v);
	void SetIFGain1(int v);
	void SetIFGain2(int v);
	void SetIFGain3(int v);
	void SetIFGain4(int v);
	void SetIFGain5(int v);
	void SetIFGain6(int v);
	void SetIFGainMode(int v);
	void SetIFRCFilter(int v);
	void SetIFFilter(int v);

	void SetBiasTee(int v);

	int HIDWrite(unsigned char *data, int length);
	int HIDRead(unsigned char *data, int length);
	bool HIDGet(char cmd, void *data, int len);
	bool HIDSet(char cmd, unsigned char value);

	//Options
	Ui::FunCubeOptions *optionUi;

	//FCD Data
	void ReadFCDOptions();
	QString fcdVersion;
	double fcdFreq;
	int fcdLNAGain;
	int fcdLNAEnhance;
	int fcdBand;
	int fcdRFFilter;
	int fcdMixerGain;
	int fcdBiasCurrent;
	int fcdMixerFilter;
	int fcdIFGain1;
	int fcdIFGain2;
	int fcdIFGain3;
	int fcdIFGain4;
	int fcdIFGain5;
	int fcdIFGain6;
	int fcdIFGainMode;
	int fcdIFRCFilter;
	int fcdIFFilter;
	int fcdDCICorrection;
	int fcdDCQCorrection;
	int fcdIQPhaseCorrection;
	int fcdIQGainCorrection;
	int fcdBiasTee; //FCD+ only 0 or 1
	bool fcdSetFreqHz; //true to use hz, false to set in khz

	hid_device *hidDev;
	hid_device_info *hidDevInfo;

	//Device constants
	static const unsigned short VID = 0x04D8; //Vendor ID is same for FCD Pro and FCD Pro Plus
	static const unsigned short FCD_PID = 0xFB56;
	static const unsigned short FCD_PLUS_PID = 0xFB31;
	int maxPacketSize;

	//Settings
	double sOffset; //Frequency offset
	//LibUSB PID VID to look for
	int sPID;
	int sVID;


};
#endif // EXAMPLESDRDEVICE_H
