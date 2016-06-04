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
	void writeSettings();

	//Hack, these should eventually be access methods
	QString m_sdrDeviceFilename;
	quint16 m_sdrDeviceNumber; //For plugins that support mulitple devices

	//If Output Sample Rate is above this, then we try to reduce it by skipping samples when we output
	int m_decimateLimit;
	bool m_postMixerDecimate; //If true, then downsample to decimate limit after mixer
	int m_framesPerBuffer;
	int m_numSpectrumBins;
	int m_numHiResSpectrumBins;
	double m_dbOffset; //DB calibration for spectrum and smeter
	//Increment for left-right and up-down in spectrum display
	int m_leftRightIncrement;
	int m_upDownIncrement;
	int m_modeOffset; //tone offset for CW
	int m_fullScaleDb; //Spectrum and waterfall full scale limit
	int m_baseScaleDb;

	//Window size and positioning
	qint16 m_windowWidth;
	qint16 m_windowHeight;
	qint16 m_windowXPos;
	qint16 m_windowYPos;

	//Fonts for consistent UI
	QFont m_smFont;
	QFont m_medFont;
	QFont m_lgFont;

	//Spectrum settings
	int m_updatesPerSecond;
	bool m_autoScaleMax;
	bool m_autoScaleMin;

	//Plugin settings
	QString m_dataPluginName;

	public slots:

private:
	QSettings *m_qSettings;
	void readSettings();

private slots:

};
