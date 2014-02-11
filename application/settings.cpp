//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "settings.h"
#include "soundcard.h"
#include "demod.h" //For DEMODMODE
#include "receiver.h"
#include "audioqt.h"

Settings::Settings(void)
{
	//Use ini files to avoid any registry problems or install/uninstall 
	//Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
	//Scope::SystemScope puts file c:\ProgramData\n1ddy
	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif

    qSettings = new QSettings(path+"/PebbleData/pebble.ini",QSettings::IniFormat);
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

	useTestBench = qSettings->value("TestBench",false).toBool();

}

//Save to disk
void Settings::WriteSettings()
{
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
	qSettings->setValue("TestBench",useTestBench);

	qSettings->sync();
}
