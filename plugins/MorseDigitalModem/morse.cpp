//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "morse.h"
#include "QPainter"
#include "QDebug"
#include "QEvent"
//#include "receiver.h"

/*
  CW Notes for reference
  Tcw is basic unit of measurement in ms
  Dash = 3 Tcw, Dot = 1Tcw, Element Space = 1 Tcw, Character Space = 3 Tcw, Word Space = 7 Tcw (min)
  Standard for measuring WPM is to use the word "PARIS", which is exactly 50Tcw
  So Tcw (ms) = 60 (sec) / (WPM * 50)
  and WPM = 1200/Tcw(ms)

  Use DOT_MAGIC to convert WPM to/from usec is 1,200,000
  DOT_MAGIC / WPM = usec per TCW

  Tcw for 12wpm = 60 / (12 * 50) = 60 / 600 = .100 sec  100ms
  Tcw for 30wpm = 60 / 1500 = .040 40ms
  Tcw for 120wpm = 60 / (120 * 50) = 60 / 6000 = .01 sec  10ms

  The 'binWidth' in the Goertzel algorithm determines how many samples should be processed before returning a result.
  The smaller the binWidth, the more samples we need to process.  See goertzel.cpp for detail
  There is a potential trade off between binWidth and Tcw resolution
  We need to make sure the 'timePerBin' (#samples per bin * timePerSample) syncs up with Tcw requirements

  Each Goertzel 'bin' should be at least Tcw * 0.25, so we get at least 4 bins per Tcw
  15wpm example
  Sampling at 8khz, each sample is 1/8000 = .000125sec
  for 20ms bin (25% of 80ms) we need to accumulate 160 samples, which also gives us a binWidth of 50hz, ie too sharp

  30wpm example
  for 10ms bin (25% of 40ms) we need to accumulate 80 samples, which also gives us a binWidth of 100hz, ie very sharp

  60wpm example
  for 5ms bin (25% of 20ms) we need to accumulate 40 samples, which also gives us a binWidth of 200hz, ie not as sharp
  Therefore, as WPM goes up, goertzel bandwidth (binWidth) goes up
  If we set for 60wpm, we can also handle 15 and 30 by accumulating 16 and 8 results instead of 4;

  A 5ms bin will handle any WPM up to 60, we just need to keep track of how many bins we need per Tcw
  For any binWidth, the maximum WPM is calculated as
  maxWpm = 1200 / (timePerBin * 4)

  If we tried to use Goertzel directly on a 96k sample rate, we would need to process 9600 samples to get a 100ms block!!!
  So to make Goertzel practical, we will need to decimate to a lower rate, like 8k


  Wikipedia reference  http://en.wikipedia.org/wiki/Morse_code

    International Morse code is composed of five elements:

    short mark, dot or 'dit' (·) — 'dot duration' is one unit long
    longer mark, dash or 'dah' (–) — three units long
    inter-element gap between the dots and dashes within a character — one dot duration or one unit long
    short gap (between letters) — three units long
    medium gap (between words) — seven units long

    or as binary

    short mark, dot or 'dit' (·) — 1
    longer mark, dash or 'dah' (–) — 111
    intra-character gap (between the dots and dashes within a character) — 0
    short gap (between letters) — 000
    medium gap (between words) — 0000000
    Note that this method assumes that dits and dahs are always separated by dot duration gaps, and that gaps are always separated by dits and dahs.

             1         2         3         4         5         6         7         8
    12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789

    M------   O----------   R------   S----   E       C----------   O----------   D------   E
    ===.===...===.===.===...=.===.=...=.=.=...=.......===.=.===.=...===.===.===...===.=.=...=
       ^               ^    ^       ^             ^
       |              dah  dit      |             |
    symbol space                letter space    word space

    Another good reference on tone detection
    Tone Detection with a Quadrature Receiver
    James E. Gilley
    http://www.efjohnsontechnologies.com/resources/dyn/files/75829/_fn/analog_tone_detection.pdf

*/

// Tone and timing magic numbers.
#define	DOT_MAGIC	1200000	// Dot length magic number - shortcut for full math.  The Dot length is 1200000/WPM Usec

//Relative lengths of elements in Tcw terms
#define TCW_DOT 1
#define TCW_DASH 3
#define TCW_CHAR 3
#define TCW_ELEMENT 1
#define TCW_WORD 7

#define MIN_WPM 5
#define MIN_SAMPLES_PER_TCW 100

Morse::Morse()
{
	m_dataUi = NULL;
	m_workingBuf = NULL;
	m_toneFilter = NULL;
	m_avgBitPowerFilter = NULL;
	m_avgThresholdFilter = NULL;
	m_decimate = NULL;
	m_mixer = NULL;
}

