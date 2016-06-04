//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "settings.h"
#include "demod.h" //For DeviceInterface::DEMODMODE
#include "receiver.h"
#include "testbench.h"


Settings::Settings(void)
{
	//Use ini files to avoid any registry problems or install/uninstall 
	//Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
	//Scope::SystemScope puts file c:\ProgramData\n1ddy

	m_qSettings = new QSettings(global->pebbleDataPath+"pebble.ini",QSettings::IniFormat);
	//qSettings->beginGroup("IQ");

	//qSetting->endGroup("IQ");

    //Set font size, get from settings eventually, change for 1024x768 vs higher res like 1440x900 (mac)
	m_smFont.setFamily("Lucida Grande");
	m_smFont.setPointSize(8);
	m_medFont.setFamily("Lucida Grande");
	m_medFont.setPointSize(10);
	m_lgFont.setFamily("Lucida Grande");
	m_lgFont.setPointSize(12);
	readSettings();
}

Settings::~Settings(void)
{
	delete m_qSettings;
}

void Settings::readSettings()
{
	//Read settings from ini file or set defaults
	//Todo: Make strings constants
	//If we don' specify a group, "General" is assumed
	//Restore efault or last window size and position
	QRect pos = global->primaryScreen->availableGeometry();
	//Default position is upper right corner of screen
	//There are no valid defaults for height and width when we're creating pebble.ini first time, set to invalid value
	m_windowHeight = m_qSettings->value("windowHeight", -1).toInt();
	m_windowWidth = m_qSettings->value("windowWidth", -1).toInt();
	//bump tight to right
	m_windowXPos = m_qSettings->value("windowXPos",pos.right() - global->mainWindow->minimumWidth()).toInt();
	m_windowYPos = m_qSettings->value("windowYPos", pos.top()).toInt();

	m_sdrDeviceFilename = m_qSettings->value("sdrDeviceFilename", "SR_V9").toString();
	m_sdrDeviceNumber = m_qSettings->value("sdrDeviceNumber",0).toUInt();

	m_decimateLimit = m_qSettings->value("DecimateLimit", 24000).toInt();
	m_postMixerDecimate = m_qSettings->value("PostMixerDecimate",true).toBool();
    //Be careful about changing this, has global impact
	m_framesPerBuffer = m_qSettings->value("FramesPerBuffer",2048).toInt();
	//Could be UI, more bins = more resolution at zoom levels
	m_numSpectrumBins = m_qSettings->value("NumSpectrumBins",4096).toInt();
	//Hires spectrum is less than 100k or 50hz per bin at 2048 bins
	m_numHiResSpectrumBins = m_qSettings->value("NumHiResSpectrumBins",2048).toInt();

	m_dbOffset = m_qSettings->value("dbOffset",-60).toFloat();
	m_leftRightIncrement = m_qSettings->value("LeftRightIncrement",10).toInt();
	m_upDownIncrement = m_qSettings->value("UpDownIncrement",100).toInt();
	m_modeOffset = m_qSettings->value("ModeOffset",1000).toInt(); //CW tone

	m_fullScaleDb = m_qSettings->value("FullScaleDb",-50).toInt();
	m_baseScaleDb = m_qSettings->value("BaseScaleDb",DB::minDb).toInt();
	m_autoScaleMax = m_qSettings->value("AutoScaleMax",true).toBool();
	m_autoScaleMin = m_qSettings->value("AutoScaleMin",true).toBool();

	m_updatesPerSecond = m_qSettings->value("UpdatesPerSec", 10).toInt();

	m_dataPluginName = m_qSettings->value("DataPluginName","No Data").toString();

	m_qSettings->beginGroup(tr("Testbench"));
	global->testBench->m_sweepStartFrequency = m_qSettings->value(tr("SweepStartFrequency"),0.0).toDouble();
	global->testBench->m_sweepStopFrequency = m_qSettings->value(tr("SweepStopFrequency"),1.0).toDouble();
	global->testBench->m_sweepRate = m_qSettings->value(tr("SweepRate"),0.0).toDouble();
	global->testBench->m_displayRate = m_qSettings->value(tr("DisplayRate"),10).toInt();
	global->testBench->m_vertRange = m_qSettings->value(tr("VertRange"),10000).toInt();
	global->testBench->m_trigIndex = m_qSettings->value(tr("TrigIndex"),0).toInt();
	global->testBench->m_trigLevel = m_qSettings->value(tr("TrigLevel"),100).toInt();
	global->testBench->m_horzSpan = m_qSettings->value(tr("HorzSpan"),100).toInt();
	global->testBench->m_profile = m_qSettings->value(tr("Profile"),0).toInt();
	global->testBench->m_timeDisplay = m_qSettings->value(tr("TimeDisplay"),false).toBool();
	global->testBench->m_genOn = m_qSettings->value(tr("GenOn"),false).toBool();
	global->testBench->m_noiseOn = m_qSettings->value(tr("NoiseOn"),false).toBool();
	global->testBench->m_peakOn = m_qSettings->value(tr("PeakOn"),false).toBool();
	global->testBench->m_pulseWidth = m_qSettings->value(tr("PulseWidth"),0.0).toDouble();
	global->testBench->m_pulsePeriod = m_qSettings->value(tr("PulsePeriod"),0.0).toDouble();
	global->testBench->m_signalPower = m_qSettings->value(tr("SignalPower"),0.0).toDouble();
	global->testBench->m_noisePower = m_qSettings->value(tr("NoisePower"),-70.0).toDouble();
	global->testBench->m_useFmGen = m_qSettings->value(tr("UseFmGen"),false).toBool();
	m_qSettings->endGroup();

}

