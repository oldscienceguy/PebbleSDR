//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "receiver.h"
#include "qmessagebox.h"
#include "processstep.h"
#include "testbench.h"
#include "db.h"

/*
Core receiver logic, coordinates soundcard, fft, demod, etc
*/
Receiver::Receiver(ReceiverWidget *rw, QMainWindow *main)
{
	m_useDemodDecimator = true;
	m_useDemodWfmDecimator = true;

	//Read ini file or set defaults if no ini file exists
	m_settings = global->settings;
	m_plugins = new Plugins(this,m_settings);

	m_mainWindow = main;
	m_receiverWidget = rw;

	QRect pos;
	pos.setX(m_settings->m_windowXPos);
	pos.setY(m_settings->m_windowYPos);
	pos.setWidth(m_settings->m_windowWidth);
	pos.setHeight(m_settings->m_windowHeight);
	//setGeometry breaks in QT 5.6 and doesn't adjust invalid width and height values
	if (m_settings->m_windowWidth < 1 || m_settings->m_windowHeight < 1) {
		//Just move window and accept default width and height
		m_mainWindow->move(pos.x(),pos.y());
	} else {
		m_mainWindow->setGeometry(pos);
	}

	m_recordingPath = global->appDirPath;
    //Make sure directory exists, else will get file error
	QDir dir(m_recordingPath);
    dir.mkdir("PebbleRecordings");
	m_recordingPath += "/PebbleRecordings/";

	//ReceiverWidget link back
	m_receiverWidget->setReceiver(this);
	QStringList welcome;

	welcome <<
        global->revision <<
		"Copyright 2010 - 2016 Richard Landsman N1DDY (pebblesdr@gmail.com)" <<
		"See PebbleGPL.txt for GPL license details" <<
		"https://github.com/oldscienceguy/PebbleSDR.git";

	m_receiverWidget->setMessage( welcome);

	m_powerOn = false;
	m_mute = false;
	m_isRecording = false;

	m_presets = NULL;

	m_sdr = NULL;
	m_mixer = NULL;
	m_demod = NULL;
	m_audioOutput = NULL;
	m_bpFilter = NULL;
	m_noiseBlanker = NULL;
	m_noiseFilter = NULL;
	m_signalStrength = NULL;
	m_signalSpectrum = NULL;
	m_agc = NULL;
	m_iqBalance = NULL;
	m_iDigitalModem = NULL;
	m_workingBuf = NULL;
	m_sampleBuf = NULL;
	m_audioBuf = NULL;
	m_dbSpectrumBuf = NULL;
	m_demodDecimator = NULL;
	m_demodWfmDecimator = NULL;

	m_sdrOptions = new SdrOptions();
	connect(m_sdrOptions,SIGNAL(restart()),this,SLOT(restart()));

    //qDebug()<<plugins->GetPluginNames();

	//mainMenu = main->menuBar(); //This gives us a menuBar, but only in the main window
	m_mainMenu = new QMenuBar(); //When parent = 0 this gives us a menuBar that can be used in any window
	m_developerMenu = new QMenu("Developer");
	m_developerMenu->addAction("TestBench",this,SLOT(openTestBench()));
	m_developerMenu->addAction("Device Info",this,SLOT(openDeviceAboutBox()));
	m_mainMenu->addAction(m_developerMenu->menuAction());
	m_helpMenu = new QMenu("Help");
	m_helpMenu->addAction("About",this,SLOT(openAboutBox())); //This will be auto-merged with Application menu on Mac
	m_helpMenu->addAction("Readme",this,SLOT(openReadMeWindow()));
	m_helpMenu->addAction("Credits", this, SLOT(openGPLWindow()));
	m_mainMenu->addAction(m_helpMenu->menuAction());

	m_readmeView = NULL;
	m_gplView = NULL;

	//ReceiverWidget connections
	connect(m_receiverWidget,SIGNAL(demodChanged(DeviceInterface::DemodMode)), this,
			SLOT(demodModeChanged(DeviceInterface::DemodMode)));
	connect(m_receiverWidget,SIGNAL(audioGainChanged(int)), this, SLOT(audioGainChanged(int)));
	connect(m_receiverWidget,SIGNAL(agcThresholdChanged(int)), this, SLOT(agcThresholdChanged(int)));
	connect(m_receiverWidget,SIGNAL(squelchChanged(double)), this, SLOT(squelchChanged(double)));
	connect(m_receiverWidget,SIGNAL(widgetMixerChanged(int)), this, SLOT(mixerChanged(int)));
	connect(m_receiverWidget,SIGNAL(filterChanged(int, int)), this, SLOT(filterChanged(int, int)));
	connect(m_receiverWidget,SIGNAL(anfChanged(bool)), this, SLOT(anfChanged(bool)));
	connect(m_receiverWidget,SIGNAL(nb1Changed(bool)), this, SLOT(nb1Changed(bool)));
	connect(m_receiverWidget,SIGNAL(nb2Changed(bool)), this, SLOT(nb2Changed(bool)));
	connect(m_receiverWidget,SIGNAL(agcModeChanged(AGC::AgcMode)), this, SLOT(agcModeChanged(AGC::AgcMode)));
	connect(m_receiverWidget,SIGNAL(muteChanged(bool)), this, SLOT(muteChanged(bool)));
}
bool Receiver::turnPowerOn()
{
	m_powerOn = true;

	//We need SDR* for transition
	m_sdr = global->sdr;
	if (m_sdr == NULL)
        return false; //Means something is wrong with plugins,

	//If plugin has multiple devices, we need to set the last one used
	m_sdr->set(DeviceInterface::Key_DeviceNumber,global->settings->m_sdrDeviceNumber);

    //Setup callback for device plugins to use when they have new IQ data
    using namespace std::placeholders;
    //function template must match exactly the method that is used in bind()
    //std::function<void(CPX *, quint16)> cb = std::bind(&Receiver::ProcessIQData, _receiver, std::placeholders::_1, std::placeholders::_2);
    //bind(Method ptr, object, arg1, ... argn)

	m_sdr->command(DeviceInterface::Cmd_ReadSettings,0); //Always start with most current
	if (!m_sdr->initialize(std::bind(&Receiver::processIQData, this, _1, _2),
						 std::bind(&Receiver::processBandscopeData, this, _1, _2),
						 std::bind(&Receiver::processAudioData, this, _1, _2),
						 m_settings->m_framesPerBuffer)) {
		turnPowerOff();
		return false;
	}

	if (!m_sdr->command(DeviceInterface::Cmd_Connect,0)){
        QMessageBox::information(NULL,"Pebble","SDR device is not connected");
		turnPowerOff();
		return false;
	}

	m_sampleRate = m_demodSampleRate = m_sdr->get(DeviceInterface::Key_SampleRate).toInt();
	m_framesPerBuffer = m_demodFrames = m_settings->m_framesPerBuffer;
	global->testBench->initProcessSteps(m_sampleRate, m_framesPerBuffer);

    //These steps work on full sample rates
	m_noiseBlanker = new NoiseBlanker(m_sampleRate,m_framesPerBuffer);
	//Todo: Don't need Mixer anymore
	m_mixer = new Mixer(m_sampleRate, m_framesPerBuffer);
	m_iqBalance = new IQBalance(m_sampleRate,m_framesPerBuffer);
	m_iqBalance->enableStep(m_sdr->get(DeviceInterface::Key_IQBalanceEnabled).toBool());
	m_iqBalance->setGainFactor(m_sdr->get(DeviceInterface::Key_IQBalanceGain).toDouble());
	m_iqBalance->setPhaseFactor(m_sdr->get(DeviceInterface::Key_IQBalancePhase).toDouble());

	m_dcRemove = new DCRemoval(m_sampleRate, m_framesPerBuffer);
	m_dcRemoveMode = m_sdr->get(DeviceInterface::Key_RemoveDC).toBool();
	m_dcRemove->enableStep(m_dcRemoveMode);

    /*
     * Decimation strategy
	 * We try to keep decimation rates at a factor of 2 to avoid needing to resample (expensive)
	 *
	 * deviceSampleRate: Used internally by device for actual sample rate from hardware.
	 *	May be decimated by device to a lower rate before being passed to receiver
	 * receiverSampleRate: Sample rate into the receiver DSP chain  FFT for raw spectrum uses this rate
     * demodSampleRate = DSP processing rate.  This is calcualted to be close to, but greater than, the final filter bandwidth
	 *  This results in the most optimal DSP steps, since we're never processing much more than the final bandwidth needed for output
	 * audioOutputRate = This post DSP decimation (resampling?) takes us to the final audio output rate.
     *  It should be just enough for the fidelity we want so we don't waste audio subsystem cpu
     *
     * FM is the same, except the DSP bandwidth is initially much higher
     * demodWfmSampleRate is the initial DSP rate, typically 300k bw
     * demodWfmSampleRate then takes us down another step for the remainder of the DSP chain
     * audioOutputRate = same as above
     */

	m_fractResampler.Init(m_framesPerBuffer);
    //Calculates the number of decimation states and returns converted sample rate
    //SetDataRate() doesn't downsample below 256k?


    //SetDataRate MaxBW should be driven by our filter selection, ie width of filter
	//For now just set to widest filter, 30k for FMN or 15k bw
	//qint32 maxBw = Demod::demodInfo[DeviceInterface::dmFMN].maxOutputBandWidth;
	if (m_useDemodDecimator) {
		//One rate for am, cw, nfm modes.  Eventually we can get more refined for each mode
		m_demodDecimator = new Decimator(m_sampleRate,m_framesPerBuffer);
		m_demodSampleRate = m_demodDecimator->buildDecimationChain(m_sampleRate, 30000);
	} else {
		//bw is 1/2 of total bw, so 10k for 20k am
		m_demodSampleRate = m_downConvert1.SetDataRate(m_sampleRate, 15000);

	}

	//audioOutRate can be fixed by remote devices, default to 11025 which is a rate supported by QTAudio and PortAudio on Mac
	m_audioOutRate = m_sdr->get(DeviceInterface::Key_AudioOutputSampleRate).toUInt();
	//audioOutRate = demodSampleRate;

	//For FMStereo, sample rate should be close to 300k
    //Different decimation technique than SetDataRate
	//MaxBw = 1/2 band pass filter, ie if 30kbp, then maxBw will be 15k.
	//maxBw = Demod::demodInfo[DeviceInterface::dmFMS].maxOutputBandWidth;

	if (m_useDemodWfmDecimator) {
		m_demodWfmDecimator = new Decimator(m_sampleRate, m_framesPerBuffer);
		m_demodWfmSampleRate = m_demodWfmDecimator->buildDecimationChain(m_sampleRate,200000);
	} else {
		//downConvert is hard wired to only use 51 tap filters and stop as soon as decimated
		//sample rate is less than 400k
		m_demodWfmSampleRate = m_downConvertWfm1.SetDataRateSimple(m_sampleRate, 200000);
	}

    //We need original sample rate, and post mixer sample rate for zoomed spectrum
	m_signalSpectrum = new SignalSpectrum(m_sampleRate, m_demodSampleRate, m_framesPerBuffer);

    //Init demod with defaults
    //Demod uses variable frame size, up to framesPerBuffer
    //Demod can also run at different sample rates, high for FMW and lower for rest
    //Todo: How to handle this for filters which need explicit sample rate
    //demod = new Demod(downSampleRate,framesPerBuffer);
	m_demod = new Demod(m_demodSampleRate, m_demodWfmSampleRate,m_framesPerBuffer); //Can't change rate later, fix

    //demodFrames = demodSampleRate / (1.0 * sampleRate) * framesPerBuffer; //Temp Hack
    //post mixer/downconvert frames are now the same as pre
	m_demodFrames = m_framesPerBuffer;

    //Sets the Mixer NCO frequency
    //downConvert.SetFrequency(-RDS_FREQUENCY);

	m_workingBuf = CPX::memalign(m_framesPerBuffer);
	m_sampleBuf = CPX::memalign(m_framesPerBuffer);
	m_audioBuf = CPX::memalign(m_framesPerBuffer);
	m_sampleBufLen = 0;
	m_dbSpectrumBuf = new double[m_framesPerBuffer];

	m_presets = new Presets(m_receiverWidget);

    //These steps work on demodSampleRate rates

    //WIP, testing QT audio as alternative to PortAudio
	m_audioOutput = Audio::Factory(NULL, m_demodFrames);

	m_noiseFilter = new NoiseFilter(m_demodSampleRate,m_demodFrames);
    //signalStrength is used by normal demod and wfm, so buffer len needs to be max
    //Calls to ProcessBlock will pass post decimation len
	m_signalStrength = new SignalStrength(m_demodSampleRate,m_framesPerBuffer);

	//disconnect in off
	connect(m_signalStrength, SIGNAL(newSignalStrength(double,double,double,double,double)),
			m_receiverWidget, SLOT(newSignalStrength(double,double,double,double,double)));

	m_iDigitalModem = NULL;

	m_bpFilter = new BandPassFilter(m_demodSampleRate, m_demodFrames);
	m_bpFilter->enableStep(true);
	
	m_agc = new AGC(m_demodSampleRate, m_demodFrames);

	m_mixerFrequency = 0;
	m_demodFrequency = 0;
	m_lastDemodFrequency = 0;

	m_converterMode = m_sdr->get(DeviceInterface::Key_ConverterMode).toBool();
	m_converterOffset = m_sdr->get(DeviceInterface::Key_ConverterOffset).toDouble();

	//This should always be last because it starts samples flowing through the processBlocks
	m_audioOutput->StartOutput(m_sdr->get(DeviceInterface::Key_OutputDeviceName).toString(), m_audioOutRate);
	m_sdr->command(DeviceInterface::Cmd_Start,0);

	//Test goertzel
	m_testGoertzel = new Goertzel(m_demodSampleRate, m_demodFrames);
	//m_testGoertzel->setTargetSampleRate(8000);
	m_testGoertzel->setTargetSampleRate(m_demodSampleRate);
	quint32 estN = m_testGoertzel->estNForShortestBit(5.0);
	//estN = 512; //For testing
	m_testGoertzel->setFreq(800, estN, 2, 2);

    //Don't set title until we connect and start.
    //Some drivers handle multiple devices (RTL2832) and we need connection data
	QTimer::singleShot(200,this,SLOT(setWindowTitle()));
	return true;
}

