#include "morse.h"
#include "QPainter"
#include "QDebug"
#include "QEvent"
#include "Receiver.h"

/*
  CW Notes for reference
  Tcw is basic unit of measurement in ms
  Dash = 3 Tcw, Dot = 1Tcw, Element Space = 1 Tcw, Character Space = 3 Tcw, Word Space = 7 Tcw (min)
  Standard for measuring WPM is to use the word "PARIS", which is exactly 50Tcw
  So Tcw (ms) = 60 (sec) / (WPM * 50)
  and WPM = 1200/Tcw(ms)
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


*/

//Relative lengths of elements in Tcw terms
#define DOT_TCW 1
#define DASH_TCW 3
#define CHARSPACE_TCW 3
#define ELEMENTSPACE_TCW 1
#define WORDSPACE_TCW 7

#define MIN_DOT_COUNT 4

Morse::Morse(int sr, int fc) : SignalProcessing(sr,fc)
{
    //Setup Goertzel
    cwGoertzel = new Goertzel(sr, fc);
    //Process Goertzel at 8k sample rate
    //!!4-27-12 changing this to 16k should dynamically change everything and still work, but it doesn't
    //Check for hard dependencies on 8k in code
    goertzelSampleRate = 8000;
    goertzelFreq = 700;

    //Start with fixed WPM for now
    fixedWPM = true;
    maxWPM = 60;
    wpm = 30;
    //binwidth for maxWPM = 60 is 200hz
    //User should set maxWPM and we should calculate binWidth, sharper = more accurate
    toneBufSize = cwGoertzel->SetFreqHz(goertzelFreq, 200, goertzelSampleRate);

    //Max length for mark and space
    maxMarkCount = 2000 / cwGoertzel->timePerBin;
    maxSpaceCount = 5000 / cwGoertzel->timePerBin;

    //Number of goertzel results we need for a reliable tcw
    //numResultsPerTcw = WpmToTcw(wpm) / cwGoertzel->timePerBin;

    //Start with min 4 results per dot/tcw
    SetElementLengths(MIN_DOT_COUNT);


    decimateFactor = sampleRate / goertzelSampleRate;
    powerBufSize = numSamples / decimateFactor;
    powerBuf = new float[powerBufSize];
    toneBuf = new bool[toneBufSize];
    toneBufCounter = 0;

    outString = "";

    countingState = NOT_COUNTING;
    markCount = 0;
    spaceCount = 0;
    countsPerDashThreshold = MIN_DOT_COUNT;

    dataUi = NULL;

}

Morse::~Morse()
{
    if (cwGoertzel != NULL) delete cwGoertzel;
    if (powerBuf != NULL) free (powerBuf);
    if (toneBuf != NULL) free (toneBuf);

}

void Morse::SetupDataUi(QWidget *parent)
{
    dataUi = new Ui::dataMorse();
    dataUi->setupUi(parent);
}

//Returns tcw in ms for any given WPM
int Morse::WpmToTcw(int w)
{
    int tcw = 60.0 / (w * 50) * 1000;
    return tcw;
}
//Returns wpm for any given tcw (in ms)
int Morse::TcwToWpm(int t)
{
    int wpm = 1200/t;
    return wpm;
}

//d is count of number of goertzel results for shortest element, ie dot
void Morse::SetElementLengths(int d)
{
    if (d < MIN_DOT_COUNT)
        d = MIN_DOT_COUNT; //Smallest possible dot count

    countsPerDot = countsPerDot * 0.70 + d * 0.30;
    //Never below minimum
    if (countsPerDot < MIN_DOT_COUNT)
        countsPerDot = MIN_DOT_COUNT;

    countsPerDash = countsPerDot * DASH_TCW;
    countsPerElementSpace = countsPerDot * ELEMENTSPACE_TCW;
    countsPerCharSpace = countsPerDot * CHARSPACE_TCW;
    countsPerWordSpace = countsPerDot * WORDSPACE_TCW;

    //Dash threshold midway between dot and dash len
    countsPerDashThreshold = countsPerDot * 2;

    //qDebug("CountsPerDashThreshold %d",countsPerDashThreshold);
    shortestCounter = 0;

}

void Morse::SetReceiver(Receiver *_rcv)
{
    rcv = _rcv;
}

