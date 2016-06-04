#include "morsegendevice.h"
#include "db.h"

//Plugin constructors are called indirectly when the plugin is loaded in Receiver
//Be careful not to access objects that are not initialized yet, do that in Initialize()
MorseGenDevice::MorseGenDevice():DeviceInterfaceBase()
{
	initSettings("morsegen");
	optionUi = NULL;
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


	//Trailing '=' is morse <BT> for new paragraph
	m_sampleText[0] = "=The quick brown fox jumped over the lazy dog 1,2,3,4,5,6,7,8,9,0 times=";
	m_sampleText[1] = "=Now is the time for all good men to come to the aid of their country=";
	m_sampleText[2] = "=Now is the time for all good men to come to the aid of their country=";
	m_sampleText[3] = "=Now is the time for all good men to come to the aid of their country=";
	//m_sampleText[4] = "=Now is the time for all good men to come to the aid of their country=";

	//Build text for all characters
	MorseSymbol *symbol;
	m_sampleText[4] = "=";
	for (quint32 i=0; i<MorseCode::c_morseTableSize; i++) {
		symbol = &MorseCode::m_morseTable[i];
		if (symbol->ascii == 0x00) {
			break; //End of table
		}
		m_sampleText[4].append(symbol->ascii);
	}
	m_sampleText[4].append("=");

	m_numProducerBuffers = 50;
	producerFreeBufPtr = NULL;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(double);
	m_readBufferSize = m_framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	m_producerConsumer.Initialize(std::bind(&MorseGenDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&MorseGenDevice::consumerWorker, this, std::placeholders::_1),m_numProducerBuffers, m_readBufferSize);
	//Must be called after Initialize
	m_producerConsumer.SetProducerInterval(m_deviceSampleRate,m_framesPerBuffer);
	m_producerConsumer.SetConsumerInterval(m_deviceSampleRate,m_framesPerBuffer);

	//Start this immediately, before connect, so we don't miss any data
	m_producerConsumer.Start(true,true);

#endif

	return true;
}

void MorseGenDevice::readSettings()
{
	// +/- db gain required to normalize to fixed level input
	// Default is 0db gain, or a factor of 1.0
	m_normalizeIQGain = DB::dBToAmplitude(0);

	//Set defaults before calling DeviceInterfaceBase
	DeviceInterfaceBase::readSettings();
}

void MorseGenDevice::writeSettings()
{
	DeviceInterfaceBase::writeSettings();
}

bool MorseGenDevice::command(DeviceInterface::StandardCommands _cmd, QVariant _arg)
{
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

			//How often do we need to read samples from files to get framesPerBuffer at sampleRate
			nsPerBuffer = (1000000000.0 / m_deviceSampleRate) * m_framesPerBuffer;
			//qDebug()<<"nsPerBuffer"<<nsPerBuffer;
			elapsedTimer.start();

			return true;

		case Cmd_Stop:
			DeviceInterfaceBase::stopDevice();
			//Device specific code follows
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
			return DeviceInterfaceBase::DT_AUDIO_IQ_DEVICE;
		default:
			return DeviceInterfaceBase::get(_key, _option);
	}
}

