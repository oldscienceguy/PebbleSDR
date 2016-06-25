#include "morsegendevice.h"
#include "db.h"
#include <QFileDialog>

//Plugin constructors are called indirectly when the plugin is loaded in Receiver
//Be careful not to access objects that are not initialized yet, do that in Initialize()
MorseGenDevice::MorseGenDevice():DeviceInterfaceBase()
{
	initSettings("morsegen");
	m_optionUi = NULL;
	m_running = false;
}

//Called when the plugins object is deleted in the ~Receiver()
//Be careful not to access objects that may already be destroyed
MorseGenDevice::~MorseGenDevice()
{

}

bool MorseGenDevice::initialize(CB_ProcessIQData _callback,
								  CB_ProcessBandscopeData _callbackBandscope,
								  CB_ProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);

	quint32 tcwMs = MorseCode::wpmToTcwMs(20);
	quint32 maxSymbolMs = MorseCode::c_maxTcwPerSymbol * tcwMs;
	quint32 outBufLen = maxSymbolMs / (1000.0/m_deviceSampleRate);
	m_outBuf = memalign(outBufLen);
	m_outBuf1 = memalign(outBufLen);
	m_outBuf2 = memalign(outBufLen);

	//Sample rate must be same for all gen and same as wav file
	m_morseGen1 = new MorseGen(m_deviceSampleRate);
	m_morseGen2 = new MorseGen(m_deviceSampleRate);
	m_morseGen3 = new MorseGen(m_deviceSampleRate);
	m_morseGen4 = new MorseGen(m_deviceSampleRate);
	m_morseGen5 = new MorseGen(m_deviceSampleRate);

	//Build text for all characters
	MorseSymbol *symbol;
	m_sampleText[ST_TABLE] = "=";
	for (quint32 i=0; i<MorseCode::c_morseTableSize; i++) {
		symbol = &MorseCode::m_morseTable[i];
		if (symbol->ascii == 0x00) {
			break; //End of table
		}
		m_sampleText[ST_TABLE].append(symbol->ascii);
	}
	m_sampleText[ST_TABLE].append("=");

	m_numProducerBuffers = 50;
	m_producerFreeBufPtr = NULL;
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(double);
	m_readBufferSize = m_framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	m_producerConsumer.Initialize(std::bind(&MorseGenDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&MorseGenDevice::consumerWorker, this, std::placeholders::_1),m_numProducerBuffers, m_readBufferSize);
	//Must be called after Initialize
	//m_producerConsumer.SetProducerInterval(m_deviceSampleRate,m_framesPerBuffer);
	//m_producerConsumer.SetConsumerInterval(m_deviceSampleRate,m_framesPerBuffer);

	return true;
}

void MorseGenDevice::readGenSettings(quint32 genNum, GenSettings *gs)
{
	//Some variation in initial default values
	GenSettings gsd; //Default values
	switch (genNum) {
		case 1: gsd = m_gs1Default; break;
		case 2: gsd = m_gs2Default; break;
		case 3: gsd = m_gs3Default; break;
		case 4: gsd = m_gs4Default; break;
		case 5: gsd = m_gs5Default; break;
		default: break;
	}

	QString group = "Gen" + QString::number(genNum);

	m_settings->beginGroup(group);
	gs->enabled = m_settings->value("Enabled", gsd.enabled).toBool();
	gs->text = m_settings->value("Text", gsd.text).toUInt();
	gs->fileName = m_settings->value("FileName",pebbleLibGlobal->appDirPath+"/PebbleData/300USAqso-1.txt").toString();
	gs->freq = m_settings->value("Freq", gsd.freq).toDouble();
	gs->amp = m_settings->value("Amp", gsd.amp).toDouble();
	gs->wpm = m_settings->value("Wpm", gsd.wpm).toUInt();
	gs->rise = m_settings->value("Rise", gsd.rise).toUInt();
	gs->fade = m_settings->value("Fade", gsd.fade).toBool();
	gs->fist = m_settings->value("Fist", gsd.fist).toBool();
	m_settings->endGroup();
}

