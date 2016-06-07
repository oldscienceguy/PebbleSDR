#include "morsegendevice.h"
#include "db.h"

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
	m_outBuf = CPX::memalign(outBufLen);
	m_outBuf1 = CPX::memalign(outBufLen);
	m_outBuf2 = CPX::memalign(outBufLen);

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

void MorseGenDevice::readSettings()
{
	// +/- db gain required to normalize to fixed level input
	// Default is 0db gain, or a factor of 1.0
	m_normalizeIQGain = DB::dBToAmplitude(0);
	m_startupDemodMode = DemodMode::dmCWL;
	//Set defaults before calling DeviceInterfaceBase
	DeviceInterfaceBase::readSettings();

	m_dbNoiseAmp = m_settings->value("DbNoiseAmp",-60).toDouble();
	m_settings->beginGroup("Gen1");
	m_gs1.enabled = m_settings->value("Enabled",true).toBool();
	m_gs1.freq = m_settings->value("Freq",1000).toDouble();
	m_gs1.amp = m_settings->value("Amp",-40).toDouble();
	m_gs1.wpm = m_settings->value("Wpm",20).toUInt();
	m_gs1.rise = m_settings->value("Rise",5).toUInt();
	m_gs1.text = m_settings->value("Text",0).toUInt();
	m_gs1.fade = m_settings->value("Fade",false).toBool();
	m_gs1.fist = m_settings->value("Fist",false).toBool();
	m_settings->endGroup();

	m_settings->beginGroup("Gen2");
	m_gs2.enabled = m_settings->value("Enabled",true).toBool();
	m_gs2.freq = m_settings->value("Freq",1500).toDouble();
	m_gs2.amp = m_settings->value("Amp",-40).toDouble();
	m_gs2.wpm = m_settings->value("Wpm",30).toUInt();
	m_gs2.rise = m_settings->value("Rise",5).toUInt();
	m_gs2.text = m_settings->value("Text",0).toUInt();
	m_gs2.fade = m_settings->value("Fade",false).toBool();
	m_gs2.fist = m_settings->value("Fist",false).toBool();
	m_settings->endGroup();

	m_settings->beginGroup("Gen3");
	m_gs3.enabled = m_settings->value("Enabled",true).toBool();
	m_gs3.freq = m_settings->value("Freq",2000).toDouble();
	m_gs3.amp = m_settings->value("Amp",-40).toDouble();
	m_gs3.wpm = m_settings->value("Wpm",40).toUInt();
	m_gs3.rise = m_settings->value("Rise",5).toUInt();
	m_gs3.text = m_settings->value("Text",0).toUInt();
	m_gs3.fade = m_settings->value("Fade",false).toBool();
	m_gs3.fist = m_settings->value("Fist",false).toBool();
	m_settings->endGroup();

	m_settings->beginGroup("Gen4");
	m_gs4.enabled = m_settings->value("Enabled",true).toBool();
	m_gs4.freq = m_settings->value("Freq",2500).toDouble();
	m_gs4.amp = m_settings->value("Amp",-40).toDouble();
	m_gs4.wpm = m_settings->value("Wpm",50).toUInt();
	m_gs4.rise = m_settings->value("Rise",5).toUInt();
	m_gs4.text = m_settings->value("Text",0).toUInt();
	m_gs4.fade = m_settings->value("Fade",false).toBool();
	m_gs4.fist = m_settings->value("Fist",false).toBool();
	m_settings->endGroup();

	m_settings->beginGroup("Gen5");
	m_gs5.enabled = m_settings->value("Enabled",true).toBool();
	m_gs5.freq = m_settings->value("Freq",2500).toDouble();
	m_gs5.amp = m_settings->value("Amp",-40).toDouble();
	m_gs5.wpm = m_settings->value("Wpm",60).toUInt();
	m_gs5.rise = m_settings->value("Rise",5).toUInt();
	m_gs5.text = m_settings->value("Text",0).toUInt();
	m_gs5.fade = m_settings->value("Fade",false).toBool();
	m_gs5.fist = m_settings->value("Fist",false).toBool();
	m_settings->endGroup();

	//Text output, read only settings
	//Trailing '=' is morse <BT> for new paragraph
	QString str1 = "=The quick brown fox jumped over the lazy dog 1,2,3,4,5,6,7,8,9,0 times=";
	QString str2 = "=Now is the time for all good men to come to the aid of their country=";
	m_sampleText[ST_SAMPLE1] = m_settings->value("Sample1",str1).toString();
	m_sampleText[ST_SAMPLE2] = m_settings->value("Sample2",str2).toString();
	m_sampleText[ST_SAMPLE3] = m_settings->value("Sample3",str2).toString();
	m_sampleText[ST_SAMPLE4] = m_settings->value("Sample4",str2).toString();

}

