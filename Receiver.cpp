//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "Receiver.h"
#include "qmessagebox.h"
#include "elektorsdr.h"
#include "sdr_iq.h"
#include "hpsdr.h"
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

	sdr = SDR::Factory(this, settings);


	if (!sdr->Connect()){
		QMessageBox::information(NULL,"Pebble","No Receiver connected to USB");
		Off();
		return false;
	}

    //Don't set title until we connect.  Some drivers handle multiple devices (RTL2832) and we need connection data
    QApplication::activeWindow()->setWindowTitle("Pebble: " + sdr->GetDeviceName());

	downSampleFactor = 1;
    //Get undecimated sample rate from RTL2832 and decimate here vs in SDR
    //For FMStereo, initial rate should be close to 300k
    decimate1SampleRate = 300000;
	sampleRate = downSampleRate = sdr->GetSampleRate();
	framesPerBuffer = downSampleFrames = settings->framesPerBuffer;

	//Downsample to target rate
	if (settings->postMixerDecimate) {
		//Keep this in sync with SoundCard decimation
		downSampleFactor = sampleRate / settings->decimateLimit;
		downSampleFactor = downSampleFactor < 1 ? 1 : downSampleFactor;
		downSampleRate = sampleRate / downSampleFactor;
		downSampleFrames = framesPerBuffer / downSampleFactor;
		//Anti alias filter for decimation
		downSampleFilter = 	new FIRFilter(sampleRate,framesPerBuffer, false, 128);
		downSampleFilter->SetLowPass(downSampleRate/2);
		downSampleFilter->setEnabled(true);
	}

    presets = new Presets(receiverWidget);

	//These steps work on full sample rates
	noiseBlanker = new NoiseBlanker(sampleRate,framesPerBuffer);
	signalSpectrum = new SignalSpectrum(sampleRate,framesPerBuffer,settings);
	mixer = new Mixer(sampleRate, framesPerBuffer);
	iqBalance = new IQBalance(sampleRate,framesPerBuffer);
    iqBalance->setEnabled(settings->iqBalanceEnable);
    iqBalance->setGainFactor(settings->iqBalanceGain);
    iqBalance->setPhaseFactor(settings->iqBalancePhase);

	//Testing with frequency domain receive chain
	//fft must be large enough to avoid circular convolution for filtering
	fftSize = framesPerBuffer *2;
	fft = new FFT(fftSize);

	//These steps work on downSample rates

	//Init demod with defaults
    //Demod uses variable frame size, up to framesPerBuffer
    //Demod can also run at different sample rates, high for FMW and lower for rest
    //Todo: How to handle this for filters which need explicit sample rate
    demod = new Demod(downSampleRate,framesPerBuffer);
	//WIP, testing QT audio as alternative to PortAudio
	//audioOutput = new AudioQT(this,sampleRate,framesPerBuffer,settings);
	audioOutput = new SoundCard(this,downSampleRate,downSampleFrames,settings);
	noiseFilter = new NoiseFilter(downSampleRate,downSampleFrames);
	signalStrength = new SignalStrength(downSampleRate,downSampleFrames);
	//FIR MAC LP Filter
	lpFilter = new FIRFilter(downSampleRate,downSampleFrames, false, 128);
	lpFilter->SetLowPass(3000);
	//lpFilter->SetHighPass(3000); //Testing 
	//lpFilter->SetBandPass(50,3000); //Testing

    morse = new Morse(downSampleRate,downSampleFrames);
    morse->SetReceiver(this);

	//Testing, time intensive for large # taps, ie @512 we lose chunks of signal
	//Check post-bandpass spectrum and make just large enough to be effective
	//64 is too small, 128 is good, ignored if useFFT arg == true
	if (useFreqDomainChain)
		//In freq domain chain we have full size fft available
		bpFilter = new FIRFilter(sampleRate, framesPerBuffer,true, 128);
	else
		bpFilter = new FIRFilter(downSampleRate, downSampleFrames,true, 128);
	bpFilter->setEnabled(true);
	
	agc = new AGC(downSampleRate, downSampleFrames);

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
    audioOutput->Start(0,downSampleRate);
	sdr->Start();

	return true;
}
//Delete everything that's based on settings, so we can change receivers, sound cards, etc
//ReceiverWidget initiates the power off and call us
bool Receiver::Off()
{
	powerOn = false;

	//Carefull with order of shutting down
	if (settings->sdrDevice == SDR::SDR_IQ) {
	}
	//Stop incoming samples first
	if (sdr != NULL)
		sdr->Stop();
	if (audioOutput != NULL)
		audioOutput->Stop();

	//Now clean up rest
	if (audioOutput != NULL) {
		delete audioOutput;
		audioOutput = NULL;
	}
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
	settings->lastDisplayMode = receiverWidget->GetDisplayMode();
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
        demod->SetDemodMode(m, sampleRate, downSampleRate);
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
}

