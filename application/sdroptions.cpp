#include "sdroptions.h"
#include "settings.h"
#include "receiver.h"
#include <QMessageBox>

SdrOptions::SdrOptions():QObject()
{
	m_sdrOptionsDialog = NULL;
	m_sd = NULL;
	m_di = NULL;
}

SdrOptions::~SdrOptions()
{
	if (m_sdrOptionsDialog != NULL)
		m_sdrOptionsDialog->hide();

}

//If b is false, close and delete options
void SdrOptions::showSdrOptions(DeviceInterface *_di, bool b)
{
	if (_di == NULL)
		return;

	m_di = _di;
	//Simplified logic from earlier checkin
	//We always delete dialog when closing so input/output devices and settings are always reset
	if (!b) {
		//We are closing dialog
		if (m_sdrOptionsDialog != NULL) {
			m_sdrOptionsDialog->setVisible(b); //hide
			delete m_sdrOptionsDialog;
			m_sdrOptionsDialog = NULL;
		}
		return;
	} else {
		//b is true
		//If we're visible, and asking to be shown again, toggle off
		if (m_sdrOptionsDialog != NULL && m_sdrOptionsDialog->isVisible()) {
			m_sdrOptionsDialog->setVisible(b); //hide
			delete m_sdrOptionsDialog;
			m_sdrOptionsDialog = NULL;
			return;
		}
		//Create new dialog and show
		int cur;
		m_sdrOptionsDialog = new QDialog();
		m_sd = new Ui::SdrOptions();
		m_sd->setupUi(m_sdrOptionsDialog);
		m_sdrOptionsDialog->setWindowTitle("Pebble Settings");

		//Populate device selector
		int deviceCount = m_di->Get(DeviceInterface::PluginNumDevices).toInt();
		for (int i=0; i<deviceCount; i++) {
			m_sd->deviceSelection->addItem(m_di->Get(DeviceInterface::DeviceName,i).toString());
		}
		//Select active device
		m_sd->deviceSelection->setCurrentIndex(global->settings->m_sdrDeviceNumber);
		//And make sure device has same number
		m_di->Set(DeviceInterface::DeviceNumber,global->settings->m_sdrDeviceNumber);
		//And connect so we get changes
		connect(m_sd->deviceSelection,SIGNAL(currentIndexChanged(int)),this,SLOT(deviceSelectionChanged(int)));

		//Read current settings now that we have correct deviceNumber
		m_di->Command(DeviceInterface::CmdReadSettings,0);

		m_sd->startupBox->addItem("Last Frequency",DeviceInterface::LASTFREQ);
		m_sd->startupBox->addItem("Set Frequency", DeviceInterface::SETFREQ);
		m_sd->startupBox->addItem("Device Default", DeviceInterface::DEFAULTFREQ);
		connect(m_sd->startupBox,SIGNAL(currentIndexChanged(int)),this,SLOT(startupChanged(int)));

		connect(m_sd->startupEdit,SIGNAL(editingFinished()),this,SLOT(startupFrequencyChanged()));
		connect(m_sd->iqGain,SIGNAL(valueChanged(double)),this,SLOT(iqGainChanged(double)));

		m_sd->IQSettings->addItem("I/Q (normal)",DeviceInterface::IQ);
		m_sd->IQSettings->addItem("Q/I (swap)",DeviceInterface::QI);
		m_sd->IQSettings->addItem("I Only",DeviceInterface::IONLY);
		m_sd->IQSettings->addItem("Q Only",DeviceInterface::QONLY);
		connect(m_sd->IQSettings,SIGNAL(currentIndexChanged(int)),this,SLOT(iqOrderChanged(int)));

		m_sd->iqBalancePhase->setMaximum(500);
		m_sd->iqBalancePhase->setMinimum(-500);
		connect(m_sd->iqBalancePhase,SIGNAL(valueChanged(int)),this,SLOT(balancePhaseChanged(int)));

		m_sd->iqBalanceGain->setMaximum(1250);
		m_sd->iqBalanceGain->setMinimum(750);
		connect(m_sd->iqBalanceGain,SIGNAL(valueChanged(int)),this,SLOT(balanceGainChanged(int)));

		connect(m_sd->iqEnableBalance,SIGNAL(toggled(bool)),this,SLOT(balanceEnabledChanged(bool)));
		connect(m_sd->iqBalanceReset,SIGNAL(clicked()),this,SLOT(balanceReset()));

		m_dcRemove = m_di->Get(DeviceInterface::RemoveDC).toBool();
		m_sd->dcRemoval->setChecked(m_dcRemove);
		connect (m_sd->dcRemoval, SIGNAL(clicked(bool)),this,SLOT(dcRemovalChanged(bool)));

		connect(m_sd->sampleRateBox,SIGNAL(currentIndexChanged(int)),this,SLOT(sampleRateChanged(int)));

		connect(m_sd->closeButton,SIGNAL(clicked(bool)),this,SLOT(closeOptions(bool)));
		connect(m_sd->resetButton,SIGNAL(clicked(bool)),this,SLOT(resetSettings(bool)));

		connect(m_sd->sourceBox,SIGNAL(currentIndexChanged(int)),this,SLOT(inputChanged(int)));
		connect(m_sd->outputBox,SIGNAL(currentIndexChanged(int)),this,SLOT(outputChanged(int)));

		connect(m_sd->converterMode,SIGNAL(clicked(bool)),this,SLOT(converterModeChanged(bool)));
		connect(m_sd->converterOffset,SIGNAL(editingFinished()),this,SLOT(converterOffsetChanged()));

		//From here on, everything can change if different device is selected
		updateOptions();

		m_sdrOptionsDialog->show();

	}
}