void MorseGenDevice::readSettings()
{
	// +/- db gain required to normalize to fixed level input
	// Default is 0db gain, or a factor of 1.0
	m_normalizeIQGain = DB::dBToAmplitude(0);
	m_startupDemodMode = DemodMode::dmCWL;
	//Set defaults before calling DeviceInterfaceBase
	DeviceInterfaceBase::readSettings();

	m_dbNoiseAmp = m_settings->value("DbNoiseAmp",-60).toDouble();

	readGenSettings(1,&m_gs1);
	readGenSettings(2,&m_gs2);
	readGenSettings(3,&m_gs3);
	readGenSettings(4,&m_gs4);
	readGenSettings(5,&m_gs5);

	//Preset 1
	m_preset1Name = m_settings->value("Preset1Name","Preset 1").toString();
	m_settings->beginGroup("Preset1");
	readGenSettings(1,&m_preset1[0]);
	readGenSettings(2,&m_preset1[1]);
	readGenSettings(3,&m_preset1[2]);
	readGenSettings(4,&m_preset1[3]);
	readGenSettings(5,&m_preset1[4]);
	m_settings->endGroup();

	m_preset2Name = m_settings->value("Preset2Name","Preset 2").toString();
	m_settings->beginGroup("Preset2");
	readGenSettings(1,&m_preset2[0]);
	readGenSettings(2,&m_preset2[1]);
	readGenSettings(3,&m_preset2[2]);
	readGenSettings(4,&m_preset2[3]);
	readGenSettings(5,&m_preset2[4]);
	m_settings->endGroup();

	m_preset3Name = m_settings->value("Preset3Name","Preset 3").toString();
	m_settings->beginGroup("Preset3");
	readGenSettings(1,&m_preset3[0]);
	readGenSettings(2,&m_preset3[1]);
	readGenSettings(3,&m_preset3[2]);
	readGenSettings(4,&m_preset3[3]);
	readGenSettings(5,&m_preset3[4]);
	m_settings->endGroup();

	m_preset4Name = m_settings->value("Preset4Name","Preset 4").toString();
	m_settings->beginGroup("Preset4");
	readGenSettings(1,&m_preset4[0]);
	readGenSettings(2,&m_preset4[1]);
	readGenSettings(3,&m_preset4[2]);
	readGenSettings(4,&m_preset4[3]);
	readGenSettings(5,&m_preset4[4]);
	m_settings->endGroup();

	m_preset5Name = m_settings->value("Preset5Name","Preset 5").toString();
	m_settings->beginGroup("Preset5");
	readGenSettings(1,&m_preset5[0]);
	readGenSettings(2,&m_preset5[1]);
	readGenSettings(3,&m_preset5[2]);
	readGenSettings(4,&m_preset5[3]);
	readGenSettings(5,&m_preset5[4]);
	m_settings->endGroup();

	//Text output, read only settings
	//Trailing '=' is morse <BT> for new paragraph
	QString str1 = "\nThe quick brown fox jumped over the lazy dog 1,2,3,4,5,6,7,8,9,0 times.";
	QString str2 = "\nNow is the time for all good men to come to the aid of their country.";



	m_sampleText[ST_SAMPLE1] = m_settings->value("Sample1",str1).toString();
	m_sampleText[ST_SAMPLE2] = m_settings->value("Sample2",str2).toString();
	m_sampleText[ST_SAMPLE3] = m_settings->value("Sample3",str2).toString();
	m_sampleText[ST_FILE] = "No longer used";
}

void MorseGenDevice::writeGenSettings(quint32 genNum, GenSettings *gs)
{
	QString group = "Gen"+QString::number(genNum);
	m_settings->beginGroup(group);
	m_settings->setValue("Enabled", gs->enabled);
	m_settings->setValue("Text",gs->text);
	m_settings->setValue("FileName",gs->fileName);
	m_settings->setValue("Freq",gs->freq);
	m_settings->setValue("Amp",gs->amp);
	m_settings->setValue("Wpm",gs->wpm);
	m_settings->setValue("Rise",gs->rise);
	m_settings->setValue("Fade",gs->fade);
	m_settings->setValue("Fist",gs->fist);
	m_settings->endGroup();
}

void MorseGenDevice::writeSettings()
{
	DeviceInterfaceBase::writeSettings();

	m_settings->setValue("DbNoiseAmp", m_dbNoiseAmp);
	writeGenSettings(1, &m_gs1);
	writeGenSettings(2, &m_gs2);
	writeGenSettings(3, &m_gs3);
	writeGenSettings(4, &m_gs4);
	writeGenSettings(5, &m_gs5);

	m_settings->setValue("Preset1Name",m_preset1Name);
	m_settings->beginGroup("Preset1");
	writeGenSettings(1,&m_preset1[0]);
	writeGenSettings(2,&m_preset1[1]);
	writeGenSettings(3,&m_preset1[2]);
	writeGenSettings(4,&m_preset1[3]);
	writeGenSettings(5,&m_preset1[4]);
	m_settings->endGroup();

	m_settings->setValue("Preset2Name",m_preset2Name);
	m_settings->beginGroup("Preset2");
	writeGenSettings(1,&m_preset2[0]);
	writeGenSettings(2,&m_preset2[1]);
	writeGenSettings(3,&m_preset2[2]);
	writeGenSettings(4,&m_preset2[3]);
	writeGenSettings(5,&m_preset2[4]);
	m_settings->endGroup();

	m_settings->setValue("Preset3Name",m_preset3Name);
	m_settings->beginGroup("Preset3");
	writeGenSettings(1,&m_preset3[0]);
	writeGenSettings(2,&m_preset3[1]);
	writeGenSettings(3,&m_preset3[2]);
	writeGenSettings(4,&m_preset3[3]);
	writeGenSettings(5,&m_preset3[4]);
	m_settings->endGroup();

	m_settings->setValue("Preset4Name",m_preset4Name);
	m_settings->beginGroup("Preset4");
	writeGenSettings(1,&m_preset4[0]);
	writeGenSettings(2,&m_preset4[1]);
	writeGenSettings(3,&m_preset4[2]);
	writeGenSettings(4,&m_preset4[3]);
	writeGenSettings(5,&m_preset4[4]);
	m_settings->endGroup();

	m_settings->setValue("Preset5Name",m_preset5Name);
	m_settings->beginGroup("Preset5");
	writeGenSettings(1,&m_preset5[0]);
	writeGenSettings(2,&m_preset5[1]);
	writeGenSettings(3,&m_preset5[2]);
	writeGenSettings(4,&m_preset5[3]);
	writeGenSettings(5,&m_preset5[4]);
	m_settings->endGroup();

	m_settings->setValue("Sample1",m_sampleText[ST_SAMPLE1]);
	m_settings->setValue("Sample2",m_sampleText[ST_SAMPLE2]);
	m_settings->setValue("Sample3",m_sampleText[ST_SAMPLE3]);

	m_settings->setValue("MorseFileName",m_morseFileName);
}