void Receiver::setWindowTitle()
{
	if (m_sdr == NULL)
        return;
	qint16 number = m_sdr->get(DeviceInterface::Key_DeviceNumber).toInt();
	QString devName = m_sdr->get(DeviceInterface::Key_DeviceName,number).toString();
    //In some cases, unknown, activeWindow() can return NULL
    QWidget *win = QApplication::activeWindow();
    if (win != NULL)
        win->setWindowTitle("Pebble II: " + devName);

}

void Receiver::openTestBench()
{
	TestBench *testBench = global->testBench;

	testBench->init(); //Sets up last device settings used
	//Anchor in upper left
	testBench->move(0,0);

	testBench->resetProfiles();
	testBench->addProfile("Incoming",TB_RAW_IQ);
	testBench->addProfile("Post Mixer",TB_POST_MIXER);
	testBench->addProfile("Post Bandpass",TB_POST_BP);
	testBench->addProfile("Post Demod",TB_POST_DEMOD);
	testBench->addProfile("Post Decimator",TB_POST_DECIMATE);

	testBench->setVisible(true);

	//Keep focus on us
	m_mainWindow->activateWindow(); //Makes main active
	m_mainWindow->raise(); //Brings it to the top
	m_mainWindow->setFocus(); //Makes sure it has keyboard focus

}

