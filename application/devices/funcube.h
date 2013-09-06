#ifndef FUNCUBE_H
#define FUNCUBE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include "hidapi.h"
#include "qsettings.h"
#include "sdr.h"
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
    void SetupOptionUi(QWidget *parent);
    double GetStartupFrequency();
	int GetStartupMode();
	double GetHighLimit();
	double GetLowLimit();
	double GetGain();
	QString GetDeviceName();
	int GetSampleRate();
    int* GetSampleRates(int &len);  //Returns array of allowable rates

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

    void SetBiasTee(int v);
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

	void ReadSettings();
	void WriteSettings();
	//Settings
    double sFCDStartup;
    double sFCDPlusStartup;

    double sFCDLow;
    double sFCDPlusLow;
    double sFCDHigh;
    double sFCDPlusHigh;

    int sFCDStartupMode;
    int sFCDPlusStartupMode;

    double sGain;
	double sOffset; //Frequency offset
	//LibUSB PID VID to look for
    int sFCD_PID;
    int sFCD_PLUS_PID;
    int sVID;



};

#endif // FUNCUBE_H
