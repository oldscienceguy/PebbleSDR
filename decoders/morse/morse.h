#ifndef MORSE_H
#define MORSE_H

#include "signalprocessing.h"
#include "goertzel.h"
#include "qframe"
#include "QTextEdit"
#include "ui/ui_data-morse.h"
#include "../fldigifilters.h"

#include "demod/downconvert.h"
#include "demod.h"
#include <QMutex>

class Receiver;

#pragma clang diagnostic ignored "-Wc++11-extensions"

//Utility functions from fldigi
inline double clamp(double x, double min, double max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}

//Weight is the factor to weight the input when calculating the average
//Weight=20: (input * 1/20) + average * (19/20) - Short average, only 20 loops to make average == steady input
//Weight=800: (input * 1/800) + average * (799/800) - Long average, stead input would take 800 to == average
inline double decayavg(double average, double input, double weight)
{
    if (weight <= 1.0) return input;
    return input * (1.0 / weight) + average * (1.0 - (1.0 / weight));
}

//Replace original Pebble lookup with this so we have one method
#define	MorseTableSize	256

//What goes into rep_buf as we receive
#define	CW_DOT_REPRESENTATION	'.'
#define	CW_DASH_REPRESENTATION	'-'


struct CW_TABLE {
    char chr;	/* The character(s) represented */
    const char *prt;	/* The printable representation of the character */
    const char *rpr;	/* Dot-dash shape of the character */
};

struct CW_XMT_TABLE {
    unsigned long code;
    const    char *prt;
};

class MorseCode {
public:
    MorseCode() {
        init();
    }
    ~MorseCode() {
    }
    void init();
    const char	*rx_lookup(char *r);
    unsigned long	tx_lookup(int c);
    const char *tx_print(int c);
private:
    CW_TABLE 		*cw_rx_lookup[256];
    CW_XMT_TABLE 	cw_tx_lookup[256];
    unsigned int 	tokenize_representation(const char *representation);
    const bool useParen = false; //Use open paren character; typically used in MARS ops
    const std::string prosigns = "=~<>%+&{}"; //CW prosigns BT AA AS AR SK KN INT HM VE


};



//We need to inherit from QObject so we can filter events.
//Remove if we eventually create a separate 'morse widget' object for UI
class Morse : public SignalProcessing
{
    Q_OBJECT

public:
    Morse(int sr, int fc);
    ~Morse();

    void SetupDataUi(QWidget *parent);

    CPX * ProcessBlock(CPX * in);
    CPX * ProcessBlockSuperRatt(CPX * in);
    CPX * ProcessBlockFldigi(CPX *in);

    void SetReceiver(Receiver *_rcv);
    void setCWMode(DEMODMODE m);


    //Updated by ProcessBlock and holds power levels.
    //Note this is smaller buffer than frameCount because of decimation
    float *powerBuf;  //Not sure if we need this

    bool *toneBuf; //true if goertzel returns tone present

    //Returns tcw in ms for any given WPM
    int WpmToTcw(int w);
    //Returns wpm for any given tcw
    int TcwToWpm(int t);

    //Sets WPM and related conters
    void SetElementLengths(int dotCount);

    void OutputData(const char *d);
    void OutputData(const char c);

public slots:
    void squelchChanged(int v);
    void refreshOutput();
    void resetOutput();
    void onBoxChecked(bool);

signals:
    void newOutput();

protected:
    Receiver *rcv;
    Ui::dataMorse *dataUi;
    CPXBuf *workingBuf;
    DEMODMODE cwMode;
    bool useNormalizingThreshold; //Switch how we compare tone values to determine rising or falling


    Goertzel *cwGoertzel;
    int modemFrequency; //CW tone we're looking for.  This is the goertzel freq or the NCO freq for mixer approach
    int modemSampleRate; //Modem (Goretzel or other) only needs 8k sample rate.  Bigger means more bins and slower
    int modemDecimateFactor; //receiver sample rate decmiate factor to get to modem sample rate
    int powerBufSize;
    int toneBufSize;

    int toneBufCounter;

    //Temp for painting
    const char *outString;
    bool outTone;

    int maxWPM; //Determines bin resolution
    bool fixedWPM;
    int wpm;
    int numResultsPerTcw;

    //Are we counting to see if we have a dot/dash mark or a element/word space
    enum {NOT_COUNTING, MARK_COUNTING, SPACE_COUNTING} countingState;
    enum {LETTER,WORD,SPACE} elementState;
    int markCount;
    int spaceCount;
    int maxMarkCount; //Timeout values
    int maxSpaceCount;

    //In 'counts' of goertzel results.  N counts = 1 tcw
    int countsPerDashThreshold; //Temp counts per dot to see if we need to reset wpm
    int shortestCounter;

    int countsPerDot;
    int countsPerDash;
    int countsPerElementSpace; //Between dots and dashes
    int countsPerCharSpace; //Between char of a word
    int countsPerWordSpace; //Between words

    bool pendingElement; //True if unprocessed element
    bool pendingChar;
    bool pendingWord;
    int element; //1 if dash, 0 if dot

