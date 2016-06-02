//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "morse.h"
#include "QPainter"
#include "QDebug"
#include "QEvent"
#include "db.h"

/*
	CW Notes for reference
	Tcw is basic unit of measurement in ms
	Dash = 3 Tcw, Dot = 1Tcw, Element Space = 1 Tcw, Character Space = 3 Tcw, Word Space = 7 Tcw (min)
	Standard for measuring WPM is to use the word "PARIS", which is exactly 50Tcw
	So Tcw (ms) = 60 (sec) / (WPM * 50)
	and WPM = c_mSecDotMagic/Tcw(ms)

	Use c_uSecDotMagic to convert WPM to/from usec is 1,200,000
	c_uSecDotMagic / WPM = usec per TCW

	Tcw for 12wpm = 60 / (12 * 50) = 60 / 600 = .100 sec  100ms
	Tcw for 30wpm = 60 / 1500 = .040 40ms
	Tcw for 120wpm = 60 / (120 * 50) = 60 / 6000 = .01 sec  10ms

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

	From MorseWpmTcw.xlsx spreadsheet in source directory
	The 'binWidth' in the Goertzel algorithm determines how many samples should be processed before returning a result.
	The more samples we process, the 'sharper' the filter but the more time per result
	Ideal tone freq is one that is centered in the bin, ie an even multiple of binWidth/2
	Tcw = 60/(wpm*50) * 1000
	Goertzel N = BW/SR
	msTcw = how many ms per Tcw at this WPM
	SR = Sample Rate. Must be at least 2x tone freq for Nyquist
	msPerSmp = how many ms each sample respresents
	msN = how many ms for N samples
	NforMinBW = N calculated for whatever is in MinBW cell
	MaxNperResult = If N is larger than this, we are too slow and miss Tcw's
	N = NMinBW or MaxN, whichever is smaller
	ActBW = If N is set for timing, what is the actual bandwidth
	As SR increases, minBW gets wider
	As SR increases, ResultsPerTcw gets smaller to maintain equivalent bandwidth
	Num Results = How many results from Goertzel do we want for each Tcw.
		Chosen to optimize MinBW and msPerResult

	Var for table					Reference
	100		Minimum BW				N	BW		ms
	8000	SampleRate				8	1000	1
	0.125	ms Per Sample			16	500		2
	80		N for min BW			24	333		3
	4		Min results per Tcw		32	250		4
									40	200		5
									48	166		6
									64	125		8
									72	111		9
									80	100		10

		ms	ms		Est		ms	BW 	Res
	Wpm	Tcw	Res		N	N	N	N	Tcw
	---	---	---		---	---	---	--- ---
	5	240	60		480	20	2.5	400	96
	10	120	30		240	20	2.5	400	48
	20	60	15		120	20	2.5	400	24
	30	40	10		80	20	2.5	400	16
	40	30	7.5		60	20	2.5	400	12
	50	24	6		48	20	2.5	400	9.6
	60	20	5		40	20	2.5	400	8
	70	17	4.25	34	20	2.5	400	6.8
	80	15	3.75	30	20	2.5	400	6
	90	13	3.25	26	20	2.5	400	5.2
	100	12	3		24	20	2.5	400	4.8
	120	10	2.5		20	20	2.5	400	4	Max speed with 4 results per Tcw

	Waveforms: see http://www.w8ji.com/cw_bandwidth_described.htm for detailed examples
				___      ---100% (full power)
			  /  .  \
			 /   .   \
			/    .    \
	-------/-----.-----\------- 50% threshold
		  /.     .     .\
		 / .     .     . \
		/  .     .     .  \
	 __    .     .     .    __ 0% (zero power)
		   |--on time--|

	 Tcw for different rise/fall times over a 40ms interval
	 60wpm = 20ms Tcw
	 Tcw starts at 50% threshold during rise and ends 50% threshold during fall
	 Rise/Fall times: 20ms, 10ms, 5ms (ARRL recommended)

	   20ms rise/fall. 10ms resolution needed to catch 50% threshold. Smoothest gausian curve
	   |------20ms-------|------20ms-------|
				|----20ms Tcw-----|

	   10ms rise/fall. 5ms resolution needed to catch 50% threshold
	   |--10ms--|--10ms--|--10ms--|--10ms--|
		   |----20ms Tcw----|

	   5ms rise/fall. 2.5ms resolution needed to catch 50% threshold
	   |-5ms|--10ms--|-5ms|------20ms------|
		 |---20ms Tcw---|

*/

