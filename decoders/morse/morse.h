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

//This comes from configuration.h in fldigi
struct {
    int CWspeed = 18; //Transmit speed (WPM)
    int CWlowerlimit = 5; //Lower RX limit (WPM)
    int CWupperlimit = 50; // Upper TX limit (WPM)
    std::string CW_prosigns = "=~<>%+&{}"; //CW prosigns BT AA AS AR SK KN INT HM VE
    bool CW_use_paren = false; //Use open paren character; typically used in MARS ops
    bool StartAtSweetSpot = false; //Always start new modems at sweet spot frequencies
    bool rx_lowercase = false; //Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB

    bool CWuse_fft_filter = false; //Use FFT overlap and add convolution filter
}progdefaults;


//Utility functions from fldigi
inline double clamp(double x, double min, double max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}

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

    void calcDotDashLength(int speed);
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

    #define	DEC_RATIO	16 //Decimation Ratio replaced with modemDecimateFactor
    #define CW_FIRLEN   512
    // Maximum number of signs (dit or dah) in a Morse char.
    #define WGT_SIZE 7

    //Filters
    #define TRACKING_FILTER_SIZE 16
    C_FIR_filter	*hilbert;   // Hilbert filter precedes sinc filter
    //!fftfilt			*cw_FFT_filter; // sinc / matched filter
    C_FIR_filter	*cw_FIR_filter; // linear phase finite impulse response filter

    Cmovavg		*bitfilter;

    //Received CW speed can be fixed (set by user) or track actual dot/dash lengths being received
    Cmovavg *trackingfilter;
    void updateAdaptiveThreshold(int idotUsec, int idashUsec);
    bool speedTracking;
    bool speedTrackingDefault = true; //Automatic receive speed tracking

    int trackingWPMRange = 10; //Tracking range for CWTRACK (WPM)


    int bitfilterlen;

    bool use_paren;
    std::string prosigns;
    bool stopflag;
    bool use_fft_filter;

    int FilterFFTLen;

    bool freqlock;

    //Use by NCO in mixer to mix cwToneFrequency
    double		phaseacc;
    double		FFTphase;
    double		FIRphase;

    double		FFTvalue;
    double		FIRvalue;


    // Receive buffering
    #define	RECEIVE_CAPACITY	256
    //This holds dots and dashes as they are received, way longer than any letter or phrase
    char dotDashBuf[RECEIVE_CAPACITY];

    // dotDash buffer current location
    int dotDashBufIndex;

    void resetDotDashBuf();

    //Tone timing
    //Modem clock ticks once for every sample, each tick represents 1/modemSampleRate second
    //Times tones and silence
    #define	USECS_PER_SEC	1000000	// Microseconds in a second
    //All usec variable are quint32 because modemClock is quint32 and we don't want conversion errors
    //quint32 is approx 71 minutes (70*60*USEC_PER_SEC)
    quint32 modemClock; //!!Do we have to do anything special to handle overflow wrap back to 0?
    quint32 modemClockToUsec(unsigned int earlier, unsigned int later);

    unsigned int toneStart;		// Tone start timestamp
    unsigned int toneEnd;		// Tone end timestamp
    quint32 noiseSpikeUsec;		// Initially ignore any tone < 10mS
    quint32 lastElementUsec = 0;	// length of last dot/dash
    quint32 elementUsec = 0;		// Time difference in usecs
    bool spaceWasSent = true;	// for word space logic


    // CW function return status codes.
    #define	CW_SUCCESS	0
    #define	CW_ERROR	-1
    // Tone and timing magic numbers.
    #define	DOT_MAGIC	1200000	// Dot length magic number.  The Dot length is 1200000/WPM Usec
    #define	TONE_SILENT	0	// 0Hz = silent 'tone'


    void syncTiming();

    enum CW_EVENT {CW_RESET_EVENT, CW_KEYDOWN_EVENT, CW_KEYUP_EVENT, CW_QUERY_EVENT};
    enum CW_RX_STATE {RS_IDLE, RS_IN_TONE, RS_AFTER_TONE};
    bool FldigiProcessEvent(CW_EVENT event, const char **c);
    CW_RX_STATE		receiveState;	// Indicates receive state
    CW_RX_STATE		lastReceiveState;
    CW_EVENT		cw_event;			// functions used by cw process routine

    // user configurable data - local copy passed in from gui
    int cw_speed;
    double	metric;
    double squelchIncrement; //Slider increments
    double squelchValue; //If squelch is on, this is compared to metric
    bool sqlonoff;

    //Fixed speed or computed speed based on actual dot/dash timing
    int receiveSpeedWpm;				// Initially 18 WPM
    quint32 receiveDotUsec;		// Length of a receive Dot, in Usec based on receiveSpeed
    quint32 receiveDashUsec;		// Length of a receive Dash, in Usec based on receiveSpeed
    //Used to restore to base case when something changes or we get lost
    int receiveSpeedDefaultWpm;
    quint32 receiveDotDefaultUsec; //Was cw_send_dot_length
    quint32 receiveDashDefaultUsec;

    bool usedefaultWPM;				// use default WPM

    //Needs description
    int upperLimitUsec;
    int lowerLimitUsec;
    // Receiving parameters:

    quint32 adaptiveThresholdUsec;		// 2-dot threshold for adaptive speed

};

#endif // MORSE_H
