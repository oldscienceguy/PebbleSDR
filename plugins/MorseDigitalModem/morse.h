#ifndef MORSE_H
#define MORSE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//#include "processstep.h"
//#include "goertzel.h"
#include "qframe"
#include "QTextEdit"
#include "ui_data-morse.h"
#include "../../pebblelib/fldigifilters.h"
#include "movingavgfilter.h"
#include "morsecode.h"
#include "../../pebblelib/downconvert.h"
#include "decimator.h"
#include "mixer.h"
//#include "demod.h"
#include <QMutex>
#include "../../pebblelib/digital_modem_interfaces.h"

class Receiver;

//General purpose class for using incoming sample count for timing signals
class SampleClock
{
public:
	SampleClock(quint32 sampleRate);
	~SampleClock();

	//Call this on every new sample to update m_clock
	void inline tick() {m_clock++;}

	//Returns uSec delta between current clock and earlier clock
	quint32 uSecToCurrent(quint32 earlierClock);

	//Returns uSec delta between two clocks
	quint32 uSecDelta(quint32 earlierClock, quint32 laterClock);

	//Returns uSec from clock start
	quint32 uSec();

	//Returns current m_clock
	quint32 inline clock() {return m_clock;}

	//Starts everything over
	void reset();

	//Saves current clock for interval timing
	void startInterval();
	//Returns interval
	quint32 uSecInterval();

private:
	quint32 m_sampleRate;
	//quint32 can clock for 4294 sec (4,294,000,000 uSec) or 71 minutes
	//So don't worry about wrap
	quint32 m_clock;
	quint32 m_startInterval;

};

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


    //Returns tcw in ms for any given WPM
	int wpmToTcw(int w);
    //Returns wpm for any given tcw
	int tcwToWpm(int t);

	void outputData(const char *d);
	void outputData(const char c);

	quint32 wpmToUsec(int w);
	int usecToWPM(quint32 u);

public slots:
    void squelchChanged(int v);
    void refreshOutput();
    void resetOutput();
    void onBoxChecked(bool b);
    void outputOptionChanged(int s);
    void lockWPMChanged(bool b);

signals:
    void newOutput();
	void testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
	void testbench(int _length, double* _buf, double _sampleRate, int _profile);
	bool addProfile(QString profileName, int profileNumber); //false if profilenumber already exists
	void removeProfile(quint16 profileNumber);


protected:
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
	char m_outputBuf[256]; //Way bigger than we need
	int m_outputBufIndex; //Position in output buf of next char, zero means buffer is empty
	QMutex m_outputBufMutex;
	bool m_outputOn;

	Decimator *m_decimate;
	Mixer *m_mixer;

    void syncFilterWithWpm();

	MorseCode m_morseCode;

	double m_agc_peak; // threshold for tone detection

    void init();

    //Fldigi samplerate is 8x
    //Filter does MAC every DEC_RATIO samples

    const static int DEC_RATIO	= 16; //Decimation Ratio replaced with modemDecimateFactor
    const static int CW_FIRLEN = 512;

    //Filters
    const static int TRACKING_FILTER_SIZE = 16;
	C_FIR_filter	*m_toneFilter; // linear phase finite impulse response filter

	//Smooths bit detection on rise and decay of tone
	MovingAvgFilter	*m_avgBitPowerFilter;

    //Received CW speed can be fixed (set by user) or track actual dot/dash lengths being received
	//Smooths changes in threshold between dot and dash
	MovingAvgFilter *m_avgThresholdFilter;
    void updateAdaptiveThreshold(quint32 idotUsec, quint32 idashUsec);

	const int m_trackingWPMRange = 10; //Tracking range for CWTRACK (WPM)
	const int m_lowerWPMLimit = 5; //Lower RX limit (WPM)
	const int m_upperWPMLimit = 50; // Upper RX limit (WPM)


	int m_bitfilterlen;

    // Receive buffering
	const static int m_receiveCapacity = 256;
    //This holds dots and dashes as they are received, way longer than any letter or phrase
	char m_dotDashBuf[m_receiveCapacity];

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
	quint32 m_usecNoiseSpike;		// Initially ignore any tone < 10mS
	quint32 m_usecLastMark = 0;	// length of last dot
	quint32 m_usecLastSpace = 0;	// length of last dot
	quint32 m_usecMark = 0;		// Time difference in usecs
	quint32 m_usecSpace = 0;
	quint32 m_usecElementThreshold = 0; //Space between dot/dash in char
    void calcDotDashLength(int _speed, quint32 & _usecDot, quint32 & _usecDash);

    void syncTiming();


    enum CW_EVENT {TONE_EVENT, NO_TONE_EVENT};
    enum DECODE_STATE {IDLE, MARK_TIMING, INTER_ELEMENT_TIMING, WORD_SPACE_TIMING};
    bool stateMachine(CW_EVENT event);
	DECODE_STATE		m_receiveState;	// Indicates receive state
	DECODE_STATE		m_lastReceiveState;
	CW_EVENT		m_cwEvent;			// functions used by cw process routine

    // user configurable data - local copy passed in from gui
	double	m_squelchMetric;
	double m_squelchIncrement; //Slider increments
	double m_squelchValue; //If squelch is on, this is compared to metric
	bool m_squelchEnabled;

    //Fixed speed or computed speed based on actual dot/dash timing
	int m_wpmSpeedCurrent;				// Initially 18 WPM
	int m_wpmSpeedFilter;     //Speed filter is initialized for
	quint32 m_usecDotCurrent;		// Length of a receive Dot, in Usec based on receiveSpeed
	quint32 m_usecDashCurrent;		// Length of a receive Dash, in Usec based on receiveSpeed
    //Used to restore to base case when something changes or we get lost
	const int m_wpmSpeedInit = 18;
	quint32 m_usecDotInit; //Was cw_send_dot_length
	quint32 m_usecDashInit;

	const bool m_useLowercase = false; //Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB


    //Fastest we can handle at sample rate
	quint32 m_usecPerSample;
	quint32 m_usecLongestMark;
	quint32 m_usecShortestMark;
    // Receiving parameters:

	quint32 m_usecAdaptiveThreshold;		// 2-dot threshold for adaptive speed

	const char *m_spaceTiming(bool lookingForChar);
    void outputString(const char *outStr);
    void addMarkToDotDash();
	bool m_markHandled;

    enum OUTPUT_MODE{CHAR_ONLY,CHAR_AND_DOTDASH,DOTDASH};
	OUTPUT_MODE m_outputMode; //What we display in output

    //Replaces tracking variables
	bool m_lockWPM; //Locks the current WPM and disables tracking

    QString stateToString(DECODE_STATE state);
    void dumpStateMachine(QString why);
};

#endif // MORSE_H