void Morse::setSampleRate(int _sampleRate, int _sampleCount)
{
	m_sampleRate = _sampleRate;
	m_numSamples = _sampleCount;
	m_out = new CPX[m_numSamples];
	m_workingBuf = CPX::memalign(m_numSamples);

	if (m_decimate == NULL)
		m_decimate = new Decimator(_sampleRate, _sampleCount);
	if (m_mixer == NULL)
		m_mixer = new Mixer(_sampleRate, _sampleCount);

	m_morseCode.init();
    resetDotDashBuf();
	m_modemClock = 0; //Start timer over


    //Modem sample rate, device sample rate decimated to this before decoding
	m_modemBandwidth = 1000; //8000; //Desired bandwidth, not sample rate
    //See if we can decimate to this rate
	int actualModemRate = m_decimate->buildDecimationChain(m_sampleRate, m_modemBandwidth, 8000);
	m_modemSampleRate = actualModemRate;

    //Testing fixed limits
    //Determine shortest and longest mark we can time at this sample rate
	m_usecPerSample = (1.0 / m_modemSampleRate) * 1000000;
	m_usecShortestMark =  m_usecPerSample * MIN_SAMPLES_PER_TCW; //100 samples per TCW
	m_usecLongestMark = DOT_MAGIC / MIN_WPM; //Longest dot at slowest speed we support 5wpm

	m_modemFrequency = 1000; //global->settings->modeOffset;


    //fldigi constructors
	m_squelchIncrement = 0.5;
	m_squelchValue = 0;
	m_squelchEnabled = false;

	m_wpmSpeedCurrent = m_wpmSpeedFilter = m_wpmSpeedInit;
	calcDotDashLength(m_wpmSpeedInit, m_usecDotInit, m_usecDashInit);

    //2 x usecDot
	m_usecAdaptiveThreshold = 2 * DOT_MAGIC / m_wpmSpeedCurrent;
	m_usecNoiseSpike = m_usecAdaptiveThreshold / 4;

	m_phaseacc = 0.0;
	m_FFTphase = 0.0;
	m_FIRphase = 0.0;
	m_FFTvalue = 0.0;

	m_agc_peak = 1.0;

	m_toneFilter = new C_FIR_filter();
    //Filter is at modem sample rate
	double f = m_wpmSpeedInit / (1.2 * m_modemSampleRate);
	m_toneFilter->init_lowpass (CW_FIRLEN, DEC_RATIO, f);

    // bit filter based on 10 msec rise time of CW waveform
	int bfv = (int)(m_modemSampleRate * .010 / DEC_RATIO); //8k*.01/16 = 5
	m_avgBitPowerFilter = new MovingAvgFilter(bfv);

	m_avgThresholdFilter = new MovingAvgFilter(TRACKING_FILTER_SIZE);

    syncTiming();

	m_demodMode = DeviceInterface::dmCWL;
	m_outputBufIndex = 0;


    init();

    //Used to update display from main thread.
    connect(this, SIGNAL(newOutput()), this, SLOT(refreshOutput()));

	m_outputOn = false;

}

//Implement pure virtual destructor from interface, otherwise we don't link
DigitalModemInterface::~DigitalModemInterface()
{

}

Morse::~Morse()
{
	if (m_workingBuf != NULL) free (m_workingBuf);

	if (m_toneFilter != NULL) delete m_toneFilter;
    //if (cw_FFT_filter) delete cw_FFT_filter;
	if (m_avgBitPowerFilter != NULL) delete m_avgBitPowerFilter;
	if (m_avgThresholdFilter != NULL) delete m_avgThresholdFilter;
	if (m_decimate != NULL) delete m_decimate;
	if (m_mixer != NULL) delete m_mixer;

}

void Morse::setupDataUi(QWidget *parent)
{
    if (parent == NULL) {
		m_outputOn = false;

        //We want to delete
		if (m_dataUi != NULL) {
			delete m_dataUi;
        }
		m_dataUi = NULL;
        return;
	} else if (m_dataUi == NULL) {
        //Create new one
		m_dataUi = new Ui::dataMorse();
		m_dataUi->setupUi(parent);

		m_dataUi->dataBar->setValue(0);
		m_dataUi->dataBar->setMin(0);
		m_dataUi->dataBar->setMax(10);
		m_dataUi->dataBar->setNumTicks(10);
		m_dataUi->dataBar->start();

		m_squelchValue = 0;
		m_squelchEnabled = false;
		m_dataUi->squelchSlider->setMinimum(0);
		m_dataUi->squelchSlider->setMaximum(10 / m_squelchIncrement);
		m_dataUi->squelchSlider->setSingleStep(1);
		m_dataUi->squelchSlider->setValue(m_squelchValue);
		connect(m_dataUi->squelchSlider,SIGNAL(valueChanged(int)),this,SLOT(squelchChanged(int)));

		m_dataUi->dataEdit->setAutoFormatting(QTextEdit::AutoNone);
		m_dataUi->dataEdit->setAcceptRichText(false);
		m_dataUi->dataEdit->setReadOnly(true);
		m_dataUi->dataEdit->setUndoRedoEnabled(false);
		m_dataUi->dataEdit->setWordWrapMode(QTextOption::WordWrap);

		connect(m_dataUi->resetButton,SIGNAL(clicked()),this,SLOT(resetOutput()));

		m_dataUi->onBox->setChecked(true);
		connect(m_dataUi->onBox,SIGNAL(toggled(bool)), this, SLOT(onBoxChecked(bool)));

		m_dataUi->outputOptionBox->addItem("Character",CHAR_ONLY);
		m_dataUi->outputOptionBox->addItem("DotDash",DOTDASH);
		m_dataUi->outputOptionBox->addItem("Both",CHAR_AND_DOTDASH);
		m_dataUi->outputOptionBox->setCurrentIndex(0);
		m_outputMode = CHAR_ONLY;
		connect(m_dataUi->outputOptionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(outputOptionChanged(int)));

		m_lockWPM = false;
		connect(m_dataUi->lockWPMBox,SIGNAL(toggled(bool)),this,SLOT(lockWPMChanged(bool)));

		m_outputOn = true;
    }

}

QString Morse::getPluginName()
{
    return "Morse";
}

QString Morse::getPluginDescription()
{
    return "Morse code";
}

//Returns tcw in ms for any given WPM
int Morse::wpmToTcw(int w)
{
    int tcw = 60.0 / (w * 50) * 1000;
    return tcw;
}
//Returns wpm for any given tcw (in ms)
int Morse::tcwToWpm(int t)
{
    int wpm = 1200/t;
    return wpm;
}

// Usec = DM / WPM
// DM = WPM * Usec
// WPM = DM / Usec
quint32 Morse::wpmToUsec(int w)
{
    return DOT_MAGIC / w;
}
int Morse::usecToWPM(quint32 u)
{
    return DOT_MAGIC / u;
}

void Morse::setDemodMode(DeviceInterface::DemodMode _demodMode)
{
	m_demodMode = _demodMode;
}

void Morse::onBoxChecked(bool b)
{
	m_outputOn = b;
}