void MorseGenDevice::updateGenerators()
{
	if (!m_running)
		return;
	m_morseGen1->setParams(m_gs1.freq,m_gs1.amp,m_gs1.wpm,m_gs1.rise);
	m_morseGen1->setTextOut(getSampleText(m_gs1));
	m_morseGen2->setParams(m_gs2.freq,m_gs2.amp,m_gs2.wpm,m_gs2.rise);
	m_morseGen2->setTextOut(getSampleText(m_gs2));
	m_morseGen3->setParams(m_gs3.freq,m_gs3.amp,m_gs3.wpm,m_gs3.rise);
	m_morseGen3->setTextOut(getSampleText(m_gs3));
	m_morseGen4->setParams(m_gs4.freq,m_gs4.amp,m_gs4.wpm,m_gs4.rise);
	m_morseGen4->setTextOut(getSampleText(m_gs4));
	m_morseGen5->setParams(m_gs5.freq,m_gs5.amp,m_gs5.wpm,m_gs5.rise);
	m_morseGen5->setTextOut(getSampleText(m_gs5));
	//Because noise is averaged, fudget +30db so it matches with generator values
	//ie -30db gen and -30db noise should be 0snr
	m_noiseAmp = DB::dBToAmplitude(m_dbNoiseAmp+20);

}

//Returns a string based on settings and filename
QString MorseGenDevice::getSampleText(GenSettings gs)
{
	switch(gs.text) {
		case ST_SAMPLE1: //Sample1 text from ini file
			return m_sampleText[ST_SAMPLE1];
			break;
		case ST_SAMPLE2: //Sample2 text from ini file
			return m_sampleText[ST_SAMPLE2];
			break;
		case ST_SAMPLE3: //Sample3 text from ini file
			return m_sampleText[ST_SAMPLE3];
			break;
		case ST_FILE: {
			QString text;
			text = "\n";
			QFile file(gs.fileName);
			 if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				 QTextStream in(&file);
				 QString line;
				 while (!in.atEnd()) {
					 line = in.readLine();
					 text.append(line);
					 text.append('\n');
				 }
				 file.close();
			 } else {
				 text = "=Error: Morse file not found.";
			 }
			return text;
			}
			break;
		case ST_WORDS:	//Random words from table
			return m_sampleText[ST_WORDS];
			break;
		case ST_ABBREV:	//Random morse abbeviations
			return m_sampleText[ST_ABBREV];
			break;
		case ST_TABLE:	//All characters in morse table
			return m_sampleText[ST_TABLE];
			break;
	}
	return "";
}

void MorseGenDevice::gen1BrowseClicked()
{
	QString pebbleDataPath = pebbleLibGlobal->appDirPath + "/PebbleData/";
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Morse Text File"), pebbleDataPath, tr("Text (*.txt)"));
	if (!fileName.isNull()) {
		QFileInfo info(fileName);
		m_gs1.fileName = fileName;
		m_optionUi->filename_1->setText(info.baseName());
	}
}
void MorseGenDevice::gen2BrowseClicked()
{
	QString pebbleDataPath = pebbleLibGlobal->appDirPath + "/PebbleData/";
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Morse Text File"), pebbleDataPath, tr("Text (*.txt)"));
	if (!fileName.isNull()) {
		QFileInfo info(fileName);
		m_gs2.fileName = fileName;
		m_optionUi->filename_2->setText(info.baseName());
	}
}
void MorseGenDevice::gen3BrowseClicked()
{
	QString pebbleDataPath = pebbleLibGlobal->appDirPath + "/PebbleData/";
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Morse Text File"), pebbleDataPath, tr("Text (*.txt)"));
	if (!fileName.isNull()) {
		QFileInfo info(fileName);
		m_gs3.fileName = fileName;
		m_optionUi->filename_3->setText(info.baseName());
	}
}
void MorseGenDevice::gen4BrowseClicked()
{
	QString pebbleDataPath = pebbleLibGlobal->appDirPath + "/PebbleData/";
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Morse Text File"), pebbleDataPath, tr("Text (*.txt)"));
	if (!fileName.isNull()) {
		QFileInfo info(fileName);
		m_gs4.fileName = fileName;
		m_optionUi->filename_4->setText(info.baseName());
	}
}
void MorseGenDevice::gen5BrowseClicked()
{
	QString pebbleDataPath = pebbleLibGlobal->appDirPath + "/PebbleData/";
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Morse Text File"), pebbleDataPath, tr("Text (*.txt)"));
	if (!fileName.isNull()) {
		QFileInfo info(fileName);
		m_gs5.fileName = fileName;
		m_optionUi->filename_5->setText(info.baseName());
	}
}

