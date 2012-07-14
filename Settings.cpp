//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "QMessageBox"
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

    //Set font size, get from settings eventually, change for 1024x768 vs higher res like 1440x900 (mac)
    smFont.setFamily("Lucida Grande");
    smFont.setPointSize(8);
    medFont.setFamily("Lucida Grande");
    medFont.setPointSize(10);
    lgFont.setFamily("Lucida Grande");
    lgFont.setPointSize(12);

    selectedSDR = 0; //1st block by default



	settingsDialog = new QDialog();
	sd = new Ui::SettingsDialog();
	sd->setupUi(settingsDialog);

    SetupSDRBlock(sd->serialBox1,sd->receiverBox1, sd->sampleRateBox1, sd->startupBox1, sd->sourceBox1, sd->outputBox1);
    SetupSDRBlock(sd->serialBox2,sd->receiverBox2, sd->sampleRateBox2, sd->startupBox2, sd->sourceBox2, sd->outputBox2);
    SetupSDRBlock(sd->serialBox3,sd->receiverBox3, sd->sampleRateBox3, sd->startupBox3, sd->sourceBox3, sd->outputBox3);
    SetupSDRBlock(sd->serialBox4,sd->receiverBox4, sd->sampleRateBox4, sd->startupBox4, sd->sourceBox4, sd->outputBox4);

    sd->saveButton->setFont(medFont);
    sd->cancelButton->setFont(medFont);
    sd->resetAllButton->setFont(medFont);

    connect(sd->saveButton,SIGNAL(clicked(bool)),this,SLOT(SaveSettings(bool)));
    connect(sd->resetAllButton,SIGNAL(clicked(bool)),this,SLOT(ResetAllSettings(bool)));

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

void Settings::SetupSDRBlock(QComboBox *serialBox, QComboBox *receiverBox, QComboBox *sampleRateBox, QComboBox *startupBox, QComboBox *sourceBox, QComboBox *outputBox)
{
    serialBox->setFont(medFont);
    serialBox->addItem("Any",-1);
    serialBox->addItem("0",0);
    serialBox->addItem("1",1);
    serialBox->addItem("2",2);
    serialBox->addItem("3",3);
    serialBox->addItem("4",4);
    serialBox->addItem("5",5);
    serialBox->addItem("6",6);
    serialBox->addItem("7",7);
    serialBox->addItem("8",8);
    serialBox->addItem("9",9);
    serialBox->setFont(medFont);

    receiverBox->setFont(medFont);
    receiverBox->addItem("SR Ensemble",SDR::SR_ENSEMBLE);
    receiverBox->addItem("SR Ensemble 2M",SDR::SR_ENSEMBLE_2M);
    receiverBox->addItem("SR Ensemble 4M",SDR::SR_ENSEMBLE_4M);
    receiverBox->addItem("SR Ensemble 6M",SDR::SR_ENSEMBLE_6M);
    receiverBox->addItem("SR V9-ABPF",SDR::SR_V9);
    receiverBox->addItem("SR LITE II",SDR::SR_LITE);
    receiverBox->addItem("Elektor SDR",SDR::ELEKTOR);
    receiverBox->addItem("RFSpace SDR-IQ",SDR::SDR_IQ);
    receiverBox->addItem("HPSDR USB",SDR::HPSDR_USB);
    //receiverBox->addItem("HPSDR TCP",SDR::HPSDR_TCP);
    receiverBox->addItem("SDR Widget",SDR::SDRWIDGET);
    receiverBox->addItem("FUNcube Dongle",SDR::FUNCUBE);
    receiverBox->addItem("File",SDR::FILE);
    receiverBox->addItem("DVB-T",SDR::DVB_T);
    connect(receiverBox,SIGNAL(currentIndexChanged(int)),this,SLOT(ReceiverChanged(int)));

    sampleRateBox->blockSignals(true);
    sampleRateBox->addItem("48k",48000);
    sampleRateBox->addItem("96k",96000);
    sampleRateBox->addItem("192k",192000);
    sampleRateBox->addItem("384k",384000);
    sampleRateBox->blockSignals(false);
    sampleRateBox->setFont(medFont);
    //connect

    startupBox->setFont(medFont);
    startupBox->addItem("Last Frequency",Settings::LASTFREQ);
    startupBox->addItem("Set Frequency", Settings::SETFREQ);
    startupBox->addItem("Device Default", Settings::DEFAULTFREQ);
    connect(startupBox,SIGNAL(currentIndexChanged(int)),this,SLOT(StartupChanged(int)));

    sourceBox->setFont(medFont);
    outputBox->setFont(medFont);

}