void Morse::outputOptionChanged(int s)
{
	m_outputMode = (OUTPUT_MODE)m_dataUi->outputOptionBox->itemData(s).toInt();
}

void Morse::lockWPMChanged(bool b)
{
	m_lockWPM = b;
    //Anything else?
}

//Called from reset button
void Morse::resetOutput()
{
	m_dataUi->dataEdit->clear();
    init(); //Reset all data
    refreshOutput();
}

//Handles newOuput signal
void Morse::refreshOutput()
{
	if (!m_outputOn)
        return;

	m_outputBufMutex.lock();
	m_dataUi->wpmLabel->setText(QString().sprintf("%d WPM",m_wpmSpeedCurrent));

	for (int i=0; i<m_outputBufIndex; i++) {
		m_dataUi->dataEdit->insertPlainText(QString(m_outputBuf[i])); //At cursor
		m_dataUi->dataEdit->moveCursor(QTextCursor::End);
    }
	m_outputBufIndex = 0;
	m_outputBufMutex.unlock();
}

void Morse::outputData(const char* d)
{
	if (m_dataUi == NULL)
        return;

	m_dataUi->dataEdit->insertPlainText(d); //At cursor
	m_dataUi->dataEdit->moveCursor(QTextCursor::End);
}
void Morse::outputData(const char c)
{
	Q_UNUSED(c);
	if (m_dataUi == NULL)
        return;
    //Remove

    return;
}

void Morse::squelchChanged(int v)
{
    if (v == 0) {
		m_squelchEnabled = false;
    } else {
		m_squelchValue = v * m_squelchIncrement; //Slider is in .5 increments so 2 inc = 1 squelch value
		m_squelchEnabled = true;
    }
}


//Early experiment with Goertzel abandoned in favor of fldigi style filter
//But we may go back and test Goertzel vs fldigi at some point in the future
//fldigi has it's own goertzel algorithm, choose one or the other
//We also experiemented with processing the entire sample buffer and turning it into a tone buffer
//Then process the tone buffer looking for characters.
//Advantage is that we an look back and look ahead to make a decision about anomolies
//Disadvantage is we have to carry tone buffer across sample buffers so we don't loose anything



/*
Implemented ideas from Fldigi algorithms
Portions from JSDR
Copyright (C) 2008, 2009, 2010
Jan van Katwijk (J.vanKatwijk@gmail.com

From AG1LE Blog
Main modules are in CW.CXX  and MORSE.CXX.  After initialization the  cw:rx_process() function receives new audio data 512 samples on continuous basis (this block size is actually configured in sound.h  - see #define SCBLOCKSIZE  ).

Once a block of data is received, there are some checks done if user has changed the low pass filter bandwidth. Then the  algorithm creates a baseband signal by mixing with audio carrier frequency of selected signal. The next step is to run a low pass filter,  demodulate by taking the magnitude of the signal and finally running the envelope through a moving average filter.  There is also AGC (automatic gain control) function with fast attack and slow decay. A check is done if squelch is on and if signal exceeds squelch value (this is to reduce noise created errors).  If the signal has upward trend the control gets passed to handle_event() function and same is done with downward trend.  Handle_event() keeps track of  "dits" and "dahs"  and eventually returns decoded character using morse::rx_lookup() function.

There is actually much more going on than the above simple explanation. The software keeps track of morse speed, automatic gain control,  dit/dah ratio and various other parameters.  Also, the user interface is updated - if you have signal scope on the signal amplitude value is updated between characters etc.
*/


/*
RAL Notes
fldigi sample rate is 8k
mixer runs at sample rate
then is filtered
filter returns result every DEC_RATIO samples

*/

// SHOULD ONLY BE CALLED FROM THE rx_processing loop
// This is called on every ProcessBlock loop to see if anything has changed between loops
// Should be renamed checkForChanges() or something
void Morse::syncFilterWithWpm()
{
    //If anything had changed from the defaults we started with, rest
    //if (use_fft_filter != progdefaults.CWuse_fft_filter ||
	if(	m_wpmSpeedCurrent != m_wpmSpeedFilter ) {

		m_wpmSpeedFilter = m_wpmSpeedCurrent;

        //if (use_fft_filter) { // FFT filter
        //	cw_FFT_filter->create_lpf(progdefaults.CWspeed/(1.2 * modemSampleRate));
        //	FFTphase = 0;
        //} else { // FIR filter
			m_toneFilter->init_lowpass (CW_FIRLEN, DEC_RATIO , m_wpmSpeedCurrent / (1.2 * m_modemSampleRate));
			m_FIRphase = 0;
        //}

		m_phaseacc = 0.0;
		m_FFTphase = 0.0;
		m_FIRphase = 0.0;
		m_FFTvalue = 0.0;

		m_agc_peak = 0;
    }
}

//Not used in fldigi, but there for CPX samples
CPX Morse::mixer(CPX in)
{
	CPX z (cos(m_phaseacc), sin(m_phaseacc));
    z = z * in;

	m_phaseacc += TWOPI * m_modemFrequency / m_modemSampleRate;
	if (m_phaseacc > M_PI)
		m_phaseacc -= TWOPI;
	else if (m_phaseacc < M_PI)
		m_phaseacc += TWOPI;

    return z;
}

void Morse::init()
{
	m_wpmSpeedCurrent = m_wpmSpeedInit;
	m_usecAdaptiveThreshold = 2 * DOT_MAGIC / m_wpmSpeedCurrent;
    syncTiming(); //Based on wpmSpeedCurrent & adaptive threshold

	m_useNormalizingThreshold = true; //Fldigi mode
	m_agc_peak = 0;
	m_outputMode = CHAR_ONLY;
	m_avgThresholdFilter->reset();
    resetModemClock();
    resetDotDashBuf(); //So reset can refresh immediately
	m_lastReceiveState = m_receiveState;
	m_receiveState = IDLE; //Will reset clock and dot dash
}