void Receiver::openAboutBox()
{
	QString welcome;

	welcome += "About Pebble SDR";
	welcome += "\n\n";
	welcome += global->revision;
	welcome += "\nCopyright 2010, 2011, 2012, 2013, 2014, 2015 Richard Landsman N1DDY (pebblesdr@gmail.com)";
	welcome += "\nSee PebbleGPL.txt for GPL license details";
	welcome += "\nhttps://github.com/oldscienceguy/PebbleSDR.git";
	welcome += "\nCompiled with Qt Version ";
	welcome += QT_VERSION_STR;
#ifdef USE_FFTW
	welcome += "\nDSP = FFTW";
#endif
#ifdef USE_FFTCUTE
	welcome += "\nDSP = Cute";
#endif
#ifdef USE_FFTOOURA
	welcome += "\nDSP = Ooura";
#endif
#ifdef USE_FFTACCELERATE
	welcome += "\nDSP = Mac Accelerate";
#endif
#ifdef USE_QT_AUDIO
	welcome += "\nAUDIO = QT Audio";
#endif
#ifdef USE_PORT_AUDIO
	welcome += "\nAUDIO = Port Audio";
#endif

	QMessageBox::about(this->m_mainWindow,"About Pebble",welcome);
}

void Receiver::openDeviceAboutBox()
{
	if (!m_powerOn)
		return;
	QString about;
	about += "\nAbout Device\n\n";
	about += m_sdr->get(DeviceInterface::Key_DeviceName).toString();
	about += "\n";
	about += m_sdr->get(DeviceInterface::Key_DeviceDescription).toString();
	about += "\nFrequency Range ";
	about += QString::number(m_sdr->get(DeviceInterface::Key_LowFrequency).toDouble()/1000000.0);
	about += " to ";
	about += QString::number(m_sdr->get(DeviceInterface::Key_HighFrequency).toDouble()/1000000.0);
	about += " mHz\n";
	if (m_sdr->get(DeviceInterface::Key_DeviceSlave).toBool()) {
		about += "Device is in Slave mode to server";
	}

	QMessageBox::about(this->m_mainWindow,"About Device",about);

}

