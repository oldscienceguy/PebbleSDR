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

	//Hack, these should eventually be access methods
	STARTUP startup;
	double startupFreq;
	double lastFreq;
	int lastMode;
	int lastDisplayMode; //Spectrum, waterfall, etc
	int inputDevice;
	int outputDevice;
	int sampleRate;
	//If Output Sample Rate is above this, then we try to reduce it by skipping samples when we output
	int decimateLimit;
	bool postMixerDecimate; //If true, then downsample to decimate limit after mixer
	int framesPerBuffer;
	double dbOffset; //DB calibration for spectrum and smeter
	SDR::SDRDEVICE sdrDevice;
	int sdrNumber; //For SoftRocks, selects last digit in serial number
	//Increment for left-right and up-down in spectrum display
	int leftRightIncrement;
	int upDownIncrement;

    //Fonts for consisten UI
    QFont smFont;
    QFont medFont;
    QFont lgFont;


	public slots:

signals:
	//Settings changed, turn off and restart with new settings
	void Restart();

private:
	QDialog *settingsDialog;
	Ui::SettingsDialog *sd;
	QSettings *qSettings;
	void ReadSettings();

private slots:
	void StartupChanged(int);
	void SaveSettings(bool b);
	void ReceiverChanged(int i);


};