// Compare two timestamps, and return the difference between them in usecs.

quint32 Morse::modemClockToUsec(unsigned int earlier, unsigned int later)
{
    quint32 usec = 0;
    // Compare the timestamps.
    // At 4 WPM, the dash length is 3*(1200000/4)=900,000 usecs, and
    // the word gap is 2,100,000 usecs.
    if (earlier < later) {
		usec = (quint32) (((double) (later - earlier) * m_usecsPerSec) / m_modemSampleRate);
    }
    return usec;
}

//Recalcs all values that are speed related
//Called in constructor, reset_rx_filter, synctiming
//Init values should never change, so this should only be called once with wpmSpeedInit as arg
void Morse::calcDotDashLength(int _speed, quint32 & _usecDot, quint32 & _usecDash)
{
    //Special case where _speed is zero
    if (_speed > 0)
        _usecDot = DOT_MAGIC / _speed;
    else
        _usecDot = DOT_MAGIC / 5;

    _usecDash = 3 * _usecDot;

}

//=======================================================================
// updateAdaptiveThreshold()
// This gets called everytime we have a dot dash sequence or a dash dot
// sequence. Since we have semi validated tone durations, we can try and
// track the cw speed by adjusting the cw_adaptive_receive_threshold variable.
// This is done with moving average filters for both dot & dash.
//=======================================================================

//This should be only place where adaptiveThreshold and wpmSpeedCurrent are changed
void Morse::updateAdaptiveThreshold(quint32 idotUsec, quint32 idashUsec)
{
    quint32 dotUsec, dashUsec;
	if (idotUsec > m_usecShortestMark && idotUsec < m_usecLongestMark)
        dotUsec = idotUsec;
    else
		dotUsec = m_usecDotInit;
	if (idashUsec > m_usecShortestMark && idashUsec < m_usecLongestMark)
        dashUsec = idashUsec;
    else
		dashUsec = m_usecDashInit;

    //Result is midway between last short and long mark, assumed to be dot and dash
	m_usecAdaptiveThreshold = (quint32)m_avgThresholdFilter->newSample((dashUsec + dotUsec) / 2);

    //Current speed estimate
	if (!m_lockWPM)
		m_wpmSpeedCurrent = DOT_MAGIC / (m_usecAdaptiveThreshold / 2);


}

// sync_parameters()
// Synchronize the dot, dash, end of element, end of character, and end
// of word timings and ranges to new values of Morse speed, or receive tolerance.
// Changes
// usecNoiseSpike
// useUpperLimit and usecLowerLimit based on wpm

//Called once per buffer to update timings that we don't want to change every mark/space
//UpdateAdaptiveThreshold called every dot/dash for timings that need to change faster
void Morse::syncTiming()
{
    //CalcDotDash handles special case where speed is zero
	calcDotDashLength(m_wpmSpeedCurrent, m_usecDotCurrent, m_usecDashCurrent);
    //Adaptive threshold is roughly 2 TCW, so midway between dot and dash
	m_usecNoiseSpike = m_usecAdaptiveThreshold / 4;
    //Make this zero to accept any inter mark space as valid
    //Adaptive threshold is typically 2TCW so /8 = .25TCW
	m_usecElementThreshold = m_usecAdaptiveThreshold / 8;

}

//All the things we need to do to start a new char
void Morse::resetDotDashBuf()
{
	memset(m_dotDashBuf, 0, sizeof(m_dotDashBuf));
	m_dotDashBufIndex = 0; //Index to dotDashBuf
}

//Reset and sync all timing elements, usually called with resetDotDashBuf()
void Morse::resetModemClock()
{
	m_modemClock = 0;
	m_toneStart = 0;
	m_toneEnd = 0;
	m_usecMark = 0;
	m_usecSpace = 0;
}

/*
 * Prep for modem, downsample and mix to CWU or CWL offset freq
 * For each sample
 *  Call modem to get tone value (all samples first so we can do look ahead/back for analysis?, or sample at a time?)
 *  Smooth to filter noise spikes and signal dropouts
 *  Call decode_stream to turn into dot/dash/space/noise (rename)
 *  Call state machine to process dot/dash
 *  Check for character to output
 *  Adapt timing values for next char
 *
 *  Other thoughts
 *  Look back at previous tone values when short noise spike or signal drop occurs and infer that last in-range value should be coppied
 *  to replace noise or signal drop?
 *
 *  Keep agc-Min value to track average noise floor to make value comparison more relevant?
 *
*/

