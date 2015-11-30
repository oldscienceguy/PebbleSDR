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
	QRect pos = qApp->screens()[0]->geometry();
	//Default position is upper right corner of screen
	windowHeight = qSettings->value("windowHeight", -1).toInt();
	windowWidth = qSettings->value("windowWidth", -1).toInt();
	//bump tight to right
	windowXPos = qSettings->value("windowXPos",pos.right() - global->mainWindow->width()).toInt();
	windowYPos = qSettings->value("windowYPos", 0).toInt();

    sdrDeviceFilename = qSettings->value("sdrDeviceFilename", "SR_V9").toString();
    sdrDeviceNumber = qSettings->value("sdrDeviceNumber",0).toUInt();

    decimateLimit = qSettings->value("DecimateLimit", 24000).toInt();
    postMixerDecimate = qSettings->value("PostMixerDecimate",true).toBool();
    //Be careful about changing this, has global impact
    framesPerBuffer = qSettings->value("FramesPerBuffer",2048).toInt();

    dbOffset = qSettings->value("dbOffset",-60).toFloat();
	leftRightIncrement = qSettings->value("LeftRightIncrement",10).toInt();
	upDownIncrement = qSettings->value("UpDownIncrement",100).toInt();
    modeOffset = qSettings->value("ModeOffset",1000).toInt(); //CW tone

	fullScaleDb = qSettings->value("FullScaleDb",-50).toInt();

	updatesPerSecond = qSettings->value("UpdatesPerSec", 10).toInt();

	qSettings->beginGroup(tr("Testbench"));
	global->testBench->m_SweepStartFrequency = qSettings->value(tr("SweepStartFrequency"),0.0).toDouble();
	global->testBench->m_SweepStopFrequency = qSettings->value(tr("SweepStopFrequency"),1.0).toDouble();
	global->testBench->m_SweepRate = qSettings->value(tr("SweepRate"),0.0).toDouble();
	global->testBench->m_DisplayRate = qSettings->value(tr("DisplayRate"),10).toInt();
	global->testBench->m_VertRange = qSettings->value(tr("VertRange"),10000).toInt();
	global->testBench->m_TrigIndex = qSettings->value(tr("TrigIndex"),0).toInt();
	global->testBench->m_TrigLevel = qSettings->value(tr("TrigLevel"),100).toInt();
	global->testBench->m_HorzSpan = qSettings->value(tr("HorzSpan"),100).toInt();
	global->testBench->m_Profile = qSettings->value(tr("Profile"),0).toInt();
	global->testBench->m_TimeDisplay = qSettings->value(tr("TimeDisplay"),false).toBool();
	global->testBench->m_GenOn = qSettings->value(tr("GenOn"),false).toBool();
	global->testBench->m_PeakOn = qSettings->value(tr("PeakOn"),false).toBool();
	global->testBench->m_PulseWidth = qSettings->value(tr("PulseWidth"),0.0).toDouble();
	global->testBench->m_PulsePeriod = qSettings->value(tr("PulsePeriod"),0.0).toDouble();
	global->testBench->m_SignalPower = qSettings->value(tr("SignalPower"),0.0).toDouble();
	global->testBench->m_NoisePower = qSettings->value(tr("NoisePower"),-70.0).toDouble();
	global->testBench->m_UseFmGen = qSettings->value(tr("UseFmGen"),false).toBool();
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
	qSettings->setValue("LeftRightIncrement",leftRightIncrement);
	qSettings->setValue("UpDownIncrement",upDownIncrement);
    qSettings->setValue("ModeOffset",modeOffset);
	qSettings->setValue("FullScaleDb",fullScaleDb);

	qSettings->setValue("UpdatesPerSec",updatesPerSecond);

	qSettings->beginGroup(tr("Testbench"));

	qSettings->setValue(tr("SweepStartFrequency"),global->testBench->m_SweepStartFrequency);
	qSettings->setValue(tr("SweepStopFrequency"),global->testBench->m_SweepStopFrequency);
	qSettings->setValue(tr("SweepRate"),global->testBench->m_SweepRate);
	qSettings->setValue(tr("DisplayRate"),global->testBench->m_DisplayRate);
	qSettings->setValue(tr("VertRange"),global->testBench->m_VertRange);
	qSettings->setValue(tr("TrigIndex"),global->testBench->m_TrigIndex);
	qSettings->setValue(tr("TimeDisplay"),global->testBench->m_TimeDisplay);
	qSettings->setValue(tr("HorzSpan"),global->testBench->m_HorzSpan);
	qSettings->setValue(tr("TrigLevel"),global->testBench->m_TrigLevel);
	qSettings->setValue(tr("Profile"),global->testBench->m_Profile);
	qSettings->setValue(tr("GenOn"),global->testBench->m_GenOn);
	qSettings->setValue(tr("PeakOn"),global->testBench->m_PeakOn);
	qSettings->setValue(tr("PulseWidth"),global->testBench->m_PulseWidth);
	qSettings->setValue(tr("PulsePeriod"),global->testBench->m_PulsePeriod);
	qSettings->setValue(tr("SignalPower"),global->testBench->m_SignalPower);
	qSettings->setValue(tr("NoisePower"),global->testBench->m_NoisePower);
	qSettings->setValue(tr("UseFmGen"),global->testBench->m_UseFmGen);

	qSettings->endGroup();

	qSettings->sync();
}