CPX * Morse::ProcessBlockSuperRatt(CPX *in)
{
    bool isMark;
    int lastSpaceCount;

    cwGoertzel->ProcessBlock(in,toneBuf);

    //toneBuf now has binary results, decode
    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
    for (int i=0; i<toneBufSize; i++) {
        isMark = toneBuf[i];

        //Update sMeter
        //db = powerToDb(power);
        //rcv->GetSignalStrength()->setExtValue(db);
#if 0
        if (isMark)
            //rcv->GetSignalStrength()->setExtValue(-13); //Full scale
            rcv->receiverWidget->DataBit(true);
        else
            //rcv->GetSignalStrength()->setExtValue(-127); //0
            rcv->receiverWidget->DataBit(false);
#endif
        switch (countingState) {
        case NOT_COUNTING:
            //Initial state when we're looking for something to start syncing with
            lastChar = 0;
            if (isMark) {
                //Start counting
                countingState = MARK_COUNTING;
                markCount = 1;
                spaceCount = 0;
            }
            break;
        case MARK_COUNTING:
            if (isMark) {
                markCount++;
                //Is it too long?
                if (markCount > maxMarkCount) {
                    countingState = NOT_COUNTING;
                    markCount = 0;
                    spaceCount = 0;
                }
            } else {
                //We have a transition from mark to space
                //Dynamic WPM
                if (markCount < countsPerDot) {
                    //Update average whenever we dot length is decreasing or we have a dash
                    //SetWPM(markCount);
                    //Count is too shart for dot,
                    markCount = 0;
                    spaceCount = 0;
                    countingState = NOT_COUNTING;
                } else {
                    //We want to keep countsPerAverageDot midway between current dot and dash length
                    if (markCount > countsPerDashThreshold) {
                        //Assume Dash
                        element = 1;
                        pendingElement = true;
                        //Update whenever we think we have a dash
                        SetElementLengths(markCount / DASH_TCW);

                    } else {
                        //Dot
                        element = 0;
                        pendingElement = true;
                        //Weight dots * 2 so countsPerAverageDot is midrange between dot and dash
                        SetElementLengths(markCount);

                    }
                    countingState = SPACE_COUNTING;
                    markCount = 0;
                    spaceCount = 1;
                }
            }
            break;
        case SPACE_COUNTING:
            if (!isMark) {
                spaceCount++;
                lastSpaceCount = spaceCount;
            } else {
                //Transition from space to mark
                countingState = MARK_COUNTING;
                markCount = 1;
                lastSpaceCount = spaceCount;
                spaceCount = 0;
            }
            //Is space long enough to build char or output something?
            if (lastSpaceCount >= countsPerElementSpace) {
                //Process any pending elements
                if (pendingElement) {
                    //If we haven't started a char, set hob
                    if (lastChar == 0)
                        lastChar = 1;
                    lastChar = lastChar << 1; //Shift left to make room
                    lastChar = lastChar | element; //Set lob
                    pendingChar = true;
                    pendingElement = false;
                    pendingWord = false;
                }
            }
            if (lastSpaceCount >= countsPerCharSpace) {
                //Process char
                if (pendingChar) {
                    //qDebug("%d",lastChar);
                    rcv->OutputData(MorseToAscii(lastChar));
                    lastChar = 0;
                    pendingChar = false;
                    pendingWord = true;
                }
            }
            if (lastSpaceCount >= countsPerWordSpace) {
                //Process word
                if (pendingWord) {
                    rcv->OutputData(" ");
                    pendingWord = false;
                }
            }
            if (lastSpaceCount > maxSpaceCount) {
                //Output last char?
                //Reset
                countingState = NOT_COUNTING;
                markCount = 0;
                spaceCount = 0;
                pendingElement = pendingChar = pendingWord = false;
            }

            break;
        }
    }
    return in;
}

