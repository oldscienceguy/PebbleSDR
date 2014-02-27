#include "sdroptions.h"
#include "sdr.h"
#include "settings.h"
#include "receiver.h"
#include <QMessageBox>

SdrOptions::SdrOptions():QObject()
{
	sdrOptionsDialog = NULL;
	sd = NULL;
	di = NULL;
}

SdrOptions::~SdrOptions()
{
	if (sdrOptionsDialog != NULL)
		sdrOptionsDialog->hide();

}

//If b is false, close and delete options
void SdrOptions::ShowSdrOptions(DeviceInterface *_di, bool b)
{
	if (_di == NULL)
		return;

	di = _di;
	//Simplified logic from earlier checkin
	//We always delete dialog when closing so input/output devices and settings are always reset
	if (!b) {
		//We are closing dialog
		if (sdrOptionsDialog != NULL) {
			sdrOptionsDialog->setVisible(b); //hide
			delete sdrOptionsDialog;
			sdrOptionsDialog = NULL;
		}
		return;
	} else {
		//b is true
		//If we're visible, and asking to be shown again, toggle off
		if (sdrOptionsDialog != NULL && sdrOptionsDialog->isVisible()) {
			sdrOptionsDialog->setVisible(b); //hide
			delete sdrOptionsDialog;
			sdrOptionsDialog = NULL;
			return;
		}
		//Create new dialog and show
		int cur;
		sdrOptionsDialog = new QDialog();
		sd = new Ui::SdrOptions();
		sd->setupUi(sdrOptionsDialog);
		sdrOptionsDialog->setWindowTitle("Pebble Settings");

		//Populate device selector
		int deviceCount = di->Get(DeviceInterface::PluginNumDevices).toInt();
		for (int i=0; i<deviceCount; i++) {
			sd->deviceSelection->addItem(di->Get(DeviceInterface::DeviceName,i).toString());
		}
		//Select active device
		sd->deviceSelection->setCurrentIndex(global->settings->sdrDeviceNumber);
		//And make sure device has same number
		di->Set(DeviceInterface::DeviceNumber,global->settings->sdrDeviceNumber);
		//And connect so we get changes
		connect(sd->deviceSelection,SIGNAL(currentIndexChanged(int)),this,SLOT(DeviceSelectionChanged(int)));

		//Read current settings now that we have correct deviceNumber
		di->ReadSettings();

		sd->startupBox->addItem("Last Frequency",SDR::LASTFREQ);
		sd->startupBox->addItem("Set Frequency", SDR::SETFREQ);
		sd->startupBox->addItem("Device Default", SDR::DEFAULTFREQ);
		connect(sd->startupBox,SIGNAL(currentIndexChanged(int)),this,SLOT(StartupChanged(int)));

		connect(sd->startupEdit,SIGNAL(editingFinished()),this,SLOT(StartupFrequencyChanged()));
		connect(sd->iqGain,SIGNAL(valueChanged(double)),this,SLOT(IQGainChanged(double)));

		sd->IQSettings->addItem("I/Q (normal)",DeviceInterface::IQ);
		sd->IQSettings->addItem("Q/I (swap)",DeviceInterface::QI);
		sd->IQSettings->addItem("I Only",DeviceInterface::IONLY);
		sd->IQSettings->addItem("Q Only",DeviceInterface::QONLY);
		connect(sd->IQSettings,SIGNAL(currentIndexChanged(int)),this,SLOT(IQOrderChanged(int)));

		sd->iqBalancePhase->setMaximum(500);
		sd->iqBalancePhase->setMinimum(-500);
		connect(sd->iqBalancePhase,SIGNAL(valueChanged(int)),this,SLOT(BalancePhaseChanged(int)));

		sd->iqBalanceGain->setMaximum(1250);
		sd->iqBalanceGain->setMinimum(750);
		connect(sd->iqBalanceGain,SIGNAL(valueChanged(int)),this,SLOT(BalanceGainChanged(int)));

		connect(sd->iqEnableBalance,SIGNAL(toggled(bool)),this,SLOT(BalanceEnabledChanged(bool)));
		connect(sd->iqBalanceReset,SIGNAL(clicked()),this,SLOT(BalanceReset()));

		connect(sd->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

		connect(sd->closeButton,SIGNAL(clicked(bool)),this,SLOT(CloseOptions(bool)));
		connect(sd->resetAllButton,SIGNAL(clicked(bool)),this,SLOT(ResetAllSettings(bool)));

		sd->testBenchBox->setChecked(global->settings->useTestBench);
		connect(sd->testBenchBox, SIGNAL(toggled(bool)), this, SLOT(TestBenchChanged(bool)));

		connect(sd->sourceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(InputChanged(int)));
		connect(sd->outputBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OutputChanged(int)));

		//From here on, everything can change if different device is selected
		UpdateOptions();

		sdrOptionsDialog->show();

	}
}

