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
	m_dotDashThresholdFilter = NULL;
	m_decimate = NULL;
	m_mixer = NULL;
	m_sampleClock = NULL;
	m_goertzel = NULL;
	m_useGoertzel = true;
	m_sampleRate = 0;
	m_numSamples = 0;
	m_filterSamplesPerResult = 16;
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
	m_out = memalign(m_numSamples);

	if (m_workingBuf != NULL)
		delete m_workingBuf;
	m_workingBuf = memalign(m_numSamples);

	if (m_decimate != NULL)
		delete m_decimate;
	m_decimate = new Decimator(_sampleRate, _sampleCount);

	if (m_mixer != NULL)
		delete m_mixer;
	m_mixer = new Mixer(_sampleRate, _sampleCount);

	//demodMode gets used later, init early
	m_demodMode = DeviceInterface::dmCWL;

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

    //fldigi constructors
	m_squelchIncrement = 0.5;
	m_squelchThreshold = 0;
	m_squelchEnabled = false;

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
		m_usecPerResult = m_filterSamplesPerResult * (1e6/m_modemSampleRate);

	} else {
		if (m_goertzel != NULL)
			delete m_goertzel;
		m_goertzel = new GoertzelOOK(_sampleRate, _sampleCount);
		m_goertzel->setTargetSampleRate(m_modemSampleRate);
		updateGoertzel(c_defaultModemFrequency, c_goertzelDefaultSamplesPerResult);
	}


	if (m_dotDashThresholdFilter != NULL)
		delete m_dotDashThresholdFilter;
	m_dotDashThresholdFilter = new MovingAvgFilter(c_thresholdFilterSize);

	wpmBoxChanged(0); //Setup default wpm ranges, also calls init()
	m_wpmSpeedFilter = m_wpmSpeedCurrent;

	m_agc_peak = 1.0;

	m_outputOn = true;

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
	if (m_dotDashThresholdFilter != NULL) delete m_dotDashThresholdFilter;
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
		emit removeProfile(MorseModem);

		m_dataUi = NULL;
        return;
	} else if (m_dataUi == NULL) {
        //Create new one
		m_dataUi = new Ui::dataMorse();
		m_dataUi->setupUi(parent);

		//Add option to display data in testBench
		emit addProfile("Morse Modem", MorseModem);

		m_dataUi->dataBar->setValue(DB::minDb);
		m_dataUi->dataBar->setMin(-60); //1/2 normal range, see processBlock meterValue to stay in sync
		m_dataUi->dataBar->setMax(0);
		m_dataUi->dataBar->setNumTicks(10);
		m_dataUi->dataBar->start();

		m_squelchThreshold = 0;
		m_squelchEnabled = false;
		m_dataUi->squelchSlider->setMinimum(0);
		m_dataUi->squelchSlider->setMaximum(10 / m_squelchIncrement);
		m_dataUi->squelchSlider->setSingleStep(1);
		m_dataUi->squelchSlider->setValue(m_squelchThreshold);
		connect(m_dataUi->squelchSlider,SIGNAL(valueChanged(int)),this,SLOT(squelchChanged(int)));

		m_dataUi->dataEdit->setAutoFormatting(QTextEdit::AutoNone);
		m_dataUi->dataEdit->setAcceptRichText(true); //For international characters
		m_dataUi->dataEdit->setReadOnly(true);
		m_dataUi->dataEdit->setUndoRedoEnabled(false);
		m_dataUi->dataEdit->setWordWrapMode(QTextOption::WordWrap);
		m_dataUi->dataEdit->ensureCursorVisible(); //Auto scrolls so cursor is always visible
		m_dataUi->dataEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); //Ignored on mac, always transient

		connect(m_dataUi->resetButton,SIGNAL(clicked()),this,SLOT(resetOutput()));

		m_dataUi->outputOptionBox->addItem("Character",CHAR_ONLY);
		m_dataUi->outputOptionBox->addItem("DotDash",DOTDASH);
		m_dataUi->outputOptionBox->addItem("Both",CHAR_AND_DOTDASH);
		m_dataUi->outputOptionBox->setCurrentIndex(0);
		m_outputMode = CHAR_ONLY;
		connect(m_dataUi->outputOptionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(outputOptionChanged(int)));

		//data is index to m_minMaxTable.  Keep in sync
		m_dataUi->wpmBox->addItem("5-50 wpm");
		m_dataUi->wpmBox->addItem("50-100 wpm");
		m_dataUi->wpmBox->addItem("100-200 wpm");
		connect(m_dataUi->wpmBox,SIGNAL(currentIndexChanged(int)),this,SLOT(wpmBoxChanged(int)));

		//Threshold selection
		//Changes display and meaning of threshold slider
		m_dataUi->thresholdOptionBox->addItem("Auto", TH_AUTO); //All threshold are automatically set
		m_dataUi->thresholdOptionBox->addItem("Tone",TH_TONE); //Threshold between tone/no-tone
		m_dataUi->thresholdOptionBox->addItem("Dot/Dash",TH_DASH); //Sets dot/dash threshold
		m_dataUi->thresholdOptionBox->addItem("Char/Word",TH_WORD); //Sets char/word threshold
		m_dataUi->thresholdOptionBox->addItem("Squelch",TH_SQUELCH); //Sets noise level, no detection below this level
		connect(m_dataUi->thresholdOptionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(thresholdOptionChanged(int)));
		//Todo: Save/Restore threshold settings



		m_lockWPM = false;
		m_dataUi->freezeButton->setCheckable(true);
		m_dataUi->freezeButton->setChecked(false); //Off
		connect(m_dataUi->freezeButton,SIGNAL(clicked(bool)),this,SLOT(freezeButtonPressed(bool)));

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
	//If demodMode changes, change mixer or goertzel center frequency
	m_demodMode = _demodMode;
	updateGoertzel(c_defaultModemFrequency, m_goertzelSamplesPerResult);
}

