//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "receiver.h"
#include "qmessagebox.h"
#include "devices/elektorsdr.h"
#include "devices/sdr_iq.h"
#include "devices/hpsdr.h"
#include "signalprocessing.h"
#include "audioqt.h"
#include "testbench.h"

/*
Core receiver logic, coordinates soundcard, fft, demod, etc
*/
Receiver::Receiver(ReceiverWidget *rw, QMainWindow *main)
{

	//Read ini file or set defaults if no ini file exists
	settings = new Settings();
    global->settings = settings;

    plugins = new Plugins(this,settings);

    mainWindow = main;
    receiverWidget = rw;

    //Get dim of primary screen
    QRect pos = qApp->screens()[0]->geometry();
    pos.setX(pos.right() - main->width()); //bump tight to right
    pos.setY(0); //Top
    pos.setWidth(-1);
    pos.setHeight(-1);
    mainWindow->setGeometry(pos );

    //Move to constructor
    recordingPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        recordingPath.chop(25);
#endif
    //Make sure directory exists, else will get file error
    QDir dir(recordingPath);
    dir.mkdir("PebbleRecordings");
    recordingPath += "PebbleRecordings/";

    //Testing 1 place to switch between PortAudio and QTAudio
    //WARNING: When you change this, delete pebble.ini or manually reset all settings because input/output names may change
    Audio::useQtAudio = false; //Read from setting TBD

	//ReceiverWidget link back
	receiverWidget->SetReceiver(this);
	QStringList welcome;

	welcome <<
        global->revision <<
        "Copyright 2010, 2011, 2012, 2013 Richard Landsman N1DDY (pebblesdr@gmail.com)" <<
        "See PebbleGPL.txt for GPL license details";

	receiverWidget->SetMessage( welcome);

	powerOn = false;
	mute = false;
    isRecording = false;

	presets = NULL;

	sdr = NULL;
	mixer = NULL;
	demod = NULL;
	audioOutput = NULL;
	bpFilter = NULL;
	noiseBlanker = NULL;
	noiseFilter = NULL;
	signalStrength = NULL;
	signalSpectrum = NULL;
	agc = NULL;
	iqBalance = NULL;
	sdrThread = NULL;
    iDigitalModem = NULL;

	//Testing
	useFreqDomainChain = false;

    //qDebug()<<plugins->GetPluginNames();

}
bool Receiver::On()
{
	powerOn = true;

    sdr = global->sdr;
    if (sdr == NULL)
        return false; //Means something is wrong with plugins,

    //Setup callback for device plugins to use when they have new IQ data
    using namespace std::placeholders;
    //function template must match exactly the method that is used in bind()
    //std::function<void(CPX *, quint16)> cb = std::bind(&Receiver::ProcessIQData, _receiver, std::placeholders::_1, std::placeholders::_2);
    //bind(Method ptr, object, arg1, ... argn)

    sdr->ReadSettings(); //Always start with most current
    sdr->Initialize(std::bind(&Receiver::ProcessIQData, this, _1, _2),settings->framesPerBuffer);

	if (!sdr->Connect()){
        QMessageBox::information(NULL,"Pebble","SDR device is not connected");
		Off();
		return false;
	}

    if (sdr->GetTestBenchChecked()) {
        CTestBench *testBench = global->testBench;

        testBench->Init(); //Sets up last device settings used
        //Anchor in upper left
        testBench->setGeometry(0,0,-1,-1);

        testBench->ResetProfiles();
        testBench->AddProfile("Incoming",testBenchRawIQ);
        testBench->AddProfile("Post Mixer",testBenchPostMixer);
        testBench->AddProfile("Post Bandpass",testBenchPostBandpass);
        testBench->AddProfile("Post Demod",testBenchPostDemod);

        testBench->setVisible(true);

        //Keep focus on us
        mainWindow->activateWindow(); //Makes main active
        mainWindow->raise(); //Brings it to the top
        mainWindow->setFocus(); //Makes sure it has keyboard focus
    }

    //Don't set title until we connect.  Some drivers handle multiple devices (RTL2832) and we need connection data
    QApplication::activeWindow()->setWindowTitle("Pebble II: " + sdr->GetDeviceName());

    sampleRate = demodSampleRate = sdr->GetSampleRate();
    framesPerBuffer = demodFrames = settings->framesPerBuffer;
    //These steps work on full sample rates
    noiseBlanker = new NoiseBlanker(sampleRate,framesPerBuffer);
    //Don't need Mixer anymore - TBD
    mixer = new Mixer(sampleRate, framesPerBuffer);
    iqBalance = new IQBalance(sampleRate,framesPerBuffer);
    iqBalance->setEnabled(sdr->GetIQBalanceEnabled());
    iqBalance->setGainFactor(sdr->GetIQBalanceGain());
    iqBalance->setPhaseFactor(sdr->GetIQBalancePhase());

    /*
     * Decimation strategy
     * sampleRate = device A/D rate.  FFT for spectrum uses this rate
     * demodSampleRate = DSP processing rate.  This is calcualted to be close to, but greater than, the final filter bandwidth
     *  This results in the most optimap DSP steps, since we're never processing much more than the final bandwidth needed for output
     * audioOutputRate = This post DSP decimation (resampling?) takes us to the final audio output rate.
     *  It should be just enough for the fidelity we want so we don't waste audio subsystem cpu
     *
     * FM is the same, except the DSP bandwidth is initially much higher
     * demodWfmSampleRate is the initial DSP rate, typically 300k bw
     * demodWfmSampleRate then takes us down another step for the remainder of the DSP chain
     * audioOutputRate = same as above
     */

    fractResampler.Init(framesPerBuffer);
    //Calculates the number of decimation states and returns converted sample rate
    //SetDataRate() doesn't downsample below 256k?
    audioOutRate = 11025; //This rate supported by QTAudio and PortAudio on Mac
    //SetDataRate MaxBW should be driven by our filter selection, ie width of filter
    //For now just set to widest filter, which is 20k for AM, 30k for FMN
    demodSampleRate = downConvert1.SetDataRate(sampleRate, Demod::demodInfo[dmFMN].maxOutputBandWidth);

    //For FMStereo, initial rate should be close to 300k
    //Different decimation technique than SetDataRate
    demodWfmSampleRate = downConvertWfm1.SetDataRateSimple(sampleRate, Demod::demodInfo[dmFMS].maxOutputBandWidth);

    //We need original sample rate, and post mixer sample rate for zoomed spectrum
    signalSpectrum = new SignalSpectrum(sampleRate, demodSampleRate, framesPerBuffer, settings);

    //Init demod with defaults
    //Demod uses variable frame size, up to framesPerBuffer
    //Demod can also run at different sample rates, high for FMW and lower for rest
    //Todo: How to handle this for filters which need explicit sample rate
    //demod = new Demod(downSampleRate,framesPerBuffer);
    demod = new Demod(demodSampleRate, demodWfmSampleRate,framesPerBuffer); //Can't change rate later, fix

    //demodFrames = demodSampleRate / (1.0 * sampleRate) * framesPerBuffer; //Temp Hack
    //post mixer/downconvert frames are now the same as pre
    demodFrames = framesPerBuffer;

    //Sets the Mixer NCO frequency
    //downConvert.SetFrequency(-RDS_FREQUENCY);

    workingBuf = CPXBuf::malloc(framesPerBuffer);
    sampleBuf = CPXBuf::malloc(framesPerBuffer);
    audioBuf = CPXBuf::malloc(framesPerBuffer);
    sampleBufLen = 0;

    presets = new Presets(receiverWidget);


	//Testing with frequency domain receive chain
	//fft must be large enough to avoid circular convolution for filtering
	fftSize = framesPerBuffer *2;
    fft = new FFTfftw();
    fft->FFTParams(fftSize, +1, 0, sampleRate);


    //These steps work on demodSampleRate rates

    //WIP, testing QT audio as alternative to PortAudio
    audioOutput = Audio::Factory(this,demodFrames, settings);

    noiseFilter = new NoiseFilter(demodSampleRate,demodFrames);
    //signalStrength is used by normal demod and wfm, so buffer len needs to be max
    //Calls to ProcessBlock will pass post decimation len
    signalStrength = new SignalStrength(demodSampleRate,framesPerBuffer);

    iDigitalModem = NULL;

	//Testing, time intensive for large # taps, ie @512 we lose chunks of signal
	//Check post-bandpass spectrum and make just large enough to be effective
	//64 is too small, 128 is good, ignored if useFFT arg == true
	if (useFreqDomainChain)
		//In freq domain chain we have full size fft available
		bpFilter = new FIRFilter(sampleRate, framesPerBuffer,true, 128);
	else
        bpFilter = new FIRFilter(demodSampleRate, demodFrames,true, 128);
	bpFilter->setEnabled(true);
	
    agc = new AGC(demodSampleRate, demodFrames);
    SetAgcMode(AGC::FAST); //Default mode

	//Limit tuning range and mixer range
	//Todo: Get from SDR and enforce in UI
	receiverWidget->SetLimits(sdr->GetHighLimit(),sdr->GetLowLimit(),sampleRate/2,-sampleRate/2);
    receiverWidget->SetDisplayedGain(30,1,100);  //20%
    receiverWidget->SetDisplayedSquelch(global->minDb);
	
    if (sdr->GetSDRDevice() == SDR::FILE || sdr->GetStartup() == SDR::DEFAULTFREQ) {
        frequency=sdr->GetStartupFrequency();
        receiverWidget->SetFrequency(frequency);
        //This triggers indirect frequency set, so make sure we set widget first
        receiverWidget->SetMode((DEMODMODE)sdr->GetStartupMode());
    }
    else if (sdr->GetStartup() == SDR::SETFREQ) {
        frequency = sdr->GetFreqToSet();
        receiverWidget->SetFrequency(frequency);

        receiverWidget->SetMode((DEMODMODE)sdr->GetStartupMode());
    }
    else if (sdr->GetStartup() == SDR::LASTFREQ) {
        frequency = sdr->GetLastFreq();
        receiverWidget->SetFrequency(frequency);

        receiverWidget->SetMode((DEMODMODE)sdr->GetLastMode());
	}
	else {
		frequency = 10000000;
        receiverWidget->SetFrequency(frequency);

		receiverWidget->SetMode(dmAM);
	}

	//This should always be last because it starts samples flowing through the processBlocks
    audioOutput->StartOutput(sdr->GetOutputDeviceName(), audioOutRate);
	sdr->Start();

	return true;
}
//Delete everything that's based on settings, so we can change receivers, sound cards, etc
//ReceiverWidget initiates the power off and call us
bool Receiver::Off()
{

	powerOn = false;

    //If we're closing app, don't bother to set title, crashes sometime
    QWidget *app = QApplication::activeWindow();
    if (app != NULL)
        app->setWindowTitle("Pebble II");

    if (global->testBench->isVisible())
        global->testBench->setVisible(false);

    if (isRecording) {
        recordingFile.Close();
        isRecording = false;
    }
	//Carefull with order of shutting down
    //if (settings->sdrDevice == SDR::SDR_IQ_USB) {
    //}

	//Stop incoming samples first
    if (sdr != NULL) {
        sdr->WriteSettings(); //Always save last mode, last freq, etc
        sdr->Stop();
    }
	if (audioOutput != NULL)
		audioOutput->Stop();

    iDigitalModem = NULL;

	//Now clean up rest
	if (sdr != NULL){
        //Save any run time settings
        sdr->SetLastFreq(frequency);

		sdr->Disconnect();
        sdr = NULL;
	}
	if (demod != NULL) {
		delete demod;
		demod = NULL;
	}
	if (mixer != NULL) {
		delete mixer;
		mixer = NULL;
	}
	if (bpFilter != NULL) {
		delete bpFilter;
		bpFilter = NULL;
	}
	if (noiseBlanker != NULL) {
		delete noiseBlanker;
		noiseBlanker = NULL;
	}
	if (noiseFilter != NULL) {
		delete noiseFilter;
		noiseFilter = NULL;
	}
	if (signalStrength != NULL) {
		delete signalStrength;
		signalStrength = NULL;
	}
	if (signalSpectrum != NULL) {
		delete signalSpectrum;
		signalSpectrum = NULL;
	}
	if (agc != NULL) {
		delete agc;
		agc = NULL;
	}
	if(iqBalance != NULL) {
		delete iqBalance;
		iqBalance=NULL;
	}
    //Making this last to work around a shut down order problem with consumer threads
    if (audioOutput != NULL) {
        delete audioOutput;
        audioOutput = NULL;
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

    if (workingBuf != NULL) {
        delete workingBuf;
        workingBuf = NULL;
    }
    if (sampleBuf != NULL) {
        delete sampleBuf;
        sampleBuf = NULL;
    }
    if (audioBuf != NULL) {
        delete audioBuf;
        audioBuf = NULL;
    }
    return true;
}
void Receiver::Close()
{
    Off();
    settings->WriteSettings();
}
Receiver::~Receiver(void)
{
	if (settings != NULL)
		delete settings;
    if (sdr != NULL)
        delete sdr;
}

//Connected to ReceiverWidget REC button
/*
 * Records to standard file name in data directory
 * If file exists, appends number until finds one that doesn't
 * Simple, no prompt for file name, can be instantly toggled on/off
 */
void Receiver::RecToggled(bool on)
{
    if (!powerOn)
        return;

    if (on) {
        //We could use QTemporaryFile to get a unique file name, but need more control
        QString baseName = "PebbleIQ";
        QFileInfo fInfo;
        for (int i=1; i<1000; i++) {
            recordingFileName = recordingPath + baseName + QString::number(i) + ".wav";
            fInfo.setFile(recordingFileName);
            if (!fInfo.exists())
                break; //Got a unique name
            //When we overrun counter, last filename will be continually overwritten
        }

        recordingFile.OpenWrite(recordingFileName, sampleRate, frequency, demod->DemodMode(), 0);
        isRecording = true;
    } else {
        recordingFile.Close();
        isRecording = false;
    }
}

bool Receiver::Power(bool on)
{

	if (on)
	{
		return On();
	}else {
		//return Off();
		Close(); //Same as Off but writes settings
		return true;
	}
}
//Triggered by settings change or something else that requires us to reset all objects
void Receiver::Restart()
{
	//If power off, do nothing
	if (powerOn) {
		//Tell receiver widget to power off, then on.
		//Receiver widget controls logic and will call receiver as necessary
		receiverWidget->powerToggled(false);
        //Just power off for now
        //receiverWidget->powerToggled(true);
	}
}
double Receiver::SetFrequency(double fRequested, double fCurrent)
{
	if (sdr==NULL)
		return 0;
    demod->ResetDemod();

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
	//mutex.lock();
	double actual = sdr->SetFrequency(fRequested,fCurrent);
	//mutex.unlock();
#if(0)
	if (restart)
		soundCard->Restart();
#endif
	frequency = actual;

	return actual;
}
//Sets demod modes and default bandpass filter for each mode
void Receiver::SetMode(DEMODMODE m)
{
	if(demod != NULL) {
        if (m == dmFMM || m == dmFMS)
            signalSpectrum->SetSampleRate(sampleRate, demodWfmSampleRate);
        else
            signalSpectrum->SetSampleRate(sampleRate, demodSampleRate);

        //Demod will return new demo
        demod->SetDemodMode(m, sampleRate, demodSampleRate);
        sdr->SetLastMode(m);
        sampleBufLen = 0;
	}
    if (iDigitalModem != NULL) {
        iDigitalModem->SetDemodMode(m);
    }
}	
void Receiver::SetFilter(int lo, int hi)
{
	if (demod == NULL)
		return;
	bpFilter->SetBandPass(lo,hi);
    demod->SetBandwidth(hi - lo);
}
void Receiver::SetAnfEnabled(bool b)
{
	noiseFilter->setAnfEnabled(b);
}
void Receiver::SetNbEnabled(bool b)
{
	noiseBlanker->setNbEnabled(b);
}
void Receiver::SetNb2Enabled(bool b)
{
	noiseBlanker->setNb2Enabled(b);
}
void Receiver::SetAgcMode(AGC::AGCMODE m)
{
	agc->setAgcMode(m);
	//AGC sets a default gain with mode, update widget
    receiverWidget->SetDisplayedAgcThreshold(agc->getAgcThreshold());
}
void Receiver::SetAgcThreshold(int g)
{
    agc->setAgcThreshold(g);
}

void Receiver::SetMute(bool b)
{
	mute = b;
}

void Receiver::SetGain(int g)
{
	//Convert dbGain to amplitude so we can use it to scale
	//No magic DSP math, just found something that sounds right to convert slider 0-100 reasonable vol
    gain=DB::dbToAmplitude(25 + g * 0.35);
}
void Receiver::SetSquelch(int s)
{
	squelch = s;
}
void Receiver::SetMixer(int f)
{
    if (mixer != NULL) {
        mixerFrequency = f;
		mixer->SetFrequency(f);
        demod->ResetDemod();
    }
}

void Receiver::SdrOptionsPressed()
{
    //2 states, power on and off
    //If power on, use active sdr to make changes
    if (sdr == NULL) {
        //Power is off, create temporary one so we can set settings
        sdr = global->sdr;
    }
    sdr->ShowSdrOptions(true);
}
//Called by receiver widget with device selection changes to make sure any open options ui is closed
void Receiver::CloseSdrOptions()
{
    if (sdr != NULL) {
        sdr->ShowSdrOptions(false);
        sdr = NULL;
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
void Receiver::ProcessIQData(CPX *in, quint16 numSamples)
{
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
    global->testBench->CreateGeneratorSamples(numSamples, in, sampleRate);
    global->testBench->MixNoiseSamples(numSamples, in, sampleRate);

    if (isRecording)
        recordingFile.WriteSamples(in,numSamples);

	float tmp;
    //Configure IQ order if not default
    if (sdr->GetIQOrder() != SDR::IQ)
        for (int i=0;i<framesPerBuffer;i++)
        {
            switch(sdr->GetIQOrder())
            {
            case SDR::IQ:
                    //No change, this is the default order
                    break;
            case SDR::QI:
                    tmp = in[i].im;
                    in[i].im = in[i].re;
                    in[i].re = tmp;
                    break;
            case SDR::IONLY:
                    in[i].im = in[i].re;
                    break;
            case SDR::QONLY:
                    in[i].re = in[i].im;
                    break;
            }
        }

	//Spectrum wide processing

#if(0)
	//Add a configurable delay so we can force port audio buffer overflow for testing
	Sleep(100); //ms delay
#endif
	//Handy way to test filters. Spectrum display will show filter response since we're doing
	//this as first step.
	//Disable postMixerDecimate before testing
	//nextStep = lpFilter->ProcessBlock(nextStep);

	//Used to check gain/loss across steps
	//float pre = SignalProcessing::totalPower(nextStep,framesPerBuffer);

	//Normalize device gain
	//Note that we DO modify IN buffer in this step
	sdrGain = sdr->GetGain(); //Allow sdr to change gain while running
    if (sdrGain != 1)
        CPXBuf::scale(in,in,sdrGain * sdr->GetIQGain(),framesPerBuffer);

	CPX *nextStep = in;

    global->testBench->DisplayData(numSamples,nextStep,sampleRate,testBenchRawIQ);

    //global->perform.StartPerformance();
    /*
      3 step decimation
      1. Decimate to max spectrum, ie RTL2832 samples at >1msps
      2. Decimate to pre demod steps
      3. Decimate to demod steps
      4. Decimate to final audio
    */
	//Adj IQ to get 90deg phase and I==Q gain
    nextStep = iqBalance->ProcessBlock(nextStep);

	//Noise blankers
    nextStep = noiseBlanker->ProcessBlock(nextStep);
    nextStep = noiseBlanker->ProcessBlock2(nextStep);

    //Spectrum display, in buffer is not modified
    //Spectrum is displayed before mixer, so that changes in mixer don't change spectrum
    //
    signalSpectrum->Unprocessed(nextStep,
        0,//audio->inBufferUnderflowCount,
        0,//audio->inBufferOverflowCount,
        0,//audio->outBufferUnderflowCount,
        0);//audio->outBufferOverflowCount);
	//audio->ClearCounts();
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

    //global->perform.StartPerformance();
    if (demod->DemodMode() == dmFMM || demod->DemodMode() == dmFMS) {
        //These steps are at demodWfmSampleRate NOT demodSampleRate
        //Special handling for wide band fm

        //Replaces Mixer.cpp and mixes and decimates
        downConvertWfm1.SetFrequency(mixerFrequency);
        // InLength must be a multiple of 2^N where N is the maximum decimation by 2 stages expected.
        //We need a separated input/output buffer for downConvert
        int downConvertLen = downConvertWfm1.ProcessData(framesPerBuffer,nextStep,workingBuf);
        //We are always decimating by a factor of 2
        //so we know we can accumulate a full fft buffer at this lower sample rate
        for (int i=0; i<downConvertLen; i++) {
            sampleBuf[sampleBufLen++] = workingBuf[i];
        }

        if (sampleBufLen < framesPerBuffer)
            return; //Nothing to do until we have full buffer

        sampleBufLen = 0;
        //Create zoomed spectrum
        signalSpectrum->Zoomed(sampleBuf, framesPerBuffer);
        nextStep = sampleBuf;

        nextStep = signalStrength->ProcessBlock(nextStep, framesPerBuffer, squelch);

        nextStep = demod->ProcessBlock(nextStep, framesPerBuffer);

        resampRate = (demodWfmSampleRate*1.0) / (audioOutRate*1.0);

    } else {

        //Replaces Mixer.cpp
        downConvert1.SetFrequency(mixerFrequency);
        int downConvertLen = downConvert1.ProcessData(framesPerBuffer,nextStep,workingBuf);
        //This is a significant change from the way we used to process post downconvert
        //We used to process every downConvertLen samples, 32 for a 2m sdr sample rate
        //Which didn't work when we added zoomed spectrum, we need full framesPerBuffer to get same fidelity as unprocessed
        //One that worked, it didn't make any sense to process smaller chunks through the rest of the chain!

        //We are always decimating by a factor of 2
        //so we know we can accumulate a full fft buffer at this lower sample rate
        for (int i=0; i<downConvertLen; i++) {
            sampleBuf[sampleBufLen++] = workingBuf[i];
        }

        if (sampleBufLen < framesPerBuffer)
            return; //Nothing to do until we have full buffer

        sampleBufLen = 0;
        //Create zoomed spectrum
        signalSpectrum->Zoomed(sampleBuf, framesPerBuffer);

        nextStep = sampleBuf;

        //Mixer shows no loss in testing
        //nextStep = mixer->ProcessBlock(nextStep);
        //float post = SignalProcessing::TotalPower(nextStep,frameCount);
        global->testBench->DisplayData(demodFrames,nextStep,demodSampleRate,testBenchPostMixer);

        //global->perform.StopPerformance(100);

        //global->perform.StartPerformance();

        //float pre = SignalProcessing::TotalPower(nextStep,frameCount);
        nextStep = bpFilter->ProcessBlock(nextStep);
        //Crude AGC, too much fluctuation
        //CPXBuf::scale(nextStep,nextStep,pre/post,frameCount);
        global->testBench->DisplayData(demodFrames,nextStep,demodSampleRate,testBenchPostBandpass);

        //global->perform.StopPerformance(100);

        //global->perform.StartPerformance();

        //If squelch is set, and we're below threshold and should set output to zero
        //Do this in SignalStrength, since that's where we're calculating average signal strength anyway
        nextStep = signalStrength->ProcessBlock(nextStep, framesPerBuffer, squelch);
        //global->perform.StopPerformance(100);
        //global->perform.StartPerformance();

        //Tune only mode, no demod or output
        if (demod->DemodMode() == dmNONE){
            CPXBuf::clear(audioBuf,framesPerBuffer);
            return;
        }
        nextStep = noiseFilter->ProcessBlock(nextStep);

        //nr->ProcessBlock(out, in, size);
        //float post = SignalProcessing::totalPower(nextStep,framesPerBuffer);

        nextStep = agc->ProcessBlock(nextStep);
        //global->perform.StopPerformance(100);

        //global->perform.StartPerformance();

        //Data decoders come before demod
        if (iDigitalModem != NULL)
            nextStep = iDigitalModem->ProcessBlock(nextStep);

        nextStep = demod->ProcessBlock(nextStep, framesPerBuffer);
        global->testBench->DisplayData(demodFrames,nextStep,demodSampleRate,testBenchPostDemod);

        //global->perform.StopPerformance(100);

        resampRate = (demodSampleRate*1.0) / (audioOutRate*1.0);

    }

    int numResamp = fractResampler.Resample(demodFrames,resampRate,nextStep,audioBuf);

    // apply volume setting, mute and output
    //global->perform.StartPerformance();
    audioOutput->SendToOutput(audioBuf,numResamp, gain, mute);
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
        CPXBuf::clear(out, demodFrames);
	else
        CPXBuf::scale(out, nextStep, gain, demodFrames);

    audioOutput->SendToOutput(out,demodFrames);

}
#endif

void Receiver::SetDigitalModem(QString _name, QWidget *_parent)
{
    if (_name == NULL || _parent == NULL) {
        if (iDigitalModem != NULL)
            iDigitalModem->SetupDataUi(NULL);

        iDigitalModem = NULL;
        return;
    }
    iDigitalModem = plugins->GetModemInterface(_name);
    if (iDigitalModem != NULL) {

        //Display array of CPX data in TestBench
        connect(iDigitalModem->asQObject(), SIGNAL(Testbench(int, CPX*, double, int)),global->testBench,SLOT(DisplayData(int, CPX*, double, int)));

        //Display array of double data in TestBench
        connect(iDigitalModem->asQObject(), SIGNAL(Testbench(int, double*, double, int)),global->testBench,SLOT(DisplayData(int, double*, double, int)));

        connect(iDigitalModem->asQObject(), SIGNAL(AddProfile(QString,int)), global->testBench,SLOT(AddProfile(QString,int)));

        connect(iDigitalModem->asQObject(), SIGNAL(RemoveProfile(quint16)), global->testBench,SLOT(RemoveProfile(quint16)));

        iDigitalModem->SetSampleRate(demodSampleRate, demodFrames);
        iDigitalModem->SetupDataUi(_parent);
    }

}

