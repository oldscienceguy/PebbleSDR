#ifndef MORSE_H
#define MORSE_H

#include "SignalProcessing.h"
#include "goertzel.h"
#include "qframe"
#include "QTextEdit"
#include "ui/ui_data-morse.h"
#include "../fldigifilters.h"

class Receiver;

//This comes from configuration.h in fldigi
struct {
    int defCWspeed = 24; //Default speed (WPM)
    int CWspeed = 18; //Transmit speed (WPM)
    double CWrisetime = 4.0; //Leading and trailing edge rise times (milliseconds)
    //QSK edge shape. Values are as follows
    //0: Hanning; 1: BlackmanRaised cosine = Hannin
    int QSKshape = 0;                                 \
    bool CWtrack = true; //Automatic receive speed tracking
    int CWfarnsworth = 18; //Speed for Farnsworth timing (WPM)
    int CWrange = 10; //Tracking range for CWTRACK (WPM)
    int CWlowerlimit = 5; //Lower RX limit (WPM)
    int CWupperlimit = 50; // Upper TX limit (WPM)
    std::string CW_prosigns = "=~<>%+&{}"; //CW prosigns BT AA AS AR SK KN INT HM VE
    bool CW_use_paren = false; //Use open paren character; typically used in MARS ops
    double CWsweetspot = 1000.0; //Default CW tracking point (Hz)
    bool StartAtSweetSpot = false; //Always start new modems at sweet spot frequencies
    double CWupper = 0.6; //Detector hysterisis, upper threshold
    double CWlower = 0.4; //Detector hysterisis, lower threshold
    bool rx_lowercase = false; //Print Rx in lowercase for CW, RTTY, CONTESTIA and THROB
    int CWbandwidth = 150; //Filter bandwidth (Hz)
    bool CWmfilt = false; //Matched Filter in use
    bool CWuse_fft_filter = false; //Use FFT overlap and add convolution filter
}progdefaults;

struct {
    int		carrier;
    bool	sqlonoff;
    double  sldrSquelchValue;

} progStatus;

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

public:
    Morse(int sr, int fc);
    ~Morse();

    void SetupDataUi(QWidget *parent);

    CPX * ProcessBlock(CPX * in);
    CPX * ProcessBlockSuperRatt(CPX * in);
    CPX * ProcessBlockFldigi(CPX *in);

    void SetReceiver(Receiver *_rcv);


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