void Receiver::openReadMeWindow()
{
	if (m_readmeView == NULL)
		m_readmeView = new QWebEngineView(0);
	//readmeView->load(QUrl("http://amazon.com"));

	m_readmeView->load(QUrl::fromLocalFile(global->pebbleDataPath+"readme.html"));
	m_readmeView->show();
}

void Receiver::openGPLWindow()
{
	if (m_gplView == NULL)
		m_gplView = new QWebEngineView(0);
	//readmeView->load(QUrl("http://amazon.com"));

	m_gplView->load(QUrl::fromLocalFile(global->pebbleDataPath+"gpl.html"));
	//gplView->setGeometry(0,0,100,100);
	m_gplView->show();

}

//Delete everything that's based on settings, so we can change receivers, sound cards, etc
//ReceiverWidget initiates the power off and call us
bool Receiver::turnPowerOff()
{

	m_powerOn = false;

    //If we're closing app, don't bother to set title, crashes sometime
    QWidget *app = QApplication::activeWindow();
	if (app != NULL) {
		//Not Closing, just power off
        app->setWindowTitle("Pebble II");
	}

	if (m_sdrOptions != NULL)
		m_sdrOptions->showSdrOptions(m_sdr, false);

	if (m_isRecording) {
		m_recordingFile.Close();
		m_isRecording = false;
    }
	//Carefull with order of shutting down
	//if (settings->sdrDevice == DeviceInterface::SDR_IQ_USB) {
    //}

	//Stop incoming samples first
	if (m_sdr != NULL) {
		m_sdr->command(DeviceInterface::Cmd_Stop,0);
    }
	if (m_audioOutput != NULL)
		m_audioOutput->Stop();

	m_iDigitalModem = NULL;

	//Now clean up rest
	if (m_sdr != NULL){
        //Save any run time settings
		m_sdr->set(DeviceInterface::Key_LastFrequency,m_frequency);
		m_sdr->command(DeviceInterface::Cmd_WriteSettings,0); //Always save last mode, last freq, etc

		m_sdr->command(DeviceInterface::Cmd_Disconnect,0);
		//Make sure SDR threads are fully stopped, otherwise processIQ objects below will crash in destructor
		//Active SDR object survives on/off and is kept in global.  Do not delete
		//delete sdr;
		m_sdr = NULL;
	}
	if (m_demodDecimator != NULL) {
		delete m_demodDecimator;
		m_demodDecimator = NULL;
	}
	if (m_demodWfmDecimator != NULL) {
		delete m_demodWfmDecimator;
		m_demodWfmDecimator = NULL;
	}
	if (m_demod != NULL) {
		delete m_demod;
		m_demod = NULL;
	}
	if (m_mixer != NULL) {
		delete m_mixer;
		m_mixer = NULL;
	}
	if (m_bpFilter != NULL) {
		delete m_bpFilter;
		m_bpFilter = NULL;
	}
	if (m_noiseBlanker != NULL) {
		delete m_noiseBlanker;
		m_noiseBlanker = NULL;
	}
	if (m_noiseFilter != NULL) {
		delete m_noiseFilter;
		m_noiseFilter = NULL;
	}

	disconnect(m_signalStrength, SIGNAL(newSignalStrength(double,double,double,double,double)),
			m_receiverWidget, SLOT(newSignalStrength(double,double,double,double,double)));
	if (m_signalStrength != NULL) {
		delete m_signalStrength;
		m_signalStrength = NULL;
	}
	if (m_signalSpectrum != NULL) {
		delete m_signalSpectrum;
		m_signalSpectrum = NULL;
	}
	if (m_agc != NULL) {
		delete m_agc;
		m_agc = NULL;
	}
	if(m_iqBalance != NULL) {
		delete m_iqBalance;
		m_iqBalance=NULL;
	}
    //Making this last to work around a shut down order problem with consumer threads
	if (m_audioOutput != NULL) {
		delete m_audioOutput;
		m_audioOutput = NULL;
    }

#if(0)
	if (iqBalanceOptions != NULL) {
		if (iqBalanceOptions->isVisible()) {
			iqBalanceOptions->setHidden(true);
		}
		delete iqBalanceOptions;
		iqBalanceOptions = NULL;
	}
#endif

	if (m_workingBuf != NULL) {
		free (m_workingBuf);
		m_workingBuf = NULL;
    }
	if (m_sampleBuf != NULL) {
		free (m_sampleBuf);
		m_sampleBuf = NULL;
    }
	if (m_audioBuf != NULL) {
		free (m_audioBuf);
		m_audioBuf = NULL;
    }
	if (m_dbSpectrumBuf != NULL) {
		free (m_dbSpectrumBuf);
		m_dbSpectrumBuf = NULL;
	}
    return true;
}
void Receiver::close()
{
	turnPowerOff();
	if (global->testBench->isVisible())
		global->testBench->setVisible(false);
	if (m_readmeView != NULL && m_readmeView->isVisible())
		m_readmeView->setVisible(false);
	if (m_gplView != NULL && m_gplView->isVisible())
		m_gplView->setVisible(false);
	m_settings->writeSettings();
}
Receiver::~Receiver(void)
{
	//Delete all the plugins
	if (m_plugins != NULL)
		delete m_plugins;
	//settings is deleted by pebbleii
	//plugins (sdr) are deleted by ~Plugins()
}