    //Characters are output from the main receive thread and can't be output directly to textedit
    //due to Qt requirement that only main thread does Gui output
    //So we put characters into this buffer in thread and pick them up in GUI with timer
    char outputBuf[256]; //Way bigger than we need
    int outputBufIndex; //Position in output buf of next char, zero means buffer is empty
    QMutex outputBufMutex;
    bool outputOn;

    //FLDigi rename vars after working

    CDownConvert modemDownConvert; //Get to modem rate and mix

    CPX mixer(CPX in);
    void reset_rx_filter();

    MorseCode morseCode;
    void decode_stream(double value);
    double agc_peak; // threshold for tone detection

    void rx_init();
    void init();

    //Fldigi samplerate is 8x
    //Filter does MAC every DEC_RATIO samples

    const static int DEC_RATIO	= 16; //Decimation Ratio replaced with modemDecimateFactor
    const static int CW_FIRLEN = 512;

    //Filters
    const static int TRACKING_FILTER_SIZE = 16;
    C_FIR_filter	*hilbert;   // Hilbert filter precedes sinc filter
    //!fftfilt			*cw_FFT_filter; // sinc / matched filter
    C_FIR_filter	*cw_FIR_filter; // linear phase finite impulse response filter

    Cmovavg		*bitfilter;

    //Received CW speed can be fixed (set by user) or track actual dot/dash lengths being received
    Cmovavg *trackingfilter;
    void updateAdaptiveThreshold(quint32 idotUsec, quint32 idashUsec);
    bool speedTrackingCurrent;
    bool speedTrackingInit = true; //Automatic receive speed tracking

    const int trackingWPMRange = 10; //Tracking range for CWTRACK (WPM)
    const int lowerWPMLimit = 5; //Lower RX limit (WPM)
    const int upperWPMLimit = 50; // Upper RX limit (WPM)


    int bitfilterlen;

    const bool useFFTFilter = false;

    int FilterFFTLen;

    //Use by NCO in mixer to mix cwToneFrequency
    double		phaseacc;
    double		FFTphase;
    double		FIRphase;

    double		FFTvalue;
    double		FIRvalue;


    // Receive buffering
    const static int RECEIVE_CAPACITY = 256;
    //This holds dots and dashes as they are received, way longer than any letter or phrase
    char dotDashBuf[RECEIVE_CAPACITY];

    // dotDash buffer current location
    int dotDashBufIndex;

    void resetDotDashBuf();
    void resetModemClock();

    //Tone timing
    //Modem clock ticks once for every sample, each tick represents 1/modemSampleRate second
    //Times tones and silence
    const static int USECS_PER_SEC = 1000000;	// Microseconds in a second
    //All usec variable are quint32 because modemClock is quint32 and we don't want conversion errors
    //quint32 is approx 71 minutes (70*60*USEC_PER_SEC)
    quint32 modemClock; //!!Do we have to do anything special to handle overflow wrap back to 0?
    quint32 modemClockToUsec(unsigned int earlier, unsigned int later);

    quint32 toneStart;		// Tone start timestamp
    quint32 toneEnd;		// Tone end timestamp
    quint32 usecNoiseSpike;		// Initially ignore any tone < 10mS
    quint32 usecLastElement = 0;	// length of last dot/dash
    quint32 usecElement = 0;		// Time difference in usecs
    bool spaceWasSent = true;	// for word space logic

    void calcDotDashLength(int _speed, quint32 & _usecDot, quint32 & _usecDash);

    void syncTiming();

    enum CW_EVENT {RESET_EVENT, KEYDOWN_EVENT, KEYUP_EVENT, QUERY_EVENT};
    enum DECODE_STATE {IDLE, IN_TONE, AFTER_TONE};
    bool FldigiProcessEvent(CW_EVENT event, const char **outStr);
    DECODE_STATE		receiveState;	// Indicates receive state
    DECODE_STATE		lastReceiveState;
    CW_EVENT		cw_event;			// functions used by cw process routine

    // user configurable data - local copy passed in from gui
    double	metric;
    double squelchIncrement; //Slider increments
    double squelchValue; //If squelch is on, this is compared to metric
    bool sqlonoff;

    //Fixed speed or computed speed based on actual dot/dash timing
    int wpmSpeedCurrent;				// Initially 18 WPM
    quint32 usecDotCurrent;		// Length of a receive Dot, in Usec based on receiveSpeed
    quint32 usecDashCurrent;		// Length of a receive Dash, in Usec based on receiveSpeed
    //Used to restore to base case when something changes or we get lost
    const int wpmSpeedInit = 18;
    quint32 usecDotInit; //Was cw_send_dot_length
    quint32 usecDashInit;

    bool usedefaultWPM;				// use default WPM
    const bool useLowercase = false; //Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB


    //Needs description
    quint32 usecUpperLimit;
    quint32 usecLowerLimit;
    // Receiving parameters:

    quint32 usecAdaptiveThreshold;		// 2-dot threshold for adaptive speed

    void dumpStateMachine(QString why);
};

#endif // MORSE_H