void MorseGenDevice::writeSettings()
{
	DeviceInterfaceBase::writeSettings();

	m_settings->setValue("DbNoiseAmp", m_dbNoiseAmp);
	m_settings->beginGroup("Gen1");
	m_settings->setValue("Enabled", m_gs1.enabled);
	m_settings->setValue("Freq",m_gs1.freq);
	m_settings->setValue("Amp",m_gs1.amp);
	m_settings->setValue("Wpm",m_gs1.wpm);
	m_settings->setValue("Rise",m_gs1.rise);
	m_settings->setValue("Text",m_gs1.text);
	m_settings->setValue("Fade",m_gs1.fade);
	m_settings->setValue("Fist",m_gs1.fist);
	m_settings->endGroup();

	m_settings->beginGroup("Gen2");
	m_settings->setValue("Enabled", m_gs2.enabled);
	m_settings->setValue("Freq",m_gs2.freq);
	m_settings->setValue("Amp",m_gs2.amp);
	m_settings->setValue("Wpm",m_gs2.wpm);
	m_settings->setValue("Rise",m_gs2.rise);
	m_settings->setValue("Text",m_gs2.text);
	m_settings->setValue("Fade",m_gs2.fade);
	m_settings->setValue("Fist",m_gs2.fist);
	m_settings->endGroup();

	m_settings->beginGroup("Gen3");
	m_settings->setValue("Enabled", m_gs3.enabled);
	m_settings->setValue("Freq",m_gs3.freq);
	m_settings->setValue("Amp",m_gs3.amp);
	m_settings->setValue("Wpm",m_gs3.wpm);
	m_settings->setValue("Rise",m_gs3.rise);
	m_settings->setValue("Text",m_gs3.text);
	m_settings->setValue("Fade",m_gs3.fade);
	m_settings->setValue("Fist",m_gs3.fist);
	m_settings->endGroup();

	m_settings->beginGroup("Gen4");
	m_settings->setValue("Enabled", m_gs4.enabled);
	m_settings->setValue("Freq",m_gs4.freq);
	m_settings->setValue("Amp",m_gs4.amp);
	m_settings->setValue("Wpm",m_gs4.wpm);
	m_settings->setValue("Rise",m_gs4.rise);
	m_settings->setValue("Text",m_gs4.text);
	m_settings->setValue("Fade",m_gs4.fade);
	m_settings->setValue("Fist",m_gs4.fist);
	m_settings->endGroup();

	m_settings->beginGroup("Gen5");
	m_settings->setValue("Enabled", m_gs5.enabled);
	m_settings->setValue("Freq",m_gs5.freq);
	m_settings->setValue("Amp",m_gs5.amp);
	m_settings->setValue("Wpm",m_gs5.wpm);
	m_settings->setValue("Rise",m_gs5.rise);
	m_settings->setValue("Text",m_gs5.text);
	m_settings->setValue("Fade",m_gs5.fade);
	m_settings->setValue("Fist",m_gs5.fist);
	m_settings->endGroup();

	m_settings->setValue("Sample1",m_sampleText[ST_SAMPLE1]);
	m_settings->setValue("Sample2",m_sampleText[ST_SAMPLE2]);
	m_settings->setValue("Sample3",m_sampleText[ST_SAMPLE3]);
	m_settings->setValue("Sample4",m_sampleText[ST_SAMPLE4]);
}