//Connected to ReceiverWidget REC button
/*
 * Records to standard file name in data directory
 * If file exists, appends number until finds one that doesn't
 * Simple, no prompt for file name, can be instantly toggled on/off
 */
void Receiver::recToggled(bool on)
{
	if (!m_powerOn)
        return;

    if (on) {
        //We could use QTemporaryFile to get a unique file name, but need more control
		QString baseName = "PebbleIQ_";
		baseName += QString::number(m_frequency/1000,'f',0);
		baseName +="kHz_";
		baseName += QString::number(m_sampleRate/1000);
		baseName += "kSps_";
        QFileInfo fInfo;
        for (int i=1; i<1000; i++) {
			m_recordingFileName = m_recordingPath + baseName + QString::number(i) + ".wav";
			fInfo.setFile(m_recordingFileName);
            if (!fInfo.exists())
                break; //Got a unique name
            //When we overrun counter, last filename will be continually overwritten
        }

		m_recordingFile.OpenWrite(m_recordingFileName, m_sampleRate, m_frequency, m_demod->demodMode(), 0);
		m_isRecording = true;
    } else {
		m_recordingFile.Close();
		m_isRecording = false;
    }
}

bool Receiver::togglePower(bool _on)
{

	if (_on)
	{
		return turnPowerOn();
	}else {
		//return Off();
		close(); //Same as Off but writes settings
		return true;
	}
}
//Triggered by settings change or something else that requires us to reset all objects
void Receiver::restart()
{
	//If power off, do nothing
	if (m_powerOn) {
		//Tell receiver widget to power off, then on.
		//Receiver widget controls logic and will call receiver as necessary
		m_receiverWidget->powerToggled(false);
        //Just power off for now
        //receiverWidget->powerToggled(true);
	}
}
double Receiver::setSDRFrequency(double fRequested, double fCurrent)
{
	if (m_sdr==NULL)
		return 0;
	m_demod->resetDemod();

	//There's a problem with 'popping' when we are scrolling freq up and down
	//Especially in large increments
	//si570 supports 'smooth tuning' which changes freq without stopping and starting the chip
	//Range is +/- 3500 ppm
	//So for 40m, max smooth tuning increment is 2 * 3500 * 7 = 49khz
	//If increment is greater than 'smooth', we need to do something to stop pops
#if(0)
	double smoothLimit = fCurrent * 0.0035;
	bool restart = false;
	if (abs(fRequested - fCurrent) >= smoothLimit) {
		soundCard->Pause();
		restart = true;
	}
#endif
	if (m_converterMode) {
		fRequested += m_converterOffset;
	}
	if (m_sdr->set(DeviceInterface::Key_DeviceFrequency,fRequested)) {
		m_frequency = fRequested;
	} else {
		//Failed, return current freq without change
		fRequested =  fCurrent;
	}

	m_lastDemodFrequency = m_demodFrequency;
	m_demodFrequency = m_frequency + m_mixerFrequency;
	return fRequested;
}

//Called by ReceiverWidget to sets demod mode and default bandpass filter for each mode
void Receiver::demodModeChanged(DeviceInterface::DemodMode _demodMode)
{
	if(m_demod != NULL) {
		if (_demodMode == DeviceInterface::dmFMM || _demodMode == DeviceInterface::dmFMS)
			m_signalSpectrum->setSampleRate(m_sampleRate, m_demodWfmSampleRate);
        else
			m_signalSpectrum->setSampleRate(m_sampleRate, m_demodSampleRate);

        //Demod will return new demo
		m_demod->setDemodMode(_demodMode, m_sampleRate, m_demodSampleRate);
		m_sdr->set(DeviceInterface::Key_LastDemodMode,_demodMode);
		m_sampleBufLen = 0;
	}
	if (m_iDigitalModem != NULL) {
		m_iDigitalModem->setDemodMode(_demodMode);
    }
}
//Called by ReceiverWidget when UI changes filter settings
void Receiver::filterChanged(int lo, int hi)
{
	if (m_demod == NULL)
		return;
	m_bpFilter->setBandPass(lo,hi);
	m_demod->setBandwidth(hi - lo);
}
//Called by ReceiverWidget when UI changes ANF
void Receiver::anfChanged(bool b)
{
	m_noiseFilter->enableStep(b);
}
//Called by ReceiverWidget when UI changes NB
void Receiver::nb1Changed(bool b)
{
	m_noiseBlanker->setNbEnabled(b);
}
//Called by ReceiverWidget when UI changed NB2
void Receiver::nb2Changed(bool b)
{
	m_noiseBlanker->setNb2Enabled(b);
}
//Called by ReceiverWidget when UI changes AGC, returns new threshold for display
void Receiver::agcModeChanged(AGC::AgcMode _mode)
{
	m_agc->setAgcMode(_mode);
	//AGC sets a default gain with mode
	//return m_agc->getAgcThreshold();
}
//Called by ReceiverWidget
void Receiver::agcThresholdChanged(int g)
{
	m_agc->setAgcThreshold(g);
}
//Called by ReceiverWidget
void Receiver::muteChanged(bool b)
{
	m_mute = b;
}
//Called by ReceiverWidget
void Receiver::audioGainChanged(int g)
{
	//Convert dbGain to amplitude so we can use it to scale
	//No magic DSP math, just found something that sounds right to convert slider 0-100 reasonable vol
	//gain = DB::dbToAmplitude(25 + g * 0.35);
	m_gain = g; //Adjust in audio classes
}
//Called by ReceiverWidget
void Receiver::squelchChanged(double s)
{
	m_squelchDb = s;
}
//Called by ReceiverWidget
void Receiver::mixerChanged(int f)
{
	m_mixerFrequency = f;

	if (m_useDemodDecimator && m_mixer != NULL) {
		m_mixer->SetFrequency(f);
		m_demod->resetDemod();
	} else {
		//Only if using downConvert
		m_downConvert1.SetFrequency(m_mixerFrequency);
		m_downConvertWfm1.SetFrequency(m_mixerFrequency);
		m_demod->resetDemod();
	}
	m_lastDemodFrequency = m_demodFrequency;
	m_demodFrequency = m_frequency + m_mixerFrequency;
}