//Called initially and whenever we need to change goretzel tone or 'N'
void Morse::updateGoertzel(int modemFreq, int samplesPerResult)
{
	m_goertzelSamplesPerResult = samplesPerResult;

	//We need to account for modemOffset in ReceiverWidget added so we hear tone but freq display is correct
	//Actual freq for CWU will be freq + modemFrequency for CWL will be freq -modemFrequency.
	//And we want actual freq to be at baseband
	if (m_demodMode == DeviceInterface::dmCWL || m_demodMode == DeviceInterface::dmLSB) {
		m_modemFrequency = -modemFreq;
	} else if (m_demodMode == DeviceInterface::dmCWU || m_demodMode == DeviceInterface::dmUSB) {
		m_modemFrequency = modemFreq;
	} else {
		m_modemFrequency = modemFreq;
	}

	// bit filter based on 10 msec rise time of CW waveform
	int jitterCount = (int)(m_modemSampleRate * m_secRiseFall / m_goertzelSamplesPerResult);
	if (m_jitterFilter != NULL)
		delete m_jitterFilter;
	m_jitterFilter = new MovingAvgFilter(jitterCount);

	if (m_useGoertzel)
		m_goertzel->setFreq(m_modemFrequency, m_goertzelSamplesPerResult, jitterCount);
	else
		m_mixer->setFrequency(m_modemFrequency);

	m_usecPerResult = m_goertzelSamplesPerResult * (1e6/m_modemSampleRate);

}

void Morse::setMinMaxMark(int wpmLow, int wpmHigh)
{
	//Shortest mark we can time.  Dot time at upper wpm limit
	m_usecShortestMark =  c_uSecDotMagic / (wpmHigh * 1.10); //Slightly faster than highest speed
	//Longest mark we can time. Dash time at slowest wpm limit
	m_usecLongestMark = 3 * (c_uSecDotMagic / (wpmLow * 0.90)); //SLightly longer than slowest speed
}

void Morse::outputOptionChanged(int s)
{
	m_outputMode = (OUTPUT_MODE)m_dataUi->outputOptionBox->itemData(s).toInt();
}

void Morse::freezeButtonPressed(bool b)
{
	Q_UNUSED(b);

	m_outputOn = !m_outputOn;
}

