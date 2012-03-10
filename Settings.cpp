//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "Settings.h"
#include "soundcard.h"
#include "demod.h" //For DEMODMODE

Settings::Settings(void)
{
	//Use ini files to avoid any registry problems or install/uninstall 
	//Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
	//Scope::SystemScope puts file c:\ProgramData\n1ddy
	QString path = QCoreApplication::applicationDirPath();
	qSettings = new QSettings(path+"/pebble.ini",QSettings::IniFormat);
	//qSettings->beginGroup("IQ");

	//qSetting->endGroup("IQ");

	settingsDialog = new QDialog();
	sd = new Ui::SettingsDialog();
	sd->setupUi(settingsDialog);
	sd->serialBox->addItem("Any",-1);
	sd->serialBox->addItem("0",0);
	sd->serialBox->addItem("1",1);
	sd->serialBox->addItem("2",2);
	sd->serialBox->addItem("3",3);
	sd->serialBox->addItem("4",4);
	sd->serialBox->addItem("5",5);
	sd->serialBox->addItem("6",6);
	sd->serialBox->addItem("7",7);
	sd->serialBox->addItem("8",8);
	sd->serialBox->addItem("9",9);
	connect(sd->saveButton,SIGNAL(clicked(bool)),this,SLOT(SaveSettings(bool)));

	sd->receiverBox->addItem("SR Ensemble",SDR::SR_ENSEMBLE);
	sd->receiverBox->addItem("SR Ensemble 2M",SDR::SR_ENSEMBLE_2M);
	sd->receiverBox->addItem("SR Ensemble 4M",SDR::SR_ENSEMBLE_4M);
	sd->receiverBox->addItem("SR Ensemble 6M",SDR::SR_ENSEMBLE_6M);
	sd->receiverBox->addItem("SR V9-ABPF",SDR::SR_V9);
	sd->receiverBox->addItem("SR LITE II",SDR::SR_LITE);
	sd->receiverBox->addItem("Elektor SDR",SDR::ELEKTOR);
	sd->receiverBox->addItem("RFSpace SDR-IQ",SDR::SDR_IQ);
	sd->receiverBox->addItem("HPSDR USB",SDR::HPSDR_USB);
	//sd->receiverBox->addItem("HPSDR TCP",SDR::HPSDR_TCP);
	sd->receiverBox->addItem("SDR Widget",SDR::SDRWIDGET);
	sd->receiverBox->addItem("FUNcube Dongle",SDR::FUNCUBE);
	connect(sd->receiverBox,SIGNAL(currentIndexChanged(int)),this,SLOT(ReceiverChanged(int)));

	sd->startupBox->addItem("Last Frequency",Settings::LASTFREQ);
	sd->startupBox->addItem("Set Frequency", Settings::SETFREQ);
	sd->startupBox->addItem("Device Default", Settings::DEFAULTFREQ);
	connect(sd->startupBox,SIGNAL(currentIndexChanged(int)),this,SLOT(StartupChanged(int)));

	ReadSettings();
}