bool MorseGenDevice::set(DeviceInterface::StandardKeys _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case Key_DeviceFrequency:
			return true; //Must be handled by device

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
	qint64 nsRemaining = nsPerBuffer - elapsedTimer.nsecsElapsed();

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
			elapsedTimer.start(); //Restart elapsed timer

			if ((producerFreeBufPtr = (CPX*)m_producerConsumer.AcquireFreeBuffer()) == NULL)
				return;
#if 0
			while (running) {
				//This ignores producer thread slices and runs as fast as possible to get samples
				//May be used for sample rates where thread slice is less than 1ms
				//Get data from device and put into producerFreeBufPtr
			}
#else
			//Get data from device and put into producerFreeBufPtr
			//Return and wait for next producer time slice
#endif
#if 0
			//For testing device sample format
			if (producerIBuf[i] > maxSample) {
				maxSample = producerIBuf[i];
				qDebug()<<"New Max sample "<<maxSample;
			}
			if (producerQBuf[i] > maxSample) {
				maxSample = producerQBuf[i];
				qDebug()<<"New Max sample "<<maxSample;
			}
			if (producerIBuf[i] < minSample) {
				minSample = producerIBuf[i];
				qDebug()<<"New Min sample "<<minSample;
			}
			if (producerQBuf[i] < minSample) {
				minSample = producerQBuf[i];
				qDebug()<<"New Min sample "<<minSample;
			}
#endif

			m_producerConsumer.ReleaseFilledBuffer();
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void MorseGenDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	unsigned char *consumerFilledBufferPtr;

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (m_producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((consumerFilledBufferPtr = m_producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//Process data in filled buffer and convert to Pebble format in consumerBuffer

				//perform.StartPerformance("ProcessIQ");
				processIQData(consumerBuffer,m_framesPerBuffer);
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
	if (optionUi != NULL)
		delete optionUi;

	optionUi = new Ui::MorseGenOptions();
	optionUi->setupUi(parent);
	parent->setVisible(true);

	optionUi->noiseBox->addItem("-40db", -40);
	optionUi->noiseBox->addItem("-45db", -45);
	optionUi->noiseBox->addItem("-50db", -50);
	optionUi->noiseBox->addItem("-55db", -55);
	optionUi->noiseBox->addItem("-60db", -60);
	optionUi->noiseBox->addItem("-60db", -65);
	optionUi->noiseBox->addItem("-60db", -70);
	optionUi->noiseBox->addItem("-60db", -75);
	optionUi->noiseBox->addItem("-60db", -80);
	optionUi->noiseBox->setCurrentText("-60db");

	optionUi->sourceBox_1->addItem("Sample1",0);
	optionUi->sourceBox_1->addItem("Sample2",1);
	optionUi->sourceBox_1->addItem("Sample3",2);
	optionUi->sourceBox_1->addItem("Sample4",3);
	optionUi->sourceBox_1->addItem("Table",4);

	optionUi->wpmBox_1->addItem("5 wpm", 5);
	optionUi->wpmBox_1->addItem("10 wpm", 10);
	optionUi->wpmBox_1->addItem("15 wpm", 15);
	optionUi->wpmBox_1->addItem("20 wpm", 20);
	optionUi->wpmBox_1->addItem("30 wpm", 30);
	optionUi->wpmBox_1->addItem("40 wpm", 40);
	optionUi->wpmBox_1->addItem("50 wpm", 50);
	optionUi->wpmBox_1->addItem("60 wpm", 60);
	optionUi->wpmBox_1->addItem("70 wpm", 70);
	optionUi->wpmBox_1->addItem("80 wpm", 80);

	optionUi->dbBox_1->addItem("-30db", -30);
	optionUi->dbBox_1->addItem("-35db", -35);
	optionUi->dbBox_1->addItem("-40db", -40);
	optionUi->dbBox_1->addItem("-45db", -45);
	optionUi->dbBox_1->addItem("-50db", -50);
	optionUi->dbBox_1->addItem("-55db", -55);
	optionUi->dbBox_1->addItem("-60db", -60);

	//2
	optionUi->sourceBox_2->addItem("Sample1",0);
	optionUi->sourceBox_2->addItem("Sample2",1);
	optionUi->sourceBox_2->addItem("Sample3",2);
	optionUi->sourceBox_2->addItem("Sample4",3);
	optionUi->sourceBox_2->addItem("Table",4);

	optionUi->wpmBox_2->addItem("5 wpm", 5);
	optionUi->wpmBox_2->addItem("10 wpm", 10);
	optionUi->wpmBox_2->addItem("15 wpm", 15);
	optionUi->wpmBox_2->addItem("20 wpm", 20);
	optionUi->wpmBox_2->addItem("30 wpm", 30);
	optionUi->wpmBox_2->addItem("40 wpm", 40);
	optionUi->wpmBox_2->addItem("50 wpm", 50);
	optionUi->wpmBox_2->addItem("60 wpm", 60);
	optionUi->wpmBox_2->addItem("70 wpm", 70);
	optionUi->wpmBox_2->addItem("80 wpm", 80);

	optionUi->dbBox_2->addItem("-30db", -30);
	optionUi->dbBox_2->addItem("-35db", -35);
	optionUi->dbBox_2->addItem("-40db", -40);
	optionUi->dbBox_2->addItem("-45db", -45);
	optionUi->dbBox_2->addItem("-50db", -50);
	optionUi->dbBox_2->addItem("-55db", -55);
	optionUi->dbBox_2->addItem("-60db", -60);

	//3
	optionUi->sourceBox_3->addItem("Sample1",0);
	optionUi->sourceBox_3->addItem("Sample2",1);
	optionUi->sourceBox_3->addItem("Sample3",2);
	optionUi->sourceBox_3->addItem("Sample4",3);
	optionUi->sourceBox_3->addItem("Table",4);

	optionUi->wpmBox_3->addItem("5 wpm", 5);
	optionUi->wpmBox_3->addItem("10 wpm", 10);
	optionUi->wpmBox_3->addItem("15 wpm", 15);
	optionUi->wpmBox_3->addItem("20 wpm", 20);
	optionUi->wpmBox_3->addItem("30 wpm", 30);
	optionUi->wpmBox_3->addItem("40 wpm", 40);
	optionUi->wpmBox_3->addItem("50 wpm", 50);
	optionUi->wpmBox_3->addItem("60 wpm", 60);
	optionUi->wpmBox_3->addItem("70 wpm", 70);
	optionUi->wpmBox_3->addItem("80 wpm", 80);

	optionUi->dbBox_3->addItem("-30db", -30);
	optionUi->dbBox_3->addItem("-35db", -35);
	optionUi->dbBox_3->addItem("-40db", -40);
	optionUi->dbBox_3->addItem("-45db", -45);
	optionUi->dbBox_3->addItem("-50db", -50);
	optionUi->dbBox_3->addItem("-55db", -55);
	optionUi->dbBox_3->addItem("-60db", -60);

	//4
	optionUi->sourceBox_4->addItem("Sample1",0);
	optionUi->sourceBox_4->addItem("Sample2",1);
	optionUi->sourceBox_4->addItem("Sample3",2);
	optionUi->sourceBox_4->addItem("Sample4",3);
	optionUi->sourceBox_4->addItem("Table",4);

	optionUi->wpmBox_4->addItem("5 wpm", 5);
	optionUi->wpmBox_4->addItem("10 wpm", 10);
	optionUi->wpmBox_4->addItem("15 wpm", 15);
	optionUi->wpmBox_4->addItem("20 wpm", 20);
	optionUi->wpmBox_4->addItem("30 wpm", 30);
	optionUi->wpmBox_4->addItem("40 wpm", 40);
	optionUi->wpmBox_4->addItem("50 wpm", 50);
	optionUi->wpmBox_4->addItem("60 wpm", 60);
	optionUi->wpmBox_4->addItem("70 wpm", 70);
	optionUi->wpmBox_4->addItem("80 wpm", 80);

	optionUi->dbBox_4->addItem("-30db", -30);
	optionUi->dbBox_4->addItem("-35db", -35);
	optionUi->dbBox_4->addItem("-40db", -40);
	optionUi->dbBox_4->addItem("-45db", -45);
	optionUi->dbBox_4->addItem("-50db", -50);
	optionUi->dbBox_4->addItem("-55db", -55);
	optionUi->dbBox_4->addItem("-60db", -60);

	//5
	optionUi->sourceBox_5->addItem("Sample1",0);
	optionUi->sourceBox_5->addItem("Sample2",1);
	optionUi->sourceBox_5->addItem("Sample3",2);
	optionUi->sourceBox_5->addItem("Sample4",3);
	optionUi->sourceBox_5->addItem("Table",4);

	optionUi->wpmBox_5->addItem("5 wpm", 5);
	optionUi->wpmBox_5->addItem("10 wpm", 10);
	optionUi->wpmBox_5->addItem("15 wpm", 15);
	optionUi->wpmBox_5->addItem("20 wpm", 20);
	optionUi->wpmBox_5->addItem("30 wpm", 30);
	optionUi->wpmBox_5->addItem("40 wpm", 40);
	optionUi->wpmBox_5->addItem("50 wpm", 50);
	optionUi->wpmBox_5->addItem("60 wpm", 60);
	optionUi->wpmBox_5->addItem("70 wpm", 70);
	optionUi->wpmBox_5->addItem("80 wpm", 80);

	optionUi->dbBox_5->addItem("-30db", -30);
	optionUi->dbBox_5->addItem("-35db", -35);
	optionUi->dbBox_5->addItem("-40db", -40);
	optionUi->dbBox_5->addItem("-45db", -45);
	optionUi->dbBox_5->addItem("-50db", -50);
	optionUi->dbBox_5->addItem("-55db", -55);
	optionUi->dbBox_5->addItem("-60db", -60);

	connect(optionUi->resetButton,SIGNAL(clicked(bool)),this,SLOT(resetButtonClicked(bool)));

	resetButtonClicked(true);
}

void MorseGenDevice::resetButtonClicked(bool clicked)
{
	Q_UNUSED(clicked);

	optionUi->enabledBox_1->setChecked(true);
	optionUi->freqencyEdit_1->setText("1000");
	optionUi->sourceBox_1->setCurrentIndex(0);
	optionUi->wpmBox_1->setCurrentIndex(3);
	optionUi->dbBox_1->setCurrentIndex(2);

	optionUi->enabledBox_2->setChecked(true);
	optionUi->freqencyEdit_2->setText("1500");
	optionUi->sourceBox_2->setCurrentIndex(1);
	optionUi->wpmBox_2->setCurrentIndex(4);
	optionUi->dbBox_2->setCurrentIndex(2);

	optionUi->enabledBox_3->setChecked(true);
	optionUi->freqencyEdit_3->setText("2000");
	optionUi->sourceBox_3->setCurrentIndex(2);
	optionUi->wpmBox_3->setCurrentIndex(5);
	optionUi->dbBox_3->setCurrentIndex(2);

	optionUi->enabledBox_4->setChecked(true);
	optionUi->freqencyEdit_4->setText("2500");
	optionUi->sourceBox_4->setCurrentIndex(3);
	optionUi->wpmBox_4->setCurrentIndex(6);
	optionUi->dbBox_4->setCurrentIndex(2);

	optionUi->enabledBox_5->setChecked(true);
	optionUi->freqencyEdit_5->setText("3000");
	optionUi->sourceBox_5->setCurrentIndex(4);
	optionUi->wpmBox_5->setCurrentIndex(7);
	optionUi->dbBox_5->setCurrentIndex(2);

}

void MorseGenDevice::generate()
{

	double gen1Freq = optionUi->freqencyEdit_1->text().toDouble();
	double gen1Amp = optionUi->dbBox_1->currentData().toDouble();
	quint32 gen1Wpm = optionUi->wpmBox_1->currentData().toUInt();
	quint32 gen1Rise = 5; //Add to UI?
	quint32 gen1Text = optionUi->sourceBox_1->currentData().toUInt();
	bool gen1Enabled = optionUi->enabledBox_1->isChecked();
	if (gen1Enabled) {
		m_morseGen1->setParams(gen1Freq,gen1Amp,gen1Wpm,gen1Rise);
		m_morseGen1->setTextOut(m_sampleText[gen1Text]);
	}

	double gen2Freq = optionUi->freqencyEdit_2->text().toDouble();
	double gen2Amp = optionUi->dbBox_2->currentData().toDouble();
	quint32 gen2Wpm = optionUi->wpmBox_2->currentData().toUInt();
	quint32 gen2Rise = 5; //Add to UI?
	quint32 gen2Text = optionUi->sourceBox_2->currentData().toUInt();
	bool gen2Enabled = optionUi->enabledBox_2->isChecked();
	if (gen2Enabled) {
		m_morseGen2->setParams(gen2Freq,gen2Amp,gen2Wpm,gen2Rise);
		m_morseGen2->setTextOut(m_sampleText[gen2Text]);
	}

	double gen3Freq = optionUi->freqencyEdit_3->text().toDouble();
	double gen3Amp = optionUi->dbBox_3->currentData().toDouble();
	quint32 gen3Wpm = optionUi->wpmBox_3->currentData().toUInt();
	quint32 gen3Rise = 5; //Add to UI?
	quint32 gen3Text = optionUi->sourceBox_3->currentData().toUInt();
	bool gen3Enabled = optionUi->enabledBox_3->isChecked();
	if (gen3Enabled) {
		m_morseGen3->setParams(gen3Freq,gen3Amp,gen3Wpm,gen3Rise);
		m_morseGen3->setTextOut(m_sampleText[gen3Text]);
	}

	double gen4Freq = optionUi->freqencyEdit_4->text().toDouble();
	double gen4Amp = optionUi->dbBox_4->currentData().toDouble();
	quint32 gen4Wpm = optionUi->wpmBox_4->currentData().toUInt();
	quint32 gen4Rise = 5; //Add to UI?
	quint32 gen4Text = optionUi->sourceBox_4->currentData().toUInt();
	bool gen4Enabled = optionUi->enabledBox_4->isChecked();
	if (gen4Enabled) {
		m_morseGen4->setParams(gen4Freq,gen4Amp,gen4Wpm,gen4Rise);
		m_morseGen4->setTextOut(m_sampleText[gen4Text]);
	}

	double gen5Freq = optionUi->freqencyEdit_5->text().toDouble();
	double gen5Amp = optionUi->dbBox_5->currentData().toDouble();
	quint32 gen5Wpm = optionUi->wpmBox_5->currentData().toUInt();
	quint32 gen5Rise = 5; //Add to UI?
	quint32 gen5Text = optionUi->sourceBox_5->currentData().toUInt();
	bool gen5Enabled = optionUi->enabledBox_5->isChecked();
	if (gen5Enabled) {
		m_morseGen5->setParams(gen5Freq,gen5Amp,gen5Wpm,gen5Rise);
		m_morseGen5->setTextOut(m_sampleText[gen5Text]);
	}

	double dbNoiseAmp = optionUi->noiseBox->currentData().toDouble();
	//Because noise is averaged, fudget +30db so it matches with generator values
	//ie -30db gen and -30db noise should be 0snr
	double noiseAmp = DB::dBToAmplitude(dbNoiseAmp+20);

	CPX cpx1,cpx2,cpx3,cpx4,cpx5,cpx6, out;
	//!!!!
	for (quint32 i=0; i<m_framesPerBuffer; i++) {
		if (gen1Enabled) {
			cpx1 = m_morseGen1->nextOutputSample();
			if (optionUi->fadeBox_1->isChecked()) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx1 *= ampRand;
			}
		}
		if (gen2Enabled) {
			cpx2 = m_morseGen2->nextOutputSample();
			if (optionUi->fadeBox_2->isChecked()) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx2 *= ampRand;
			}
		}
		if (gen3Enabled) {
			cpx3 = m_morseGen3->nextOutputSample();
			if (optionUi->fadeBox_3->isChecked()) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx3 *= ampRand;
			}
		}
		if (gen4Enabled) {
			cpx4 = m_morseGen4->nextOutputSample();
			if (optionUi->fadeBox_4->isChecked()) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx4 *= ampRand;
			}
		}
		if (gen5Enabled) {
			cpx5 = m_morseGen5->nextOutputSample();
			if (optionUi->fadeBox_5->isChecked()) {
				//Fading
				double dbRand = -rand() % c_dbFadeRange;
				double ampRand = DB::dBToAmplitude(dbRand);
				cpx5 *= ampRand;
			}
		}
		cpx6 = nextNoiseSample(noiseAmp);
		out = cpx1 + cpx2 + cpx3 + cpx4 + cpx5 + cpx6;
		//m_wavOutFile.WriteSamples(&out, 1);
	}
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