void MorseGenDevice::updateGenerators()
{
	if (!m_running)
		return;
	m_morseGen1->setParams(m_gs1.freq,m_gs1.amp,m_gs1.wpm,m_gs1.rise);
	m_morseGen1->setTextOut(m_sampleText[m_gs1.text]);
	m_morseGen2->setParams(m_gs2.freq,m_gs2.amp,m_gs2.wpm,m_gs2.rise);
	m_morseGen2->setTextOut(m_sampleText[m_gs2.text]);
	m_morseGen3->setParams(m_gs3.freq,m_gs3.amp,m_gs3.wpm,m_gs3.rise);
	m_morseGen3->setTextOut(m_sampleText[m_gs3.text]);
	m_morseGen4->setParams(m_gs4.freq,m_gs4.amp,m_gs4.wpm,m_gs4.rise);
	m_morseGen4->setTextOut(m_sampleText[m_gs4.text]);
	m_morseGen5->setParams(m_gs5.freq,m_gs5.amp,m_gs5.wpm,m_gs5.rise);
	m_morseGen5->setTextOut(m_sampleText[m_gs5.text]);
	//Because noise is averaged, fudget +30db so it matches with generator values
	//ie -30db gen and -30db noise should be 0snr
	m_noiseAmp = DB::dBToAmplitude(m_dbNoiseAmp+20);

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
		m_morseGen1->setTextOut(m_sampleText[m_gs1.text]);
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
		m_morseGen2->setTextOut(m_sampleText[m_gs2.text]);
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
		m_morseGen3->setTextOut(m_sampleText[m_gs3.text]);
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
		m_morseGen4->setTextOut(m_sampleText[m_gs4.text]);
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
		m_morseGen5->setTextOut(m_sampleText[m_gs5.text]);
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

	m_optionUi->sourceBox_1->addItem("Sample1",ST_SAMPLE1);
	m_optionUi->sourceBox_1->addItem("Sample2",ST_SAMPLE2);
	m_optionUi->sourceBox_1->addItem("Sample3",ST_SAMPLE3);
	m_optionUi->sourceBox_1->addItem("Sample4",ST_SAMPLE4);
	m_optionUi->sourceBox_1->addItem("Words",ST_WORDS);
	m_optionUi->sourceBox_1->addItem("Abbrev",ST_ABBREV);
	m_optionUi->sourceBox_1->addItem("Table",ST_TABLE);

	m_optionUi->wpmBox_1->addItem("5 wpm", 5);
	m_optionUi->wpmBox_1->addItem("10 wpm", 10);
	m_optionUi->wpmBox_1->addItem("15 wpm", 15);
	m_optionUi->wpmBox_1->addItem("20 wpm", 20);
	m_optionUi->wpmBox_1->addItem("30 wpm", 30);
	m_optionUi->wpmBox_1->addItem("40 wpm", 40);
	m_optionUi->wpmBox_1->addItem("50 wpm", 50);
	m_optionUi->wpmBox_1->addItem("60 wpm", 60);
	m_optionUi->wpmBox_1->addItem("70 wpm", 70);
	m_optionUi->wpmBox_1->addItem("80 wpm", 80);

	m_optionUi->dbBox_1->addItem("-30db", -30);
	m_optionUi->dbBox_1->addItem("-35db", -35);
	m_optionUi->dbBox_1->addItem("-40db", -40);
	m_optionUi->dbBox_1->addItem("-45db", -45);
	m_optionUi->dbBox_1->addItem("-50db", -50);
	m_optionUi->dbBox_1->addItem("-55db", -55);
	m_optionUi->dbBox_1->addItem("-60db", -60);

	m_optionUi->enabledBox_1->setChecked(m_gs1.enabled);
	index = m_optionUi->sourceBox_1->findData(m_gs1.text);
	m_optionUi->sourceBox_1->setCurrentIndex(index);
	m_optionUi->freqencyEdit_1->setText(QString::number(m_gs1.freq,'f',0));
	index = m_optionUi->wpmBox_1->findData(m_gs1.wpm);
	m_optionUi->wpmBox_1->setCurrentIndex(index);
	index = m_optionUi->dbBox_1->findData(m_gs1.amp);
	m_optionUi->dbBox_1->setCurrentIndex(index);
	m_optionUi->fadeBox_1->setChecked(m_gs1.fade);
	m_optionUi->fistBox_1->setChecked(m_gs1.fist);

	connect(m_optionUi->enabledBox_1,SIGNAL(clicked(bool)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->sourceBox_1,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->freqencyEdit_1,SIGNAL(textChanged(QString)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->wpmBox_1,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->dbBox_1,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->fadeBox_1,SIGNAL(clicked(bool)),this,SLOT(updateGen1Fields()));
	connect(m_optionUi->fistBox_1,SIGNAL(clicked(bool)),this,SLOT(updateGen1Fields()));

	//2
	m_optionUi->sourceBox_2->addItem("Sample1",ST_SAMPLE1);
	m_optionUi->sourceBox_2->addItem("Sample2",ST_SAMPLE2);
	m_optionUi->sourceBox_2->addItem("Sample3",ST_SAMPLE3);
	m_optionUi->sourceBox_2->addItem("Sample4",ST_SAMPLE4);
	m_optionUi->sourceBox_2->addItem("Words",ST_WORDS);
	m_optionUi->sourceBox_2->addItem("Abbrev",ST_ABBREV);
	m_optionUi->sourceBox_2->addItem("Table",ST_TABLE);

	m_optionUi->wpmBox_2->addItem("5 wpm", 5);
	m_optionUi->wpmBox_2->addItem("10 wpm", 10);
	m_optionUi->wpmBox_2->addItem("15 wpm", 15);
	m_optionUi->wpmBox_2->addItem("20 wpm", 20);
	m_optionUi->wpmBox_2->addItem("30 wpm", 30);
	m_optionUi->wpmBox_2->addItem("40 wpm", 40);
	m_optionUi->wpmBox_2->addItem("50 wpm", 50);
	m_optionUi->wpmBox_2->addItem("60 wpm", 60);
	m_optionUi->wpmBox_2->addItem("70 wpm", 70);
	m_optionUi->wpmBox_2->addItem("80 wpm", 80);

	m_optionUi->dbBox_2->addItem("-30db", -30);
	m_optionUi->dbBox_2->addItem("-35db", -35);
	m_optionUi->dbBox_2->addItem("-40db", -40);
	m_optionUi->dbBox_2->addItem("-45db", -45);
	m_optionUi->dbBox_2->addItem("-50db", -50);
	m_optionUi->dbBox_2->addItem("-55db", -55);
	m_optionUi->dbBox_2->addItem("-60db", -60);

	m_optionUi->enabledBox_2->setChecked(m_gs2.enabled);
	index = m_optionUi->sourceBox_2->findData(m_gs2.text);
	m_optionUi->sourceBox_2->setCurrentIndex(index);
	m_optionUi->freqencyEdit_2->setText(QString::number(m_gs2.freq,'f',0));
	index = m_optionUi->wpmBox_2->findData(m_gs2.wpm);
	m_optionUi->wpmBox_2->setCurrentIndex(index);
	index = m_optionUi->dbBox_2->findData(m_gs2.amp);
	m_optionUi->dbBox_2->setCurrentIndex(index);
	m_optionUi->fadeBox_2->setChecked(m_gs2.fade);
	m_optionUi->fistBox_2->setChecked(m_gs2.fist);

	connect(m_optionUi->enabledBox_2,SIGNAL(clicked(bool)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->sourceBox_2,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->freqencyEdit_2,SIGNAL(textChanged(QString)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->wpmBox_2,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->dbBox_2,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->fadeBox_2,SIGNAL(clicked(bool)),this,SLOT(updateGen2Fields()));
	connect(m_optionUi->fistBox_2,SIGNAL(clicked(bool)),this,SLOT(updateGen2Fields()));

	//3
	m_optionUi->sourceBox_3->addItem("Sample1",ST_SAMPLE1);
	m_optionUi->sourceBox_3->addItem("Sample2",ST_SAMPLE2);
	m_optionUi->sourceBox_3->addItem("Sample3",ST_SAMPLE3);
	m_optionUi->sourceBox_3->addItem("Sample4",ST_SAMPLE4);
	m_optionUi->sourceBox_3->addItem("Words",ST_WORDS);
	m_optionUi->sourceBox_3->addItem("Abbrev",ST_ABBREV);
	m_optionUi->sourceBox_3->addItem("Table",ST_TABLE);

	m_optionUi->wpmBox_3->addItem("5 wpm", 5);
	m_optionUi->wpmBox_3->addItem("10 wpm", 10);
	m_optionUi->wpmBox_3->addItem("15 wpm", 15);
	m_optionUi->wpmBox_3->addItem("20 wpm", 20);
	m_optionUi->wpmBox_3->addItem("30 wpm", 30);
	m_optionUi->wpmBox_3->addItem("40 wpm", 40);
	m_optionUi->wpmBox_3->addItem("50 wpm", 50);
	m_optionUi->wpmBox_3->addItem("60 wpm", 60);
	m_optionUi->wpmBox_3->addItem("70 wpm", 70);
	m_optionUi->wpmBox_3->addItem("80 wpm", 80);

	m_optionUi->dbBox_3->addItem("-30db", -30);
	m_optionUi->dbBox_3->addItem("-35db", -35);
	m_optionUi->dbBox_3->addItem("-40db", -40);
	m_optionUi->dbBox_3->addItem("-45db", -45);
	m_optionUi->dbBox_3->addItem("-50db", -50);
	m_optionUi->dbBox_3->addItem("-55db", -55);
	m_optionUi->dbBox_3->addItem("-60db", -60);

	m_optionUi->enabledBox_3->setChecked(m_gs3.enabled);
	index = m_optionUi->sourceBox_3->findData(m_gs3.text);
	m_optionUi->sourceBox_3->setCurrentIndex(index);
	m_optionUi->freqencyEdit_3->setText(QString::number(m_gs3.freq,'f',0));
	index = m_optionUi->wpmBox_3->findData(m_gs3.wpm);
	m_optionUi->wpmBox_3->setCurrentIndex(index);
	index = m_optionUi->dbBox_3->findData(m_gs3.amp);
	m_optionUi->dbBox_3->setCurrentIndex(index);
	m_optionUi->fadeBox_3->setChecked(m_gs3.fade);
	m_optionUi->fistBox_3->setChecked(m_gs3.fist);

	connect(m_optionUi->enabledBox_3,SIGNAL(clicked(bool)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->sourceBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->freqencyEdit_3,SIGNAL(textChanged(QString)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->wpmBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->dbBox_3,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->fadeBox_3,SIGNAL(clicked(bool)),this,SLOT(updateGen3Fields()));
	connect(m_optionUi->fistBox_3,SIGNAL(clicked(bool)),this,SLOT(updateGen3Fields()));

	//4
	m_optionUi->sourceBox_4->addItem("Sample1",ST_SAMPLE1);
	m_optionUi->sourceBox_4->addItem("Sample2",ST_SAMPLE2);
	m_optionUi->sourceBox_4->addItem("Sample3",ST_SAMPLE3);
	m_optionUi->sourceBox_4->addItem("Sample4",ST_SAMPLE4);
	m_optionUi->sourceBox_4->addItem("Words",ST_WORDS);
	m_optionUi->sourceBox_4->addItem("Abbrev",ST_ABBREV);
	m_optionUi->sourceBox_4->addItem("Table",ST_TABLE);

	m_optionUi->wpmBox_4->addItem("5 wpm", 5);
	m_optionUi->wpmBox_4->addItem("10 wpm", 10);
	m_optionUi->wpmBox_4->addItem("15 wpm", 15);
	m_optionUi->wpmBox_4->addItem("20 wpm", 20);
	m_optionUi->wpmBox_4->addItem("30 wpm", 30);
	m_optionUi->wpmBox_4->addItem("40 wpm", 40);
	m_optionUi->wpmBox_4->addItem("50 wpm", 50);
	m_optionUi->wpmBox_4->addItem("60 wpm", 60);
	m_optionUi->wpmBox_4->addItem("70 wpm", 70);
	m_optionUi->wpmBox_4->addItem("80 wpm", 80);

	m_optionUi->dbBox_4->addItem("-30db", -30);
	m_optionUi->dbBox_4->addItem("-35db", -35);
	m_optionUi->dbBox_4->addItem("-40db", -40);
	m_optionUi->dbBox_4->addItem("-45db", -45);
	m_optionUi->dbBox_4->addItem("-50db", -50);
	m_optionUi->dbBox_4->addItem("-55db", -55);
	m_optionUi->dbBox_4->addItem("-60db", -60);

	m_optionUi->enabledBox_4->setChecked(m_gs4.enabled);
	index = m_optionUi->sourceBox_4->findData(m_gs4.text);
	m_optionUi->sourceBox_4->setCurrentIndex(index);
	m_optionUi->freqencyEdit_4->setText(QString::number(m_gs4.freq,'f',0));
	index = m_optionUi->wpmBox_4->findData(m_gs4.wpm);
	m_optionUi->wpmBox_4->setCurrentIndex(index);
	index = m_optionUi->dbBox_4->findData(m_gs4.amp);
	m_optionUi->dbBox_4->setCurrentIndex(index);
	m_optionUi->fadeBox_4->setChecked(m_gs4.fade);
	m_optionUi->fistBox_4->setChecked(m_gs4.fist);

	connect(m_optionUi->enabledBox_4,SIGNAL(clicked(bool)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->sourceBox_4,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->freqencyEdit_4,SIGNAL(textChanged(QString)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->wpmBox_4,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->dbBox_4,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->fadeBox_4,SIGNAL(clicked(bool)),this,SLOT(updateGen4Fields()));
	connect(m_optionUi->fistBox_4,SIGNAL(clicked(bool)),this,SLOT(updateGen4Fields()));

	//5
	m_optionUi->sourceBox_5->addItem("Sample1",ST_SAMPLE1);
	m_optionUi->sourceBox_5->addItem("Sample2",ST_SAMPLE2);
	m_optionUi->sourceBox_5->addItem("Sample3",ST_SAMPLE3);
	m_optionUi->sourceBox_5->addItem("Sample4",ST_SAMPLE4);
	m_optionUi->sourceBox_5->addItem("Words",ST_WORDS);
	m_optionUi->sourceBox_5->addItem("Abbrev",ST_ABBREV);
	m_optionUi->sourceBox_5->addItem("Table",ST_TABLE);

	m_optionUi->wpmBox_5->addItem("5 wpm", 5);
	m_optionUi->wpmBox_5->addItem("10 wpm", 10);
	m_optionUi->wpmBox_5->addItem("15 wpm", 15);
	m_optionUi->wpmBox_5->addItem("20 wpm", 20);
	m_optionUi->wpmBox_5->addItem("30 wpm", 30);
	m_optionUi->wpmBox_5->addItem("40 wpm", 40);
	m_optionUi->wpmBox_5->addItem("50 wpm", 50);
	m_optionUi->wpmBox_5->addItem("60 wpm", 60);
	m_optionUi->wpmBox_5->addItem("70 wpm", 70);
	m_optionUi->wpmBox_5->addItem("80 wpm", 80);

	m_optionUi->dbBox_5->addItem("-30db", -30);
	m_optionUi->dbBox_5->addItem("-35db", -35);
	m_optionUi->dbBox_5->addItem("-40db", -40);
	m_optionUi->dbBox_5->addItem("-45db", -45);
	m_optionUi->dbBox_5->addItem("-50db", -50);
	m_optionUi->dbBox_5->addItem("-55db", -55);
	m_optionUi->dbBox_5->addItem("-60db", -60);

	m_optionUi->enabledBox_5->setChecked(m_gs5.enabled);
	index = m_optionUi->sourceBox_5->findData(m_gs5.text);
	m_optionUi->sourceBox_5->setCurrentIndex(index);
	m_optionUi->freqencyEdit_5->setText(QString::number(m_gs5.freq,'f',0));
	index = m_optionUi->wpmBox_5->findData(m_gs5.wpm);
	m_optionUi->wpmBox_5->setCurrentIndex(index);
	index = m_optionUi->dbBox_5->findData(m_gs5.amp);
	m_optionUi->dbBox_5->setCurrentIndex(index);
	m_optionUi->fadeBox_5->setChecked(m_gs5.fade);
	m_optionUi->fistBox_5->setChecked(m_gs5.fist);

	connect(m_optionUi->enabledBox_5,SIGNAL(clicked(bool)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->sourceBox_5,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->freqencyEdit_5,SIGNAL(textChanged(QString)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->wpmBox_5,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->dbBox_5,SIGNAL(currentIndexChanged(int)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->fadeBox_5,SIGNAL(clicked(bool)),this,SLOT(updateGen5Fields()));
	connect(m_optionUi->fistBox_5,SIGNAL(clicked(bool)),this,SLOT(updateGen5Fields()));

	connect(m_optionUi->resetButton,SIGNAL(clicked(bool)),this,SLOT(resetButtonClicked(bool)));

}

void MorseGenDevice::resetButtonClicked(bool clicked)
{
	Q_UNUSED(clicked);

	m_optionUi->enabledBox_1->setChecked(true);
	m_optionUi->freqencyEdit_1->setText("1000");
	m_optionUi->sourceBox_1->setCurrentIndex(0);
	m_optionUi->wpmBox_1->setCurrentIndex(3);
	m_optionUi->dbBox_1->setCurrentIndex(2);

	m_optionUi->enabledBox_2->setChecked(true);
	m_optionUi->freqencyEdit_2->setText("1500");
	m_optionUi->sourceBox_2->setCurrentIndex(1);
	m_optionUi->wpmBox_2->setCurrentIndex(4);
	m_optionUi->dbBox_2->setCurrentIndex(2);

	m_optionUi->enabledBox_3->setChecked(true);
	m_optionUi->freqencyEdit_3->setText("2000");
	m_optionUi->sourceBox_3->setCurrentIndex(2);
	m_optionUi->wpmBox_3->setCurrentIndex(5);
	m_optionUi->dbBox_3->setCurrentIndex(2);

	m_optionUi->enabledBox_4->setChecked(true);
	m_optionUi->freqencyEdit_4->setText("2500");
	m_optionUi->sourceBox_4->setCurrentIndex(3);
	m_optionUi->wpmBox_4->setCurrentIndex(6);
	m_optionUi->dbBox_4->setCurrentIndex(2);

	m_optionUi->enabledBox_5->setChecked(true);
	m_optionUi->freqencyEdit_5->setText("3000");
	m_optionUi->sourceBox_5->setCurrentIndex(4);
	m_optionUi->wpmBox_5->setCurrentIndex(7);
	m_optionUi->dbBox_5->setCurrentIndex(2);

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
