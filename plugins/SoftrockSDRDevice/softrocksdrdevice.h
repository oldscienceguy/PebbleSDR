#ifndef SOFTROCKSDRDEVICE_H
#define SOFTROCKSDRDEVICE_H


//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "usbutil.h"
#include "ui_softrockoptions.h"


class SoftrockSDRDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	//Devices supported by this plugin
	enum SDRDEVICE {SR_LITE=0, SR_V9, SR_ENSEMBLE, SR_ENSEMBLE_2M,
		SR_ENSEMBLE_4M, SR_ENSEMBLE_6M, SR_ENSEMBLE_LF, FiFi};
	//LibUSB id for SR driver
	const quint16 SR_VID = 0x16c0; //Vendor ID
	const quint16 SR_PID = 0x05dc; //Product ID
	//Todo: The last digit on serial number can be changed to allow multiple SRocks to be used simultaneously
	const char* SR_SERIAL_NUMBER = "PE0FKO-0";
	//Si570 CMOS range
	const quint32 SI570_MIN = 3500000;
	const quint32 SI570_MAX = 260000000;

	SoftrockSDRDevice();
	~SoftrockSDRDevice();

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

protected:
	//Device specific
	bool Version(short *major, short *minor);
	bool RestartSoftRock();
	bool SetInputMux(qint16 inpNum);
	bool GetInputMux(qint16 &inpNum);
	bool FilterCrossOver();
	bool GetAutoBPF(bool &b);
	bool SetAutoBPF(bool b);
	bool SetBandPass();
	bool GetBandPass();
	bool SetTXLowPass();
	bool GetTXLowPass();
	bool WriteRegister();
	bool SetFrequencyByRegister();
	bool SetFrequencyAdjust();
	bool SetFrequencyByValue(double iFreq);
	bool SetCrystalFrequency();
	bool SetStartupFrequency();
	bool SetSmoothTune();
	bool GetFrequencyAdjust();
	double GetFrequency();
	double GetLOFrequency();
	bool GetSmoothTune();
	bool GetSRStartupFrequency();
	bool GetCrystalFrequency();
	bool GetRegisters(short &r7,short &r8, short &r9, short &r10, short &r11, short &r12);
	bool SetI2CAddress();
	bool GetCPUTemperature();
	int GetSerialNumber();
	int SetSerialNumber(int _serialNumber);
	bool PTT();
	bool GetCWLevel();

	void FiFiGetSvn();
	bool FiFiVersion(quint32 *fifiVersion);
	bool FiFiReadPreselector(int preselNum, double *freq1, double *freq2, quint32 *pattern);
	bool FiFiReadPreselectorMode(quint32 *mode);
	bool FiFiWritePreselctorMode(quint32 mode);

private slots:
		void selectAutomatic(bool b);
		void selectInput0(bool b);
		void selectInput1(bool b);
		void selectInput2(bool b);
		void selectInput3(bool b);
		void serialNumberChanged(int s);
		void fifiUseABPFChanged(bool b);

private:
	void InitSettings(QString fname);

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);

	//Device
	int usbCtrlMsgIn(int request, int value, int index, unsigned char *bytes, int size);
	int usbCtrlMsgOut(int request, int value, int index, unsigned char *bytes, int size);
	double SRFreq2Freq(qint32 iFreq);
	qint32 Freq2SRFreq(double iFreq);

	Ui::SoftRockOptions *optionUi;
	int sdrNumber; //For SoftRocks, selects last digit in serial number
	USBUtil usbUtil;

	bool useABPF;

	QSettings *srEnsembleSettings;
	QSettings *srEnsemble2MSettings;
	QSettings *srEnsemble4MSettings;
	QSettings *srEnsemble6MSettings;
	QSettings *srEnsembleLFSettings;
	QSettings *srLiteSettings;
	QSettings *srV9Settings;
	QSettings *srFifiSettings;
};


#endif // SOFTROCKSDRDEVICE_H