Settings::~Settings(void)
{
	if (settingsDialog->isVisible())
		settingsDialog->hide();
	delete qSettings;
	delete settingsDialog;
	delete sd;
}
void Settings::ShowSettings()
{
	sd->startupEdit->setText(QString::number(this->startupFreq,'f',0));
	if (startup == SETFREQ)
		sd->startupEdit->setEnabled(true);
	else
		sd->startupEdit->setEnabled(false);
	int sel = sd->startupBox->findData(startup);
	sd->startupBox->setCurrentIndex(sel);

	//Audio devices may have been plugged or unplugged, refresh list on each show
	QStringList inputDevices = SoundCard::DeviceList(true);
	QStringList outputDevices = SoundCard::DeviceList(false);
	//Adding items triggers selection, block signals until list is complete
	sd->sourceBox->blockSignals(true);
	sd->sourceBox->clear();
	int id;
	for (int i=0; i<inputDevices.count(); i++)
	{
		id = inputDevices[i].left(2).toInt();
		sd->sourceBox->addItem(inputDevices[i].mid(3),id);
		if (id == inputDevice)
			sd->sourceBox->setCurrentIndex(i);

	}
	sd->sourceBox->blockSignals(false);

	sd->outputBox->blockSignals(true);
	sd->outputBox->clear();
	for (int i=0; i<outputDevices.count(); i++)
	{
		id = outputDevices[i].left(2).toInt();
		sd->outputBox->addItem(outputDevices[i].mid(3),id);
		if (id == outputDevice)
			sd->outputBox->setCurrentIndex(i);

	}
	sd->outputBox->blockSignals(false);

	//Show IQ setting

	//Show Receiver setting
	for (int i=0; i<sd->receiverBox->count(); i++)
	{
		if (sd->receiverBox->itemData(i) == sdrDevice) {
			sd->receiverBox->setCurrentIndex(i);
			break;
		}
	}

	//Serial box only applies to SoftRocks for now
	ReceiverChanged(0); //Enable/disable
	sd->serialBox->setCurrentIndex(sdrNumber+1);
	
	//QSettings
	settingsDialog->show();
}
void Settings::StartupChanged(int i)
{
	startup = (STARTUP)sd->startupBox->itemData(i).toInt();
	if (startup == SETFREQ)
		sd->startupEdit->setEnabled(true);
	else
		sd->startupEdit->setEnabled(false);
}
void Settings::ReceiverChanged(int i)
{

	int cur = sd->receiverBox->currentIndex();
	SDR::SDRDEVICE dev = (SDR::SDRDEVICE)sd->receiverBox->itemData(cur).toInt();

	//Reset to default
	sd->serialBox->setCurrentIndex(0);

	//Serial box only applies to SoftRocks for now
	if (dev == SDR::SR_ENSEMBLE||dev==SDR::SR_ENSEMBLE_2M||dev==SDR::SR_ENSEMBLE_4M||
		dev==SDR::SR_ENSEMBLE_6M || dev==SDR::SR_ENSEMBLE_LF || dev==SDR::SR_V9)
		sd->serialBox->setEnabled(true);
	else
		sd->serialBox->setEnabled(false);
}
void Settings::ReadSettings()
{
	//Read settings from ini file or set defaults
	//Todo: Make strings constants
	//If we don' specify a group, "General" is assumed
	startup = (STARTUP)qSettings->value("Startup", 0).toInt();
	startupFreq = qSettings->value("StartupFreq", 10000000).toDouble();
	lastFreq = qSettings->value("LastFreq", 10000000).toDouble();
	inputDevice = qSettings->value("InputDevice", SoundCard::DefaultInputDevice()).toInt();
	outputDevice = qSettings->value("OutputDevice", SoundCard::DefaultOutputDevice()).toInt();
	sampleRate = qSettings->value("SampleRate", 48000).toInt();
	decimateLimit = qSettings->value("DecimateLimit", 24000).toInt();
	postMixerDecimate = qSettings->value("PostMixerDecimate",true).toBool();
	//Be careful about changing this, has global impact 
	framesPerBuffer = qSettings->value("FramesPerBuffer",2048).toInt();
	sdrDevice = (SDR::SDRDEVICE)qSettings->value("sdrDevice", SDR::SR_V9).toInt();
	sdrNumber = qSettings->value("sdrNumber",-1).toInt();
	dbOffset = qSettings->value("dbOffset",-70).toFloat();
	lastMode = qSettings->value("LastMode",0).toInt();
	lastDisplayMode = qSettings->value("LastDisplayMode",0).toInt();
	leftRightIncrement = qSettings->value("LeftRightIncrement",10).toInt();
	upDownIncrement = qSettings->value("UpDownIncrement",100).toInt();
}

//Slot for SaveSetting button or whenever we need to write ini file to disk
void Settings::SaveSettings(bool b)
{
	int cur = sd->sourceBox->currentIndex();
	inputDevice = sd->sourceBox->itemData(cur).toInt();
	cur = sd->outputBox->currentIndex();
	outputDevice = sd->outputBox->itemData(cur).toInt();
	cur = sd->receiverBox->currentIndex();
	sdrDevice = (SDR::SDRDEVICE)sd->receiverBox->itemData(cur).toInt();
	cur = sd->serialBox->currentIndex();
	sdrNumber = sd->serialBox->itemData(cur).toInt();
	startupFreq = sd->startupEdit->text().toDouble();

	WriteSettings();
	emit Restart();
}
//Save to disk
void Settings::WriteSettings()
{
	//Write back to ini file
	qSettings->setValue("Startup",startup);
	qSettings->setValue("StartupFreq",startupFreq);
	qSettings->setValue("LastFreq",lastFreq);
	qSettings->setValue("InputDevice", inputDevice);
	qSettings->setValue("OutputDevice", outputDevice);
	qSettings->setValue("sdrDevice",sdrDevice);
	qSettings->setValue("sdrNumber",sdrNumber);

	//No UI Settings, only in file
	qSettings->setValue("dbOffset",dbOffset);
	qSettings->setValue("LastMode",lastMode);
	qSettings->setValue("LastDisplayMode",lastDisplayMode);
	qSettings->setValue("SampleRate",sampleRate);
	qSettings->setValue("DecimateLimit",decimateLimit);
	qSettings->setValue("PostMixerDecimate",postMixerDecimate);
	qSettings->setValue("FramesPerBuffer",framesPerBuffer);
	qSettings->setValue("LeftRightIncrement",leftRightIncrement);
	qSettings->setValue("UpDownIncrement",upDownIncrement);

	qSettings->sync();
}
