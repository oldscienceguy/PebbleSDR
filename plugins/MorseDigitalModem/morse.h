#ifndef MORSE_H
#define MORSE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "qframe"
#include "QTextEdit"
#include "ui_data-morse.h"
//Todo: replace with Fir
#include "../../pebblelib/fldigifilters.h"
#include "movingavgfilter.h"
#include "morsecode.h"
//#include "../../pebblelib/downconvert.h"
#include "decimator.h"
#include "mixer.h"
#include "goertzel.h"
//#include "demod.h"
#include <QMutex>
#include "../../pebblelib/digital_modem_interfaces.h"
#include "sampleclock.h"

class Receiver;

//We need to inherit from QObject so we can filter events.
//Remove if we eventually create a separate 'morse widget' object for UI
class Morse : public QObject, public DigitalModemInterface
{
    Q_OBJECT
    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DigitalModemInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DigitalModemInterface)


public:
    Morse();
    ~Morse();
    //DigitalModemInterface
	void setSampleRate(int _sampleRate, int _sampleCount);
	void setDemodMode(DeviceInterface::DemodMode _demodMode);
	CPX * processBlock(CPX * in);
	void setupDataUi(QWidget *parent);
	QString getPluginName();
	QString getPluginDescription();
    QObject *asQObject() {return (QObject *)this;}

	void outputData(const char *d);
	void outputData(const char c);

public slots:
    void squelchChanged(int v);
    void refreshOutput();
    void resetOutput();
    void outputOptionChanged(int s);
	void freezeButtonPressed(bool b);
	void wpmBoxChanged(int s);
	void thresholdOptionChanged(int s);

signals:
    void newOutput();
	void testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
	void testbench(int _length, double* _buf, double _sampleRate, int _profile);
	bool addProfile(QString profileName, int profileNumber); //false if profilenumber already exists
	void removeProfile(quint16 profileNumber);