void Receiver::ProcessBlockTimeDomain(CPX *in, CPX *out, int frameCount)
{
	CPX *nextStep = in;

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

	//Signal (Specific frequency) processing
	//Mixer shows no loss in testing
	nextStep = mixer->ProcessBlock(nextStep);
	//float post = SignalProcessing::TotalPower(nextStep,frameCount); 
	signalSpectrum->PostMixer(nextStep);



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

    if (demod->DemodMode() == dmFMMono || demod->DemodMode() == dmFMStereo) {
        //These steps are NOT at downSamplerate
        //Special handling for wide band fm

        //Do we need bandPass filter or handle in demod?
        nextStep = demod->ProcessBlock(nextStep,framesPerBuffer);

        //Demod wil Decimate to audio output rate

    } else {
        //Filter above downSampleRate so we don't get aliases
        if (settings->postMixerDecimate && downSampleFactor != 1){
            //We need to worry about aliasing, so LP filter to match downSample
            nextStep = downSampleFilter->ProcessBlock(nextStep);
            for(int i=0,j=0; i<framesPerBuffer; i+=downSampleFactor,j++)
                nextStep[j] = nextStep[i];
        }

        //float pre = SignalProcessing::TotalPower(nextStep,frameCount);
        nextStep = bpFilter->ProcessBlock(nextStep);
        //Crude AGC, too much fluctuation
        //CPXBuf::scale(nextStep,nextStep,pre/post,frameCount);
        signalSpectrum->PostBandPass(nextStep,downSampleFrames);

        //If squelch is set, and we're below threshold and should set output to zero
        //Do this in SignalStrength, since that's where we're calculating average signal strength anyway
        nextStep = signalStrength->ProcessBlock(nextStep, squelch);

        //Tune only mode, no demod or output
        if (demod->DemodMode() == dmNONE){
            CPXBuf::clear(out,downSampleFrames);
            return;
        }
        nextStep = noiseFilter->ProcessBlock(nextStep);

        //nr->ProcessBlock(out, in, size);
        //float post = SignalProcessing::totalPower(nextStep,framesPerBuffer);

        nextStep = agc->ProcessBlock(nextStep);

        nextStep = demod->ProcessBlock(nextStep, downSampleFrames);

        /*
          Todo: Add post demod FFT to check FM stereo and other composite formats
        */

        //Testing Goertzel
        if (dataSelection == ReceiverWidget::CW_DATA)
            nextStep = morse->ProcessBlock(nextStep);


        //Testing LPF to get rid of some noise after demod
        nextStep = lpFilter->ProcessBlock(nextStep);
    }
	// apply volume setting
	//Todo: gain should be a factor of slider, receiver, filter, and mode, and should be normalized
	//	so changing anything other than slider doesn't impact overall volume.
	//	or should filter normalize gain

	//CPXBuf::scale(out, nextStep, gain * filterLoss, frameCount);
	if (mute)
		CPXBuf::clear(out, downSampleFrames);
	else
		CPXBuf::scale(out, nextStep, gain, downSampleFrames);

	audioOutput->SendToOutput(out);
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
	for(int i=0,j=0; i<framesPerBuffer; i+=downSampleFactor,j++)
		nextStep[j] = nextStep[i];

    nextStep = demod->ProcessBlock(nextStep,downSampleFrames);


	if (mute)
		CPXBuf::clear(out, downSampleFrames);
	else
		CPXBuf::scale(out, nextStep, gain, downSampleFrames);

	audioOutput->SendToOutput(out);

}