//My version of FIRprocess + rx_processing for Pebble
CPX * Morse::processBlock(CPX *in)
{

    CPX z;
	double tonePower;

    //If WPM changed in last block, dynamicall or by user, update filter with new speed
    syncFilterWithWpm();

    //Downconverter first mixes in place, ie changes in!  So we have to work with a copy
	CPX::copyCPX(m_workingBuf,in,m_numSamples);

    //We need to account for modemOffset in ReceiverWidget added so we hear tone but freq display is correct
    //Actual freq for CWU will be freq + modemFrequency for CWL will be freq -modemFrequency.
    //And we want actual freq to be at baseband
	if (m_demodMode == DeviceInterface::dmCWL)
		m_mixer->SetFrequency(-m_modemFrequency);
	else if (m_demodMode == DeviceInterface::dmCWU)
		m_mixer->SetFrequency(m_modemFrequency);
    else
        //Other modes, like DIGU and DIGL will still work with cursor on signal, but we won't hear tones.  Feature?
		m_mixer->SetFrequency(0);

	CPX *mixedBuf = m_mixer->ProcessBlock(m_workingBuf); //In place
	int numModemSamples = m_decimate->process(mixedBuf,m_out,m_numSamples);

    //Now at lower modem rate with bandwidth set by modemDownConvert in constructor
    //Verify that testbench post banpass signal looks the same, just at modemSampleRate
    //global->testBench->DisplayData(numModemSamples,this->out, modemSampleRate, PROFILE_5);

    for (int i = 0; i<numModemSamples; i++) {
		m_modemClock++; //1 tick per sample

#if 0
        //Mix to get modemFrequency at baseband so narrow filter can detect magnitude
        z = CPX (cos(FIRphase), sin(FIRphase) ) * out[i];
        z *= 1.95;
        FIRphase += TWOPI * (double)modemFrequency / (double)modemSampleRate;
        if (FIRphase > M_PI)
            FIRphase -= TWOPI;
        else if (FIRphase < M_PI)
            FIRphase += TWOPI;
#endif
		z = m_out[i] * 1.95; //A little gain - same as fldigi

		//DEC_RATIO is the equivalent of N in Goertzel
        //cw_FIR_filter does MAC and returns 1 result every DEC_RATIO samples
        //LowPass filter establishes the detection bandwidth (freq)
        //and Detection time (DEC_RATIO)
        //If the LP filter passes the tone, we have detection
        //We need some sort of AGC here to get consistent tone thresholds
		if (m_toneFilter->run ( z, z )) {

            // demodulate and get power in signal
			tonePower = z.mag(); //sqrt sum of squares
            //or
			//tonePower = z.sqrMag(); //just sum of squares

            //Moving average to handle jitter during CW rise and fall periods
			tonePower = m_avgBitPowerFilter->newSample(tonePower);

            //Main logic for timing and character decoding
			double thresholdUp;
			double thresholdDown;

			// Compute a variable threshold value for tone detection
			// Fast attack and slow decay.
			if (tonePower > m_agc_peak)
				m_agc_peak = MovingAvgFilter::weightedAvg(m_agc_peak, tonePower, 20); //Input has more weight on increasing signal than decreasing
			else
				m_agc_peak = MovingAvgFilter::weightedAvg(m_agc_peak, tonePower, 800);

			//metric is what we use to determine whether to squelch or not
			//Also what could be displayed on 0-100 tuning bar
			//Clamp to min max value, peak can never be higher or lower than this
			m_squelchMetric = qBound(0.0, m_agc_peak * 2000 , 100.0);
			//m_squelchMetric = qBound(0.0, m_agc_peak * 1000 , 100.0); //JSPR uses 1000, where does # come from?

			//global->testBench->SendDebugTxt(QString().sprintf("V: %f A: %f, M: %f",value, agc_peak, metric));
			//qDebug() << QString().sprintf("V: %f A: %f, M: %f",value, agc_peak, metric);

			if (m_useNormalizingThreshold) {
			// Calc threshold between rising and falling tones by normalizing value to agc_peak (Fldigi technique)
				tonePower = m_agc_peak > 0 ? tonePower/m_agc_peak : 0;
				thresholdUp = 0.60; //Fixed value because we normalize above
				thresholdDown = 0.40;
			} else {
				//Super-Ratt and JSDR technique
				//Divide agc_peak in thirds,  upper is rising, lower is falling, middle is stable
				thresholdUp = m_agc_peak * 0.67;
				thresholdDown = m_agc_peak * 0.33;
			}

			//Check squelch to avoid noise errors

			if (!m_squelchEnabled || m_squelchMetric > m_squelchValue ) {
				//State machine handles all transitions and decisions
				//We just determine if value indicated key down or up status

				// Power detection using hysterisis detector
				// upward trend means tone starting
				if (tonePower > thresholdUp) {
					stateMachine(TONE_EVENT);
				}

				//Don't do anything if in the middle?  Prevents jitter that would occur if just compare with single value

				// downward trend means tone stopping
				if (tonePower < thresholdDown) {
					stateMachine(NO_TONE_EVENT);
				}

			} else {
				//Light in-squelch indicator (TBD)
				//Keep timing
				qDebug()<<"In squelch";
			}

        }


        //Its possible we get called right after dataUi has been been instantiated
        //since receive loop is running when CW window is opened
		if (m_dataUi != NULL && m_dataUi->dataBar!=NULL)
			m_dataUi->dataBar->setValue(m_squelchMetric); //Tuning bar
    }

    return in;
}


/*
 State          Event       Action                  New State
 -----------------------------------------------------------------------------
 IDLE           TONE        ResetClock
                            ResetDotDash            MARK_TIMING

                NO_TONE     Ignore                  IDLE
 -----------------------------------------------------------------------------

 MARK_TIMING    TONE        Keep timing             MARK_TIMING

                NO_TONE     Possible end of tone
                            ToneEnd/usecMark        INTER_ELEMENT
 -----------------------------------------------------------------------------

 INTER_ELEMENT  TONE        Valid? DotDash          MARK_TIMING
                            Invalid? Option1        MARK_TIMING
                            Invalid? Option2        IDLE

                NO_TONE     !ShortDrop & !Handled
                            Add DotDash
                            Handled = true

                            < ElementThreshold?
                            Keep timing space       INTER_ELEMENT
                            > ElementThreshold?
                            Output Char             WORD_TIMING
 -----------------------------------------------------------------------------
 WORD_TIMING    TONE        Reset                   MARK_TIMING

                NO_TONE     < Word Threshold
                            Keep timing             WORD_TIMING

                            > Word Threshold
                            Output Space            IDLE
 -----------------------------------------------------------------------------

*/