CPX * Morse::ProcessBlock(CPX * in)
{
    return ProcessBlockSuperRatt(in);

    bool res;
    bool isMark;
    float db;
    float power;

    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
    for (int i=0, j=0; i<numSamples; i=i+decimateFactor, j++) {
        //res = cwGoertzel->NewSample(in[i], power);
        if (res) {
            //Update sMeter
            db = powerToDb(power);
            rcv->GetSignalStrength()->setExtValue(db);

            isMark = cwGoertzel->binaryOutput;

            switch (countingState) {
            case NOT_COUNTING:
                //Initial state when we're looking for something to start syncing with
                if (isMark) {
                    //Start counting
                    countingState = MARK_COUNTING;
                    markCount = 1;
                    spaceCount = 0;
                }
                break;
            case MARK_COUNTING:
                if (isMark) {
                    markCount++;
                    //Is it too long?
                } else {
                    //We have a transition from mark to space
                    //Dynamic WPM
                    //Look for repeated dot length 3 times then set new countsPer...

                    //Is markCount long enough? if not, do we ignore space and continue with looking for mark?
                    if (markCount > countsPerDot) {
                        //Valid, is it long enough for dash?
                        if (markCount > countsPerDash) {
                            rcv->OutputData("-");

                        } else {
                            rcv->OutputData(".");
                        }
                        markCount = 0;
                        spaceCount = 1;
                        countingState = SPACE_COUNTING;

                    } else {
                        //markCount too short
                        //Reset state and start over
                        countingState = NOT_COUNTING;
                        markCount = 0;
                        spaceCount = 0;
                    }

                }
                break;
            case SPACE_COUNTING:
                if (!isMark) {
                    spaceCount++;
                } else {
                    //Transition from space to mark
                    //Is spaceCount long enough
                    if (spaceCount > countsPerElementSpace) {
                        if (spaceCount > countsPerWordSpace) {

                        } else if (spaceCount > countsPerCharSpace) {

                        }

                    } else {
                        //Too short
                        countingState = NOT_COUNTING;
                        markCount = 0;
                        spaceCount = 0;
                    }
                }
                //Is space long
                break;
            }

#if 0
            //We have accumulated enough samples to get output of Goertzel
            //Store last N tones
            if (toneBufCounter < toneBufSize) {
                toneBuf[toneBufCounter] = cwGoertzel->binaryOutput;
                toneBufCounter++;
            }
            qDebug("Tone %d",cwGoertzel->binaryOutput);

            if (toneBufCounter > numResultsPerTcw) {
                //We need at least numResultsPerTcw results
                //if (toneBuf[0] && toneBuf[1] && toneBuf[2] && toneBuf[3]) {
                markCount = spaceCount = 0;
                for (int i=0; i<numResultsPerTcw; i++) {
                    if (toneBuf[i])
                        markCount++;
                    else
                        spaceCount++;
                }
                if (markCount == numResultsPerTcw) {
                    //All results are mark
                    //1 Tcw Is it dash or dot
                    outString = "M";
                    outTone = true;
                    toneBufCounter = 0;
                    rcv->OutputData(outString);
                } else if (spaceCount == numResultsPerTcw) {
                    //All results are space
                    //1 Tcw, is it letter or word space
                    outString = "S";
                    outTone = false;
                    toneBufCounter = 0;
                    rcv->OutputData(outString);
                } else {
                    //Not all the same, shift and try again
                    for (int i=0; i<numResultsPerTcw - 1; i++) {
                        toneBuf[i] = toneBuf[i+1];
                    }
                    toneBufCounter --;
                }
            }
#endif
            //powerBuf[j] = power;
            //qDebug("G sample / power = %.12f / %.12f",sample,power);
            //qDebug("G MS / average / power = %c / %.12f / %.12f",cwGoertzel->binaryOutput?'M':'S', cwGoertzel->avgPower, power);

            //Do we have a new element?
        }
    }

    return in;
}

struct morseChar {
    const char* ascii;
    quint16 binaryValue; //'1' = start '0'=dit '1'=dash
    const char* dotDash;
};

//Same encoding as my SuperRatt code in 1983, see 6502 asm code extract below
// TABLE OF CW CODES IN ASCII ORDER FROM 0x20 TO 0x5F
// 1ST HIGH BIT IN # SIGNALS START OF CODE
// 0=DOT 1=DASH
//