void MorseGenDevice::updateAllFields()
{
}
void MorseGenDevice::updateGen1Fields()
{
	m_mutex.lock();
	m_gs1.enabled = m_optionUi->enabledBox_1->isChecked();
	m_gs1.freq = m_optionUi->freqencyEdit_1->text().toDouble();
	m_gs1.amp = m_optionUi->dbBox_1->currentData().toDouble();
	m_gs1.wpm = m_optionUi->wpmBox_1->currentData().toUInt();
	m_gs1.rise = 5; //Add to UI?
	m_gs1.text = m_optionUi->sourceBox_1->currentData().toUInt();
	if (m_running) {
		m_morseGen1->setParams(m_gs1.freq,m_gs1.amp,m_gs1.wpm,m_gs1.rise);
		m_morseGen1->setTextOut(getSampleText(m_gs1));
	}
	m_mutex.unlock();
}
void MorseGenDevice::updateGen2Fields()
{
	m_mutex.lock();
	m_gs2.enabled = m_optionUi->enabledBox_2->isChecked();
	m_gs2.freq = m_optionUi->freqencyEdit_2->text().toDouble();
	m_gs2.amp = m_optionUi->dbBox_2->currentData().toDouble();
	m_gs2.wpm = m_optionUi->wpmBox_2->currentData().toUInt();
	m_gs2.rise = 5; //Add to UI?
	m_gs2.text = m_optionUi->sourceBox_2->currentData().toUInt();
	if (m_running) {
		m_morseGen2->setParams(m_gs2.freq,m_gs2.amp,m_gs2.wpm,m_gs2.rise);
		m_morseGen2->setTextOut(getSampleText(m_gs2));
	}
	m_mutex.unlock();
}
void MorseGenDevice::updateGen3Fields()
{
	m_mutex.lock();
	m_gs3.enabled = m_optionUi->enabledBox_3->isChecked();
	m_gs3.freq = m_optionUi->freqencyEdit_3->text().toDouble();
	m_gs3.amp = m_optionUi->dbBox_3->currentData().toDouble();
	m_gs3.wpm = m_optionUi->wpmBox_3->currentData().toUInt();
	m_gs3.rise = 5; //Add to UI?
	m_gs3.text = m_optionUi->sourceBox_3->currentData().toUInt();
	if (m_running) {
		m_morseGen3->setParams(m_gs3.freq,m_gs3.amp,m_gs3.wpm,m_gs3.rise);
		m_morseGen3->setTextOut(getSampleText(m_gs3));
	}
	m_mutex.unlock();
}
void MorseGenDevice::updateGen4Fields()
{
	m_mutex.lock();
	m_gs4.enabled = m_optionUi->enabledBox_4->isChecked();
	m_gs4.freq = m_optionUi->freqencyEdit_4->text().toDouble();
	m_gs4.amp = m_optionUi->dbBox_4->currentData().toDouble();
	m_gs4.wpm = m_optionUi->wpmBox_4->currentData().toUInt();
	m_gs4.rise = 5; //Add to UI?
	m_gs4.text = m_optionUi->sourceBox_4->currentData().toUInt();
	if (m_running) {
		m_morseGen4->setParams(m_gs4.freq,m_gs4.amp,m_gs4.wpm,m_gs4.rise);
		m_morseGen4->setTextOut(getSampleText(m_gs4));
	}
	m_mutex.unlock();
}
void MorseGenDevice::updateGen5Fields()
{
	m_mutex.lock();
	m_gs5.enabled = m_optionUi->enabledBox_5->isChecked();
	m_gs5.freq = m_optionUi->freqencyEdit_5->text().toDouble();
	m_gs5.amp = m_optionUi->dbBox_5->currentData().toDouble();
	m_gs5.wpm = m_optionUi->wpmBox_5->currentData().toUInt();
	m_gs5.rise = 5; //Add to UI?
	m_gs5.text = m_optionUi->sourceBox_5->currentData().toUInt();
	if (m_running) {
		m_morseGen5->setParams(m_gs5.freq,m_gs5.amp,m_gs5.wpm,m_gs5.rise);
		m_morseGen5->setTextOut(getSampleText(m_gs5));
	}
	m_mutex.unlock();
}
void MorseGenDevice::updateNoiseFields()
{
	m_mutex.lock();
	m_dbNoiseAmp = m_optionUi->noiseBox->currentData().toDouble();
	//Because noise is averaged, fudget +30db so it matches with generator values
	//ie -30db gen and -30db noise should be 0snr
	m_noiseAmp = DB::dBToAmplitude(m_dbNoiseAmp+20);
	m_mutex.unlock();
}

void MorseGenDevice::updatePresetName()
{
	quint32 index = m_optionUi->presetsBox->currentIndex();
	switch (index) {
		case 0:
			m_preset1Name = m_optionUi->presetsBox->currentText();
			m_optionUi->presetsBox->setItemText(index,m_preset1Name);
			break;
		case 1:
			m_preset2Name = m_optionUi->presetsBox->currentText();
			m_optionUi->presetsBox->setItemText(index,m_preset2Name);
			break;
		case 2:
			m_preset3Name = m_optionUi->presetsBox->currentText();
			m_optionUi->presetsBox->setItemText(index,m_preset3Name);
			break;
		case 3:
			m_preset4Name = m_optionUi->presetsBox->currentText();
			m_optionUi->presetsBox->setItemText(index,m_preset4Name);
			break;
		case 4:
			m_preset5Name = m_optionUi->presetsBox->currentText();
			m_optionUi->presetsBox->setItemText(index,m_preset5Name);
			break;
		default: break;
	}
}