//Returns aren't used and are random
//We could return next state just to be useful
bool Morse::stateMachine(CW_EVENT event)
{
    const char *outStr;
    //State sequence normally follows the same order as the switch statement
    //For each state, handle all possible events
	switch (m_receiveState) {
        case IDLE:
            switch (event) {
                //Tone filter output is trending up
                case TONE_EVENT:
                    resetDotDashBuf();
                    resetModemClock(); //Start timer over
					m_lastReceiveState = m_receiveState;
					m_receiveState = MARK_TIMING;
                    break;

                //Tone filter output is trending down
                case NO_TONE_EVENT:
                    //Don't care about silence in idle state
                    break;
            }
            break;
        case MARK_TIMING:
            switch (event) {
                case TONE_EVENT:
                    //Keep timing
                    //Check for long tuning tone and ignore ??
                    break;

                case NO_TONE_EVENT:
                    //Transition from mark to space
                    // Save the current timestamp so we can see how long tone was
					m_toneEnd = m_modemClock;
					m_usecMark = modemClockToUsec(m_toneStart, m_toneEnd);
                    // If the tone length is shorter than any noise cancelling
                    // threshold that has been set, then ignore this tone.
					if (m_usecNoiseSpike > 0
						&& m_usecMark < m_usecNoiseSpike) {
                        break; //Still in mark timing state
                    }

                    //dumpStateMachine("KEYUP_EVENT enter");
					m_usecSpace = 0;
                    //Could be valid tone or could just be a short signal drop
					m_markHandled = false; //Defer to space timing
					m_lastReceiveState = m_receiveState;
					m_receiveState = INTER_ELEMENT_TIMING;
                    break;
            }
            break;

        case INTER_ELEMENT_TIMING:
            switch (event) {
                case TONE_EVENT:
                    //Looking for inter-element space
                    //If enough time has gone by we've already handled in NO_TONE_EVENT
					if (!m_markHandled) {
                        //Too short
						if (m_usecSpace < m_usecNoiseSpike) {
                        //Just continue as if space never happened
                            //Don't reset clock, we're still timing mark
							m_lastReceiveState = m_receiveState;
							m_receiveState = MARK_TIMING;
                        } else {
                        //Throw it away and start over
							m_lastReceiveState = m_receiveState;
							m_receiveState = IDLE;
                        }
                    } else {
                        //If there was enough time for char out we wouldn't get here
                        resetModemClock();
						m_lastReceiveState = m_receiveState;
						m_receiveState = MARK_TIMING;
                    }
                    break;

                case NO_TONE_EVENT:
					m_usecSpace = modemClockToUsec(m_toneEnd, m_modemClock); //Time from tone end to now
					if (!m_markHandled && m_usecSpace > m_usecElementThreshold) {
                        addMarkToDotDash();
						m_markHandled = true;
                    }
                    //Anything waiting for output?
					outStr = m_spaceTiming(true); //Looking for char
                    if (outStr != NULL) {
                        //We will only get a char between 2 and 4 TCW of space
                        outputString(outStr);
                        //dumpStateMachine(outStr);
						m_usecLastSpace = m_usecSpace; //Almost always fixed because we start looking for char at 2x usecDot
                        resetDotDashBuf(); //Ready for new char
						m_lastReceiveState = m_receiveState; //We don't use lastReceiveState, here for completeness
						m_receiveState = WORD_SPACE_TIMING;
                    }
                    break;
            }
            break;
        case WORD_SPACE_TIMING:
            switch (event) {
                case TONE_EVENT:
                    //Any char has already been output, just start timing tond
#if 0
                    //Anything waiting for output?
                    outStr = spaceTiming(false);
                    if (outStr != NULL) {
                        outputChar(outStr);
                        //dumpStateMachine(*outStr);
                    }
#endif
					m_usecLastSpace = m_usecSpace;
                    resetDotDashBuf(); //Ready for new char
                    resetModemClock();
					m_lastReceiveState = m_receiveState; //We don't use lastReceiveState, here for completeness
					m_receiveState = MARK_TIMING;
                    break;

                case NO_TONE_EVENT:
					m_usecSpace = modemClockToUsec(m_toneEnd, m_modemClock); //Time from tone end to now
                    //Anything waiting for output?
					outStr = m_spaceTiming(false);
                    if (outStr != NULL) {
                        outputString(outStr);
						m_usecLastSpace = m_usecSpace;

                        //dumpStateMachine(*outStr);
						m_lastReceiveState = m_receiveState;
						m_receiveState = IDLE; //Takes care of resets
                    }
                    break;
            }
            break;
    }
    // should never get here... catch all
    return false;
}

//Got a tone that was long enough with a post tone space that was long enough
void Morse::addMarkToDotDash()
{
    //Process last mark element
    //uSecMark has timing
    // make sure our timing values are up to date after every tone
    syncTiming();

    // Set up to track speed on dot-dash or dash-dot pairs for this test to work, we need a dot dash pair or a
    // dash dot pair to validate timing from and force the speed tracking in the right direction. This method
    // is fundamentally different than the method in the unix cw project. Great ideas come from staring at the
    // screen long enough!. Its kind of simple really ... when you have no idea how fast or slow the cw is...
    // the only way to get a threshold is by having both code elements and setting the threshold between them
    // knowing that one is supposed to be 3 times longer than the other. with straight key code... this gets
    // quite variable, but with most faster cw sent with electronic keyers, this is one relationship that is
    // quite reliable. Lawrence Glaister (ve7it@shaw.ca)
	if (m_usecLastMark > 0) {
        // check for dot dash sequence (current should be 3 x last)
		if ((m_usecMark > 2 * m_usecLastMark) &&
			(m_usecMark < 4 * m_usecLastMark)) {
            //usecLastMark is dot
			updateAdaptiveThreshold(m_usecLastMark, m_usecMark);
        }
        // check for dash dot sequence (last should be 3 x current)
		if ((m_usecLastMark > 2 * m_usecMark) &&
			(m_usecLastMark < 4 * m_usecMark)) {
            //usecMark is dot and useLastMark is dash
			updateAdaptiveThreshold(m_usecMark, m_usecLastMark);
        }

    }
	m_usecLastMark = m_usecMark;
    //qDebug()<<"Mark: "<<usecMark;

    //RL We don't care about checking inter dot/dash spacing?  Should be 1 TCW
    // ok... do we have a dit or a dah?
    // a dot is anything shorter than 2 dot times
	if (m_usecMark <= m_usecAdaptiveThreshold) {
		m_dotDashBuf[m_dotDashBufIndex++] = CW_DOT_REPRESENTATION;
    } else {
        // a dash is anything longer than 2 dot times
		m_dotDashBuf[m_dotDashBufIndex++] = CW_DASH_REPRESENTATION;
    }

    // We just added a representation to the receive buffer.
    // If it's full, then reset everything as it probably noise
	if (m_dotDashBufIndex == m_receiveCapacity - 1) {
		m_lastReceiveState = m_receiveState;
		m_receiveState = IDLE;
        return;
    } else {
        // zero terminate representation so we can handle as char* string
		m_dotDashBuf[m_dotDashBufIndex] = 0;
    }
}