morseChar morseAsciiOrder[] = {
    {" ", 0x00, "na"}, //Space Ascii 0x20
    {"!", 0x6b, "— · — · — —"},
    {"\"",0x52, "· — · · — ·"},
    {"#", 0x00, "na"},
    {"$", 0x89, "· · · — · · —"},
    {"%", 0x00, "na"},
    {"&", 0x28, "· — · · ·"},
    {"\'",0x5e, "· — — — — ·"},
    {"(", 0x36, "— · — — ·"},
    {")", 0x6D, "— · — — · —"},
    {"*", 0x00, "na"},
    {"+", 0x2a, "· — · — ·"},
    {",", 0x73, "— — · · — —"},
    {"-", 0x61, "— · · · · —"},
    {".", 0x55, "· — · — · —"},
    {"/", 0x32, "— · · — ·"}, //Ascii 0x2f

    {"0", 0x3f, "— — — — —"},
    {"1", 0x2f, "· — — — —"},
    {"2", 0x27, "· · — — —"},
    {"3", 0x23, "· · · — —"},
    {"4", 0x21, "· · · · —"},
    {"5", 0x20, "· · · · ·"},
    {"6", 0x30, "— · · · ·"},
    {"7", 0x38, "— — · · ·"},
    {"8", 0x3c, "— — — · ·"},
    {"9", 0x3e, "— — — — ·"}, //Ascii 0x39

    {":", 0x78, "— — — · · ·"},
    {";", 0x6A, "— · — · — ·"}, //Also 'Starting signal'
    {"<", 0x00, "na"},
    {"=", 0x31, "— · · · —"},
    {">", 0x00, "na"},
    {"?", 0x4c, "· · — — · ·"},
    {"@", 0x5a, "· — — · — ·"},

    {"a", 0x05, "· —"}, //Ascii 0x41
    {"b", 0x18, "— · · ·"},
    {"c", 0x1a, "— · — ·"},
    {"d", 0x0c, "— · ·"},
    {"e", 0x02, "·"},
    {"f", 0x12, "· · — ·"},
    {"g", 0x0e, "— — ·"},
    {"h", 0x10, "· · · ·"},
    {"i", 0x04, "· ·"},
    {"j", 0x17, "· — — —"},
    {"k", 0x0d, "— · —"}, //Also 'Invitation to transmit'
    {"l", 0x14, "· — · ·"},
    {"m", 0x07, "— —"},
    {"n", 0x06, "— ·"},
    {"o", 0x0f, "— — —"},
    {"p", 0x16, "· — — ·"},
    {"q", 0x1d, "— — · —"},
    {"r", 0x0a, "· — ·"},
    {"s", 0x08, "· · ·"},
    {"t", 0x03, "—"},
    {"u", 0x09, "· · —"},
    {"v", 0x11, "· · · —"},
    {"w", 0x0b, "· — —"},
    {"x", 0x1d, "— · · —"},
    {"y", 0x1b, "— · — —"},
    {"z", 0x1c, "— — · ·"},
    {"[", 0x00, "na"},
    {"\\",0x00, "na"},
    {"]", 0x00, "na"},
    {"^", 0x00, "na"},
    {"_", 0x4d, "· · — — · —"},

    //Prosigns, no interletter spacing http://en.wikipedia.org/wiki/Prosigns_for_Morse_Code
    {"(ER)", 0x100, "· · · · · · · ·"}, //Error
    {"(SN)", 0x22, "· · · - · "}, //Understood
    {"(AA)", 0x15, "· - · -"}, //AA CR/LF
    {"(BT)", 0x31, "- · · · -"}, //2 LF or new paragraph
    {"(CL)", 0x1a4, "- · - · · - · ·"}, //Going off air
    {"(CT)", 0x35, "- · - · -"}, //Start copying (same as KA)
    {"(DT)", 0x67, "- · · - - -"}, //Shift to Wabun code
    {"(KN)", 0x36, "- · - - ·"}, //Invitation to a specific station to transmit, compared to K which is general
    {"(SOS)",0x238, "· · · - - - · · ·"},
    {"(BK)", 0xc5, "- . . . - . -"}, //Break (Bk)
    {"(AR)", 0x2a, ". - . - ."}, //End of Message (AR), sometimes shown as '+'
    {"(SK)", 0x45, ". . . - . -"}, //End of QSO (SK) or End of Work
    {"(AS)", 0x28, ". - . . ."}, // Please Wait (AS)

    //Special abreviations http://en.wikipedia.org/wiki/Morse_code_abbreviations
    //Supposed to have inter-letter spacing, but often sent as single 'letter'
    //Add other abrevaitions later

    //Non English extensions (TBD from Wikipedia)



};

//Converts morse as binary (high order bit always 1 followed by 0 for dot and 1 for dash) to ascii
const char * Morse::MorseToAscii(quint16 morse)
{
    //Non optimized
    for (int i=0; i<sizeof(morseAsciiOrder); i++) {
        if (morseAsciiOrder[i].binaryValue == morse)
            return morseAsciiOrder[i].ascii;
    }
    return "*"; //error

}
const char * Morse::MorseToDotDash(quint16 morse)
{
    //Non optimized
    for (int i=0; i<sizeof(morseAsciiOrder); i++) {
        if (morseAsciiOrder[i].binaryValue == morse)
            return morseAsciiOrder[i].dotDash;
    }
    return "*"; //Error
}