bool MorseGenDevice::command(DeviceInterface::StandardCommands _cmd, QVariant _arg)
{
	quint32 randomIndex;

	switch (_cmd) {
		case Cmd_Connect:
			DeviceInterfaceBase::connectDevice();
			//Device specific code follows
			return true;

		case Cmd_Disconnect:
			DeviceInterfaceBase::disconnectDevice();
			//Device specific code follows
			return true;

		case Cmd_Start:
			DeviceInterfaceBase::startDevice();
			//Device specific code follows
			//Update random word option on each start
			m_sampleText[ST_WORDS] = "="; //CR
			for (quint32 i=0; i<MorseCode::c_commonWordsSize; i++) {
				randomIndex = rand() % MorseCode::c_commonWordsSize;
				m_sampleText[ST_WORDS].append(MorseCode::c_commonWords[randomIndex]);
				m_sampleText[ST_WORDS].append(" ");
			}
			m_sampleText[ST_WORDS].append("=");

			//Update random abbreviations option on each start
			m_sampleText[ST_ABBREV] = "="; //CR
			for (quint32 i=0; i<MorseCode::c_abbreviationsSize; i++) {
				randomIndex = rand() % MorseCode::c_abbreviationsSize;
				m_sampleText[ST_ABBREV].append(MorseCode::c_abbreviations[randomIndex]);
				m_sampleText[ST_ABBREV].append(" ");
			}
			m_sampleText[ST_ABBREV].append("=");

			m_running = true;
			updateGenerators();

			//How often do we need to read samples from files to get framesPerBuffer at sampleRate
			m_nsPerBuffer = (1000000000.0 / m_deviceSampleRate) * m_framesPerBuffer;
			//qDebug()<<"nsPerBuffer"<<nsPerBuffer;
			m_producerConsumer.Start(true,true);
			m_elapsedTimer.start();
			return true;

		case Cmd_Stop:
			DeviceInterfaceBase::stopDevice();
			m_running = false;
			//Device specific code follows
			m_producerConsumer.Stop();

			return true;

		case Cmd_ReadSettings:
			DeviceInterfaceBase::readSettings();
			//Device specific settings follow
			readSettings();
			return true;

		case Cmd_WriteSettings:
			DeviceInterfaceBase::writeSettings();
			//Device specific settings follow
			writeSettings();
			return true;
		case Cmd_DisplayOptionUi: {
			//Use QVariant::fromValue() to pass, and value<type passed>() to get back
			this->setupOptionUi(_arg.value<QWidget*>());
			return true;
		}
		default:
			return false;
	}
}

QVariant MorseGenDevice::get(DeviceInterface::StandardKeys _key, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_PluginName:
			return "Morse Generator";
			break;
		case Key_PluginDescription:
			return "Generates Morse Code";
			break;
		case Key_DeviceName:
			return "MorseGenDevice";
		case Key_DeviceType:
			return DeviceInterfaceBase::DT_IQ_DEVICE;

		case Key_HighFrequency:
			return m_sampleRate;
		case Key_LowFrequency:
			return 0;
		case Key_StartupFrequency:
			return m_sampleRate/2;
		default:
			return DeviceInterfaceBase::get(_key, _option);
	}
}

bool MorseGenDevice::set(DeviceInterface::StandardKeys _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_DeviceFrequency:
			//Fixed, so return false so it won't change in UI.  Only mixer should work
			return false;

		default:
			return DeviceInterfaceBase::set(_key, _value, _option);
	}
}