//Constructor is only called once with Pebble is started and plugins are loaded, not on powerOn
Morse::Morse()
{
	m_dataUi = NULL;
	m_workingBuf = NULL;
	m_toneFilter = NULL;
	m_jitterFilter = NULL;
	m_thresholdFilter = NULL;
	m_decimate = NULL;
	m_mixer = NULL;
	m_sampleClock = NULL;
	m_goertzel = NULL;
	m_useGoertzel = true;
	m_sampleRate = 0;
	m_numSamples = 0;
	m_filterSamplesPerResult = 16;
	m_goertzelSamplesPerResult = 20;
	m_out = NULL;

	//Used to update display from main thread.
	connect(this, SIGNAL(newOutput()), this, SLOT(refreshOutput()));
}

//setSampleRate is called everytime powerOn(), so make sure we clean up from previous run if power is toggled
void Morse::setSampleRate(int _sampleRate, int _sampleCount)
{
	m_sampleRate = _sampleRate;
	m_numSamples = _sampleCount;
	if (m_out != NULL)
		delete m_out;
	m_out = CPX::memalign(m_numSamples);

	if (m_workingBuf != NULL)
		delete m_workingBuf;
	m_workingBuf = CPX::memalign(m_numSamples);

	if (m_decimate != NULL)
		delete m_decimate;
	m_decimate = new Decimator(_sampleRate, _sampleCount);

	if (m_mixer != NULL)
		delete m_mixer;
	m_mixer = new Mixer(_sampleRate, _sampleCount);

	//ms rise/fall Adjust for WPM?
	m_secRiseFall = .005;

    resetDotDashBuf();

    //Modem sample rate, device sample rate decimated to this before decoding
	m_modemBandwidth = 1000; //8000; //Desired bandwidth, not sample rate
    //See if we can decimate to this rate
	int actualModemRate = m_decimate->buildDecimationChain(m_sampleRate, m_modemBandwidth, 8000);
	m_modemSampleRate = actualModemRate;
	m_decimationRate = m_sampleRate / m_modemSampleRate;

	//A tcw bit requires N samples to be processed by Goertzel or m_toneFilter
	//The larger N is, the narrower the filter bandwidth
	//So there's a trade off here
	//If we get as close to a 8ksps rate as possible
	//At 8ksps, each sample is .125ms, so a TCW at 30wpm or 40ms would require 320 samples
	//If N = 32, we get 10 bits (results) for each TCW
	//If WPM = 120wpm, then TCW is 10ms and requires 80 samples
	//To get 10 bits, N would have to be 8! which is going to be a very wide bandwidth

	if (m_sampleClock != NULL)
		delete m_sampleClock;
	m_sampleClock = new SampleClock(m_modemSampleRate);
	m_sampleClock->reset(); //Start timer over

    //Determine shortest and longest mark we can time at this sample rate
	m_usecShortestMark =  c_uSecDotMagic / (c_upperWPMLimit*1.10); //Slightly faster than highest speed we support
	m_usecLongestMark = c_uSecDotMagic / (c_lowerWPMLimit*0.90); //SLightly longer than slowest speed we support 5wpm

	m_modemFrequency = 1000; //global->settings->modeOffset;


    //fldigi constructors
	m_squelchIncrement = 0.5;
	m_squelchValue = 0;
	m_squelchEnabled = false;

	m_wpmSpeedCurrent = m_wpmSpeedFilter = c_wpmSpeedInit;
	calcDotDashLength(c_wpmSpeedInit, m_usecDotInit, m_usecDashInit);

    //2 x usecDot
	m_usecDotDashThreshold = 2 * c_uSecDotMagic / m_wpmSpeedCurrent;
	m_usecNoiseSpike = m_usecDotDashThreshold / 4;

	m_agc_peak = 1.0;


	if (!m_useGoertzel) {
		if (m_toneFilter != NULL)
			delete m_toneFilter;
		m_toneFilter = new C_FIR_filter();

		//Filter width changes with wpm, higher wpm = narrower filter
		//20wpm @8ksps = 480hz normalized
		//40wpm #8ksps = 240hz normalized
		double f = c_wpmSpeedInit / (c_secDotMagic * m_modemSampleRate);
		m_toneFilter->init_lowpass (c_lpFilterLen, m_filterSamplesPerResult, f);
		// bit filter based on 10 msec rise time of CW waveform
		//Effective sample rate post toneFilter is modemSampleRate / filterSamplesPerResult (decimation factor)
		int jitterCount = (int)(m_modemSampleRate * m_secRiseFall / m_filterSamplesPerResult); //8k*.01/16 = 5
		if (m_jitterFilter != NULL)
			delete m_jitterFilter;
		m_jitterFilter = new MovingAvgFilter(jitterCount);

	} else {
		if (m_goertzel != NULL)
			delete m_goertzel;
		m_goertzel = new Goertzel(_sampleRate, _sampleCount);
		m_goertzel->setTargetSampleRate(m_modemSampleRate);
		// bit filter based on 10 msec rise time of CW waveform
		int jitterCount = (int)(m_modemSampleRate * m_secRiseFall / m_goertzelSamplesPerResult);
		if (m_jitterFilter != NULL)
			delete m_jitterFilter;
		m_jitterFilter = new MovingAvgFilter(jitterCount);

		m_goertzel->setFreq(m_modemFrequency, m_goertzelSamplesPerResult, jitterCount);
	}


	if (m_thresholdFilter != NULL)
		delete m_thresholdFilter;
	m_thresholdFilter = new MovingAvgFilter(c_thresholdFilterSize);

    syncTiming();
	m_demodMode = DeviceInterface::dmCWL;
    init();

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
	if (m_jitterFilter != NULL) delete m_jitterFilter;
	if (m_thresholdFilter != NULL) delete m_thresholdFilter;
	if (m_decimate != NULL) delete m_decimate;
	if (m_mixer != NULL) delete m_mixer;
	if (m_sampleClock != NULL) delete m_sampleClock;
	if (m_goertzel != NULL) delete m_goertzel;
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

		m_dataUi->dataBar->setValue(DB::minDb);
		m_dataUi->dataBar->setMin(DB::minDb);
		m_dataUi->dataBar->setMax(DB::maxDb);
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
		m_dataUi->dataEdit->setAcceptRichText(true); //For international characters
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

void Morse::setDemodMode(DeviceInterface::DemodMode _demodMode)
{
	//If demodMode changes, mixer freq will also change in processBlock()
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

	m_dataUi->dataEdit->insertPlainText(m_output); //At cursor
	m_dataUi->dataEdit->moveCursor(QTextCursor::End); //Scrolls window so new text is always visible

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
	if(	m_wpmSpeedCurrent != m_wpmSpeedFilter ) {

		m_wpmSpeedFilter = m_wpmSpeedCurrent;
		if (!m_useGoertzel) {
			//Filter width changes with wpm, higher wpm = narrower filter
			//20wpm @8ksps = 480hz normalized
			//40wpm #8ksps = 240hz normalized
			double f = m_wpmSpeedCurrent / (c_secDotMagic * m_modemSampleRate);
			m_toneFilter->init_lowpass (c_lpFilterLen, m_filterSamplesPerResult, f);
		}
		m_agc_peak = 0;
    }
}

void Morse::init()
{
	m_wpmSpeedCurrent = c_wpmSpeedInit;
	m_usecDotDashThreshold = 2 * c_uSecDotMagic / m_wpmSpeedCurrent;
    syncTiming(); //Based on wpmSpeedCurrent & adaptive threshold

	m_useNormalizingThreshold = true; //Fldigi mode
	m_agc_peak = 0;
	m_outputMode = CHAR_ONLY;
	m_thresholdFilter->reset();
    resetModemClock();
    resetDotDashBuf(); //So reset can refresh immediately
	m_lastReceiveState = m_receiveState;
	m_receiveState = IDLE; //Will reset clock and dot dash
}

//Recalcs all values that are speed related
//Called in constructor, reset_rx_filter, synctiming
//Init values should never change, so this should only be called once with wpmSpeedInit as arg
void Morse::calcDotDashLength(int _speed, quint32 & _usecDot, quint32 & _usecDash)
{
    //Special case where _speed is zero
    if (_speed > 0)
		_usecDot = c_uSecDotMagic / _speed;
    else
		_usecDot = c_uSecDotMagic / 5;

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
void Morse::updateDotDashThreshold(quint32 idotUsec, quint32 idashUsec)
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

    //Current speed estimate
	if (!m_lockWPM) {
		//Only update if delta greater then 5wpm to avoid 'adaptive jitter'
		//Result is midway between last short and long mark, assumed to be dot and dash
		int newAdaptiveThreshold = (quint32)m_thresholdFilter->newSample((dashUsec + dotUsec) / 2);
		int newWpm = c_uSecDotMagic / (newAdaptiveThreshold / 2);
#if 0
		//Round up or down
		newWpm = ((newWpm+2) / 5) * 5;  //Increment in units of 5wpm
		if (abs(m_wpmSpeedCurrent - newWpm) >= 5) {
			m_usecAdaptiveThreshold = newAdaptiveThreshold;
			m_wpmSpeedCurrent = newWpm;
		}
#else
		m_usecDotDashThreshold = newAdaptiveThreshold;
		m_wpmSpeedCurrent = newWpm;
#endif

	}

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
	m_usecNoiseSpike = m_usecDotDashThreshold / 4;
    //Make this zero to accept any inter mark space as valid
    //Adaptive threshold is typically 2TCW so /8 = .25TCW
	m_usecElementThreshold = m_usecDotDashThreshold / 8;

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
	m_sampleClock->reset();
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
	CPX cpxFilter;
	double tonePower;
	double meterValue = -120;

    //If WPM changed in last block, dynamicall or by user, update filter with new speed
    syncFilterWithWpm();

    //Downconverter first mixes in place, ie changes in!  So we have to work with a copy
	CPX::copyCPX(m_workingBuf,in,m_numSamples);

    //We need to account for modemOffset in ReceiverWidget added so we hear tone but freq display is correct
    //Actual freq for CWU will be freq + modemFrequency for CWL will be freq -modemFrequency.
    //And we want actual freq to be at baseband
	if (m_demodMode == DeviceInterface::dmCWL || m_demodMode == DeviceInterface::dmLSB)
		m_mixer->setFrequency(-m_modemFrequency);
	else if (m_demodMode == DeviceInterface::dmCWU || m_demodMode == DeviceInterface::dmUSB)
		m_mixer->setFrequency(m_modemFrequency);
    else
        //Other modes, like DIGU and DIGL will still work with cursor on signal, but we won't hear tones.  Feature?
		m_mixer->setFrequency(0);

	int numModemSamples = m_numSamples;
	CPX *nextBuf;
	if (!m_useGoertzel) {
		nextBuf = m_mixer->processBlock(m_workingBuf); //In place
		numModemSamples = m_decimate->process(nextBuf,m_out,m_numSamples);
		nextBuf = m_out;
	} else {
		//Mixer not used for Goertzel
		numModemSamples = m_decimate->process(m_workingBuf,m_out,m_numSamples);
		nextBuf = m_out;
	}

    //Now at lower modem rate with bandwidth set by modemDownConvert in constructor
    //Verify that testbench post banpass signal looks the same, just at modemSampleRate
    //global->testBench->DisplayData(numModemSamples,this->out, modemSampleRate, PROFILE_5);

    for (int i = 0; i<numModemSamples; i++) {
		m_sampleClock->tick(); //1 tick per sample

		//m_bitSamples is the equivalent of N in Goertzel
		//cw_FIR_filter does MAC and returns 1 result every m_bitSamples samples
        //LowPass filter establishes the detection bandwidth (freq)
		//and Detection time (m_bitSamples)
        //If the LP filter passes the tone, we have detection
        //We need some sort of AGC here to get consistent tone thresholds
		bool result;
		bool aboveThreshold;
		if (!m_useGoertzel) {
			//m_toneFilter runs at modem sample rate and further decimates internally by only returning
			//results for every N samples
			result = m_toneFilter->run (nextBuf[i], cpxFilter );
			//toneFilter returns cpx, gertzel returns power
			tonePower = DB::power(cpxFilter);
			if (result) {

				//Moving average to handle jitter during CW rise and fall periods
				//Broken if jitterFilter is used, not sure why since works in Goertzel
				//Remove for now
				//tonePower = m_jitterFilter->newSample(tonePower);

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
						meterValue = DB::powerTodB(tonePower);
						stateMachine(TONE_EVENT);
					} else {
						meterValue = -120;
						stateMachine(NO_TONE_EVENT);
					}

					//Don't do anything if in the middle?  Prevents jitter that would occur if just compare with single value

					// downward trend means tone stopping


				} else {
					//Light in-squelch indicator (TBD)
					//Keep timing
					qDebug()<<"In squelch";
				}

			} //End if (result)
		} else {
			//Using goertzel
			result = m_goertzel->processSample(nextBuf[i].re, tonePower, aboveThreshold);
			if (result) {
				//Goertzel handles debounce and threshold
				meterValue = DB::powerTodB(tonePower);
				if (aboveThreshold) {
					stateMachine (TONE_EVENT);
				} else {
					stateMachine(NO_TONE_EVENT);
				}
			}
		}


        //Its possible we get called right after dataUi has been been instantiated
        //since receive loop is running when CW window is opened
		if (m_dataUi != NULL && m_dataUi->dataBar!=NULL) {
			m_dataUi->dataBar->setValue(meterValue); //Tuning bar
		}
	} //End for(...numSamples...)

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
	QString outStr;
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
					m_toneEnd = m_sampleClock->clock();
					m_usecMark = m_sampleClock->uSecDelta(m_toneStart, m_toneEnd);
					//qDebug()<<"Mark:"<<m_usecMark;
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
					m_usecSpace = m_sampleClock->uSecToCurrent(m_toneEnd); //Time from tone end to now
					if (!m_markHandled && m_usecSpace > m_usecElementThreshold) {
                        addMarkToDotDash();
						m_markHandled = true;
                    }
                    //Anything waiting for output?
					outStr = spaceTiming(true); //Looking for char
					if (!outStr.isEmpty()) {
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
					if (!outStr.isEmpty()) {
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
					m_usecSpace = m_sampleClock->uSecToCurrent(m_toneEnd); //Time from tone end to now
                    //Anything waiting for output?
					outStr = spaceTiming(false);
					if (!outStr.isEmpty()) {
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
	//If we already have max bits in dotDash buf, then trigger error
	if (m_dotDashBufIndex >= MorseCode::c_maxMorseLen) {
		m_lastReceiveState = m_receiveState;
		m_receiveState = IDLE;
		return;
	}
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
		//If we have two marks in a row, there are four options
		//dot - dash Update timing
		//dot - dot Don't do anything
		//dash - dot Update timing
		//dash - dash Don't do anything

        // check for dot dash sequence (current should be 3 x last)
		if ((m_usecMark > 2 * m_usecLastMark) &&
			(m_usecMark < 4 * m_usecLastMark)) {
			//If currentMark is between 2x and 4x lastMark, then its average is 3*lastMark and its a dash
			//usecMark is a dash and usecLastMark is a dot
			updateDotDashThreshold(m_usecLastMark, m_usecMark);
		} else if ((m_usecLastMark > 2 * m_usecMark) &&
			(m_usecLastMark < 4 * m_usecMark)) {
			//If lastMark is between 2x and 4x currentMark, then lastMark was a dash,a and currentMark is a dot
			// check for dash dot sequence (last should be 3 x current)
			//usecMark is dot and useLastMark is dash
			updateDotDashThreshold(m_usecMark, m_usecLastMark);
        }

    }
	m_usecLastMark = m_usecMark;
    //qDebug()<<"Mark: "<<usecMark;

    //RL We don't care about checking inter dot/dash spacing?  Should be 1 TCW
    // ok... do we have a dit or a dah?
    // a dot is anything shorter than 2 dot times
	if (m_usecMark <= m_usecDotDashThreshold) {
		m_dotDashBuf[m_dotDashBufIndex++] = MorseCode::c_dotChar;
    } else {
        // a dash is anything longer than 2 dot times
		m_dotDashBuf[m_dotDashBufIndex++] = MorseCode::c_dashChar;
    }

	// zero terminate representation so we can handle as char* string
	m_dotDashBuf[m_dotDashBufIndex] = 0;
}

void Morse::outputString(QString outStr) {
	if (!m_outputOn)
        return;

	if (!outStr.isEmpty()) {

        //Display can be accessing at same time, so we need to lock
		m_outputBufMutex.lock();
		if (c_useLowercase) {
			m_output = outStr.toLower();
		} else {
			m_output = outStr;
		}
		m_outputBufMutex.unlock();
        emit newOutput();

    }
}

//Processes post tone space timing
//Returns non-empty QString if space was long enough to output something
//Uses dotDashBuf, could use usec silence
QString Morse::spaceTiming(bool lookingForChar)
{
	MorseSymbol *cw;
	QString outStr;

    if (lookingForChar) {
        // SHORT time since keyup... nothing to do yet
        //Could be inter-element space (1 TCW)
		if (m_usecSpace < (2 * m_usecDotCurrent)) {
			return outStr; //Keep timing silence

		} else if (m_usecSpace >= (2 * m_usecDotCurrent) &&
			m_usecSpace <= (4 * m_usecDotCurrent)) {
            // MEDIUM time since keyup... check for character space
            // one shot through this code via receive state logic
            // FARNSWOTH MOD HERE -->

            //Char space is 3 TCW per spec, but accept 2 to 4

            // Look up the representation
			if (*m_dotDashBuf != 0x00) {
				cw = m_morseCode.rxLookup(m_dotDashBuf);
                if (cw != NULL) {
					if (m_outputMode == CHAR_ONLY)
                        outStr = cw->display;
					else if (m_outputMode == CHAR_AND_DOTDASH)
						outStr = QString("%1 [ %2 ] ").arg(cw->display).arg(cw->dotDash);
					else if (m_outputMode == DOTDASH)
						outStr = QString(" [ %s ] ").arg(cw->dotDash);
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
			return outStr;
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

	qDebug()<<"Clock: "<<m_sampleClock->clock()<<" ToneStart: "<<m_toneStart<<" ToneEnd: "<<m_toneEnd;
	qDebug()<<"Timing: Dot: "<<m_usecDotCurrent<<" Dash: "<<m_usecDashCurrent<<" Threshold: "<<m_usecDotDashThreshold<<" Element: "<<m_usecMark<<" Silence: "<<m_usecSpace;
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

