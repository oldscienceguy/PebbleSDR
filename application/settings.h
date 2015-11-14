#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QSettings>
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

	//Hack, these should eventually be access methods
    QString sdrDeviceFilename;
    quint16 sdrDeviceNumber; //For plugins that support mulitple devices

	//If Output Sample Rate is above this, then we try to reduce it by skipping samples when we output
	int decimateLimit;
	bool postMixerDecimate; //If true, then downsample to decimate limit after mixer
	int framesPerBuffer;
	double dbOffset; //DB calibration for spectrum and smeter
	//Increment for left-right and up-down in spectrum display
	int leftRightIncrement;
	int upDownIncrement;
    int modeOffset; //tone offset for CW
	int fullScaleDb; //Spectrum and waterfall full scale limit

	//Window size and positioning
	qint16 windowWidth;
	qint16 windowHeight;
	qint16 windowXPos;
	qint16 windowYPos;

	//Fonts for consistent UI
    QFont smFont;
    QFont medFont;
    QFont lgFont;

	//Spectrum settings
	int updatesPerSecond;

	public slots:

private:
	QSettings *qSettings;
	void ReadSettings();

private slots:

};
