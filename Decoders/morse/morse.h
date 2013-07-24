#ifndef MORSE_H
#define MORSE_H

#include "SignalProcessing.h"
#include "goertzel.h"
#include "qframe"
#include "QTextEdit"
#include "ui/ui_data-morse.h"

class Receiver;

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

    const char * MorseToAscii(quint16 morse);
    const char * MorseToDotDash(quint16 morse);


    void OutputData(const char *d);

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
    unsigned char lastChar;

    //FLDigi rename vars after working
    // Receive buffering
    #define	RECEIVE_CAPACITY	256
    //This holds dots and dashes as they are received, way longer than any letter or phrase
    char rx_rep_buf[RECEIVE_CAPACITY];
    //What goes into rep_buf
    #define	CW_DOT_REPRESENTATION	'.'
    #define	CW_DASH_REPRESENTATION	'-'

    int cw_rr_current;				// Receive buffer current location

    float cw_buffer[512];
    int cw_ptr;
    int clrcount;

    //This comes from configuration.h in fldigi
    struct {
        int defCWspeed; //Default speed (WPM)
        int CWspeed; //Transmit speed (WPM)
        double CWrisetime; //Leading and trailing edge rise times (milliseconds)
        //QSK edge shape. Values are as follows
        //0: Hanning; 1: BlackmanRaised cosine = Hannin
        int QSKshape;                                 \
        bool CWtrack; //Automatic receive speed tracking
        int CWfarnsworth; //Speed for Farnsworth timing (WPM)
        int CWrange; //Tracking range for CWTRACK (WPM)
        int CWlowerlimit; //Lower RX limit (WPM)
        int CWupperlimit; // Upper TX limit (WPM)
    }progdefaults;

    int			symbollen;		// length of a dot in sound samples (tx)
    int			fsymlen;        	// length of extra interelement space (farnsworth)
    double risetime;			    	// leading/trailing edge rise time (msec)
    int QSKshape;                   		// leading/trailing edge shape factor

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

    enum CW_EVENT {CW_RESET, CW_KEYDOWN, CW_KEYUP, CW_QUERY};
    enum CW_RX_STATE {RS_IDLE, RS_IN_TONE, RS_AFTER_TONE};
    bool FldigiProcessEvent(CW_EVENT event);
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