//Called when we need to update an open options dialog, like when different device is selected
void SdrOptions::updateOptions()
{
	int id;
	QString dn;
	if (m_di->Get(DeviceInterface::DeviceType).toInt() == DeviceInterface::AUDIO_IQ_DEVICE) {
		//Audio devices may have been plugged or unplugged, refresh list on each show
		//This will use PortAudio or QTAudio depending on configuration
		m_inputDevices = Audio::InputDeviceList();
		//Adding items triggers selection, block signals until list is complete
		m_sd->sourceBox->blockSignals(true);
		m_sd->sourceBox->clear();
		//Input devices may be restricted form some SDRs
		for (int i=0; i<m_inputDevices.count(); i++)
		{
			//This is portAudio specific format ID:Name
			//AudioQt will emulate
			id = m_inputDevices[i].left(2).toInt();
			dn = m_inputDevices[i].mid(3);
			m_sd->sourceBox->addItem(dn, id);
			if (dn == m_di->Get(DeviceInterface::InputDeviceName).toString())
				m_sd->sourceBox->setCurrentIndex(i);
		}
		m_sd->sourceBox->blockSignals(false);
	} else {
		m_sd->sourceLabel->setVisible(false);
		m_sd->sourceBox->setVisible(false);
	}
	m_outputDevices = Audio::OutputDeviceList();
	m_sd->outputBox->blockSignals(true);
	m_sd->outputBox->clear();
	for (int i=0; i<m_outputDevices.count(); i++)
	{
		id = m_outputDevices[i].left(2).toInt();
		dn = m_outputDevices[i].mid(3);
		m_sd->outputBox->addItem(dn, id);
		if (dn == m_di->Get(DeviceInterface::OutputDeviceName).toString())
			m_sd->outputBox->setCurrentIndex(i);
	}
	m_sd->outputBox->blockSignals(false);

	m_sd->startupBox->blockSignals(true);
	int cur = m_sd->startupBox->findData(m_di->Get(DeviceInterface::StartupType).toInt());
	m_sd->startupBox->setCurrentIndex(cur);
	m_sd->startupBox->blockSignals(false);

	m_sd->startupEdit->setText(m_di->Get(DeviceInterface::UserFrequency).toString());

	m_sd->iqGain->blockSignals(true);
	m_sd->iqGain->setValue(m_di->Get(DeviceInterface::IQGain).toDouble());
	m_sd->iqGain->blockSignals(false);

	m_sd->IQSettings->blockSignals(true);
	m_sd->IQSettings->setCurrentIndex(m_di->Get(DeviceInterface::IQOrder).toInt());
	m_sd->IQSettings->blockSignals(false);

	m_sd->iqEnableBalance->setChecked(m_di->Get(DeviceInterface::IQBalanceEnabled).toBool());

	m_sd->iqBalancePhase->blockSignals(true);
	m_sd->iqBalancePhase->setValue(m_di->Get(DeviceInterface::IQBalancePhase).toDouble() * 1000);
	m_sd->iqBalancePhase->blockSignals(false);

	m_sd->iqBalanceGain->blockSignals(true);
	m_sd->iqBalanceGain->setValue(m_di->Get(DeviceInterface::IQBalanceGain).toDouble() * 1000);
	m_sd->iqBalanceGain->blockSignals(false);

	m_sd->converterMode->blockSignals(true);
	m_sd->converterMode->setChecked(m_di->Get(DeviceInterface::ConverterMode).toBool());
	m_sd->converterMode->blockSignals(false);

	m_sd->converterOffset->blockSignals(true);
	m_sd->converterOffset->setText(m_di->Get(DeviceInterface::ConverterOffset).toString());
	m_sd->converterOffset->blockSignals(false);

	//Set up options and get allowable sampleRates from device

	int sr;
	QStringList sampleRates = m_di->Get(DeviceInterface::DeviceSampleRates).toStringList();
	if (sampleRates.isEmpty()) {
		//Device handles sample rate selection
		m_sd->sampleRateBox->hide();
		m_sd->sampleRateLabel->hide();
	} else {
		m_sd->sampleRateBox->show();
		m_sd->sampleRateLabel->show();

		m_sd->sampleRateBox->blockSignals(true);
		m_sd->sampleRateBox->clear();
		for (int i=0; i<sampleRates.count(); i++) {
			sr = sampleRates[i].toInt();
			m_sd->sampleRateBox->addItem(sampleRates[i],sr);
			if (m_di->Get(DeviceInterface::DeviceSampleRate).toUInt() == sr)
				m_sd->sampleRateBox->setCurrentIndex(i);
		}
		m_sd->sampleRateBox->blockSignals(false);
	}
	//Careful here: Fragile coding practice
	//We're calling a virtual function in a base class method and expect it to call the over-ridden method in derived class
	//This only works because ShowSdrOptions() is being called via a pointer to the derived class
	//Some other method that's called from the base class could not do this
	//di->SetupOptionUi(sd->sdrSpecific);
	m_di->Command(DeviceInterface::CmdDisplayOptionUi,QVariant::fromValue(m_sd->sdrSpecific));

}