//Save to disk
void Settings::writeSettings()
{
	//Get the current window size and position so we can save it
	m_windowHeight = global->mainWindow->height();
	m_windowWidth = global->mainWindow->width();
	m_windowXPos = global->mainWindow->x();
	m_windowYPos = global->mainWindow->y();
	m_qSettings->setValue("windowHeight",m_windowHeight);
	m_qSettings->setValue("windowWidth",m_windowWidth);
	m_qSettings->setValue("windowXPos",m_windowXPos);
	m_qSettings->setValue("windowYPos",m_windowYPos);

    //No UI Settings, only in file
	m_qSettings->setValue("sdrDeviceFilename",m_sdrDeviceFilename);
	m_qSettings->setValue("sdrDeviceNumber",m_sdrDeviceNumber);

	m_qSettings->setValue("dbOffset",m_dbOffset);
	m_qSettings->setValue("DecimateLimit",m_decimateLimit);
	m_qSettings->setValue("PostMixerDecimate",m_postMixerDecimate);
	m_qSettings->setValue("FramesPerBuffer",m_framesPerBuffer);
	m_qSettings->setValue("NumSpectrumBins",m_numSpectrumBins);
	m_qSettings->setValue("NumHiResSpectrumBins",m_numHiResSpectrumBins);
	m_qSettings->setValue("LeftRightIncrement",m_leftRightIncrement);
	m_qSettings->setValue("UpDownIncrement",m_upDownIncrement);
	m_qSettings->setValue("ModeOffset",m_modeOffset);
	m_qSettings->setValue("FullScaleDb",m_fullScaleDb);
	m_qSettings->setValue("BaseScaleDb",m_baseScaleDb);
	m_qSettings->setValue("AutoScaleMax",m_autoScaleMax);
	m_qSettings->setValue("AutoScaleMin",m_autoScaleMin);

	m_qSettings->setValue("UpdatesPerSec",m_updatesPerSecond);

	m_qSettings->setValue("DataPluginName",m_dataPluginName);

	m_qSettings->beginGroup(tr("Testbench"));

	m_qSettings->setValue(tr("SweepStartFrequency"),global->testBench->m_sweepStartFrequency);
	m_qSettings->setValue(tr("SweepStopFrequency"),global->testBench->m_sweepStopFrequency);
	m_qSettings->setValue(tr("SweepRate"),global->testBench->m_sweepRate);
	m_qSettings->setValue(tr("DisplayRate"),global->testBench->m_displayRate);
	m_qSettings->setValue(tr("VertRange"),global->testBench->m_vertRange);
	m_qSettings->setValue(tr("TrigIndex"),global->testBench->m_trigIndex);
	m_qSettings->setValue(tr("TimeDisplay"),global->testBench->m_timeDisplay);
	m_qSettings->setValue(tr("HorzSpan"),global->testBench->m_horzSpan);
	m_qSettings->setValue(tr("TrigLevel"),global->testBench->m_trigLevel);
	m_qSettings->setValue(tr("Profile"),global->testBench->m_profile);
	m_qSettings->setValue(tr("GenOn"),global->testBench->m_genOn);
	m_qSettings->setValue(tr("NoiseOn"),global->testBench->m_noiseOn);
	m_qSettings->setValue(tr("PeakOn"),global->testBench->m_peakOn);
	m_qSettings->setValue(tr("PulseWidth"),global->testBench->m_pulseWidth);
	m_qSettings->setValue(tr("PulsePeriod"),global->testBench->m_pulsePeriod);
	m_qSettings->setValue(tr("SignalPower"),global->testBench->m_signalPower);
	m_qSettings->setValue(tr("NoisePower"),global->testBench->m_noisePower);
	m_qSettings->setValue(tr("UseFmGen"),global->testBench->m_useFmGen);

	m_qSettings->endGroup();

	m_qSettings->sync();
}
