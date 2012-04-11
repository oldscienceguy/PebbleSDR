#ifndef FUNCUBE_H
#define FUNCUBE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "hidapi.h"
#include "qsettings.h"
#include "SDR.h"
#include "ui_funcubeoptions.h"

class FunCube : public SDR
{
	Q_OBJECT

public:
	FunCube(Receiver *_receiver, SDRDEVICE dev,Settings *settings);
	~FunCube(void);

	//SDR class overrides
	bool Connect();
	bool Disconnect();
	void Start();
	void Stop();

	double SetFrequency(double fRequested, double fCurrent);
	void ShowOptions();
	double GetStartupFrequency();
	int GetStartupMode();
	double GetHighLimit();
	double GetLowLimit();
	double GetGain();
	QString GetDeviceName();
	int GetSampleRate();

	//FCD HW
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


private slots:
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
	void DefaultClicked(bool b);
	void SaveClicked(bool b);


private:
    int HIDWrite(unsigned char *data, int length);
    int HIDRead(unsigned char *data, int length);
	bool HIDGet(char cmd, void *data, int len);
	bool HIDSet(char cmd, unsigned char value);

	//Options
	QDialog *fcdOptionsDialog;
	Ui::FunCubeOptions *fco;

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
	bool fcdSetFreqHz; //true to use hz, false to set in khz

    hid_device *hidDev;
    hid_device_info *hidDevInfo;

	//Device constants
	static const unsigned short VID=0x04D8;
	static const unsigned short PID=0xFB56;
	int maxPacketSize;

	void ReadSettings();
	void WriteSettings();
	QSettings *qSettings;
	//Settings
	double sStartup;
	double sLow;
	double sHigh;
	int sStartupMode;
	double sGain;
	double sOffset; //Frequency offset
	//LibUSB PID VID to look for
	int sPID;
	int sVID;



};

#endif // FUNCUBE_H
