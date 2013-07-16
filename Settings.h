#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QSettings>
#include "ui/ui_Settings.h"
#include "sdr.h"
#include "QFont"
/*
Encapsulates settings dialog, reading/writing settings file, etc
*/
class Settings:public QObject
{
		Q_OBJECT

public:
	Settings();
	~Settings(void);
	void ShowSettings();
	void WriteSettings();
	typedef enum STARTUP {SETFREQ = 0, LASTFREQ, DEFAULTFREQ} STARTUP;
    typedef enum IQORDER {IQ,QI,IONLY,QONLY} IQORDER;

	//Hack, these should eventually be access methods
    //STARTUP startup;
    //double startupFreq;
	double lastFreq;
	int lastMode;
	int lastDisplayMode; //Spectrum, waterfall, etc
    //QString inputDeviceName;
    //QString outputDeviceName;
    //int sampleRate;
	//If Output Sample Rate is above this, then we try to reduce it by skipping samples when we output
	int decimateLimit;
	bool postMixerDecimate; //If true, then downsample to decimate limit after mixer
	int framesPerBuffer;
	double dbOffset; //DB calibration for spectrum and smeter
	SDR::SDRDEVICE sdrDevice;
    //int sdrNumber; //For SoftRocks, selects last digit in serial number
	//Increment for left-right and up-down in spectrum display
	int leftRightIncrement;
	int upDownIncrement;

    //double iqGain; //Normalize device so incoming IQ levels are consistent
    //IQORDER iqOrder;
    //Image rejection (iqbalance) factors for this device
    //double iqBalanceGain;
    //double iqBalancePhase;
    //bool iqBalanceEnable;

    int selectedSDR;

    //Fonts for consisten UI
    QFont smFont;
    QFont medFont;
    QFont lgFont;


	public slots:
    void ResetAllSettings(bool b);

signals:
	//Settings changed, turn off and restart with new settings
	void Restart();

private:
	QDialog *settingsDialog;
	Ui::SettingsDialog *sd;
	QSettings *qSettings;
	void ReadSettings();
    void SetOptionsForSDR(int s);
    void SetupRecieverBox(QComboBox *receiverBox);

    //Values from ini file.  Public settings are for currently selected (1-4) SDR device
    SDR::SDRDEVICE ini_sdrDevice[4];
    int ini_sdrNumber[4]; //For SoftRocks, selects last digit in serial number
    int ini_sampleRate[4];
    STARTUP ini_startup[4];
    double ini_startupFreq[4];
    QString ini_inputDeviceName[4];
    QString ini_outputDeviceName[4];
    double ini_iqGain[4];
    IQORDER ini_iqOrder[4];
    double ini_iqBalanceGain[4];
    double ini_iqBalancePhase[4];
    bool ini_iqBalanceEnable[4];
    //Temporary *sdr so we can get defaults and write device options
    SDR *pSdr[4];
    SDR *GetActiveSDR();

    QStringList inputDevices;
    QStringList outputDevices;

private slots:
	void StartupChanged(int);
	void SaveSettings(bool b);
	void ReceiverChanged(int i);
    void SelectedSDRChanged(int s = -1);
    void IQGainChanged(double i);
    void IQOrderChanged(int);
    void ShowSDRSettings();
    void BalancePhaseChanged(int v);
    void BalanceGainChanged(int v);
    void BalanceEnabledChanged(bool b);
    void BalanceReset();

};