/*
  Abbreviations, assume letter space between char or not?
AA	All after (used after question mark to request a repetition)
AB	All before (similarly)
ARRL	American Radio Relay League
ABT	About
ADR	Address
AGN	Again
ANT	Antenna
ARND	Around
BCI	Broadcast interference
BK	Break (to pause transmission of a message, say)
BN	All between
BTR	Better
BUG	Semiautomatic mechanical key
BURO	Bureau (usually used in the phrase PLS QSL VIA BURO, "Please send QSL card via my local/national QSL bureau")
B4	Before
C	Yes; correct
CBA	Callbook address
CFM	Confirm
CK	Check
CL	Clear (I am closing my station)
CLG	Calling
CQ	Calling any station
CQD	Original International Distress Call, fell out of use before 1915
CS	Callsign
CTL	Control
CUD	Could
CUL	See you later
CUZ	Because
CW	Continuous wave (i.e., radiotelegraph)
CX	Conditions
DE	From (or "this is")
DN	Down
DR	Dear
DSW	Goodbye (Russian: до свидания [Do svidanya])
DX	Distance (sometimes refers to long distance contact), foreign countries
EMRG	Emergency
ENUF	Enough
ES	And
FB	Fine business (Analogous to "OK")
FCC	Federal Communications Commission
FER	For
FM	From
FREQ	Frequency
FWD	Forward
GA	Good afternoon or Go ahead (depending on context)
GE	Good evening
GG	Going
GL	Good luck
GM	Good morning
GN	Good night
GND	Ground (ground potential)
GUD	Good
GX	Ground
HEE	Humour intended (often repeated, e.g. HEE HEE)
HI	Humour intended (possibly derived from HEE)
HR	Here, hear
HV	Have
HW	How
II	I say again
IMP	Impedance
K	Over
KN	Over; only the station named should respond (e.g. W7PTH DE W1AW KN)
LID	Poor operator
MILS	Milliamperes
MNI	Many
MSG	Message
N	No; nine
NIL	Nothing
NM	Name
NR	Number
NW	Now
NX	Noise; noisy
OB	Old boy
OC	Old chap
OM	Old man (any male amateur radio operator is an OM regardless of age)
OO	Official observer
OP	Operator
OT	Old timer
OTC	Old timers club (ARRL-sponsored organization for radio amateurs first licensed 20 or more years ago)
OOTC	Old old timers club (organization for those whose first two-way radio contact occurred 40 or more years ago; separate from OTC and ARRL)
PLS	Please
PSE	Please
PWR	Power
PX	Prefix
QCWA	Quarter Century Wireless Association (organization for radio amateurs who have been licensed 25 or more years)
R	Are; received as transmitted (origin of "Roger"), or decimal point (depending on context)
RCVR	Receiver (radio)
RFI	Radio Frequency Interference
RIG	Radio apparatus
RPT	Repeat or report (depending on context)
RPRT	Report
RST	Signal report format (Readability-Signal Strength-Tone)
RTTY	Radioteletype
RX	Receiver, radio
SAE	Self-addressed envelope
SASE	Self-addressed, stamped envelope
SED	Said
SEZ	Says
SFR	So far (proword)
SIG	Signal or signature
SIGS	Signals
SK	Out (proword), end of contact
SK	Silent Key (a deceased radio amateur)
SKED	Schedule
SMS	Short message service
SN	Soon
SNR	Signal-to-noise ratio
SRI	Sorry
SSB	Single sideband
STN	Station
T	Zero (usually an elongated dah)
TEMP	Temperature
TFC	Traffic
TKS	Thanks
TMW	Tomorrow
TNX	Thanks
TT	That
TU	Thank you
TVI	Television interference
TX	Transmit, transmitter
TXT	Text
U	You
UR	Your or You're (depending on context)
URS	Yours
VX	Voice; phone
VY	Very
W	Watts
WA	Word after
WB	Word before
WC	Wilco
WDS	Words
WID	With
WKD	Worked
WKG	Working
WL	Will
WUD	Would
WTC	Whats the craic? (Irish Language: [Conas atá tú?])
WX	Weather
XCVR	Transceiver
XMTR	Transmitter
XYL	Wife (ex-YL)
YF	Wife
YL	Young lady (originally an unmarried female operator, now used for any female)
ZX	Zero beat
73	Best regards
88	Love and kisses
[edit]

*/

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