void Receiver::sdrOptionsPressed()
{
    //2 states, power on and off
    //If power on, use active sdr to make changes
	if (m_sdr == NULL) {
        //Power is off, create temporary one so we can set settings
		m_sdr = global->sdr;
    }
	m_sdrOptions->showSdrOptions(m_sdr, true);
}
//Called by receiver widget with device selection changes to make sure any open options ui is closed
void Receiver::closeSdrOptions()
{
	if (m_sdr != NULL) {
		m_sdrOptions->showSdrOptions(m_sdr, false);
		m_sdr = NULL;
    }
}

//processing flow for audio samples, called from SoundCard PortAudio callback
//Note: frameCount is actual # samples from audio, may be less than framesPerBuffer
/*
Each step in the receiver chain is of the form inBufferToNextStep = thisStep(outBufferFromLastStep)
Since each step mainaintains it's own out buffer and the in buffer is NEVER modified by a step, every
step looks like 'nextBuffer = thisStep(nextBuffer)'
This allows us to comment out steps, add new steps, reorder steps without having to worry about
keeping track of buffers.
Since we NEVER modify an IN buffer, we can also just return the IN buffer as the OUT buffer in a step
that is disabled and not do a useless copy from IN to OUT.
*/

//New ProcessBlock for device plugins
void Receiver::processIQData(CPX *in, quint16 numSamples)
{
	if (m_sdr == NULL || !m_powerOn)
		return;

	CPX *nextStep = in;

	//Number of samples in the buffer before each step
	//Will change with decimation and resampling
	//Use instead of numSamples
	quint32 numStepSamples = numSamples;

    /*
     * Critical timing!!
     * At 48k sample rate and 2048 samples per block, we need to process 48000/2048, rounded up = 24 blocks per second
     * So each iteration of this function must be significantly less than 1/24 seconds (416ms) to keep up
     * Max time = 1 / (SampleRate/BlockSize)
     *
     *  Sample Rate     Max time
     *  48k             .042666s
     *  96k             .021333s
     *  192k            .010666s
     *  200k            .010240
     *  250k            .008192
     *  500k            .004096
     *  1.8m            .000137 (max rate of SDR-IP or AFEDRI)
     *
     *  2/17/13         .007035 Actual- 200k max!
     */

    //global->perform.StartPerformance("ProcessBlock");

	//We make an assumption that the number of samples we get is always equal to what we asked for.
	//This is critical, since buffers are created and loops are generated using framesPerBuffer
	//If frameCount is ever not equal to framesPerBuffer, we have a problem
	//if (frameCount != framesPerBuffer)
	//	audio->inBufferUnderflowCount++; //Treat like in buffer underflow

    //Inject signals from test bench if desired
	global->testBench->genSweep(numSamples, nextStep);
	global->testBench->genNoise(numSamples, nextStep);

	if (m_isRecording)
		m_recordingFile.WriteSamples(nextStep,numSamples);

	global->testBench->displayData(numSamples,nextStep,m_sampleRate,TB_RAW_IQ);

    //global->perform.StartPerformance();
    /*
      3 step decimation
      1. Decimate to max spectrum, ie RTL2832 samples at >1msps
      2. Decimate to pre demod steps
      3. Decimate to demod steps
      4. Decimate to final audio
    */

	if (m_dcRemove->isEnabled()) {
		nextStep = m_dcRemove->process(nextStep, numSamples);
	}

	//Adj IQ to get 90deg phase and I==Q gain
	nextStep = m_iqBalance->ProcessBlock(nextStep);

	//Noise blankers
	nextStep = m_noiseBlanker->ProcessBlock(nextStep);
	nextStep = m_noiseBlanker->ProcessBlock2(nextStep);

    //Spectrum display, in buffer is not modified
	m_signalSpectrum->unprocessed(nextStep, numSamples);
    //global->perform.StopPerformance(100);

	//Signal (Specific frequency) processing

	//DOWNSAMPLED from here on
	//Todo: Variable output sample rate based on mode, requires changing output stream when mode changes
	/*
	 12khz	AM, SSB, ECSS, CW
	 24khz	FMN, DRM
	 48khz	FMW
	 */
    /*
      Problem: Broadcast Stereo FM has a freq deviation of 75khz or 150khz bandwidth
      At baseband, this is 0-150khz and should have a minimum sample rate of 300k per Nyquist
      When we decimate and filter, we lose most of the signal!
      This is not a problem for NBFM or other signals which don't have such a large bandwidth

      Solution, decimate before demod to get to 300k where sampleRate > 300k, after demod where sampleRate <300k
    */
    double resampRate;

	if (m_lastDemodFrequency != m_demodFrequency) {
		m_signalStrength->reset(); //Start new averages
		m_lastDemodFrequency = m_demodFrequency;
	}

    //global->perform.StartPerformance();
	if (m_demod->demodMode() == DeviceInterface::dmFMM || m_demod->demodMode() == DeviceInterface::dmFMS) {
        //These steps are at demodWfmSampleRate NOT demodSampleRate
        //Special handling for wide band fm

		//global->perform.StartPerformance("wfm decimator");
		if (!m_useDemodWfmDecimator) {
			//Replaces Mixer.cpp and mixes and decimates
			// InLength must be a multiple of 2^N where N is the maximum decimation by 2 stages expected.
			//We need a separated input/output buffer for downConvert
			numStepSamples = m_downConvertWfm1.ProcessData(numStepSamples,nextStep,m_workingBuf);
		} else {
			nextStep = m_mixer->ProcessBlock(nextStep);
			numStepSamples = m_demodWfmDecimator->process(nextStep, m_workingBuf, numStepSamples);
		}

		//global->perform.StopPerformance(1000);

        //We are always decimating by a factor of 2
        //so we know we can accumulate a full fft buffer at this lower sample rate
		for (int i=0; i<numStepSamples; i++) {
			m_sampleBuf[m_sampleBufLen++] = m_workingBuf[i];
        }

#if 1
		if (m_sampleBufLen < m_framesPerBuffer)
            return; //Nothing to do until we have full buffer
		numStepSamples = m_framesPerBuffer;
#endif
		m_sampleBufLen = 0;
        //Create zoomed spectrum
		m_signalSpectrum->zoomed(m_sampleBuf, numStepSamples);
		nextStep = m_sampleBuf;

		//nextStep = m_signalStrength->tdEstimate(nextStep, numStepSamples,true,true);
		//Calc this here, at lower sample rate, for efficiency
		//Uses original unprocessed spectrum data
		//There is no bandpass filter for FM, so hi and low are hard coded
		m_avgDb = m_signalStrength->fdEstimate(m_signalSpectrum->getUnprocessed(),m_signalSpectrum->binCount(),
				m_signalSpectrum->getSampleRate(),-100000, 100000, m_mixerFrequency);
		//Squelch based on last avgerages
		if (m_avgDb < m_squelchDb) {
			//We don't need to do any other processing if signal is below squelch
			return;
		}

		nextStep = m_demod->processBlock(nextStep, numStepSamples);

		resampRate = (m_demodWfmSampleRate*1.0) / (m_audioOutRate*1.0);

    } else {
		//DB::analyzeCPX(nextStep,numStepSamples,"Pre-Decimate");
		//global->perform.StartPerformance();
		if (!m_useDemodDecimator) {
        //Replaces Mixer.cpp
			numStepSamples = m_downConvert1.ProcessData(numStepSamples, nextStep,m_workingBuf);
		} else {
			nextStep = m_mixer->ProcessBlock(nextStep);
			numStepSamples = m_demodDecimator->process(nextStep, m_workingBuf, numStepSamples);
		}
		//global->perform.StopPerformance(1000);

		//This is a significant change from the way we used to process post downconvert
        //We used to process every downConvertLen samples, 32 for a 2m sdr sample rate
        //Which didn't work when we added zoomed spectrum, we need full framesPerBuffer to get same fidelity as unprocessed
        //One that worked, it didn't make any sense to process smaller chunks through the rest of the chain!

        //We are always decimating by a factor of 2
        //so we know we can accumulate a full fft buffer at this lower sample rate
		for (int i=0; i<numStepSamples; i++) {
			m_sampleBuf[m_sampleBufLen++] = m_workingBuf[i];
        }

		//Build full frame buffer
		if (m_sampleBufLen < m_framesPerBuffer)
            return; //Nothing to do until we have full buffer
		numStepSamples = m_framesPerBuffer;
		m_sampleBufLen = 0;
		nextStep = m_sampleBuf;

		//Create zoomed spectrum
		//global->perform.StartPerformance("Signal Spectrum Zoomed");
		m_signalSpectrum->zoomed(nextStep, numStepSamples);
		//global->perform.StopPerformance(100);

		global->testBench->displayData(numStepSamples,nextStep,m_demodSampleRate,TB_POST_MIXER);

        //global->perform.StartPerformance();

		//global->perform.StartPerformance("Band Pass Filter");
		nextStep = m_bpFilter->process(nextStep, numStepSamples);
		//global->perform.StopPerformance(100);

		global->testBench->displayData(numStepSamples,nextStep,m_demodSampleRate,TB_POST_BP);

		//If squelch is set, and we're below threshold and should set output to zero
		//Do this in SignalStrength, since that's where we're calculating average signal strength anyway
		//Calc this here, at lower sample rate, for efficiency
		//Uses original unprocessed spectrum data
		m_avgDb = m_signalStrength->fdEstimate(m_signalSpectrum->getUnprocessed(),m_signalSpectrum->binCount(),
				m_signalSpectrum->getSampleRate(),m_bpFilter->lowFreq(), m_bpFilter->highFreq(), m_mixerFrequency);
		//Squelch based on last avgerages
		if (m_avgDb < m_squelchDb) {
			//We don't need to do any other processing if signal is below squelch
			return;
		}

        //Tune only mode, no demod or output
		if (m_demod->demodMode() == DeviceInterface::dmNONE){
			CPX::clearCPX(m_audioBuf,m_framesPerBuffer);
            return;
        }

		//global->perform.StartPerformance("Noise Filter");
		nextStep = m_noiseFilter->ProcessBlock(nextStep);
		//global->perform.StopPerformance(100);

		//global->perform.StartPerformance("AGC");
		nextStep = m_agc->processBlock(nextStep);
		//global->perform.StopPerformance(100);


        //Data decoders come before demod
		if (m_iDigitalModem != NULL)
			nextStep = m_iDigitalModem->processBlock(nextStep);

		//global->perform.StartPerformance("Demod");
		nextStep = m_demod->processBlock(nextStep, numStepSamples);
		//global->perform.StopPerformance(100);

		//audioCpx from here on

		//Testing goertzel
		double power;
		bool result;
		for (int i=0; i<numStepSamples; i++) {
			result = m_testGoertzel->processSample(nextStep[i].re, power);
			if (result)
				m_signalStrength->setExtValue(DB::powerTodB(power));
		}

		global->testBench->displayData(numStepSamples,nextStep,m_demodSampleRate,TB_POST_DEMOD);

		resampRate = (m_demodSampleRate*1.0) / (m_audioOutRate*1.0);

    }

	//Fractional resampler is very expensive, 1000 to 1500ms
	//global->perform.StartPerformance("Fract Resampler");
	if (resampRate != 1)
		numStepSamples = m_fractResampler.Resample(numStepSamples,resampRate,nextStep,m_audioBuf);
	else
		CPX::copyCPX(m_audioBuf,nextStep,numStepSamples);
	//global->perform.StopPerformance(100);

	//global->perform.StartPerformance("Process Audio");
	processAudioData(m_audioBuf,numStepSamples);
	//global->perform.StopPerformance(100);
}

