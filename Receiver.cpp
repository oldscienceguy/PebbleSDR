//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "Receiver.h"
#include "qmessagebox.h"
#include "Devices/elektorsdr.h"
#include "Devices/sdr_iq.h"
#include "Devices/hpsdr.h"
#include "signalprocessing.h"
#include "audioqt.h"

/*
Core receiver logic, coordinates soundcard, fft, demod, etc
*/
Receiver::Receiver(ReceiverWidget *rw, QMainWindow *main)
{

	//Read ini file or set defaults if no ini file exists
	settings = new Settings();
    global->settings = settings;
	connect(settings,SIGNAL(Restart()),this,SLOT(Restart()));

    //Testing 1 place to switch between PortAudio and QTAudio
    //WARNING: When you change this, delete pebble.ini or manually reset all settings because input/output names may change
    Audio::useQtAudio = false;

	mainWindow = main;
	receiverWidget = rw;

	//ReceiverWidget link back
	receiverWidget->SetReceiver(this);
	QStringList welcome;

	welcome <<
		"\"Sometimes a little pebble is all you need...\"" <<
        global->revision <<
        "Copyright 2010, 2011, 2012, 2013 Richard Landsman N1DDY (pebblesdr@gmail.com)" <<
		"Licensed under GPL.  See PebbleGPL.txt";

	receiverWidget->SetMessage( welcome);
	//Problem:  If we set general styles, custom widgets don't paint
	//Until we figure out why, use selectors to prevent cascade
    QString rwStyle = "QFrame#comboFrame, QFrame#tunerFrame, QFrame#buttonFrame, QFrame#sliderFrame, QFrame#dataFrame, QFrame#spectrumFrame, QFrame#bandFrame {background-color: rgb(200, 200, 200);}";
	receiverWidget->setStyleSheet(rwStyle);

	powerOn = false;
	mute = false;

	presets = NULL;

	sdr = NULL;
	mixer = NULL;
	demod = NULL;
	audioOutput = NULL;
	bpFilter = NULL;
	lpFilter = NULL;
	noiseBlanker = NULL;
	noiseFilter = NULL;
	signalStrength = NULL;
	signalSpectrum = NULL;
	agc = NULL;
	iqBalance = NULL;
	sdrThread = NULL;
    morse = NULL;

	//Testing
	useFreqDomainChain = false;

}
bool Receiver::On()
{
	powerOn = true;

    sdr = SDR::Factory(this, settings->sdrDevice, settings);


	if (!sdr->Connect()){
		QMessageBox::information(NULL,"Pebble","No Receiver connected to USB");
		Off();
		return false;
	}

    //Don't set title until we connect.  Some drivers handle multiple devices (RTL2832) and we need connection data
    QApplication::activeWindow()->setWindowTitle("Pebble: " + sdr->GetDeviceName());

    sampleRate = downSample1Rate = sdr->GetSampleRate();
    framesPerBuffer = downSample1Frames = settings->framesPerBuffer;
    //These steps work on full sample rates
    noiseBlanker = new NoiseBlanker(sampleRate,framesPerBuffer);
    signalSpectrum = new SignalSpectrum(sampleRate,framesPerBuffer,settings);
    //Don't need Mixer anymore - TBD
    mixer = new Mixer(sampleRate, framesPerBuffer);
    iqBalance = new IQBalance(sampleRate,framesPerBuffer);
    iqBalance->setEnabled(settings->iqBalanceEnable);
    iqBalance->setGainFactor(settings->iqBalanceGain);
    iqBalance->setPhaseFactor(settings->iqBalancePhase);

    /*
     * Decimation strategy
     * sampleRate = device A/D rate.  FFT for spectrum uses this rate
     * downSample1Rate = DSP processing rate.  This is calcualted to be close to, but greater than, the final filter bandwidth
     *  This results in the most optimap DSP steps, since we're never processing much more than the final bandwidth needed for output
     * downSample2Rate = Not used at present, here for consistency with WFM
     * audioOutputRate = This post DSP decimation (resampling?) takes us to the final audio output rate.
     *  It should be just enough for the fidelity we want so we don't waste audio subsystem cpu
     *
     * FM is the same, except the DSP bandwidth is initially much higher
     * downSampleWfm1Rate is the initial DSP rate, typically 300k bw
     * downSampleWfm2Rate then takes us down another step for the remainder of the DSP chain
     * audioOutputRate = same as above
     */

    fractResampler.Init(framesPerBuffer);
    //Calculates the number of decimation states and returns converted sample rate
    //SetDataRate() doesn't downsample below 256k?
    audioOutRate = 11025; //This rate supported by QTAudio and PortAudio on Mac
    //SetDataRate MaxBW should be driven by our filter selection, ie width of filter
    //For now just set to widest filter, which is 16k for AM
    downSample1Rate = downConvert1.SetDataRate(sampleRate, 16000.0);

    //For FMStereo, initial rate should be close to 300k
    downSampleWfm1Rate = downConvertWfm1.SetDataRateSimple(sampleRate, 300000.0);
    //Init demod with defaults
    //Demod uses variable frame size, up to framesPerBuffer
    //Demod can also run at different sample rates, high for FMW and lower for rest
    //Todo: How to handle this for filters which need explicit sample rate
    //demod = new Demod(downSampleRate,framesPerBuffer);
    demod = new Demod(downSample1Rate, downSampleWfm1Rate,framesPerBuffer); //Can't change rate later, fix

    downSample1Frames = downSample1Rate / (1.0 * sampleRate) * framesPerBuffer; //Temp Hack
    //Sets the Mixer NCO frequency
    //downConvert.SetFrequency(-RDS_FREQUENCY);

    workingBuf = CPXBuf::malloc(framesPerBuffer);

    presets = new Presets(receiverWidget);


	//Testing with frequency domain receive chain
	//fft must be large enough to avoid circular convolution for filtering
	fftSize = framesPerBuffer *2;
	fft = new FFT(fftSize);

    //These steps work on downSample1 rates

    //WIP, testing QT audio as alternative to PortAudio
    audioOutput = Audio::Factory(this,downSample1Frames, settings);

    noiseFilter = new NoiseFilter(downSample1Rate,downSample1Frames);
    signalStrength = new SignalStrength(downSample1Rate,downSample1Frames);
	//FIR MAC LP Filter
    lpFilter = new FIRFilter(downSample1Rate,downSample1Frames, false, 128);
	lpFilter->SetLowPass(3000);
	//lpFilter->SetHighPass(3000); //Testing 
	//lpFilter->SetBandPass(50,3000); //Testing

    morse = new Morse(downSample1Rate,downSample1Frames);
    morse->SetReceiver(this);

	//Testing, time intensive for large # taps, ie @512 we lose chunks of signal
	//Check post-bandpass spectrum and make just large enough to be effective
	//64 is too small, 128 is good, ignored if useFFT arg == true
	if (useFreqDomainChain)
		//In freq domain chain we have full size fft available
		bpFilter = new FIRFilter(sampleRate, framesPerBuffer,true, 128);
	else
        bpFilter = new FIRFilter(downSample1Rate, downSample1Frames,true, 128);
	bpFilter->setEnabled(true);
	
    agc = new AGC(downSample1Rate, downSample1Frames);

	//Limit tuning range and mixer range
	//Todo: Get from SDR and enforce in UI
	receiverWidget->SetLimits(sdr->GetHighLimit(),sdr->GetLowLimit(),sampleRate/2,-sampleRate/2);
	receiverWidget->SetGain(30,1,100);  //20%
	receiverWidget->SetSquelch(-100);
	
    if (sdr->GetDevice() == SDR::FILE || settings->startup == Settings::DEFAULTFREQ) {
        frequency=sdr->GetStartupFrequency();
        receiverWidget->SetMode((DEMODMODE)sdr->GetStartupMode());
    }
    else if (settings->startup == Settings::SETFREQ) {
		frequency = settings->startupFreq;
        receiverWidget->SetMode((DEMODMODE)sdr->GetStartupMode());
    }
	else if (settings->startup == Settings::LASTFREQ) {
		frequency = settings->lastFreq;
		receiverWidget->SetMode((DEMODMODE)settings->lastMode);
	}
	else {
		frequency = 10000000;
		receiverWidget->SetMode(dmAM);
	}

	//receiver->SetFrequency(frequency);
	receiverWidget->SetFrequency(frequency);

	//This should always be last because it starts samples flowing through the processBlocks
    audioOutput->Start(0,audioOutRate);
	sdr->Start();

	return true;
}
//Delete everything that's based on settings, so we can change receivers, sound cards, etc
//ReceiverWidget initiates the power off and call us
bool Receiver::Off()
{
	powerOn = false;

	//Carefull with order of shutting down
    if (settings->sdrDevice == SDR::SDR_IQ_USB) {
	}
	//Stop incoming samples first
	if (sdr != NULL)
		sdr->Stop();
	if (audioOutput != NULL)
		audioOutput->Stop();

	//Now clean up rest
	if (sdr != NULL){
		sdr->Disconnect();
		delete sdr;
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
	if (lpFilter != NULL) {
		delete lpFilter;
		lpFilter = NULL;
	}
	if (agc != NULL) {
		delete agc;
		agc = NULL;
	}
	if(iqBalance != NULL) {
		delete iqBalance;
		iqBalance=NULL;
	}
    if (morse != NULL) {
        delete morse;
        morse = NULL;
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
	return true;
}
void Receiver::Close()
{
	Off();
	//Save any run time settings
	settings->lastFreq = frequency;
	settings->WriteSettings();
}
Receiver::~Receiver(void)
{
	if (settings != NULL)
		delete settings;
}
void Receiver::SetDataSelection(ReceiverWidget::DATA_SELECTION d)
{
    dataSelection = d;
    switch (d) {
        case ReceiverWidget::NO_DATA:
            break;
        case ReceiverWidget::BAND_DATA:
            break;
        case ReceiverWidget::CW_DATA:
            break;
        case ReceiverWidget::RTTY_DATA:
            break;
    }
}

void Receiver::OutputData(const char *d)
{
    receiverWidget->OutputData(d);
}
void Receiver::OutputData(QString s)
{
    //receiverWidget->OutputData(s.toAscii());  //toAscii deprecated
    receiverWidget->OutputData(s.toLatin1());
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
		receiverWidget->powerToggled(true);
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
        demod->SetDemodMode(m, sampleRate, downSample1Rate);
		settings->lastMode = m;
	}
}	
void Receiver::SetFilter(int lo, int hi)
{
	if (demod == NULL)
		return;
	bpFilter->SetBandPass(lo,hi);
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
	receiverWidget->SetAgcGainTop(agc->getAgcGainTop());
}
void Receiver::SetAgcGainTop(int g)
{
	agc->setAgcGainTop(g);
}

void Receiver::SetLpfEnabled(bool b)
{
	lpFilter->setEnabled(b);
}
void Receiver::SetMute(bool b)
{
	mute = b;
}

void Receiver::SetGain(int g)
{
	//Convert dbGain to amplitude so we can use it to scale
	//No magic DSP math, just found something that sounds right to convert slider 0-100 reasonable vol
	gain=SignalProcessing::dbToAmplitude(25 + g * 0.35);
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

void Receiver::ShowSettings(bool b)
{
	settings->ShowSettings();
}

void Receiver::ShowSdrSettings(bool b)
{
    if (sdr != NULL)
		sdr->ShowOptions();
}

//processing flow for audio samples, called from SoundCard PortAudio callback
//Note: frameCount is actual # samples from audio, may be less than framesPerBuffer
//in = samples, out = soundcard
/*
Each step in the receiver chain is of the form inBufferToNextStep = thisStep(outBufferFromLastStep)
Since each step mainaintains it's own out buffer and the in buffer is NEVER modified by a step, every
step looks like 'nextBuffer = thisStep(nextBuffer)'
This allows us to comment out steps, add new steps, reorder steps without having to worry about
keeping track of buffers.
Since we NEVER modify an IN buffer, we can also just return the IN buffer as the OUT buffer in a step
that is disabled and not do a useless copy from IN to OUT.
*/
void Receiver::ProcessBlock(CPX *in, CPX *out, int frameCount)
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


	float tmp;
	//Configure IQ order
	for (int i=0;i<framesPerBuffer;i++)
	{
        switch(settings->iqOrder)
		{
        case Settings::IQ:
				//No change, this is the default order
				break;
        case Settings::QI:
				tmp = in[i].im;
				in[i].im = in[i].re;
				in[i].re = tmp;
				break;
        case Settings::IONLY:
				in[i].im = in[i].re;
				break;
        case Settings::QONLY:
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
    //if (sdrGain != 1)
        CPXBuf::scale(in,in,sdrGain * settings->iqGain,framesPerBuffer);

	//End of common processing for both receive chains, pick one
	//Time domain or Frequency domain receive chains
	if (!useFreqDomainChain)
		ProcessBlockTimeDomain(in,out,frameCount);
	else
		ProcessBlockFreqDomain(in,out,frameCount);

    //global->perform.StopPerformance(100);
}

void Receiver::ProcessBlockTimeDomain(CPX *in, CPX *out, int frameCount)
{
	CPX *nextStep = in;

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
	//Mixer shows no loss in testing
    //!!Test - trying this in wfm step
    //nextStep = mixer->ProcessBlock(nextStep);
	//float post = SignalProcessing::TotalPower(nextStep,frameCount); 
    //signalSpectrum->PostMixer(nextStep);

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
    if (demod->DemodMode() == dmFMMono || demod->DemodMode() == dmFMStereo) {
        //These steps are NOT at downSamplerate
        //Special handling for wide band fm
        //Set demod sample rate?

        //Replaces Mixer.cpp
        downConvertWfm1.SetFrequency(mixerFrequency);
        // InLength must be a multiple of 2^N where N is the maximum decimation by 2 stages expected.
        //We need a separated input/output buffer for downConvert
        int downConvertLen = downConvertWfm1.ProcessData(framesPerBuffer,nextStep,workingBuf);
        //Do we need bandPass filter or handle in demod?
        nextStep = demod->ProcessBlock(workingBuf,downConvertLen);
        downSample1Frames = downConvertLen;
        resampRate = (downSampleWfm1Rate*1.0) / (audioOutRate*1.0);

    } else {
        //Replaces Mixer.cpp
        downConvert1.SetFrequency(mixerFrequency);
        int downConvertLen = downConvert1.ProcessData(framesPerBuffer,nextStep,workingBuf);

        //global->perform.StopPerformance(100);

        //global->perform.StartPerformance();

        //float pre = SignalProcessing::TotalPower(nextStep,frameCount);
        nextStep = bpFilter->ProcessBlock(nextStep);
        //Crude AGC, too much fluctuation
        //CPXBuf::scale(nextStep,nextStep,pre/post,frameCount);
        signalSpectrum->PostBandPass(nextStep,downConvertLen);
        //global->perform.StopPerformance(100);

        //global->perform.StartPerformance();

        //If squelch is set, and we're below threshold and should set output to zero
        //Do this in SignalStrength, since that's where we're calculating average signal strength anyway
        nextStep = signalStrength->ProcessBlock(nextStep, squelch);
        //global->perform.StopPerformance(100);
        //global->perform.StartPerformance();

        //Tune only mode, no demod or output
        if (demod->DemodMode() == dmNONE){
            CPXBuf::clear(out,downConvertLen);
            return;
        }
        nextStep = noiseFilter->ProcessBlock(nextStep);

        //nr->ProcessBlock(out, in, size);
        //float post = SignalProcessing::totalPower(nextStep,framesPerBuffer);

        nextStep = agc->ProcessBlock(nextStep);
        //global->perform.StopPerformance(100);

        //global->perform.StartPerformance();

        nextStep = demod->ProcessBlock(nextStep, downConvertLen);

        //global->perform.StopPerformance(100);

        /*
          Todo: Add post demod FFT to check FM stereo and other composite formats
        */

        //Testing Goertzel
        if (dataSelection == ReceiverWidget::CW_DATA)
            nextStep = morse->ProcessBlock(nextStep);

        //global->perform.StartPerformance();

        //Testing LPF to get rid of some noise after demod
        nextStep = lpFilter->ProcessBlock(nextStep);
        //global->perform.StopPerformance(100);

        downSample1Frames = downConvertLen;
        resampRate = (downSample1Rate*1.0) / (audioOutRate*1.0);

    }

    int numResamp = fractResampler.Resample(downSample1Frames,resampRate,nextStep,out);

	// apply volume setting
	//Todo: gain should be a factor of slider, receiver, filter, and mode, and should be normalized
	//	so changing anything other than slider doesn't impact overall volume.
	//	or should filter normalize gain
    //global->perform.StartPerformance();

	//CPXBuf::scale(out, nextStep, gain * filterLoss, frameCount);

	if (mute)
        CPXBuf::clear(out, numResamp);
	else
        CPXBuf::scale(out, out, gain, numResamp);

    audioOutput->SendToOutput(out,numResamp);
    //global->perform.StopPerformance(100);

}

/*
 Alternate receive chain that does most of the processing in frequency domain
 */
void Receiver::ProcessBlockFreqDomain(CPX *in, CPX *out, int frameCount)
{
	CPX *nextStep = in;

	//Convert to freq domain
	fft->DoFFTWForward(in,NULL,frameCount);
	//fft->freqDomain is in 0 to sampleRate order, not -F to 0 to +F

	//Pass freqDomain to SignalSpectrum for display
	signalSpectrum->MakeSpectrum(fft,signalSpectrum->Unprocessed());

	//Frequency domain mixer?


	//Filter in place
	bpFilter->Convolution(fft);
	signalSpectrum->MakeSpectrum(fft,signalSpectrum->PostBandPass());

	//Convert back to time domain for final processing using fft internal buffers
	fft->DoFFTWInverse(NULL,NULL,fftSize);
	//Do overlap/add to reduce to frameCount
	fft->OverlapAdd(out, frameCount);

	nextStep = out;

	//downsample
    //Broken after change to downconvert TBD
//	for(int i=0,j=0; i<framesPerBuffer; i+=downSampleFactor,j++)
//		nextStep[j] = nextStep[i];

    nextStep = demod->ProcessBlock(nextStep,downSample1Frames);


	if (mute)
        CPXBuf::clear(out, downSample1Frames);
	else
        CPXBuf::scale(out, nextStep, gain, downSample1Frames);

    audioOutput->SendToOutput(out,downSample1Frames);

}