//Finds the optimum N that results in a bandwidth of 100-300 and N of 12 to 4
quint32 Morse::findBestGoertzelN(quint32 wpmLow, quint32 wpmHigh)
{
	//Optimize for middle and verify with low and high
	quint32 wpmMid = (wpmLow + wpmHigh) / 2;
	quint32 nMid, nLow, nHigh;
	quint32 bwMid, bwLow, bwHigh; //Bandwidth
	quint32 resPerTcw; //Results per Tcw
	quint32 resPerTcwLow, resPerTcwHigh;
	quint32 usecWpmLow, usecWpmMid,usecWpmHigh;
	usecWpmLow = MorseCode::wpmToTcwUsec(wpmLow);
	usecWpmMid = MorseCode::wpmToTcwUsec(wpmMid);
	usecWpmHigh = MorseCode::wpmToTcwUsec(wpmHigh);
	quint32 usecPerSample = 1.0e6 / m_modemSampleRate;
	//Find N with largest resPerTcw and smallest bandwidth
	for (resPerTcw = 4; resPerTcw<40; resPerTcw++) {
		nLow = (usecWpmLow/resPerTcw) / usecPerSample;
		nMid = (usecWpmMid/resPerTcw) / usecPerSample;
		nHigh = (usecWpmHigh/resPerTcw) / usecPerSample;
		//If we chose nMid, how many results at low and high
		resPerTcwLow = usecWpmLow / (nMid * usecPerSample);
		resPerTcwHigh = usecWpmHigh / (nMid * usecPerSample);
		bwLow = m_modemSampleRate / nMid;
		bwMid = m_modemSampleRate / nMid;
		bwHigh = m_modemSampleRate / nMid;
		if (bwLow >= 100 && bwLow <= 750 && bwMid >= 100 && bwMid <= 750 &&bwHigh >= 100 && bwHigh <= 750 &&
			//We can tolerate fewer results per Tcw at high end of range
			resPerTcwLow >= 4 && resPerTcwHigh >= 3) {

			break;
		}
		//Otherwise bandwidth is the best it will be at 4 results per Tcw
	}

	//qDebug()<<"WPM:"<<wpmLow<<"resPerTcw:"<<resPerTcw<<" samplesPerResult:"<<nLow<<" bandWidth: "<<bwLow;
	qDebug()<<"WPM:"<<wpmMid<<"resPerTcw:"<<resPerTcw<<" samplesPerResult:"<<nMid<<" bandWidth: "<<bwMid;
	//qDebug()<<"WPM:"<<wpmHigh<<"resPerTcw:"<<resPerTcw<<" samplesPerResult:"<<nHigh<<" bandWidth: "<<bwHigh;

	return nMid;
}

void Morse::wpmBoxChanged(int s)
{
	int index = s; //m_dataUi->wpmBox->currentData().toInt();
	m_lockWPM = false;
	m_aboveWpmRange = false;
	m_belowWpmRange = false;
	m_wpmLimitLow = m_minMaxTable[index].wpmLow;
	m_wpmLimitHigh = m_minMaxTable[index].wpmHigh;
	int wpm = (m_wpmLimitLow + m_wpmLimitHigh) / 2;
	m_wpmFixed = wpm;
	m_wpmSpeedCurrent = wpm;
	quint32 samplesPerResult = findBestGoertzelN(m_wpmLimitLow, m_wpmLimitHigh);
	updateGoertzel(c_defaultModemFrequency, samplesPerResult);
	setMinMaxMark(m_wpmLimitLow, m_wpmLimitHigh);
	init(wpm);
}

void Morse::thresholdOptionChanged(int s)
{
	Q_UNUSED(s);

	//Todo
	THRESHOLD_OPTIONS th = (THRESHOLD_OPTIONS)m_dataUi->thresholdOptionBox->currentData().toInt();
	switch (th) {
		case TH_AUTO:
			break;
		//Power threshold between tone/no-tone in Goertzel
		case TH_TONE:
			break;
		//Timing threshold between dot and dash
		//m_usecDotDashThreshold
		case TH_DASH:
			break;
		//Timing threshold between char and word
		//m_usecCharThreshold
		//m_usecWord Threshold
		case TH_WORD:
			break;
		//Power threshold for data squelch
		//m_squelchThreshold
		case TH_SQUELCH:
			break;
		//Timing threshold between for interval between dots and dashes
		//m_usecElementThreshold
	}
}

//Called from clear button
void Morse::resetOutput()
{
	m_dataUi->dataEdit->clear();
    refreshOutput();
}