private:
	//Use DOT_MAGIC to convert WPM to/from usec is 1,200,000
	//c_uSecDotMagic / WPM = usec per TCW
	//c_mSecDotMagic / WPM = msec per TCW
	//c_secDotMagic / WPM = sec per TCW
	const quint32 c_uSecDotMagic = 1200000; //units are micro-seconts
	const quint32 c_mSecDotMagic = 1200; //units are milli-seconds
	const double c_secDotMagic = 1.2; //units are seconds

	enum THRESHOLD_OPTIONS {TH_AUTO, TH_TONE, TH_DASH, TH_WORD, TH_SQUELCH};
	//Configurable thresholds
	quint32 m_usecDotDashThreshold; //Determines whether mark is dot or dash
	double m_squelchThreshold; //If squelch is on, this is compared to metric
	quint32 m_usecSpikeThreshold; // Initially ignore any tone shorter than this
	quint32 m_usecFadeThreshold; //Ignore any space shorter than this as possible fading
	quint32 m_usecElementThreshold; //Space between dot/dash in char
	quint32 m_usecCharThreshold;
	quint32 m_usecWordThreshold;


	//1 = auto, 2 = ... > 5 is fixed WPM
	int m_wpmOption;
	//Used to restrict auto tracking to a specified range
	quint32 m_wpmLimitLow;
	quint32 m_wpmLimitHigh;
	bool m_aboveWpmRange;
	bool m_belowWpmRange;

    //From SignalProcessing
	int m_numSamples;
	int m_sampleRate;
	CPX *m_out;

	Ui::dataMorse *m_dataUi;
	CPX *m_workingBuf;
	int m_demodMode; //See DeviceInterface::DEMODMODE for enum, move to lib?
	bool m_useNormalizingThreshold; //Switch how we compare tone values to determine rising or falling

	int m_modemFrequency; //CW tone we're looking for.  This is the goertzel freq or the NCO freq for mixer approach
	int m_modemSampleRate; //Modem (Goretzel or other) only needs 8k sample rate.  Bigger means more bins and slower
	int m_modemBandwidth; //Desired bw after decimation

    //Characters are output from the main receive thread and can't be output directly to textedit
    //due to Qt requirement that only main thread does Gui output
    //So we put characters into this buffer in thread and pick them up in GUI with timer
	QString m_output; //UTF-8 compatible for international char
	QMutex m_outputBufMutex;
	bool m_outputOn;

	Decimator *m_decimate;
	int m_decimationRate;
	Mixer *m_mixer;

    void syncFilterWithWpm();

	MorseCode m_morseCode;

	double m_agc_peak; // threshold for tone detection

	void init(quint32 wpm);

    //Fldigi samplerate is 8x
    //Filter does MAC every DEC_RATIO samples


    //Filters
	//
	const int c_lpFilterLen = 512;
	//N = binWidth = #samples per bin
	int m_filterSamplesPerResult; //For Fldigi algorithm
	int m_goertzelSamplesPerResult; //For Goertzel algorithm
	const int c_goertzelDefaultSamplesPerResult = 20;
	const int c_defaultModemFrequency = 1000;

	C_FIR_filter	*m_toneFilter; // linear phase finite impulse response filter

	//Smooths bit detection on rise and decay of tone
	double m_secRiseFall; //Adjust to sync with low and high speed WPM .005, .010, .020

	MovingAvgFilter	*m_jitterFilter;

    //Received CW speed can be fixed (set by user) or track actual dot/dash lengths being received
	//Smooths changes in threshold between dot and dash
	//Too long and adapting to slower speeds takes a long time. Max 16
	//Too short and errors due to jitter increase. Min 8
	//Todo: Do we need to change this based on selected wpm range? <40 8 >40 16?
	const int c_thresholdFilterSize = 8;
	MovingAvgFilter *m_dotDashThresholdFilter;
	void updateThresholds(quint32 usecNewMark, bool forceUpdate);

	//Current fixed speed if m_isAutoSpeed = false
	int m_wpmFixed;

	struct wpmMinMax {
		quint32 wpmLow;
		quint32 wpmHigh;
	};
	const quint32 m_wpmVar = 2;
	//Low and High should be slightly below and above displayed range to allow for minor wpm variance
	//We only really need 3 ranges, each with a wpm low/high factor of 2
	//For example all of the speeds between 50 and 100 can be handled by the same goertzel filter settings
	//Same for 100-200
	wpmMinMax m_minMaxTable[3] {
		{5-m_wpmVar,50+m_wpmVar},
		{50-m_wpmVar,100+m_wpmVar},
		{100-m_wpmVar,200+m_wpmVar}
	};

	//Fastest we can handle at current auto or fixed speed
	quint32 m_usecLongestMark;
	quint32 m_usecShortestMark;
	void setMinMaxMark(int wpmLow, int wpmHigh);


    // Receive buffering
	const int c_dotDashBufLen = 256;
    //This holds dots and dashes as they are received, way longer than any letter or phrase
	char m_dotDashBuf[256];

    // dotDash buffer current location
	int m_dotDashBufIndex;

    void resetDotDashBuf();
    void resetModemClock();

    //Tone timing
    //Modem clock ticks once for every sample, each tick represents 1/modemSampleRate second
    //Times tones and silence
	SampleClock *m_sampleClock;

	quint32 m_toneStart;		// Tone start timestamp
	quint32 m_toneEnd;		// Tone end timestamp
	quint32 m_usecLastMark = 0;	// length of last dot
	quint32 m_usecLastSpace = 0;	// length of last dot
	quint32 m_usecMark = 0;		// Time difference in usecs
	quint32 m_usecSpace = 0;
	//How many usec the detection algorithm (Goertzel, Filter, etc) takes to return a result
	//In some cases we may only have 3 or 4 results per Tcw and need this to keep accurate timing
	quint32 m_usecPerResult;

    enum CW_EVENT {TONE_EVENT, NO_TONE_EVENT};
    enum DECODE_STATE {IDLE, MARK_TIMING, INTER_ELEMENT_TIMING, WORD_SPACE_TIMING};
    bool stateMachine(CW_EVENT event);
	DECODE_STATE		m_receiveState;	// Indicates receive state
	DECODE_STATE		m_lastReceiveState;
	CW_EVENT		m_cwEvent;			// functions used by cw process routine

	double m_squelchIncrement; //Slider increments
	bool m_squelchEnabled;
	double	m_squelchMetric;

    //Fixed speed or computed speed based on actual dot/dash timing
	int m_wpmSpeedCurrent;				// Initially 20 WPM
	int m_wpmSpeedFilter;     //Speed filter is initialized for
	quint32 m_usecDotCurrent;		// Length of a receive Dot, in Usec based on receiveSpeed
	quint32 m_usecDashCurrent;		// Length of a receive Dash, in Usec based on receiveSpeed
    //Used to restore to base case when something changes or we get lost
	const int c_wpmSpeedInit = 18; //At or below use Farnsworth spacing, above normal spacing
	quint32 m_usecDotInit; //Was cw_send_dot_length
	quint32 m_usecDashInit;

	const bool c_useLowercase = false; //Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB

	void outputString(QString outStr);
	bool m_markHandled;

    enum OUTPUT_MODE{CHAR_ONLY,CHAR_AND_DOTDASH,DOTDASH};
	OUTPUT_MODE m_outputMode; //What we display in output

    //Replaces tracking variables
	bool m_lockWPM; //Locks the current WPM and disables tracking

    QString stateToString(DECODE_STATE state);
    void dumpStateMachine(QString why);

	bool m_useGoertzel;
	Goertzel *m_goertzel;
	void updateGoertzel(int modemFreq, int samplesPerResult);
	quint32 findBestGoertzelN(quint32 wpmLow, quint32 wpmHigh);
};

#endif // MORSE_H