void Morse::outputString(const char *outStr) {
	if (!m_outputOn)
        return;

    if (outStr != NULL) {
        char c;

        //Display can be accessing at same time, so we need to lock
		m_outputBufMutex.lock();

        //Output character(s)
        int i = 0;
		while ((unsigned long)m_outputBufIndex < sizeof(m_outputBuf) &&
               outStr[i] != 0x00) {
            c = outStr[i++];
			m_outputBuf[m_outputBufIndex++] = m_useLowercase ? tolower(c) : c;
        }
        //Always null terminate
		m_outputBuf[m_outputBufIndex++] = 0x00;

		m_outputBufMutex.unlock();
        emit newOutput();

    }
}

//Processes post tone space timing
//Returns true if space was long enough to output something
//Return char or string as needed
//Uses dotDashBuf, could use usec silence
const char* Morse::m_spaceTiming(bool lookingForChar)
{
    CW_TABLE *cw;
    const char *outStr;

    if (lookingForChar) {
        // SHORT time since keyup... nothing to do yet
        //Could be inter-element space (1 TCW)
		if (m_usecSpace < (2 * m_usecDotCurrent)) {
            return NULL; //Keep timing silence

		} else if (m_usecSpace >= (2 * m_usecDotCurrent) &&
			m_usecSpace <= (4 * m_usecDotCurrent)) {
            // MEDIUM time since keyup... check for character space
            // one shot through this code via receive state logic
            // FARNSWOTH MOD HERE -->

            //Char space is 3 TCW per spec, but accept 2 to 4

            // Look up the representation
            outStr = NULL;
			if (*m_dotDashBuf != 0x00) {
				cw = m_morseCode.rx_lookup(m_dotDashBuf);
                if (cw != NULL) {
					if (m_outputMode == CHAR_ONLY)
                        outStr = cw->display;
					else if (m_outputMode == CHAR_AND_DOTDASH)
                        outStr = QString().sprintf("%s [ %s ] ",cw->display,cw->dotDash).toLocal8Bit();
					else if (m_outputMode == DOTDASH)
                        outStr = QString().sprintf(" [ %s ] ",cw->dotDash).toLocal8Bit();
                } else {
                    // invalid decode... let user see error
                    outStr = "*";
                }
            }
            return outStr; //Something output
        } else {
            //Should never get here looking for char
            dumpStateMachine("Invalid character timing");
        }
    } else {
        //Word space is 7TCW, accept anything greater than 4 or 5
		if ((m_usecSpace > (4 * m_usecDotCurrent))) {
            //We only get here if we've already output a char
            // LONG time since keyup... check for a word space
            // FARNSWOTH MOD HERE -->

            outStr = " ";
            //dumpStateMachine("Word Space");
            return outStr;
        } else {
            //Do nothing, char output and not long enough for word space
            return NULL;
        }
    }
    //If we get here it means that usecSilence wasn't reset properly, should be impossible
    dumpStateMachine("Impossible space Timing");
    return NULL;
}

QString Morse::stateToString(DECODE_STATE state)
{
    if (state==IDLE)
        return "IDLE";
    else if(state==MARK_TIMING)
        return "MARK";
    else if(state==INTER_ELEMENT_TIMING)
        return "INTER ELEMENT";
    else if (state == WORD_SPACE_TIMING)
        return "WORD";
    else
        return "UNKNOWN";

}

//Outputs all relevant information to qDebug()
void Morse::dumpStateMachine(QString why)
{
    //return;

    qDebug()<<"--- "<<why<<" ---";

	qDebug()<<"From state: "<<stateToString(m_lastReceiveState)<<" To state: "<<stateToString(m_receiveState);

	qDebug()<<"Clock: "<<m_modemClock<<" ToneStart: "<<m_toneStart<<" ToneEnd: "<<m_toneEnd;
	qDebug()<<"Timing: Dot: "<<m_usecDotCurrent<<" Dash: "<<m_usecDashCurrent<<" Threshold: "<<m_usecAdaptiveThreshold<<" Element: "<<m_usecMark<<" Silence: "<<m_usecSpace;
	qDebug()<<"Last Mark: "<<m_usecLastMark<<" Last Space: "<<m_usecLastSpace;
	qDebug()<<"WPM: "<<m_wpmSpeedCurrent;
}