//Called when we need to update an open options dialog, like when different device is selected
void SdrOptions::UpdateOptions()
{
	int id;
	QString dn;
	if (di->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ) {
		//Audio devices may have been plugged or unplugged, refresh list on each show
		//This will use PortAudio or QTAudio depending on configuration
		inputDevices = Audio::InputDeviceList();
		//Adding items triggers selection, block signals until list is complete
		sd->sourceBox->blockSignals(true);
		sd->sourceBox->clear();
		//Input devices may be restricted form some SDRs
		for (int i=0; i<inputDevices.count(); i++)
		{
			//This is portAudio specific format ID:Name
			//AudioQt will emulate
			id = inputDevices[i].left(2).toInt();
			dn = inputDevices[i].mid(3);
			sd->sourceBox->addItem(dn, id);
			if (dn == di->Get(DeviceInterface::InputDeviceName).toString())
				sd->sourceBox->setCurrentIndex(i);
		}
		sd->sourceBox->blockSignals(false);
	} else {
		sd->sourceLabel->setVisible(false);
		sd->sourceBox->setVisible(false);
	}
	outputDevices = Audio::OutputDeviceList();
	sd->outputBox->blockSignals(true);
	sd->outputBox->clear();
	for (int i=0; i<outputDevices.count(); i++)
	{
		id = outputDevices[i].left(2).toInt();
		dn = outputDevices[i].mid(3);
		sd->outputBox->addItem(dn, id);
		if (dn == di->Get(DeviceInterface::OutputDeviceName).toString())
			sd->outputBox->setCurrentIndex(i);
	}
	sd->outputBox->blockSignals(false);

	sd->startupBox->blockSignals(true);
	int cur = sd->startupBox->findData(di->Get(DeviceInterface::StartupType).toInt());
	sd->startupBox->setCurrentIndex(cur);
	sd->startupBox->blockSignals(false);

	sd->startupEdit->setText(di->Get(DeviceInterface::UserFrequency).toString());

	sd->iqGain->blockSignals(true);
	sd->iqGain->setValue(di->Get(DeviceInterface::IQGain).toDouble());
	sd->iqGain->blockSignals(false);

	sd->IQSettings->blockSignals(true);
	sd->IQSettings->setCurrentIndex(di->Get(DeviceInterface::IQOrder).toInt());
	sd->IQSettings->blockSignals(false);

	sd->iqEnableBalance->setChecked(di->Get(DeviceInterface::IQBalanceEnabled).toBool());

	sd->iqBalancePhase->blockSignals(true);
	sd->iqBalancePhase->setValue(di->Get(DeviceInterface::IQBalancePhase).toDouble() * 1000);
	sd->iqBalancePhase->blockSignals(false);

	sd->iqBalanceGain->blockSignals(true);
	sd->iqBalanceGain->setValue(di->Get(DeviceInterface::IQBalanceGain).toDouble() * 1000);
	sd->iqBalanceGain->blockSignals(false);

	//Set up options and get allowable sampleRates from device

	int sr;
	QStringList sampleRates = di->Get(DeviceInterface::DeviceSampleRates).toStringList();
	sd->sampleRateBox->blockSignals(true);
	sd->sampleRateBox->clear();
	for (int i=0; i<sampleRates.count(); i++) {
		sr = sampleRates[i].toInt();
		sd->sampleRateBox->addItem(sampleRates[i],sr);
		if (di->Get(DeviceInterface::DeviceSampleRate).toUInt() == sr)
			sd->sampleRateBox->setCurrentIndex(i);
	}
	sd->sampleRateBox->blockSignals(false);

	//Careful here: Fragile coding practice
	//We're calling a virtual function in a base class method and expect it to call the over-ridden method in derived class
	//This only works because ShowSdrOptions() is being called via a pointer to the derived class
	//Some other method that's called from the base class could not do this
	di->SetupOptionUi(sd->sdrSpecific);

}

void SdrOptions::CloseOptions(bool b)
{
	if (sdrOptionsDialog != NULL)
		sdrOptionsDialog->close();
}

void SdrOptions::DeviceSelectionChanged(int i) {
	global->settings->sdrDeviceNumber = i;
	//Set device number
	di->Set(DeviceInterface::DeviceNumber, i); //Which ini file to read from
	//Read settings, ReadSettings will switch on deviceNumber
	di->ReadSettings();
	//ReadSettings will read in last device number and overwrite our change, so reset it again
	di->Set(DeviceInterface::DeviceNumber, i); //Which ini file to read from
	//Update sdr specific section
	UpdateOptions();
}

void SdrOptions::TestBenchChanged(bool b)
{
	global->settings->useTestBench = b;
	global->settings->WriteSettings();
}