void Settings::ShowSettings()
{
    sd->startupEdit1->setFont(medFont);
    sd->startupEdit1->setText(QString::number(this->ini_startupFreq[0],'f',0));
    sd->startupEdit2->setFont(medFont);
    sd->startupEdit2->setText(QString::number(this->ini_startupFreq[1],'f',0));
    sd->startupEdit3->setFont(medFont);
    sd->startupEdit3->setText(QString::number(this->ini_startupFreq[2],'f',0));
    sd->startupEdit4->setFont(medFont);
    sd->startupEdit4->setText(QString::number(this->ini_startupFreq[3],'f',0));

#if 0
	if (startup == SETFREQ)
		sd->startupEdit->setEnabled(true);
	else
		sd->startupEdit->setEnabled(false);
#endif

	//Audio devices may have been plugged or unplugged, refresh list on each show
	QStringList inputDevices = SoundCard::DeviceList(true);
	QStringList outputDevices = SoundCard::DeviceList(false);
	//Adding items triggers selection, block signals until list is complete
    sd->sourceBox1->blockSignals(true);
    sd->sourceBox2->blockSignals(true);
    sd->sourceBox3->blockSignals(true);
    sd->sourceBox4->blockSignals(true);

    sd->sourceBox1->clear();
    sd->sourceBox2->clear();
    sd->sourceBox3->clear();
    sd->sourceBox4->clear();
    int id;
	for (int i=0; i<inputDevices.count(); i++)
	{
		id = inputDevices[i].left(2).toInt();
        sd->sourceBox1->addItem(inputDevices[i].mid(3),id);
        sd->sourceBox2->addItem(inputDevices[i].mid(3),id);
        sd->sourceBox3->addItem(inputDevices[i].mid(3),id);
        sd->sourceBox4->addItem(inputDevices[i].mid(3),id);
        if (id == ini_inputDevice[0])
            sd->sourceBox1->setCurrentIndex(i);
        if (id == ini_inputDevice[1])
            sd->sourceBox2->setCurrentIndex(i);
        if (id == ini_inputDevice[2])
            sd->sourceBox3->setCurrentIndex(i);
        if (id == ini_inputDevice[3])
            sd->sourceBox4->setCurrentIndex(i);


	}
    sd->sourceBox1->blockSignals(false);
    sd->sourceBox2->blockSignals(false);
    sd->sourceBox3->blockSignals(false);
    sd->sourceBox4->blockSignals(false);

    sd->outputBox1->blockSignals(true);
    sd->outputBox2->blockSignals(true);
    sd->outputBox3->blockSignals(true);
    sd->outputBox4->blockSignals(true);

    sd->outputBox1->clear();
    sd->outputBox2->clear();
    sd->outputBox3->clear();
    sd->outputBox4->clear();

    for (int i=0; i<outputDevices.count(); i++)
	{
		id = outputDevices[i].left(2).toInt();
        sd->outputBox1->addItem(outputDevices[i].mid(3),id);
        sd->outputBox2->addItem(outputDevices[i].mid(3),id);
        sd->outputBox3->addItem(outputDevices[i].mid(3),id);
        sd->outputBox4->addItem(outputDevices[i].mid(3),id);
        if (id == ini_outputDevice[0])
            sd->outputBox1->setCurrentIndex(i);
        if (id == ini_outputDevice[1])
            sd->outputBox2->setCurrentIndex(i);
        if (id == ini_outputDevice[2])
            sd->outputBox3->setCurrentIndex(i);
        if (id == ini_outputDevice[3])
            sd->outputBox4->setCurrentIndex(i);

	}
    sd->outputBox1->blockSignals(false);
    sd->outputBox2->blockSignals(false);
    sd->outputBox3->blockSignals(false);
    sd->outputBox4->blockSignals(false);

    sd->iqGain1->setValue(ini_iqGain[0]);
    sd->iqGain2->setValue(ini_iqGain[1]);
    sd->iqGain3->setValue(ini_iqGain[2]);
    sd->iqGain4->setValue(ini_iqGain[3]);

    int cur;
    cur = sd->receiverBox1->findData(ini_sdrDevice[0]);
    sd->receiverBox1->setCurrentIndex(cur);
    cur = sd->receiverBox2->findData(ini_sdrDevice[1]);
    sd->receiverBox2->setCurrentIndex(cur);
    cur = sd->receiverBox3->findData(ini_sdrDevice[2]);
    sd->receiverBox3->setCurrentIndex(cur);
    cur = sd->receiverBox4->findData(ini_sdrDevice[3]);
    sd->receiverBox4->setCurrentIndex(cur);

    cur = sd->sampleRateBox1->findData(ini_sampleRate[0]);
    sd->sampleRateBox1->setCurrentIndex(cur);
    cur = sd->sampleRateBox2->findData(ini_sampleRate[1]);
    sd->sampleRateBox2->setCurrentIndex(cur);
    cur = sd->sampleRateBox3->findData(ini_sampleRate[2]);
    sd->sampleRateBox3->setCurrentIndex(cur);
    cur = sd->sampleRateBox4->findData(ini_sampleRate[3]);
    sd->sampleRateBox4->setCurrentIndex(cur);

    cur = sd->startupBox1->findData(ini_startup[0]);
    sd->startupBox1->setCurrentIndex(cur);
    cur = sd->startupBox2->findData(ini_startup[1]);
    sd->startupBox2->setCurrentIndex(cur);
    cur = sd->startupBox3->findData(ini_startup[2]);
    sd->startupBox3->setCurrentIndex(cur);
    cur = sd->startupBox4->findData(ini_startup[3]);
    sd->startupBox4->setCurrentIndex(cur);

	//Serial box only applies to SoftRocks for now
	ReceiverChanged(0); //Enable/disable
    sd->serialBox1->setCurrentIndex(ini_sdrNumber[0]+1);
    sd->serialBox2->setCurrentIndex(ini_sdrNumber[1]+1);
    sd->serialBox3->setCurrentIndex(ini_sdrNumber[2]+1);
    sd->serialBox4->setCurrentIndex(ini_sdrNumber[3]+1);

	//QSettings
	settingsDialog->show();
}
void Settings::StartupChanged(int i)
{
#if 0
	startup = (STARTUP)sd->startupBox->itemData(i).toInt();
	if (startup == SETFREQ)
		sd->startupEdit->setEnabled(true);
	else
		sd->startupEdit->setEnabled(false);
#endif
}
void Settings::ReceiverChanged(int i)
{
#if 0
	int cur = sd->receiverBox->currentIndex();
	SDR::SDRDEVICE dev = (SDR::SDRDEVICE)sd->receiverBox->itemData(cur).toInt();

	//Reset to default
    //!!sd->serialBox->setCurrentIndex(0);


	//Serial box only applies to SoftRocks for now
	if (dev == SDR::SR_ENSEMBLE||dev==SDR::SR_ENSEMBLE_2M||dev==SDR::SR_ENSEMBLE_4M||
        dev==SDR::SR_ENSEMBLE_6M || dev==SDR::SR_ENSEMBLE_LF || dev==SDR::SR_V9)
		sd->serialBox->setEnabled(true);
	else
		sd->serialBox->setEnabled(false);
#endif
}
void Settings::ReadSettings()
{
	//Read settings from ini file or set defaults
	//Todo: Make strings constants
	//If we don' specify a group, "General" is assumed
    ini_startup[0] = (STARTUP)qSettings->value("Startup1", 0).toInt();
    ini_startup[1] = (STARTUP)qSettings->value("Startup2", 0).toInt();
    ini_startup[2] = (STARTUP)qSettings->value("Startup3", 0).toInt();
    ini_startup[3] = (STARTUP)qSettings->value("Startup4", 0).toInt();
    startup = ini_startup[0]; //!!

    ini_startupFreq[0] = qSettings->value("StartupFreq1", 10000000).toDouble();
    ini_startupFreq[1] = qSettings->value("StartupFreq2", 10000000).toDouble();
    ini_startupFreq[2] = qSettings->value("StartupFreq3", 10000000).toDouble();
    ini_startupFreq[3] = qSettings->value("StartupFreq4", 10000000).toDouble();
    startupFreq = ini_startupFreq[0]; //!!

	lastFreq = qSettings->value("LastFreq", 10000000).toDouble();

    ini_inputDevice[0] = qSettings->value("InputDevice1", SoundCard::DefaultInputDevice()).toInt();
    ini_inputDevice[1] = qSettings->value("InputDevice2", SoundCard::DefaultInputDevice()).toInt();
    ini_inputDevice[2] = qSettings->value("InputDevice3", SoundCard::DefaultInputDevice()).toInt();
    ini_inputDevice[3] = qSettings->value("InputDevice4", SoundCard::DefaultInputDevice()).toInt();
    inputDevice = ini_inputDevice[0]; //!!

    ini_outputDevice[0] = qSettings->value("OutputDevice1", SoundCard::DefaultOutputDevice()).toInt();
    ini_outputDevice[1] = qSettings->value("OutputDevice2", SoundCard::DefaultOutputDevice()).toInt();
    ini_outputDevice[2] = qSettings->value("OutputDevice3", SoundCard::DefaultOutputDevice()).toInt();
    ini_outputDevice[3] = qSettings->value("OutputDevice4", SoundCard::DefaultOutputDevice()).toInt();
    outputDevice = ini_outputDevice[0]; //!!

    ini_sampleRate[0] = qSettings->value("SampleRate1", 48000).toInt();
    ini_sampleRate[1] = qSettings->value("SampleRate2", 48000).toInt();
    ini_sampleRate[2] = qSettings->value("SampleRate3", 48000).toInt();
    ini_sampleRate[3] = qSettings->value("SampleRate4", 48000).toInt();
    sampleRate = ini_sampleRate[0]; //!!

	decimateLimit = qSettings->value("DecimateLimit", 24000).toInt();
	postMixerDecimate = qSettings->value("PostMixerDecimate",true).toBool();
	//Be careful about changing this, has global impact 
	framesPerBuffer = qSettings->value("FramesPerBuffer",2048).toInt();

    ini_sdrDevice[0] = (SDR::SDRDEVICE)qSettings->value("sdrDevice1", SDR::SR_V9).toInt();
    ini_sdrDevice[1] = (SDR::SDRDEVICE)qSettings->value("sdrDevice2", SDR::SR_V9).toInt();
    ini_sdrDevice[2] = (SDR::SDRDEVICE)qSettings->value("sdrDevice3", SDR::SR_V9).toInt();
    ini_sdrDevice[3] = (SDR::SDRDEVICE)qSettings->value("sdrDevice4", SDR::SR_V9).toInt();
    sdrDevice = ini_sdrDevice[0]; //!!Replace with sdrselection

    ini_sdrNumber[0] = qSettings->value("sdrNumber1",-1).toInt();
    ini_sdrNumber[1] = qSettings->value("sdrNumber2",-1).toInt();
    ini_sdrNumber[2] = qSettings->value("sdrNumber3",-1).toInt();
    ini_sdrNumber[3] = qSettings->value("sdrNumber4",-1).toInt();
    sdrNumber = ini_sdrNumber[0]; //!!Replace with sdrselection

    ini_iqGain[0] = qSettings->value("iqGain1",0).toInt();
    ini_iqGain[1] = qSettings->value("iqGain2",0).toInt();
    ini_iqGain[2] = qSettings->value("iqGain3",0).toInt();
    ini_iqGain[3] = qSettings->value("iqGain4",0).toInt();

    dbOffset = qSettings->value("dbOffset",-70).toFloat();
	lastMode = qSettings->value("LastMode",0).toInt();
	lastDisplayMode = qSettings->value("LastDisplayMode",0).toInt();
	leftRightIncrement = qSettings->value("LeftRightIncrement",10).toInt();
	upDownIncrement = qSettings->value("UpDownIncrement",100).toInt();
}