/*
  For historic purposes: Some source code fragments from my SuperRatt AppleII CW code circa 1983

*TABLE OF CW CODES IN ASCII ORDER FROM $20 TO $3F
*1ST HIGH BIT IN # SIGNALS START OF CODE
*0=DIT 1=DAH
*
CWTBL    HEX   00C5002A45253600 ;SP,!,",#,$,%,&,'
         HEX   6D6D152873315532 ;(,),*,+,{,},-,.,/
         HEX   3F2F272321203038 ;0,1,2,3,4,5,6,7
         HEX   3C3E786A0000004C ;8,9,:,;,<,=,>,?
         HEX   0005181A0C02120E ;@,A,B,C,D,E,F,G
         HEX   1004170D1407060F ;H,I,J,K,L,M,N,O
         HEX   161D0A080309110B ;P,Q,R,S,T,U,V,W
         HEX   191B1C6D326D0000 ;X,Y,Z,{,\,},^,NA
*(#)=END OF MESSAGE ($)=END OF QSO (%)=END OF WORK (&)=GO AHEAD
*(*)=END OF LINE (+)=WAIT (!)=BREAK

* CW BYTE IN
* ENTRY:
* EXIT:
         AST   80
*
*
CBYTEIN  LDA   CWFLG      ;WAS LAST CHAR A WORD?
         BEQ   CWIN0      ;NO-CONTINUE
         LDA   #$A0       ;YES-PRINT SPACE
         DEC   CWFLG      ;RESET FLAG
         RTS
*
CWIN0    LDA   #1         ;INIT CHAR LOC
         STA   CWCHAR
         LDA   CWDITLEN   ;GET DITLEN IN MS
         NOP              ;IN CASE LSR NEEDED LATER
         LSR
         STA   LOC0       ;DELAY=1/20TH DIT LEN
         JMP   CWIN2      ;START
*
*ENTER HERE ONLY IF KEYPRESSED
CWIN1    CMP   #$99       ;CTRL Y?
         BNE   CWIN1A     ;NO
         LDA   #1
         STA   CWCHAR
         LDA   #40        ;1 LOOP=1/20TH DIT, SO 40 LOOPS=2 DITS
         STA   CWDIT
         LDA   #0
         STA   KEYLOC
         JMP   CWIN0
*
CWIN1A   RTS
*
CWIN2    JSR   CBITIN     ;KEY UP OR DOWN?
         BCC   KEYDOWN
*
KEYUP    LDA   #0         ;INIT COUNTER
         STA   CWUP
KEYUP1   INC   CWUP       ;ADD TIME TO COUNT
         BNE   KEYUP2
         LDA   CWCHAR     ;KEY UP TOO LONG
         CMP   #1         ;ANY CHAR TO DECODE
         BEQ   CWIN2      ;NO-CONTINUE
         INC   CWFLG      ;YES SET WORD FLAG
         JMP   CWIN6      ;AND GO TRY DECODE
*
KEYUP2   JSR   DELAY
         JSR   KEYTEST
         LDA   KEYLOC
         BEQ   KEYUP2A
         JMP   CWIN1      ;GO PROCESS CTRL KEY
KEYUP2A  JSR   CBITIN     ;STILL UP?
         BCS   KEYUP1     ;YES
*KEY WENT DOWN
* SEE IF UPTIME < DIT AVERAGE (BIT INTERVAL)
         LDA   CWUP       ;GET KEY UP TIME
         CMP   CWDIT      ;>BIT INTERVAL?
         BCC   CWIN2      ;NO,GET MORE BITS
* IF > 4*CWDIT THEN WORD BREAK
         LSR
         LSR
         CMP   CWDIT
         BCC   KEYUP3     ;LETTER
         INC   CWFLG      ;YES-SET WORD FLAG
KEYUP3   JMP   CWIN6      ;DECODE CHARACTER
*
* KEY DOWN TIMING ROUTINE
*
KEYDOWN  LDA   #0         ;INIT COUNTERS
         STA   CWDOWN
*
KEYDOWN1 INC   CWDOWN
         BNE   KEYDOWN2
         DEC   CWDOWN     ;KEEP AT $FF UNTILL KEY UP
*
KEYDOWN2 JSR   DELAY
         JSR   KEYTEST
         LDA   KEYLOC
         BEQ   KD2B
         JMP   CWIN1
KD2B     JSR   CBITIN     ;DOWN?
         BCC   KEYDOWN1   ;YES
*
* KEY WENT UP
*DIT OR DASH?
*
         LDA   CWDOWN     ;GET DOWN TIME
         CMP   #15        ;1DIT AT STD SPEED=20LOOPS
         BCS   DOT:DASH   ;NO
         JMP   CWIN2      ;YES-START OVER
DOT:DASH CMP   CWDIT      ;> AVERAGE DIT?
         BCS   CWDASH     ;YES-DASH
*
*RECEIVED DIT
*
         ASL   CWCHAR     ;ROTATE 0 INTO CHARACTER
         JMP   NEWAVG
*
*RECEIVED DASH
*
CWDASH   ROL   CWCHAR     ;PUT 1 INTO CHARACTER
         TAY
         JSR   $E301      ;FLOAT Y
         JSR   $EB63      ;MOVE FAC INTO ARG
         LDY   #3
         JSR   $E301
         JSR   $EA69      ;DIVIDE ARG BY FAC
         JSR   $E6FB      ;AND PUT IN X
         TXA
*
NEWAVG   ASL              ;DIT TIME *2
         BCC   NEWAVG3    ;NOT > 255
         LDX   #$AA       ;TOO MUCH
         JMP   CWIN8
*
NEWAVG3  STA   CWDOWN     ;STORE IT
         LDY   CWDIT      ;GET LAST AVG
         JSR   $E301
         JSR   $EB63      ;MOVE TO ARG
         LDY   #5
         JSR   $E301
         JSR   $E982      ;MULTIPLY
         JSR   $EB63      ;MOVE RESULT TO ARG
         LDY   CWDOWN
         JSR   $E301      ;FLOAT IT
         JSR   $E7C1      ;ADD TO MULT
         JSR   $EB63      ;MOVE TO FAC
         LDY   #6         ;DIVIDE BY 6
         JSR   $E301
         JSR   $EA69      ;DIVIDE
         JSR   $E6FB      ;RESULT IN X
         STX   CWDIT
NEWAVG2  JMP   CWIN2      ;CONTINUE
*
* DECODE CW CHARACTER
*
CWIN6    LDX   #$60       ;#ASCII CHARACTERS
         LDA   CWCHAR
CWIN7    CMP   CWTBL-$20,X ;MATCH?
         BEQ   CWIN8      ;YEP!
         DEX
         BNE   CWIN7      ;CONTINUE SEARCH
         LDX   #$AA       ;LOAD * FOR NO MATCH
CWIN8    TXA              ;PUT CHAR IN ACCUM
         JMP   BYT3       ;EXIT THROUGH BYTEIN
*
*
         AST   80

*/