void SdrOptions::closeOptions(bool b)
{
	if (m_sdrOptionsDialog != NULL)
		m_sdrOptionsDialog->close();
}

void SdrOptions::converterModeChanged(bool b)
{
	m_di->Set(DeviceInterface::ConverterMode,b);
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::converterOffsetChanged()
{
	m_di->Set(DeviceInterface::ConverterOffset, m_sd->converterOffset->text().toDouble());
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::dcRemovalChanged(bool b)
{
	m_dcRemove = b;
	m_di->Set(DeviceInterface::RemoveDC,b);
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
	if (!global->receiver->getPowerOn())
		return;
	global->receiver->m_dcRemove->enableStep(b);

}

void SdrOptions::deviceSelectionChanged(int i) {
	global->settings->m_sdrDeviceNumber = i;
	//Set device number
	m_di->Set(DeviceInterface::DeviceNumber, i); //Which ini file to read from
	//Read settings, ReadSettings will switch on deviceNumber
	m_di->Command(DeviceInterface::CmdReadSettings,0);
	//ReadSettings will read in last device number and overwrite our change, so reset it again
	m_di->Set(DeviceInterface::DeviceNumber, i); //Which ini file to read from
	//Update sdr specific section
	updateOptions();
}

void SdrOptions::sampleRateChanged(int i)
{
	int cur = m_sd->sampleRateBox->currentIndex();
	m_di->Set(DeviceInterface::DeviceSampleRate, m_sd->sampleRateBox->itemText(cur).toInt());
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::inputChanged(int i)
{
	int cur = m_sd->sourceBox->currentIndex();
	m_di->Set(DeviceInterface::InputDeviceName, m_sd->sourceBox->itemText(cur));
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::outputChanged(int i)
{
	int cur = m_sd->outputBox->currentIndex();
	m_di->Set(DeviceInterface::OutputDeviceName, m_sd->outputBox->itemText(cur));
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::startupChanged(int i)
{
	m_di->Set(DeviceInterface::StartupType, m_sd->startupBox->itemData(i).toInt());
	m_sd->startupEdit->setText(m_di->Get(DeviceInterface::UserFrequency).toString());
	if (m_di->Get(DeviceInterface::StartupType).toInt() == DeviceInterface::SETFREQ) {
		m_sd->startupEdit->setEnabled(true);
	}
	else {
		m_sd->startupEdit->setEnabled(false);
	}
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::startupFrequencyChanged()
{
	m_di->Set(DeviceInterface::UserFrequency, m_sd->startupEdit->text().toDouble());
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

//IQ gain can be changed in real time, without saving
void SdrOptions::iqGainChanged(double i)
{
	m_di->Set(DeviceInterface::IQGain, i);
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

//IQ order can be changed in real time, without saving
void SdrOptions::iqOrderChanged(int i)
{
	m_di->Set(DeviceInterface::IQOrder, m_sd->IQSettings->itemData(i).toInt());
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::balancePhaseChanged(int v)
{
	//v is an integer, convert to fraction -.500 to +.500
	double newValue = v/1000.0;
	m_sd->iqBalancePhaseLabel->setText("Phase: " + QString::number(newValue));
	m_di->Set(DeviceInterface::IQBalancePhase, newValue);

	if (!global->receiver->getPowerOn())
		return;
	global->receiver->getIQBalance()->setPhaseFactor(newValue);
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
}

void SdrOptions::balanceGainChanged(int v)
{
	//v is an integer, convert to fraction -.750 to +1.250
	double newValue = v/1000.0;
	m_sd->iqBalanceGainLabel->setText("Gain: " + QString::number(newValue));
	m_di->Set(DeviceInterface::IQBalanceGain, newValue);
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
	//Update in realtime
	if (!global->receiver->getPowerOn())
		return;
	global->receiver->getIQBalance()->setGainFactor(newValue);
}

void SdrOptions::balanceEnabledChanged(bool b)
{
	m_di->Set(DeviceInterface::IQBalanceEnabled, b);
	m_di->Command(DeviceInterface::CmdWriteSettings,0);
	if (!global->receiver->getPowerOn())
		return;
	global->receiver->getIQBalance()->enableStep(b);
}

void SdrOptions::balanceReset()
{
	m_sd->iqBalanceGain->setValue(1000);
	m_sd->iqBalancePhase->setValue(0);
}

//Note, this should also reset all device settings, just delete all .ini files
void SdrOptions::resetSettings(bool b)
{
	//Confirm
	QMessageBox::StandardButton bt = QMessageBox::question(NULL,
			tr("Confirm Reset"),
			tr("Are you sure you want to reset the settings for this device?")
			);
	if (bt == QMessageBox::Ok || bt == QMessageBox::Yes) {
		//Delete ini files and restart
		if (m_sdrOptionsDialog != NULL)
			m_sdrOptionsDialog->close();
		//Disabled
		emit restart(); //Shut receiver off so current options aren't written to ini file
		//fname gets absolute path
		QString fname = m_di->Get(DeviceInterface::SettingsFile).toString();
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

