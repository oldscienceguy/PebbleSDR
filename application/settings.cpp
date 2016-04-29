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

	qSettings = new QSettings(global->pebbleDataPath+"pebble.ini",QSettings::IniFormat);
	//qSettings->beginGroup("IQ");

	//qSetting->endGroup("IQ");

    //Set font size, get from settings eventually, change for 1024x768 vs higher res like 1440x900 (mac)
    smFont.setFamily("Lucida Grande");
    smFont.setPointSize(8);
    medFont.setFamily("Lucida Grande");
    medFont.setPointSize(10);
    lgFont.setFamily("Lucida Grande");
    lgFont.setPointSize(12);
	ReadSettings();
}

Settings::~Settings(void)
{
	delete qSettings;
}

void Settings::ReadSettings()
{
	//Read settings from ini file or set defaults
	//Todo: Make strings constants
	//If we don' specify a group, "General" is assumed
	//Restore efault or last window size and position
	QRect pos = global->primaryScreen->availableGeometry();
	//Default position is upper right corner of screen
	//There are no valid defaults for height and width when we're creating pebble.ini first time, set to invalid value
	windowHeight = qSettings->value("windowHeight", -1).toInt();
	windowWidth = qSettings->value("windowWidth", -1).toInt();
	//bump tight to right
	windowXPos = qSettings->value("windowXPos",pos.right() - global->mainWindow->minimumWidth()).toInt();
	windowYPos = qSettings->value("windowYPos", pos.top()).toInt();

    sdrDeviceFilename = qSettings->value("sdrDeviceFilename", "SR_V9").toString();
    sdrDeviceNumber = qSettings->value("sdrDeviceNumber",0).toUInt();

    decimateLimit = qSettings->value("DecimateLimit", 24000).toInt();
    postMixerDecimate = qSettings->value("PostMixerDecimate",true).toBool();
    //Be careful about changing this, has global impact
    framesPerBuffer = qSettings->value("FramesPerBuffer",2048).toInt();
	//Could be UI, more bins = more resolution at zoom levels
	numSpectrumBins = qSettings->value("NumSpectrumBins",4096).toInt();
	//Hires spectrum is less than 100k or 50hz per bin at 2048 bins
	numHiResSpectrumBins = qSettings->value("NumHiResSpectrumBins",2048).toInt();

    dbOffset = qSettings->value("dbOffset",-60).toFloat();
	leftRightIncrement = qSettings->value("LeftRightIncrement",10).toInt();
	upDownIncrement = qSettings->value("UpDownIncrement",100).toInt();
    modeOffset = qSettings->value("ModeOffset",1000).toInt(); //CW tone

	fullScaleDb = qSettings->value("FullScaleDb",-50).toInt();
	baseScaleDb = qSettings->value("BaseScaleDb",DB::minDb).toInt();
	autoScaleMax = qSettings->value("AutoScaleMax",true).toBool();
	autoScaleMin = qSettings->value("AutoScaleMin",true).toBool();

	updatesPerSecond = qSettings->value("UpdatesPerSec", 10).toInt();

	qSettings->beginGroup(tr("Testbench"));
	global->testBench->m_sweepStartFrequency = qSettings->value(tr("SweepStartFrequency"),0.0).toDouble();
	global->testBench->m_sweepStopFrequency = qSettings->value(tr("SweepStopFrequency"),1.0).toDouble();
	global->testBench->m_sweepRate = qSettings->value(tr("SweepRate"),0.0).toDouble();
	global->testBench->m_displayRate = qSettings->value(tr("DisplayRate"),10).toInt();
	global->testBench->m_vertRange = qSettings->value(tr("VertRange"),10000).toInt();
	global->testBench->m_trigIndex = qSettings->value(tr("TrigIndex"),0).toInt();
	global->testBench->m_trigLevel = qSettings->value(tr("TrigLevel"),100).toInt();
	global->testBench->m_horzSpan = qSettings->value(tr("HorzSpan"),100).toInt();
	global->testBench->m_profile = qSettings->value(tr("Profile"),0).toInt();
	global->testBench->m_timeDisplay = qSettings->value(tr("TimeDisplay"),false).toBool();
	global->testBench->m_genOn = qSettings->value(tr("GenOn"),false).toBool();
	global->testBench->m_noiseOn = qSettings->value(tr("NoiseOn"),false).toBool();
	global->testBench->m_peakOn = qSettings->value(tr("PeakOn"),false).toBool();
	global->testBench->m_pulseWidth = qSettings->value(tr("PulseWidth"),0.0).toDouble();
	global->testBench->m_pulsePeriod = qSettings->value(tr("PulsePeriod"),0.0).toDouble();
	global->testBench->m_signalPower = qSettings->value(tr("SignalPower"),0.0).toDouble();
	global->testBench->m_noisePower = qSettings->value(tr("NoisePower"),-70.0).toDouble();
	global->testBench->m_useFmGen = qSettings->value(tr("UseFmGen"),false).toBool();
	qSettings->endGroup();

}