//Should be ProcessSpectrumData, using it for now
//Will expect values from 0 to 255 equivalent to -255db to 0db
//But smallest expected value would be -120db
void Receiver::processBandscopeData(quint8 *in, quint16 numPoints)
{
	if (m_sdr == NULL || !m_powerOn)
		return;

	if (numPoints > m_framesPerBuffer)
		numPoints = m_framesPerBuffer;

	for (int i=0; i<numPoints; i++) {
		m_dbSpectrumBuf[i] = -in[i] * 1.0;
	}
	m_signalSpectrum->setSpectrum(m_dbSpectrumBuf);
}

//Called by devices and other call backs to output audio data
void Receiver::processAudioData(CPX *in, quint16 numSamples)
{
	// apply volume setting, mute and output
	//global->perform.StartPerformance();
	m_audioOutput->SendToOutput(in,numSamples, m_gain, m_mute);
	//global->perform.StopPerformance(100);
}

#if 0
/*
 Alternate receive chain that does most of the processing in frequency domain
 */

//!!!! Out of date and never worked properly, here for reference
void Receiver::ProcessBlockFreqDomain(CPX *in, CPX *out, int frameCount)
{
    return;

	CPX *nextStep = in;

	//Convert to freq domain
    fft->FFTForward(in,NULL,frameCount);
	//fft->freqDomain is in 0 to sampleRate order, not -F to 0 to +F


	//Frequency domain mixer?


	//Filter in place
	bpFilter->Convolution(fft);

	//Convert back to time domain for final processing using fft internal buffers
    fft->FFTInverse(NULL,NULL,fftSize);
	//Do overlap/add to reduce to frameCount
	fft->OverlapAdd(out, frameCount);

	nextStep = out;

	//downsample
    //Broken after change to downconvert TBD
//	for(int i=0,j=0; i<framesPerBuffer; i+=downSampleFactor,j++)
//		nextStep[j] = nextStep[i];

    nextStep = demod->ProcessBlock(nextStep,demodFrames);


	if (mute)
		CPX::clearCPX(out, demodFrames);
	else
		CPX::scaleCPX(out, nextStep, gain, demodFrames);

    audioOutput->SendToOutput(out,demodFrames);

}
#endif