//Slot for SaveSetting button or whenever we need to write ini file to disk
void Settings::SaveSettings(bool b)
{
    int cur;
    cur = sd->sourceBox1->currentIndex();
    ini_inputDevice[0] = sd->sourceBox1->itemData(cur).toInt();
    cur = sd->sourceBox2->currentIndex();
    ini_inputDevice[1] = sd->sourceBox2->itemData(cur).toInt();
    cur = sd->sourceBox3->currentIndex();
    ini_inputDevice[2] = sd->sourceBox3->itemData(cur).toInt();
    cur = sd->sourceBox4->currentIndex();
    ini_inputDevice[3] = sd->sourceBox4->itemData(cur).toInt();

    cur = sd->outputBox1->currentIndex();
    ini_outputDevice[0] = sd->outputBox1->itemData(cur).toInt();
    cur = sd->outputBox2->currentIndex();
    ini_outputDevice[1] = sd->outputBox2->itemData(cur).toInt();
    cur = sd->outputBox3->currentIndex();
    ini_outputDevice[2] = sd->outputBox3->itemData(cur).toInt();
    cur = sd->outputBox4->currentIndex();
    ini_outputDevice[3] = sd->outputBox4->itemData(cur).toInt();

    cur = sd->receiverBox1->currentIndex();
    ini_sdrDevice[0] = (SDR::SDRDEVICE)sd->receiverBox1->itemData(cur).toInt();
    cur = sd->receiverBox2->currentIndex();
    ini_sdrDevice[1] = (SDR::SDRDEVICE)sd->receiverBox2->itemData(cur).toInt();
    cur = sd->receiverBox3->currentIndex();
    ini_sdrDevice[2] = (SDR::SDRDEVICE)sd->receiverBox3->itemData(cur).toInt();
    cur = sd->receiverBox4->currentIndex();
    ini_sdrDevice[3] = (SDR::SDRDEVICE)sd->receiverBox4->itemData(cur).toInt();

    cur = sd->serialBox1->currentIndex();
    ini_sdrNumber[0] = sd->serialBox1->itemData(cur).toInt();
    cur = sd->serialBox2->currentIndex();
    ini_sdrNumber[1] = sd->serialBox2->itemData(cur).toInt();
    cur = sd->serialBox3->currentIndex();
    ini_sdrNumber[2] = sd->serialBox3->itemData(cur).toInt();
    cur = sd->serialBox4->currentIndex();
    ini_sdrNumber[3] = sd->serialBox4->itemData(cur).toInt();

    cur = sd->sampleRateBox1->currentIndex();
    ini_sampleRate[0] = sd->sampleRateBox1->itemData(cur).toInt();
    cur = sd->sampleRateBox2->currentIndex();
    ini_sampleRate[1] = sd->sampleRateBox2->itemData(cur).toInt();
    cur = sd->sampleRateBox3->currentIndex();
    ini_sampleRate[2] = sd->sampleRateBox3->itemData(cur).toInt();
    cur = sd->sampleRateBox4->currentIndex();
    ini_sampleRate[3] = sd->sampleRateBox4->itemData(cur).toInt();

    cur = sd->startupBox1->currentIndex();
    ini_startup[0] = (STARTUP)sd->startupBox1->itemData(cur).toInt();
    cur = sd->startupBox2->currentIndex();
    ini_startup[1] = (STARTUP)sd->startupBox2->itemData(cur).toInt();
    cur = sd->startupBox3->currentIndex();
    ini_startup[2] = (STARTUP)sd->startupBox3->itemData(cur).toInt();
    cur = sd->startupBox4->currentIndex();
    ini_startup[3] = (STARTUP)sd->startupBox4->itemData(cur).toInt();

    ini_iqGain[0] = sd->iqGain1->value();
    ini_iqGain[1] = sd->iqGain2->value();
    ini_iqGain[2] = sd->iqGain3->value();
    ini_iqGain[3] = sd->iqGain4->value();

    ini_startupFreq[0] = sd->startupEdit1->text().toDouble();
    ini_startupFreq[1] = sd->startupEdit2->text().toDouble();
    ini_startupFreq[2] = sd->startupEdit3->text().toDouble();
    ini_startupFreq[3] = sd->startupEdit4->text().toDouble();

	WriteSettings();
    ReadSettings();
	emit Restart();
}
//Save to disk
void Settings::WriteSettings()
{
	//Write back to ini file
    qSettings->setValue("Startup1",ini_startup[0]);
    qSettings->setValue("Startup2",ini_startup[1]);
    qSettings->setValue("Startup3",ini_startup[2]);
    qSettings->setValue("Startup4",ini_startup[3]);

    qSettings->setValue("StartupFreq1",ini_startupFreq[0]);
    qSettings->setValue("StartupFreq2",ini_startupFreq[1]);
    qSettings->setValue("StartupFreq3",ini_startupFreq[2]);
    qSettings->setValue("StartupFreq4",ini_startupFreq[3]);

	qSettings->setValue("LastFreq",lastFreq);
    qSettings->setValue("InputDevice1", ini_inputDevice[0]);
    qSettings->setValue("InputDevice2", ini_inputDevice[1]);
    qSettings->setValue("InputDevice3", ini_inputDevice[2]);
    qSettings->setValue("InputDevice4", ini_inputDevice[3]);

    qSettings->setValue("OutputDevice1", ini_outputDevice[0]);
    qSettings->setValue("OutputDevice2", ini_outputDevice[1]);
    qSettings->setValue("OutputDevice3", ini_outputDevice[2]);
    qSettings->setValue("OutputDevice4", ini_outputDevice[3]);

    qSettings->setValue("sdrDevice1",ini_sdrDevice[0]);
    qSettings->setValue("sdrDevice2",ini_sdrDevice[1]);
    qSettings->setValue("sdrDevice3",ini_sdrDevice[2]);
    qSettings->setValue("sdrDevice4",ini_sdrDevice[3]);

    qSettings->setValue("sdrNumber1",ini_sdrNumber[0]);
    qSettings->setValue("sdrNumber2",ini_sdrNumber[1]);
    qSettings->setValue("sdrNumber3",ini_sdrNumber[2]);
    qSettings->setValue("sdrNumber4",ini_sdrNumber[3]);

    qSettings->setValue("SampleRate1",ini_sampleRate[0]);
    qSettings->setValue("SampleRate2",ini_sampleRate[1]);
    qSettings->setValue("SampleRate3",ini_sampleRate[2]);
    qSettings->setValue("SampleRate4",ini_sampleRate[3]);

    qSettings->setValue("iqGain1",ini_iqGain[0]);
    qSettings->setValue("iqGain2",ini_iqGain[1]);
    qSettings->setValue("iqGain3",ini_iqGain[2]);
    qSettings->setValue("iqGain4",ini_iqGain[3]);

    //No UI Settings, only in file
	qSettings->setValue("dbOffset",dbOffset);
	qSettings->setValue("LastMode",lastMode);
	qSettings->setValue("LastDisplayMode",lastDisplayMode);
	qSettings->setValue("DecimateLimit",decimateLimit);
	qSettings->setValue("PostMixerDecimate",postMixerDecimate);
	qSettings->setValue("FramesPerBuffer",framesPerBuffer);
	qSettings->setValue("LeftRightIncrement",leftRightIncrement);
	qSettings->setValue("UpDownIncrement",upDownIncrement);

	qSettings->sync();
}

void Settings::ResetAllSettings(bool b)
{
    //Confirm
    QMessageBox::StandardButton bt = QMessageBox::question(NULL,
            tr("Confirm Reset"),
            tr("Are you sure you want to reset all settings (ini files)")
            );
    if (bt != QMessageBox::Ok)
        return;

    //Delete all ini files and restart

    emit Restart();
}