//Save to disk
void Settings::WriteSettings()
{
	//Get the current window size and position so we can save it
	windowHeight = global->mainWindow->height();
	windowWidth = global->mainWindow->width();
	windowXPos = global->mainWindow->x();
	windowYPos = global->mainWindow->y();
	qSettings->setValue("windowHeight",windowHeight);
	qSettings->setValue("windowWidth",windowWidth);
	qSettings->setValue("windowXPos",windowXPos);
	qSettings->setValue("windowYPos",windowYPos);

    //No UI Settings, only in file
    qSettings->setValue("sdrDeviceFilename",sdrDeviceFilename);
    qSettings->setValue("sdrDeviceNumber",sdrDeviceNumber);

    qSettings->setValue("dbOffset",dbOffset);
	qSettings->setValue("DecimateLimit",decimateLimit);
	qSettings->setValue("PostMixerDecimate",postMixerDecimate);
	qSettings->setValue("FramesPerBuffer",framesPerBuffer);
	qSettings->setValue("NumSpectrumBins",numSpectrumBins);
	qSettings->setValue("NumHiResSpectrumBins",numHiResSpectrumBins);
	qSettings->setValue("LeftRightIncrement",leftRightIncrement);
	qSettings->setValue("UpDownIncrement",upDownIncrement);
    qSettings->setValue("ModeOffset",modeOffset);
	qSettings->setValue("FullScaleDb",fullScaleDb);
	qSettings->setValue("BaseScaleDb",baseScaleDb);
	qSettings->setValue("AutoScaleMax",autoScaleMax);
	qSettings->setValue("AutoScaleMin",autoScaleMin);

	qSettings->setValue("UpdatesPerSec",updatesPerSecond);

	qSettings->beginGroup(tr("Testbench"));

	qSettings->setValue(tr("SweepStartFrequency"),global->testBench->m_sweepStartFrequency);
	qSettings->setValue(tr("SweepStopFrequency"),global->testBench->m_sweepStopFrequency);
	qSettings->setValue(tr("SweepRate"),global->testBench->m_sweepRate);
	qSettings->setValue(tr("DisplayRate"),global->testBench->m_displayRate);
	qSettings->setValue(tr("VertRange"),global->testBench->m_vertRange);
	qSettings->setValue(tr("TrigIndex"),global->testBench->m_trigIndex);
	qSettings->setValue(tr("TimeDisplay"),global->testBench->m_timeDisplay);
	qSettings->setValue(tr("HorzSpan"),global->testBench->m_horzSpan);
	qSettings->setValue(tr("TrigLevel"),global->testBench->m_trigLevel);
	qSettings->setValue(tr("Profile"),global->testBench->m_profile);
	qSettings->setValue(tr("GenOn"),global->testBench->m_genOn);
	qSettings->setValue(tr("NoiseOn"),global->testBench->m_noiseOn);
	qSettings->setValue(tr("PeakOn"),global->testBench->m_peakOn);
	qSettings->setValue(tr("PulseWidth"),global->testBench->m_pulseWidth);
	qSettings->setValue(tr("PulsePeriod"),global->testBench->m_pulsePeriod);
	qSettings->setValue(tr("SignalPower"),global->testBench->m_signalPower);
	qSettings->setValue(tr("NoisePower"),global->testBench->m_noisePower);
	qSettings->setValue(tr("UseFmGen"),global->testBench->m_useFmGen);

	qSettings->endGroup();

	qSettings->sync();
}