//Handles newOuput signal
void Morse::refreshOutput()
{
	if (!m_outputOn)
        return;

	m_outputBufMutex.lock();
	if (m_lockWPM)
		m_dataUi->wpmBox->setCurrentText(QString().sprintf("%d wpm fixed", m_wpmFixed));
	else if (m_aboveWpmRange)
		m_dataUi->wpmBox->setCurrentText(QString().sprintf(">%d wpm",m_wpmLimitHigh-m_wpmVar));
	else if (m_belowWpmRange)
		m_dataUi->wpmBox->setCurrentText(QString().sprintf("<%d wpm",m_wpmLimitLow+m_wpmVar));
	else
		m_dataUi->wpmBox->setCurrentText(QString().sprintf("%d wpm", m_wpmSpeedCurrent));

	m_dataUi->dataEdit->insertPlainText(m_output); //At cursor
	m_dataUi->dataEdit->moveCursor(QTextCursor::End); //Scrolls window so new text is always visible
	m_output.clear();
	m_outputBufMutex.unlock();
}

void Morse::outputData(const char* d)
{
	if (m_dataUi == NULL)
        return;

	m_dataUi->dataEdit->insertPlainText(d); //At cursor
	//m_dataUi->dataEdit->moveCursor(QTextCursor::End);
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
		m_squelchThreshold = v * m_squelchIncrement; //Slider is in .5 increments so 2 inc = 1 squelch value
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

void Morse::init(quint32 wpm)
{
	m_output.clear(); //Clear any text that hasn't been output
	m_wpmSpeedCurrent = wpm;
	updateThresholds(c_uSecDotMagic / wpm, true); //Assume 1st mark is a dot

	m_useNormalizingThreshold = true; //Fldigi mode
	m_agc_peak = 0;
	m_outputMode = CHAR_ONLY;
	m_dotDashThresholdFilter->reset();
    resetModemClock();
    resetDotDashBuf(); //So reset can refresh immediately
	m_lastReceiveState = m_receiveState;
	m_receiveState = IDLE; //Will reset clock and dot dash
}


// Set up to track speed on dot-dash or dash-dot pairs for this test to work, we need a dot dash pair or a
// dash dot pair to validate timing from and force the speed tracking in the right direction. This method
// is fundamentally different than the method in the unix cw project. Great ideas come from staring at the
// screen long enough!. Its kind of simple really ... when you have no idea how fast or slow the cw is...
// the only way to get a threshold is by having both code elements and setting the threshold between them
// knowing that one is supposed to be 3 times longer than the other. with straight key code... this gets
// quite variable, but with most faster cw sent with electronic keyers, this is one relationship that is
// quite reliable. Lawrence Glaister (ve7it@shaw.ca)

//Called on every mark mark/elementSpace sequence to update thresholds
//This should be only place where thresholds are changed
//If forceUpdate is true, then we assume newMark is a dot and update everything.  Used for reset
void Morse::updateThresholds(quint32 usecNewMark, bool forceUpdate)
{
	quint32 usecDot=0, usecDash=0;

	if (forceUpdate) {
		usecDot = usecNewMark;
		usecDash = usecDot * 3;
		m_usecLastMark = usecDot;
	} else {
		//If we're locked, we don't tweak thresholds at all.
		//Typically for machine to machine morse
		if (m_lockWPM)
			return;


		if (m_usecLastMark == 0)
			return; //1st time called

	#if 0
		//Do we still need this since we're checking for wpm low/high range now?
		//Ignore if too short or too long
		if (usecNewMark < m_usecShortestMark ||
			usecNewMark > m_usecLongestMark)
			return;
	#endif

		double ratio = (float)usecNewMark / (float)m_usecLastMark;

		//Determine if new mark can be assumed to be dot, dash, or indeterminate (???)
		//There several options we have to check for
		//dot followed by dash: Update timing
		//dot followed by dot: Don't do anything
		//dot followed by something out of range: Could be speed change
		//dash followed by dot: Update timing
		//dash followed by dash: Don't do anything
		//dash followed by something out of range: Could be speed change

		//There has to be at least a 2x difference between lastMark and newMark for us to make any assumption about
		//which one is a dot and which one a dash
		//If difference is greater than 4x, then there's too much difference for us to make any assumptions
		//Otherwise could just be normal variations
		//      newMark < lastMark  |  newMark > lastMark
		//  ------------------------|-----------------------
		//       0.25    0.50       1.0        2.0    4.0      newMark / lastMark ratio
		//	  ---->|<---->|<---->lastMark<---->|<---->|<----
		//  |  ??? |  dot |  ???    |     ???  | dash | ???  |
		//Perfect morse will return ratios of .333 (dot/dash), 3.0 (dash/dot), or 1.0 (dot/dot or dash/dash)

		//Perfect ratio of 3 is in the middle
		if (ratio >=2 && ratio <= 4) {
			//newMark is greater than lastMark and long enough to be a dash
			usecDot = m_usecLastMark;
			usecDash = usecNewMark;
		//Perfect ration of .333 is in the middle
		} else if (ratio >= 0.25 && ratio <= 0.50) {
			//newMark is less than lastMark and short enough to be a dot
			usecDot = usecNewMark;
			usecDash = m_usecLastMark;
		} else if (ratio > 0.5 && ratio < 2.0) {
			//Accepted variation within current thresholds
			return;
		} else {
			//either newMark or lastMark is out of range, wait till in range
			return;
		}
	} //End if forceUpdate

    //Current speed estimate
	//Result is midway between last short and long mark, assumed to be dot and dash
	quint32 newDotDashThreshold = (quint32)m_dotDashThresholdFilter->newSample((usecDash + usecDot) / 2);
	usecDot = newDotDashThreshold / 2; //Filtered dot length
	quint32 newWpm = c_uSecDotMagic / usecDot;
	if (!forceUpdate && newWpm < m_wpmLimitLow) {
		m_belowWpmRange = true;
		m_aboveWpmRange = false;
	} else if (!forceUpdate && newWpm > m_wpmLimitHigh) {
		m_belowWpmRange = false;
		m_aboveWpmRange = true;
	} else {
		//forceUpdate is true or we're within range
		m_belowWpmRange = false;
		m_aboveWpmRange = false;
		//Keep wpm within range, taking into account variance
		if (newWpm > (m_wpmLimitHigh - m_wpmVar)) //In the grey zone between displayed range and var
			newWpm -= m_wpmVar;
		else if (newWpm < m_wpmLimitLow + m_wpmVar)
			newWpm += m_wpmVar;

		m_usecDotDashThreshold = newDotDashThreshold;
#if 0
		//dotDash threshold is roughly 2 TCW, so midway between dot and dash
		m_usecNoiseSpike = m_usecDotDashThreshold / 4;
		//Make this zero to accept any inter mark space as valid
		//Adaptive threshold is typically 2TCW so /8 = .25TCW
		m_usecElementThreshold = m_usecDotDashThreshold / 8;
#else
		//Based on usecDot
		//If we're timing space, ignore marks shorter than this which could be a noise spike
		m_usecSpikeThreshold = usecDot * 0.50; //
		//If we're timing mark, ignore space shorter than this which could be fadeing
		m_usecFadeThreshold = usecDot * 0.50; // Ignore space less than this as possible fading
		//I think elementThreshold should be midway between dotDashThreshold
		m_usecElementThreshold = usecDot * 0.25; // Greater than this is an element space
#endif
		//Perfect timing based on Tcw
		// Mark:  |-dot(1)-|-----dash(3)-----|
		// Space: |-el(1)--|--character(3)---|-----------------------------------|-- word space(7)
		// Tcw:   0 | |    1        2        3        4        5        6        7
		// Thresholds
		// Mark:  |                 |----dotDashThreshold
		// Mark:  | |fadeThreshold
		// Space: |   |elementThreshold
		// Space: |                 |----characterThreshold
		// Space: |                                   |----wordThreshold
		// Space: | |spikeThreshold

		m_wpmSpeedCurrent = newWpm;
		m_usecDotCurrent = usecDot;
		m_usecDashCurrent = usecDash;
		m_usecCharThreshold = usecDot * 2; //Greater than this is a character space
		//Between is a character space
		m_usecWordThreshold = usecDot * 4; //Greater than this is a word space
	}
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
	copyCPX(m_workingBuf,in,m_numSamples);

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

				if (!m_squelchEnabled || m_squelchMetric > m_squelchThreshold ) {
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
			//FP works
			//result = m_goertzel->processSample(nextBuf[i].re, tonePower, aboveThreshold);
			//CPX with non-integer k Goertzel works
			result = m_goertzel->processSample(nextBuf[i], tonePower, aboveThreshold);
			if (result) {
				//Goertzel handles debounce and threshold
				meterValue = DB::powerTodB(tonePower);
				meterValue = DB::clip(meterValue); //limit -120 to 0
				//Set bargraph range to match
				meterValue /= 2; //Range is now -60 to 0 so we get more variance at higher end

				if (aboveThreshold) {
					stateMachine (TONE_EVENT);
				} else {
					stateMachine(NO_TONE_EVENT);
				}
				//Only update testbench result when we have a goertzel result
				//m_testBenchValue.real(tonePower);
				//Convert from -120 to 0, to 0 to 120, and scale to 0..1 for testbench
				m_testBenchValue.real((meterValue+120) / 1200);
				if (aboveThreshold)
					m_testBenchValue.imag(.05);
				else
					m_testBenchValue.imag(0);

			}
		}
		//To view goertzel power and on/off train, set testBench to display time data and select MorseModem as input
		//Adj vert range as needed and use trigger and single shot to capture and freeze
		//Update on every sample, but testBenchValue will only change every result
		emit testbench(1, &m_testBenchValue, m_modemSampleRate, MorseModem);
		//Raw incoming data
		//emit testbench(1, &nextBuf[i], m_modemSampleRate, MorseModem);

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
					resetModemClock();
					//The time for this 1st tone will be included when we detect the end of the tone
					// Res Res Res Res Res Res Res Res Res Res
					//     ___ ___ ___           ___ ___ ___
					// ___             ___ ___ ___
					//        | Tone detected after 1 mark result
					//                    | End of tone detected after 1 space result
					//        | usecMark  | usecMark correctly returns timing
					m_lastReceiveState = IDLE;
					m_receiveState = MARK_TIMING;
                    break;

                //Tone filter output is trending down
                case NO_TONE_EVENT:
                    //Don't care about silence in idle state
					m_lastReceiveState = IDLE;
					m_receiveState = IDLE;
                    break;
            }
            break;
        case MARK_TIMING:
            switch (event) {
                case TONE_EVENT:
                    //Keep timing
                    //Check for long tuning tone and ignore ??
					m_lastReceiveState = MARK_TIMING;
					m_receiveState = MARK_TIMING;
                    break;

                case NO_TONE_EVENT:
                    //Transition from mark to space
                    // Save the current timestamp so we can see how long tone was
					m_toneEnd = m_sampleClock->clock();
					m_usecMark = m_sampleClock->uSecDelta(m_toneStart, m_toneEnd);
					//qDebug()<<"Mark:"<<m_usecMark;

					// make sure our timing values are up to date after every tone
					//This allows us to adapt to changing wpm speeds that existing thresholds could filter
					//at the risk of occasionally picking up a false mark.  But dotDashThreshold filter should
					//deal with this.
					updateThresholds(m_usecMark, false);
					m_usecLastMark = m_usecMark; //Save for next check

					// If the tone length is shorter than any noiseThreshold, no_tone a noise dropout
					if (m_usecSpikeThreshold > 0
						&& m_usecMark < m_usecSpikeThreshold) {
						//Noise spike, go back to idle, element or word timing mode
						//so don't reset modemClock so we keep timing
						m_receiveState = m_lastReceiveState;
						//This doesn't help with general noise,just spikes
						break;
                    }

                    //dumpStateMachine("KEYUP_EVENT enter");
					m_usecSpace = 0;
                    //Could be valid tone or could just be a short signal drop
					m_markHandled = false; //Defer to space timing
					m_lastReceiveState = MARK_TIMING;
					m_receiveState = INTER_ELEMENT_TIMING;
                    break;
            }
            break;

        case INTER_ELEMENT_TIMING:
            switch (event) {
                case TONE_EVENT:
					//If usecSpace > elementThreshold, then we've processed the element, char, or word space
					//and can start mark timing
					if (!m_markHandled) {
#if 0
						//Checking for noise pulses doesn't work or doesn't improve anything, revisit logic
						//usecSpace was less than elementThreshold
						if (m_usecSpace < m_usecFadeThreshold) {
							//Just continue as if tone never happened
                            //Don't reset clock, we're still timing mark
							m_lastReceiveState = INTER_ELEMENT_TIMING;
							m_receiveState = INTER_ELEMENT_TIMING;
                        } else {
							//Throw it away and start over
							m_lastReceiveState = INTER_ELEMENT_TIMING;
							m_receiveState = IDLE;
                        }
#endif
                    } else {
						//usecSpace was > elementThreshold and we are starting a new mark
                        resetModemClock();
						m_lastReceiveState = INTER_ELEMENT_TIMING;
						m_receiveState = MARK_TIMING;
                    }
                    break;

                case NO_TONE_EVENT:
					//Timing inter-element or character space
					m_usecSpace = m_sampleClock->uSecToCurrent(m_toneEnd); //Time from tone end to now

					if (!m_markHandled && m_usecSpace > m_usecElementThreshold) {
						//inter-element space (1 TCW), process mark
						//If we already have max bits in dotDash buf, then trigger error
						if (m_dotDashBufIndex >= MorseCode::c_maxMorseLen) {
							m_lastReceiveState = m_receiveState;
							m_receiveState = IDLE;
							return true;
						}
						//Process last mark element
						if (m_usecMark <= m_usecDotDashThreshold) {
							m_dotDashBuf[m_dotDashBufIndex++] = MorseCode::c_dotChar;
						} else {
							m_dotDashBuf[m_dotDashBufIndex++] = MorseCode::c_dashChar;
						}

						// zero terminate representation so we can handle as char* string
						m_dotDashBuf[m_dotDashBufIndex] = 0;
						m_markHandled = true;
                    }
					if (m_usecSpace < m_usecCharThreshold) {
						// SHORT time since keyup... nothing to do yet
						//Keep timing
						m_lastReceiveState = INTER_ELEMENT_TIMING;
						m_receiveState = INTER_ELEMENT_TIMING;
					} else if (m_usecSpace >= m_usecCharThreshold &&
						m_usecSpace <= m_usecWordThreshold) {
						// MEDIUM time since keyup... check for character space
						// one shot through this code via receive state logic
						// FARNSWOTH MOD HERE -->

						// Look up the representation if there is a character waiting in dotDashBuf
						if (*m_dotDashBuf != 0x00 && m_dotDashBufIndex > 0) {
							MorseSymbol *cw = m_morseCode.tokenLookup(m_dotDashBuf);
							if (cw != NULL) {
								if (m_outputMode == CHAR_ONLY)
									outStr = cw->display;
								else if (m_outputMode == CHAR_AND_DOTDASH)
									outStr = QString("%1 [ %2 ] ").arg(cw->display).arg(cw->dotDash);
								else if (m_outputMode == DOTDASH)
									outStr = QString(" [ %1 ] ").arg(cw->dotDash);
							} else {
								// invalid decode... let user see error
								outStr = "*";
							}
							//Display char
							outputString(outStr);
							m_usecLastSpace = m_usecSpace; //Almost always fixed because we start looking for char at 2x usecDot
							resetDotDashBuf(); //Ready for new char See if we should inline this
							m_lastReceiveState = INTER_ELEMENT_TIMING;
							m_receiveState = WORD_SPACE_TIMING;
						} else {
							//Nothing to output
							m_lastReceiveState = INTER_ELEMENT_TIMING;
							m_receiveState = IDLE;
						}

					} else {
						//usecSpace is > wordThreshold
						//Restart state machine
						m_lastReceiveState = INTER_ELEMENT_TIMING;
						m_receiveState = IDLE;
					}

                    break;
			} //End switch(event)
            break;
        case WORD_SPACE_TIMING:
            switch (event) {
                case TONE_EVENT:
					//Any char has already been output, just start timing tone
					m_usecLastSpace = m_usecSpace;
                    resetDotDashBuf(); //Ready for new char
                    resetModemClock();
					m_lastReceiveState = WORD_SPACE_TIMING;
					m_receiveState = MARK_TIMING;
                    break;

                case NO_TONE_EVENT:
					m_usecSpace = m_sampleClock->uSecToCurrent(m_toneEnd); //Time from tone end to now
					if (m_usecSpace < m_usecWordThreshold) {
						//Not long enough for word space, keep timing
						m_lastReceiveState = WORD_SPACE_TIMING;
						m_receiveState = WORD_SPACE_TIMING;
					} else {
						//We only get here if we've already output a char
						// LONG time since keyup... check for a word space
						// FARNSWOTH MOD HERE -->

						outStr = " "; //End of word
						//dumpStateMachine("Word Space");
						outputString(outStr);
						m_usecLastSpace = m_usecSpace;

						//dumpStateMachine(*outStr);
						m_lastReceiveState = WORD_SPACE_TIMING;
						m_receiveState = IDLE; //Takes care of resets
					}

                    break;
            }
            break;
    }
	//
    return false;
}

void Morse::outputString(QString outStr) {
	if (!outStr.isEmpty()) {

        //Display can be accessing at same time, so we need to lock
		m_outputBufMutex.lock();
		//Append to m_output in case display is frozen or behind
		//refreshOutput() will clear it
		if (c_useLowercase) {
			m_output.append(outStr.toLower());
		} else {
			m_output.append(outStr);
		}
		m_outputBufMutex.unlock();
        emit newOutput();

    }
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