void Receiver::setDigitalModem(QString _name, QWidget *_parent)
{
    if (_name == NULL || _parent == NULL) {
		if (m_iDigitalModem != NULL)
			m_iDigitalModem->setupDataUi(NULL);

		m_iDigitalModem = NULL;
        return;
    }
	m_iDigitalModem = m_plugins->GetModemInterface(_name);
	if (m_iDigitalModem != NULL) {

        //Display array of CPX data in TestBench
		connect(m_iDigitalModem->asQObject(), SIGNAL(testbench(int, CPX*, double, int)),global->testBench,SLOT(displayData(int, CPX*, double, int)));

        //Display array of double data in TestBench
		connect(m_iDigitalModem->asQObject(), SIGNAL(testbench(int, double*, double, int)),global->testBench,SLOT(displayData(int, double*, double, int)));

		connect(m_iDigitalModem->asQObject(), SIGNAL(addProfile(QString,int)), global->testBench,SLOT(addProfile(QString,int)));

		connect(m_iDigitalModem->asQObject(), SIGNAL(removeProfile(quint16)), global->testBench,SLOT(removeProfile(quint16)));

		m_iDigitalModem->setSampleRate(m_demodSampleRate, m_demodFrames);
		m_iDigitalModem->setupDataUi(_parent);
    }

}

