#include "sdroptions.h"
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
		di->Command(DeviceInterface::CmdReadSettings,0);

		sd->startupBox->addItem("Last Frequency",DeviceInterface::LASTFREQ);
		sd->startupBox->addItem("Set Frequency", DeviceInterface::SETFREQ);
		sd->startupBox->addItem("Device Default", DeviceInterface::DEFAULTFREQ);
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

		dcRemove = di->Get(DeviceInterface::RemoveDC).toBool();
		sd->dcRemoval->setChecked(dcRemove);
		connect (sd->dcRemoval, SIGNAL(clicked(bool)),this,SLOT(dcRemovalChanged(bool)));

		connect(sd->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

		connect(sd->closeButton,SIGNAL(clicked(bool)),this,SLOT(CloseOptions(bool)));
		connect(sd->resetButton,SIGNAL(clicked(bool)),this,SLOT(ResetSettings(bool)));

		connect(sd->sourceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(InputChanged(int)));
		connect(sd->outputBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OutputChanged(int)));

		connect(sd->converterMode,SIGNAL(clicked(bool)),this,SLOT(ConverterModeChanged(bool)));
		connect(sd->converterOffset,SIGNAL(editingFinished()),this,SLOT(ConverterOffsetChanged()));

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
	if (di->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ_DEVICE) {
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

	sd->converterMode->blockSignals(true);
	sd->converterMode->setChecked(di->Get(DeviceInterface::ConverterMode).toBool());
	sd->converterMode->blockSignals(false);

	sd->converterOffset->blockSignals(true);
	sd->converterOffset->setText(di->Get(DeviceInterface::ConverterOffset).toString());
	sd->converterOffset->blockSignals(false);

	//Set up options and get allowable sampleRates from device

	int sr;
	QStringList sampleRates = di->Get(DeviceInterface::DeviceSampleRates).toStringList();
	if (sampleRates.isEmpty()) {
		//Device handles sample rate selection
		sd->sampleRateBox->hide();
		sd->sampleRateLabel->hide();
	} else {
		sd->sampleRateBox->show();
		sd->sampleRateLabel->show();

		sd->sampleRateBox->blockSignals(true);
		sd->sampleRateBox->clear();
		for (int i=0; i<sampleRates.count(); i++) {
			sr = sampleRates[i].toInt();
			sd->sampleRateBox->addItem(sampleRates[i],sr);
			if (di->Get(DeviceInterface::DeviceSampleRate).toUInt() == sr)
				sd->sampleRateBox->setCurrentIndex(i);
		}
		sd->sampleRateBox->blockSignals(false);
	}
	//Careful here: Fragile coding practice
	//We're calling a virtual function in a base class method and expect it to call the over-ridden method in derived class
	//This only works because ShowSdrOptions() is being called via a pointer to the derived class
	//Some other method that's called from the base class could not do this
	//di->SetupOptionUi(sd->sdrSpecific);
	di->Command(DeviceInterface::CmdDisplayOptionUi,QVariant::fromValue(sd->sdrSpecific));

}

void SdrOptions::CloseOptions(bool b)
{
	if (sdrOptionsDialog != NULL)
		sdrOptionsDialog->close();
}