void SdrOptions::SampleRateChanged(int i)
{
	int cur = sd->sampleRateBox->currentIndex();
	di->Set(DeviceInterface::DeviceSampleRate, sd->sampleRateBox->itemText(cur).toInt());
	di->WriteSettings();
}

void SdrOptions::InputChanged(int i)
{
	int cur = sd->sourceBox->currentIndex();
	di->Set(DeviceInterface::InputDeviceName, sd->sourceBox->itemText(cur));
	di->WriteSettings();
}

void SdrOptions::OutputChanged(int i)
{
	int cur = sd->outputBox->currentIndex();
	di->Set(DeviceInterface::OutputDeviceName, sd->outputBox->itemText(cur));
	di->WriteSettings();
}

void SdrOptions::StartupChanged(int i)
{
	di->Set(DeviceInterface::StartupType, sd->startupBox->itemData(i).toInt());
	sd->startupEdit->setText(di->Get(DeviceInterface::UserFrequency).toString());
	if (di->Get(DeviceInterface::StartupType).toInt() == DeviceInterface::SETFREQ) {
		sd->startupEdit->setEnabled(true);
	}
	else {
		sd->startupEdit->setEnabled(false);
	}
	di->WriteSettings();
}

void SdrOptions::StartupFrequencyChanged()
{
	di->Set(DeviceInterface::UserFrequency, sd->startupEdit->text().toDouble());
	di->WriteSettings();
}

//IQ gain can be changed in real time, without saving
void SdrOptions::IQGainChanged(double i)
{
	di->Set(DeviceInterface::IQGain, i);
	di->WriteSettings();
}

//IQ order can be changed in real time, without saving
void SdrOptions::IQOrderChanged(int i)
{
	di->Set(DeviceInterface::IQOrder, sd->IQSettings->itemData(i).toInt());
	di->WriteSettings();
}

void SdrOptions::BalancePhaseChanged(int v)
{
	//v is an integer, convert to fraction -.500 to +.500
	double newValue = v/1000.0;
	sd->iqBalancePhaseLabel->setText("Phase: " + QString::number(newValue));
	di->Set(DeviceInterface::IQBalancePhase, newValue);

	if (!global->receiver->GetPowerOn())
		return;
	global->receiver->GetIQBalance()->setPhaseFactor(newValue);
	di->WriteSettings();
}

void SdrOptions::BalanceGainChanged(int v)
{
	//v is an integer, convert to fraction -.750 to +1.250
	double newValue = v/1000.0;
	sd->iqBalanceGainLabel->setText("Gain: " + QString::number(newValue));
	di->Set(DeviceInterface::IQBalanceGain, newValue);
	di->WriteSettings();
	//Update in realtime
	if (!global->receiver->GetPowerOn())
		return;
	global->receiver->GetIQBalance()->setGainFactor(newValue);
}

void SdrOptions::BalanceEnabledChanged(bool b)
{
	di->Set(DeviceInterface::IQBalanceEnabled, b);
	di->WriteSettings();
	if (!global->receiver->GetPowerOn())
		return;
	global->receiver->GetIQBalance()->setEnabled(b);
}

void SdrOptions::BalanceReset()
{
	sd->iqBalanceGain->setValue(1000);
	sd->iqBalancePhase->setValue(0);
}

//Note, this should also reset all device settings, just delete all .ini files
void SdrOptions::ResetAllSettings(bool b)
{
	//Confirm
	QMessageBox::StandardButton bt = QMessageBox::question(NULL,
			tr("Confirm Reset"),
			tr("Are you sure you want to reset all settings for all devices")
			);
	if (bt == QMessageBox::Ok || bt == QMessageBox::Yes) {
		//Delete ini files and restart
		if (sdrOptionsDialog != NULL)
			sdrOptionsDialog->close();
		//Disabled
		//emit Restart();
		//This returns the path to the application directory
		//If a Mac application, we have to get back to the bundle directory
		//If a Mac console app, we don't have a bundle and need to stay where we are
		QString path = QCoreApplication::applicationDirPath();
	#ifdef Q_OS_MAC
		// RegExp for /Pebble.app/contents/macos
		int pos = path.lastIndexOf(QRegExp("/.+\\.app/contents/macos$",Qt::CaseInsensitive));
		if (pos > 0)
			path.truncate(pos);
	#endif
		QDir settingsDir(path + "/PebbleData/");
		settingsDir.setNameFilters({"*.ini"});
		QStringList iniList = settingsDir.entryList();
		for (int i=0; i<iniList.count(); i++) {
			if (QFile::remove(settingsDir.absoluteFilePath(iniList[i])))
				qDebug()<<"Deleting "<<iniList[i];
		}
	}
}