protected:
    Receiver *rcv;
    Ui::dataMorse *dataUi;


    Goertzel *cwGoertzel;
    int goertzelFreq; //CW tone we're looking for, normally 700hz
    int goertzelSampleRate; //Goertzel filter only needs 8k sample rate.  Bigger means more bins and slower
    int decimateFactor; //receiver sample rate decmiate factor to get to goertzel sample rate
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

    //FLDigi rename vars after working
    void reset_rx_filter();
    void rx_FIRprocess(const double *buf, int len);
    int rx_process(const double *buf, int len);
    MorseCode morseCode;
    void decode_stream(double value);
    double agc_peak; // threshold for tone detection
    int normalize(float *v, int n, int twodots);
    void rx_init();
    void update_Status();
    void init();

    #define CLRCOUNT 16
    #define	DEC_RATIO	16
    #define CW_FIRLEN   512
    // Maximum number of signs (dit or dah) in a Morse char.
    #define WGT_SIZE 7

    //Filters
    #define TRACKING_FILTER_SIZE 16
    C_FIR_filter	*hilbert;   // Hilbert filter precedes sinc filter
    //!fftfilt			*cw_FFT_filter; // sinc / matched filter
    C_FIR_filter	*cw_FIR_filter; // linear phase finite impulse response filter

    Cmovavg		*bitfilter;
    Cmovavg		*trackingfilter;

    int bitfilterlen;

    bool use_paren;
    std::string prosigns;
    bool stopflag;
    int bandwidth; //CW filter bandwidth
    int fragmentsize;
    bool use_fft_filter;
    double lower_threshold;
    double upper_threshold;
    int FilterFFTLen;
    bool use_matched_filter;
    double frequency;
    bool freqlock;

    double		phaseacc;		// used by NCO for rx/tx tones
    double		FFTphase;
    double		FIRphase;
    double		FFTvalue;
    double		FIRvalue;


    //!!Check usage
    #define	OUTBUFSIZE	16384
    double outbuf[OUTBUFSIZE];
    double qskbuf[OUTBUFSIZE];			// signal array for qsk drive


    // Receive buffering
    #define	RECEIVE_CAPACITY	256
    //This holds dots and dashes as they are received, way longer than any letter or phrase
    char rx_rep_buf[RECEIVE_CAPACITY];

    int cw_rr_current;				// Receive buffer current location
    void clrRepBuf(); //Reset rx_rep_buf and related pointers

    float cw_buffer[512];
    int cw_ptr;
    int clrcount;

    //!!What is this?  decimated decoder rate?
    #define	CWSampleRate 8000
    #define	CWMaxSymLen		4096		// AG1LE: - was 4096

    #define MAX_PIPE_SIZE (22 * CWSampleRate * 12 / 800)
    double pipe[MAX_PIPE_SIZE+1];			// storage for sync scope data
    double clearpipe[MAX_PIPE_SIZE+1];
    //!!mbuffer<double, MAX_PIPE_SIZE + 1, 4> scopedata;
    int pipeptr;
    int pipesize;
    bool scope_clear;

    int symbollen;  // length of a dot in sound samples (tx)
    int fsymlen; // length of extra interelement space (farnsworth)
    double risetime; // leading/trailing edge rise time (msec)
    int QSKshape; // leading/trailing edge shape factor

    // CW function return status codes.
    #define	CW_SUCCESS	0
    #define	CW_ERROR	-1
    // Tone and timing magic numbers.
    #define	DOT_MAGIC	1200000	// Dot length magic number.  The Dot length is 1200000/WPM Usec
    #define	TONE_SILENT	0	// 0Hz = silent 'tone'

    #define	USECS_PER_SEC	1000000	// Microseconds in a second

    int usec_diff(unsigned int earlier, unsigned int later);
    void update_tracking(int idot, int idash);
    void sync_parameters();

    enum CW_EVENT {CW_RESET_EVENT, CW_KEYDOWN_EVENT, CW_KEYUP_EVENT, CW_QUERY_EVENT};
    enum CW_RX_STATE {RS_IDLE, RS_IN_TONE, RS_AFTER_TONE};
    bool FldigiProcessEvent(CW_EVENT event, const char **c);
    CW_RX_STATE		cw_receive_state;	// Indicates receive state
    CW_RX_STATE		old_cw_receive_state;
    CW_EVENT		cw_event;			// functions used by cw process routine

    unsigned int	smpl_ctr;		// sample counter for timing cw rx
    unsigned int cw_rr_start_timestamp;		// Tone start timestamp
    unsigned int cw_rr_end_timestamp;		// Tone end timestamp
    long int cw_noise_spike_threshold;		// Initially ignore any tone < 10mS

    // user configurable data - local copy passed in from gui
    int cw_speed;
    int cw_bandwidth;
    int cw_squelch;
    double	metric;

    int cw_send_speed;				// Initially 18 WPM
    int cw_receive_speed;				// Initially 18 WPM
    bool usedefaultWPM;				// use default WPM

    // for CW modem use only
    bool	cwTrack;
    bool	cwLock;
    double	cwRcvWPM;
    double	cwXmtWPM;

    //Needs description
    int cw_upper_limit;
    int cw_lower_limit;
    // Receiving parameters:
    long int cw_receive_dot_length;		// Length of a receive Dot, in Usec
    long int cw_receive_dash_length;		// Length of a receive Dash, in Usec

    //Check usage, we don't support send
    long int cw_send_dot_length;			// Length of a send Dot, in Usec
    long int cw_send_dash_length;			// Length of a send Dash, in Usec
    long int cw_adaptive_receive_threshold;		// 2-dot threshold for adaptive speed

};

#endif // MORSE_H