void SdrOptions::ConverterModeChanged(bool b)
{
	di->Set(DeviceInterface::ConverterMode,b);
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::ConverterOffsetChanged()
{
	di->Set(DeviceInterface::ConverterOffset, sd->converterOffset->text().toDouble());
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::dcRemovalChanged(bool b)
{
	dcRemove = b;
	di->Set(DeviceInterface::RemoveDC,b);
	di->Command(DeviceInterface::CmdWriteSettings,0);
	if (!global->receiver->getPowerOn())
		return;
	global->receiver->dcRemove->enableStep(b);

}

void SdrOptions::DeviceSelectionChanged(int i) {
	global->settings->sdrDeviceNumber = i;
	//Set device number
	di->Set(DeviceInterface::DeviceNumber, i); //Which ini file to read from
	//Read settings, ReadSettings will switch on deviceNumber
	di->Command(DeviceInterface::CmdReadSettings,0);
	//ReadSettings will read in last device number and overwrite our change, so reset it again
	di->Set(DeviceInterface::DeviceNumber, i); //Which ini file to read from
	//Update sdr specific section
	UpdateOptions();
}

void SdrOptions::SampleRateChanged(int i)
{
	int cur = sd->sampleRateBox->currentIndex();
	di->Set(DeviceInterface::DeviceSampleRate, sd->sampleRateBox->itemText(cur).toInt());
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::InputChanged(int i)
{
	int cur = sd->sourceBox->currentIndex();
	di->Set(DeviceInterface::InputDeviceName, sd->sourceBox->itemText(cur));
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::OutputChanged(int i)
{
	int cur = sd->outputBox->currentIndex();
	di->Set(DeviceInterface::OutputDeviceName, sd->outputBox->itemText(cur));
	di->Command(DeviceInterface::CmdWriteSettings,0);
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
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::StartupFrequencyChanged()
{
	di->Set(DeviceInterface::UserFrequency, sd->startupEdit->text().toDouble());
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

//IQ gain can be changed in real time, without saving
void SdrOptions::IQGainChanged(double i)
{
	di->Set(DeviceInterface::IQGain, i);
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

//IQ order can be changed in real time, without saving
void SdrOptions::IQOrderChanged(int i)
{
	di->Set(DeviceInterface::IQOrder, sd->IQSettings->itemData(i).toInt());
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::BalancePhaseChanged(int v)
{
	//v is an integer, convert to fraction -.500 to +.500
	double newValue = v/1000.0;
	sd->iqBalancePhaseLabel->setText("Phase: " + QString::number(newValue));
	di->Set(DeviceInterface::IQBalancePhase, newValue);

	if (!global->receiver->getPowerOn())
		return;
	global->receiver->getIQBalance()->setPhaseFactor(newValue);
	di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::BalanceGainChanged(int v)
{
	//v is an integer, convert to fraction -.750 to +1.250
	double newValue = v/1000.0;
	sd->iqBalanceGainLabel->setText("Gain: " + QString::number(newValue));
	di->Set(DeviceInterface::IQBalanceGain, newValue);
	di->Command(DeviceInterface::CmdWriteSettings,0);
	//Update in realtime
	if (!global->receiver->getPowerOn())
		return;
	global->receiver->getIQBalance()->setGainFactor(newValue);
}

void SdrOptions::BalanceEnabledChanged(bool b)
{
	di->Set(DeviceInterface::IQBalanceEnabled, b);
	di->Command(DeviceInterface::CmdWriteSettings,0);
	if (!global->receiver->getPowerOn())
		return;
	global->receiver->getIQBalance()->enableStep(b);
}

void SdrOptions::BalanceReset()
{
	sd->iqBalanceGain->setValue(1000);
	sd->iqBalancePhase->setValue(0);
}

//Note, this should also reset all device settings, just delete all .ini files
void SdrOptions::ResetSettings(bool b)
{
	//Confirm
	QMessageBox::StandardButton bt = QMessageBox::question(NULL,
			tr("Confirm Reset"),
			tr("Are you sure you want to reset the settings for this device?")
			);
	if (bt == QMessageBox::Ok || bt == QMessageBox::Yes) {
		//Delete ini files and restart
		if (sdrOptionsDialog != NULL)
			sdrOptionsDialog->close();
		//Disabled
		emit Restart(); //Shut receiver off so current options aren't written to ini file
		//fname gets absolute path
		QString fname = di->Get(DeviceInterface::SettingsFile).toString();
		qDebug()<<"Device file name: "<<fname;
		QFile::remove(fname);
#if 0
		QDir settingsDir(global->pebbleDataPath);
		settingsDir.setNameFilters({"*.ini"});
		QStringList iniList = settingsDir.entryList();
		for (int i=0; i<iniList.count(); i++) {
			if (QFile::remove(settingsDir.absoluteFilePath(iniList[i])))
				qDebug()<<"Deleting "<<iniList[i];
		}
#endif
	}
}