void MorseGenDevice::producerWorker(cbProducerConsumerEvents _event)
{
#if 0
	//For verifying device data format min/max so we can normalize later
	static short maxSample = 0;
	static short minSample = 0;
#endif
	timespec req, rem;
	qint64 nsRemaining = m_nsPerBuffer - m_elapsedTimer.nsecsElapsed();

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			//Govern output rate to match sampleRate, same as if actual device was outputing samples
			if (nsRemaining > 0) {
				req.tv_sec = 0;
				//We want to get close to exact time, but not go over
				//So we sleep for 1/2 of the remaining time each sequence
				req.tv_nsec = nsRemaining;
				if (nanosleep(&req,&rem) < 0) {
					qDebug()<<"nanosleep failed";
				}
			}
			m_elapsedTimer.start(); //Restart elapsed timer

			if ((m_producerFreeBufPtr = (CPX*)m_producerConsumer.AcquireFreeBuffer()) == NULL)
				return;
			//Get data from device and put into producerFreeBufPtr
			generate(m_producerFreeBufPtr);
			//Return and wait for next producer time slice
			m_producerConsumer.ReleaseFilledBuffer();
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void MorseGenDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (m_producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((m_consumerFilledBufPtr = (CPX*)m_producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//Process data in filled buffer and convert to Pebble format in consumerBuffer

				//perform.StartPerformance("ProcessIQ");
				processIQData(m_consumerFilledBufPtr,m_framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				m_producerConsumer.ReleaseFreeBuffer();

			}
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void MorseGenDevice::initSourceBox(QComboBox *box)
{
	box->addItem("Sample1",ST_SAMPLE1);
	box->addItem("Sample2",ST_SAMPLE2);
	box->addItem("Sample3",ST_SAMPLE3);
	box->addItem("File", ST_FILE);
	box->addItem("Words",ST_WORDS);
	box->addItem("Abbrev",ST_ABBREV);
	box->addItem("Table",ST_TABLE);
}
void MorseGenDevice::initWpmBox(QComboBox * box)
{
	box->addItem("5", 5);
	box->addItem("10", 10);
	box->addItem("15", 15);
	box->addItem("20", 20);
	box->addItem("30", 30);
	box->addItem("40", 40);
	box->addItem("50", 50);
	box->addItem("60", 60);
	box->addItem("70", 70);
	box->addItem("80", 80);
	box->addItem("100", 100);
	box->addItem("120", 120);
	box->addItem("140", 140);
	box->addItem("160", 160);
	box->addItem("180", 180);
	box->addItem("200", 200);
}
void MorseGenDevice::initDbBox(QComboBox * box)
{
	box->addItem("-30", -30);
	box->addItem("-35", -35);
	box->addItem("-40", -40);
	box->addItem("-45", -45);
	box->addItem("-50", -50);
	box->addItem("-55", -55);
	box->addItem("-60", -60);
}

void MorseGenDevice::setupOptionUi(QWidget *parent)
{
	if (m_optionUi != NULL)
		delete m_optionUi;

	m_optionUi = new Ui::MorseGenOptions();
	m_optionUi->setupUi(parent);
	parent->setVisible(true);

	quint32 index;

	m_optionUi->noiseBox->addItem("-40db", -40);
	m_optionUi->noiseBox->addItem("-45db", -45);
	m_optionUi->noiseBox->addItem("-50db", -50);
	m_optionUi->noiseBox->addItem("-55db", -55);
	m_optionUi->noiseBox->addItem("-60db", -60);
	m_optionUi->noiseBox->addItem("-65db", -65);
	m_optionUi->noiseBox->addItem("-70db", -70);
	m_optionUi->noiseBox->addItem("-90db", -90);
	m_optionUi->noiseBox->addItem("-120db", -120);
	index = m_optionUi->noiseBox->findData(m_dbNoiseAmp);
	m_optionUi->noiseBox->setCurrentIndex(index);
	connect(m_optionUi->noiseBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateNoiseFields()));

	m_optionUi->presetsBox->addItem(m_preset1Name,1);
	m_optionUi->presetsBox->addItem(m_preset2Name,2);
	m_optionUi->presetsBox->addItem(m_preset3Name,3);
	m_optionUi->presetsBox->addItem(m_preset4Name,4);
	m_optionUi->presetsBox->addItem(m_preset5Name,5);
	connect(m_optionUi->presetsBox,SIGNAL(currentTextChanged(QString)),this,SLOT(updatePresetName()));

	initSourceBox(m_optionUi->sourceBox_1);
	initWpmBox(m_optionUi->wpmBox_1);
	initDbBox(m_optionUi->dbBox_1);

	setGen1Ui(m_gs1);

	connect(m_optionUi->enabledBox_1,SIGNAL(clicked(bool)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->sourceBox_1,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->browseButton_1,SIGNAL(clicked(bool)),this,SLOT(gen1BrowseClicked()));
	connect(m_optionUi->freqencyEdit_1,SIGNAL(textChanged(QString)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->wpmBox_1,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->dbBox_1,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->fadeBox_1,SIGNAL(clicked(bool)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->fistBox_1,SIGNAL(clicked(bool)),this,SLOT(updateGen1Fields()));

	//2
	initSourceBox(m_optionUi->sourceBox_2);
	initWpmBox(m_optionUi->wpmBox_2);
	initDbBox(m_optionUi->dbBox_2);

	setGen2Ui(m_gs2);

	connect(m_optionUi->enabledBox_2,SIGNAL(clicked(bool)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->sourceBox_2,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->browseButton_2,SIGNAL(clicked(bool)),this,SLOT(gen2BrowseClicked()));
	connect(m_optionUi->freqencyEdit_2,SIGNAL(textChanged(QString)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->wpmBox_2,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->dbBox_2,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->fadeBox_2,SIGNAL(clicked(bool)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->fistBox_2,SIGNAL(clicked(bool)),this,SLOT(updateGen2Fields()));

	//3
	initSourceBox(m_optionUi->sourceBox_3);
	initWpmBox(m_optionUi->wpmBox_3);
	initDbBox(m_optionUi->dbBox_3);

	setGen3Ui(m_gs3);

	connect(m_optionUi->enabledBox_3,SIGNAL(clicked(bool)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->sourceBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->browseButton_3,SIGNAL(clicked(bool)),this,SLOT(gen3BrowseClicked()));
	connect(m_optionUi->freqencyEdit_3,SIGNAL(textChanged(QString)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->wpmBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->dbBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->fadeBox_3,SIGNAL(clicked(bool)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->fistBox_3,SIGNAL(clicked(bool)),this,SLOT(updateGen3Fields()));

	//4
	initSourceBox(m_optionUi->sourceBox_4);
	initWpmBox(m_optionUi->wpmBox_4);
	initDbBox(m_optionUi->dbBox_4);

	setGen4Ui(m_gs4);

	connect(m_optionUi->enabledBox_4,SIGNAL(clicked(bool)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->sourceBox_4,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->browseButton_4,SIGNAL(clicked(bool)),this,SLOT(gen4BrowseClicked()));
	connect(m_optionUi->freqencyEdit_4,SIGNAL(textChanged(QString)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->wpmBox_4,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->dbBox_4,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->fadeBox_4,SIGNAL(clicked(bool)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->fistBox_4,SIGNAL(clicked(bool)),this,SLOT(updateGen4Fields()));

	//5
	initSourceBox(m_optionUi->sourceBox_5);
	initWpmBox(m_optionUi->wpmBox_5);
	initDbBox(m_optionUi->dbBox_5);

	setGen5Ui(m_gs5);

	connect(m_optionUi->enabledBox_5,SIGNAL(clicked(bool)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->sourceBox_5,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->browseButton_5,SIGNAL(clicked(bool)),this,SLOT(gen5BrowseClicked()));
	connect(m_optionUi->freqencyEdit_5,SIGNAL(textChanged(QString)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->wpmBox_5,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->dbBox_5,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->fadeBox_5,SIGNAL(clicked(bool)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->fistBox_5,SIGNAL(clicked(bool)),this,SLOT(updateGen5Fields()));

	connect(m_optionUi->loadButton,SIGNAL(clicked(bool)),this,SLOT(loadPresetClicked(bool)));
	connect(m_optionUi->saveButton,SIGNAL(clicked(bool)),this,SLOT(savePresetClicked(bool)));

}

void MorseGenDevice::setGen1Ui(GenSettings gs)
{
	quint32 index;
	m_optionUi->enabledBox_1->setChecked(gs.enabled);
	index = m_optionUi->sourceBox_1->findData(gs.text);
	m_optionUi->sourceBox_1->setCurrentIndex(index);
	m_optionUi->filename_1->setReadOnly(true);
	QFileInfo info(gs.fileName);
	m_optionUi->filename_1->setText(info.baseName());
	m_optionUi->freqencyEdit_1->setText(QString::number(gs.freq,'f',0));
	index = m_optionUi->wpmBox_1->findData(gs.wpm);
	m_optionUi->wpmBox_1->setCurrentIndex(index);
	index = m_optionUi->dbBox_1->findData(gs.amp);
	m_optionUi->dbBox_1->setCurrentIndex(index);
	m_optionUi->fadeBox_1->setChecked(gs.fade);
	m_optionUi->fistBox_1->setChecked(gs.fist);
}

void MorseGenDevice::setGen2Ui(MorseGenDevice::GenSettings gs)
{
	quint32 index;
	m_optionUi->enabledBox_2->setChecked(gs.enabled);
	index = m_optionUi->sourceBox_2->findData(gs.text);
	m_optionUi->sourceBox_2->setCurrentIndex(index);
	m_optionUi->filename_2->setReadOnly(true);
	QFileInfo info(gs.fileName);
	m_optionUi->filename_2->setText(info.baseName());
	m_optionUi->freqencyEdit_2->setText(QString::number(gs.freq,'f',0));
	index = m_optionUi->wpmBox_2->findData(gs.wpm);
	m_optionUi->wpmBox_2->setCurrentIndex(index);
	index = m_optionUi->dbBox_2->findData(gs.amp);
	m_optionUi->dbBox_2->setCurrentIndex(index);
	m_optionUi->fadeBox_2->setChecked(gs.fade);
	m_optionUi->fistBox_2->setChecked(gs.fist);
}

void MorseGenDevice::setGen3Ui(MorseGenDevice::GenSettings gs)
{
	quint32 index;
	m_optionUi->enabledBox_3->setChecked(gs.enabled);
	index = m_optionUi->sourceBox_3->findData(gs.text);
	m_optionUi->sourceBox_3->setCurrentIndex(index);
	m_optionUi->filename_3->setReadOnly(true);
	QFileInfo info(gs.fileName);
	m_optionUi->filename_3->setText(info.baseName());
	m_optionUi->freqencyEdit_3->setText(QString::number(gs.freq,'f',0));
	index = m_optionUi->wpmBox_3->findData(gs.wpm);
	m_optionUi->wpmBox_3->setCurrentIndex(index);
	index = m_optionUi->dbBox_3->findData(gs.amp);
	m_optionUi->dbBox_3->setCurrentIndex(index);
	m_optionUi->fadeBox_3->setChecked(gs.fade);
	m_optionUi->fistBox_3->setChecked(gs.fist);
}

void MorseGenDevice::setGen4Ui(MorseGenDevice::GenSettings gs)
{
	quint32 index;
	m_optionUi->enabledBox_4->setChecked(gs.enabled);
	index = m_optionUi->sourceBox_4->findData(gs.text);
	m_optionUi->sourceBox_4->setCurrentIndex(index);
	m_optionUi->filename_4->setReadOnly(true);
	QFileInfo info(gs.fileName);
	m_optionUi->filename_4->setText(info.baseName());
	m_optionUi->freqencyEdit_4->setText(QString::number(gs.freq,'f',0));
	index = m_optionUi->wpmBox_4->findData(gs.wpm);
	m_optionUi->wpmBox_4->setCurrentIndex(index);
	index = m_optionUi->dbBox_4->findData(gs.amp);
	m_optionUi->dbBox_4->setCurrentIndex(index);
	m_optionUi->fadeBox_4->setChecked(gs.fade);
	m_optionUi->fistBox_4->setChecked(gs.fist);
}

void MorseGenDevice::setGen5Ui(MorseGenDevice::GenSettings gs)
{
	quint32 index;
	m_optionUi->enabledBox_5->setChecked(gs.enabled);
	index = m_optionUi->sourceBox_5->findData(gs.text);
	m_optionUi->sourceBox_5->setCurrentIndex(index);
	m_optionUi->filename_5->setReadOnly(true);
	QFileInfo info(gs.fileName);
	m_optionUi->filename_5->setText(info.baseName());
	m_optionUi->freqencyEdit_5->setText(QString::number(gs.freq,'f',0));
	index = m_optionUi->wpmBox_5->findData(gs.wpm);
	m_optionUi->wpmBox_5->setCurrentIndex(index);
	index = m_optionUi->dbBox_5->findData(gs.amp);
	m_optionUi->dbBox_5->setCurrentIndex(index);
	m_optionUi->fadeBox_5->setChecked(gs.fade);
	m_optionUi->fistBox_5->setChecked(gs.fist);
}

//Change to loadPresetClicked
void MorseGenDevice::loadPresetClicked(bool clicked)
{
	Q_UNUSED(clicked);

	quint32 preset = m_optionUi->presetsBox->currentData().toUInt();
	switch (preset) {
		case 1:
			setGen1Ui(m_preset1[0]);
			setGen2Ui(m_preset1[1]);
			setGen3Ui(m_preset1[2]);
			setGen4Ui(m_preset1[3]);
			setGen5Ui(m_preset1[4]);
			break;
		case 2:
			setGen1Ui(m_preset2[0]);
			setGen2Ui(m_preset2[1]);
			setGen3Ui(m_preset2[2]);
			setGen4Ui(m_preset2[3]);
			setGen5Ui(m_preset2[4]);
			break;
		case 3:
			setGen1Ui(m_preset3[0]);
			setGen2Ui(m_preset3[1]);
			setGen3Ui(m_preset3[2]);
			setGen4Ui(m_preset3[3]);
			setGen5Ui(m_preset3[4]);
			break;
		case 4:
			setGen1Ui(m_preset4[0]);
			setGen2Ui(m_preset4[1]);
			setGen3Ui(m_preset4[2]);
			setGen4Ui(m_preset4[3]);
			setGen5Ui(m_preset4[4]);
			break;
		case 5:
			setGen1Ui(m_preset5[0]);
			setGen2Ui(m_preset5[1]);
			setGen3Ui(m_preset5[2]);
			setGen4Ui(m_preset5[3]);
			setGen5Ui(m_preset5[4]);
			break;
		default:
			break;
	}

}

void MorseGenDevice::savePresetClicked(bool clicked)
{
	Q_UNUSED(clicked);
	quint32 preset = m_optionUi->presetsBox->currentData().toUInt();
	switch (preset) {
		case 1:
			m_preset1[0] = m_gs1;
			m_preset1[1] = m_gs2;
			m_preset1[2] = m_gs3;
			m_preset1[3] = m_gs4;
			m_preset1[4] = m_gs5;
			break;
		case 2:
			m_preset2[0] = m_gs1;
			m_preset2[1] = m_gs2;
			m_preset2[2] = m_gs3;
			m_preset2[3] = m_gs4;
			m_preset2[4] = m_gs5;
			break;
		case 3:
			m_preset3[0] = m_gs1;
			m_preset3[1] = m_gs2;
			m_preset3[2] = m_gs3;
			m_preset3[3] = m_gs4;
			m_preset3[4] = m_gs5;
			break;
		case 4:
			m_preset4[0] = m_gs1;
			m_preset4[1] = m_gs2;
			m_preset4[2] = m_gs3;
			m_preset4[3] = m_gs4;
			m_preset4[4] = m_gs5;
			break;
		case 5:
			m_preset5[0] = m_gs1;
			m_preset5[1] = m_gs2;
			m_preset5[2] = m_gs3;
			m_preset5[3] = m_gs4;
			m_preset5[4] = m_gs5;
			break;
		default:
			break;
	}

}

void MorseGenDevice::generate(CPX *out)
{
	CPX cpx1, cpx2, cpx3, cpx4, cpx5, cpx6;

	m_mutex.lock();
	for (quint32 i=0; i<m_framesPerBuffer; i++) {
		if (m_gs1.enabled) {
			cpx1 = m_morseGen1->nextOutputSample();
			if (m_gs1.fade) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx1 *= ampRand;
			}
		}
		if (m_gs2.enabled) {
			cpx2 = m_morseGen2->nextOutputSample();
			if (m_gs2.fade) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx2 *= ampRand;
			}
		}
		if (m_gs3.enabled) {
			cpx3 = m_morseGen3->nextOutputSample();
			if (m_gs3.fade) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx3 *= ampRand;
			}
		}
		if (m_gs4.enabled) {
			cpx4 = m_morseGen4->nextOutputSample();
			if (m_gs4.fade) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx4 *= ampRand;
			}
		}
		if (m_gs5.enabled) {
			cpx5 = m_morseGen5->nextOutputSample();
			if (m_gs5.fade) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx5 *= ampRand;
			}
		}
		cpx6 = nextNoiseSample(m_noiseAmp);
		out[i] = cpx1 + cpx2 + cpx3 + cpx4 + cpx5 + cpx6;
	}
	m_mutex.unlock();
}

//Derived from nco.cpp
//Algorithm from http://dspguru.com/dsp/howtos/how-to-generate-white-gaussian-noise
//Uses Box-Muller Method, optimized to Polar Method [Ros88] A First Course on Probability
//Also http://www.dspguide.com/ch2/6.htm
//https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform R Knop algorithm
CPX MorseGenDevice::nextNoiseSample(double _dbGain)
{
	double u1;
	double u2;
	double s;
	double rad;
	double x;
	double y;

	//R Knop
	do {
		u1 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX;
		u2 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX;
		//Collapsing steps
		s = u1 * u1 + u2 * u2;
	} while (s >= 1.0 || s == 0.0);
	// 0 < s < 1
	rad = sqrt(-2.0 * log(s) / s);
	x = _dbGain * u1 * rad;
	y = _dbGain * u2 * rad;
	return CPX(x,y);
}
