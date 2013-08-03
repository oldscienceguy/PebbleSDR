#include "morse.h"
#include "QPainter"
#include "QDebug"
#include "QEvent"
#include "Receiver.h"
#include "testbench.h"

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

    Another good reference on tone detection
    Tone Detection with a Quadrature Receiver
    James E. Gilley
    http://www.efjohnsontechnologies.com/resources/dyn/files/75829/_fn/analog_tone_detection.pdf

*/

//Fldigi and earlier code
/* ---------------------------------------------------------------------- */


/*
 * Morse code characters table.  This table allows lookup of the Morse
 * shape of a given alphanumeric character.  Shapes are held as a string,
 * with '-' representing dash, and '.' representing dot.  The table ends
 * with a NULL entry.
 *
 * This is the main table from which the other tables are computed.
 *
 * The Prosigns are also defined in the configuration.h file
 * The user can specify the character which substitutes for the prosign
 */

static CW_TABLE cw_table[] = {
    /* Prosigns characters can be redefined in progdefaults.CW_prosigns */
    {'=',	"<BT>",   "-...-"	}, // 0 2 LF or new paragraph
    {'~',	"<AA>",   ".-.-"	}, // 1 CR/LF
    {'<',	"<AS>",   ".-..."	}, // 2 Please Wait
    {'>',	"<AR>",   ".-.-."	}, // 3 End of Message, sometimes shown as '+'
    {'%',	"<SK>",   "...-.-"	}, // 4 End of QSO or End of Work
    {'+',	"<KN>",   "-.--."	}, // 5 Invitation to a specific station to transmit, compared to K which is general
    {'&',	"<INT>",  "..-.-"	}, // 6
    {'{',	"<HM>",   "....--"	}, // 7
    {'}',	"<VE>",   "...-."	}, // 8
    /*
     * Additional prosigns from earlier Pebble that aren't integrated yet
        //Prosigns, no interletter spacing http://en.wikipedia.org/wiki/Prosigns_for_Morse_Code
        //Special abreviations http://en.wikipedia.org/wiki/Morse_code_abbreviations
        //Supposed to have inter-letter spacing, but often sent as single 'letter'
        //Non English extensions (TBD from Wikipedia)
        {"(ER)", 0x100, "· · · · · · · ·"}, //Error
        {"(SN)", 0x22, "· · · - · "}, //Understood
        {"(CL)", 0x1a4, "- · - · · - · ·"}, //Going off air
        {"(CT)", 0x35, "- · - · -"}, //Start copying (same as KA)
        {"(DT)", 0x67, "- · · - - -"}, //Shift to Wabun code
        {"(SOS)",0x238, "· · · - - - · · ·"},
        {"(BK)", 0xc5, "- . . . - . -"}, //Break (Bk)

     */

    /* ASCII 7bit letters */
    {'A',	"A",	".-"	},
    {'B',	"B",	"-..."	},
    {'C',	"C",	"-.-."	},
    {'D',	"D",	"-.."	},
    {'E',	"E",	"."		},
    {'F',	"F",	"..-."	},
    {'G',	"G",	"--."	},
    {'H',	"H",	"...."	},
    {'I',	"I",	".."	},
    {'J',	"J",	".---"	},
    {'K',	"K",	"-.-"	},
    {'L',	"L",	".-.."	},
    {'M',	"M",	"--"	},
    {'N',	"N",	"-."	},
    {'O',	"O",	"---"	},
    {'P',	"P",	".--."	},
    {'Q',	"Q",	"--.-"	},
    {'R',	"R",	".-."	},
    {'S',	"S",	"..."	},
    {'T',	"T",	"-"		},
    {'U',	"U",	"..-"	},
    {'V',	"V",	"...-"	},
    {'W',	"W",	".--"	},
    {'X',	"X",	"-..-"	},
    {'Y',	"Y",	"-.--"	},
    {'Z',	"Z",	"--.."	},
    /* Numerals */
    {'0',	"0",	"-----"	},
    {'1',	"1",	".----"	},
    {'2',	"2",	"..---"	},
    {'3',	"3",	"...--"	},
    {'4',	"4",	"....-"	},
    {'5',	"5",	"....."	},
    {'6',	"6",	"-...."	},
    {'7',	"7",	"--..."	},
    {'8',	"8",	"---.."	},
    {'9',	"9",	"----."	},
    /* Punctuation */
    {'\\',	"\\",	".-..-."	},
    {'\'',	"'",	".----."	},
    {'$',	"$",	"...-..-"	},
    {'(',	"(",	"-.--."		},
    {')',	")",	"-.--.-"	},
    {',',	",",	"--..--"	},
    {'-',	"-",	"-....-"	},
    {'.',	".",	".-.-.-"	},
    {'/',	"/",	"-..-."		},
    {':',	":",	"---..."	},
    {';',	";",	"-.-.-."	},
    {'?',	"?",	"..--.."	},
    {'_',	"_",	"..--.-"	},
    {'@',	"@",	".--.-."	},
    {'!',	"!",	"-.-.--"	},
    {0, NULL, NULL}
};

// ISO 8859-1 accented characters
//	{"\334\374",	"\334\374",	"..--"},	// U diaeresis
//	{"\304\344",	"\304\344",	".-.-"},	// A diaeresis
//	{"\307\347",	"\307\347",	"-.-.."},	// C cedilla
//	{"\326\366",	"\325\366",	"---."},	// O diaeresis
//	{"\311\351",	"\311\351",	"..-.."},	// E acute
//	{"\310\350",	"\310\350",".-..-"},	// E grave
//	{"\305\345",	"\305\345",	".--.-"},	// A ring
//	{"\321\361",	"\321\361",	"--.--"},	// N tilde
//	ISO 8859-2 accented characters
//	{"\252",		"\252",		"----" },	// S cedilla
//	{"\256",		"\256",		"--..-"},	// Z dot above


/**
 * cw_tokenize_representation()
 *
 * Return a token value, in the range 2-255, for a lookup table representation.
 * The routine returns 0 if no valid token could be made from the string.  To
 * avoid casting the value a lot in the caller (we want to use it as an array
 * index), we actually return an unsigned int.
 *
 * This token algorithm is designed ONLY for valid CW representations; that is,
 * strings composed of only '.' and '-', and in this case, strings shorter than
 * eight characters.  The algorithm simply turns the representation into a
 * 'bitmask', based on occurrences of '.' and '-'.  The first bit set in the
 * mask indicates the start of data (hence the 7-character limit).  This mask
 * is viewable as an integer in the range 2 (".") to 255 ("-------"), and can
 * be used as an index into a fast lookup array.
 */
unsigned int MorseCode::tokenize_representation(const char *representation)
{
    unsigned int token;	/* Return token value */
    const char *sptr;	/* Pointer through string */

    /*
     * Our algorithm can handle only 6 characters of representation.
     * And we insist on there being at least one character, too.
     */
    if (strlen(representation) > 6 || strlen(representation) < 1)
        return 0;

    /*
     * Build up the token value based on the dots and dashes.  Start the
     * token at 1 - the sentinel (start) bit.
     */
    for (sptr = representation, token = 1; *sptr != 0; sptr++) {
        /*
         * Left-shift the sentinel (start) bit.
         */
        token <<= 1;

        /*
         * If the next element is a dash, OR in another bit.  If it is
         * not a dash or a dot, then there is an error in the repres-
         * entation string.
         */
        if (*sptr == CW_DASH_REPRESENTATION)
            token |= 1;
        else if (*sptr != CW_DOT_REPRESENTATION)
            return 0;
    }

    /* Return the value resulting from our tokenization of the string. */
    return token;
}

/* ---------------------------------------------------------------------- */

void MorseCode::init()
{
    CW_TABLE *cw;	/* Pointer to table entry */
    unsigned int i;
    long code;
    int len;
// Update the char / prosign relationship
    if (progdefaults.CW_prosigns.length() == 9) {
        for (int i = 0; i < 9; i++) {
            cw_table[i].chr = progdefaults.CW_prosigns[i];
        }
    }
// Clear the RX & TX tables
    for (i = 0; i < MorseTableSize; i++) {
        cw_tx_lookup[i].code = 0x04;
        cw_tx_lookup[i].prt = 0;
        cw_rx_lookup[i] = 0;
    }
// For each main table entry, create a token entry.
    for (cw = cw_table; cw->chr != 0; cw++) {
        if ((cw->chr == '(') && !progdefaults.CW_use_paren) continue;
        if ((cw->chr == '<') && progdefaults.CW_use_paren) continue;
        i = tokenize_representation(cw->rpr);
        if (i != 0)
            cw_rx_lookup[i] = cw;
    }
// Build TX table
    for (cw = cw_table; cw->chr != 0; cw++) {
        if ((cw->chr == '(') && !progdefaults.CW_use_paren) continue;
        if ((cw->chr == '<') && progdefaults.CW_use_paren) continue;
        len = strlen(cw->rpr);
        code = 0x04;
        while (len-- > 0) {
            if (cw->rpr[len] == CW_DASH_REPRESENTATION) {
                code = (code << 1) | 1;
                code = (code << 1) | 1;
                code = (code << 1) | 1;
            } else
                code = (code << 1) | 1;
            code <<= 1;
        }
        cw_tx_lookup[(int)cw->chr].code = code;
        cw_tx_lookup[(int)cw->chr].prt = cw->prt;
    }
}

const char *MorseCode::rx_lookup(char *r)
{
    int			token;
    CW_TABLE *cw;

    if ((token = tokenize_representation(r)) == 0)
        return NULL;

    if ((cw = cw_rx_lookup[token]) == NULL)
        return NULL;

    return cw->prt;
}

const char *MorseCode::tx_print(int c)
{
    if (cw_tx_lookup[toupper(c)].prt)
        return cw_tx_lookup[toupper(c)].prt;
    else
        return "";
}

unsigned long MorseCode::tx_lookup(int c)
{
    return cw_tx_lookup[toupper(c)].code;
}


//Original Pebble lookup

//Relative lengths of elements in Tcw terms
#define DOT_TCW 1
#define DASH_TCW 3
#define CHARSPACE_TCW 3
#define ELEMENTSPACE_TCW 1
#define WORDSPACE_TCW 7

#define MIN_DOT_COUNT 4

Morse::Morse(int sr, int fc) : SignalProcessing(sr,fc)
{
    morseCode.init();
    clrRepBuf();

    workingBuf = new CPXBuf(numSamples);

    //Setup Goertzel
    cwGoertzel = new Goertzel(sr, fc);
    //modemSampleRate = 8000;
    modemSampleRate = CWSampleRate;
    modemFrequency = global->settings->modeOffset;
    //Sample rate coming in is in sampleRate
    //modemSampleRate is typically 8k
    modemDecimateFactor = sampleRate / modemSampleRate;
    qDebug()<<"ModemDecimate = "<<modemDecimateFactor;

    //Start with fixed WPM for now
    fixedWPM = true;
    maxWPM = 60;
    wpm = 30;
    //binwidth for maxWPM = 60 is 200hz
    //User should set maxWPM and we should calculate binWidth, sharper = more accurate
    toneBufSize = cwGoertzel->SetFreqHz(modemFrequency, 200, modemSampleRate);

    //Max length for mark and space
    maxMarkCount = 2000 / cwGoertzel->timePerBin;
    maxSpaceCount = 5000 / cwGoertzel->timePerBin;

    //Number of goertzel results we need for a reliable tcw
    //numResultsPerTcw = WpmToTcw(wpm) / cwGoertzel->timePerBin;

    //Start with min 4 results per dot/tcw
    SetElementLengths(MIN_DOT_COUNT);

    powerBufSize = numSamples / modemDecimateFactor;
    powerBuf = new float[powerBufSize];
    toneBuf = new bool[toneBufSize];
    toneBufCounter = 0;

    outString = "";

    countingState = NOT_COUNTING;
    markCount = 0;
    spaceCount = 0;
    countsPerDashThreshold = MIN_DOT_COUNT;

    dataUi = NULL;

    //fldigi constructors
    squelchIncrement = 0.5;
    squelchValue = 2.0;
    sqlonoff = true;

    freqlock = false;
    usedefaultWPM = false;

    cw_speed  = progdefaults.CWspeed; //!!Whats the difference between speed, default speed and receive speed?
    cw_default_speed = cw_speed;
    cw_receive_speed = cw_speed;

    cw_adaptive_receive_threshold = 2 * DOT_MAGIC / cw_default_speed;
    cw_noise_spike_threshold = cw_adaptive_receive_threshold / 4;
    cw_default_dot_length = DOT_MAGIC / cw_default_speed;
    cw_default_dash_length = 3 * cw_default_dot_length;

    memset(rx_rep_buf, 0, sizeof(rx_rep_buf));

    cwTrack = true;
    phaseacc = 0.0;
    FFTphase = 0.0;
    FIRphase = 0.0;
    FFTvalue = 0.0;
    FIRvalue = 0.0;

    //Upper and lower don't seem to be used, just progdefaults.CWUpper
    upper_threshold = progdefaults.CWupper;
    lower_threshold = progdefaults.CWlower;

    agc_peak = 1.0;

    use_fft_filter = progdefaults.CWuse_fft_filter;

    hilbert = new C_FIR_filter(); // hilbert transform used by FFT filter
    hilbert->init_hilbert(37, 1);

    cw_FIR_filter = new C_FIR_filter();
    //Filter is at modem sample rate and self decimates with DEC_RATIO in LP filter
    double f = progdefaults.CWspeed/(1.2 * modemSampleRate);
    cw_FIR_filter->init_lowpass (CW_FIRLEN, DEC_RATIO, f);

    //overlap and add filter length should be a factor of 2
    // low pass implementation
    FilterFFTLen = 4096;
    //!!cw_FFT_filter = new fftfilt(progdefaults.CWspeed/(1.2 * modemSampleRate), FilterFFTLen);

    // bit filter based on 10 msec rise time of CW waveform
    int bfv = (int)(modemSampleRate * .010 / DEC_RATIO); //8k*.01/16 = 5
    bitfilter = new Cmovavg(bfv);

    trackingfilter = new Cmovavg(TRACKING_FILTER_SIZE);

    sync_parameters();

    modemDownConvert.SetDataRate(sampleRate,modemSampleRate);
    modemDownConvert.SetFrequency(modemFrequency);

    cwMode = dmCWL;

    //Used to update display from main thread.
    connect(this, SIGNAL(newOutput()), this, SLOT(refreshOutput()));

    outputOn = true;

}


Morse::~Morse()
{
    if (workingBuf != NULL) delete workingBuf;
    if (cwGoertzel != NULL) delete cwGoertzel;
    if (powerBuf != NULL) free (powerBuf);
    if (toneBuf != NULL) free (toneBuf);

    if (hilbert) delete hilbert;
    if (cw_FIR_filter) delete cw_FIR_filter;
    //if (cw_FFT_filter) delete cw_FFT_filter;
    if (bitfilter) delete bitfilter;
    if (trackingfilter) delete trackingfilter;


}

void Morse::SetupDataUi(QWidget *parent)
{
    if (parent == NULL) {
        //We want to delete
        if (dataUi != NULL) {
            delete dataUi;
        }
        dataUi = NULL;
        return;
    } else if (dataUi == NULL) {
        //Create new one
        dataUi = new Ui::dataMorse();
        dataUi->setupUi(parent);

        dataUi->dataBar->setMin(0);
        dataUi->dataBar->setMax(10);

        dataUi->squelchSlider->setMinimum(0);
        dataUi->squelchSlider->setMaximum(10 / squelchIncrement);
        dataUi->squelchSlider->setSingleStep(1);
        dataUi->squelchSlider->setValue(squelchValue);
        connect(dataUi->squelchSlider,SIGNAL(valueChanged(int)),this,SLOT(squelchChanged(int)));

        dataUi->dataEdit->setAutoFormatting(QTextEdit::AutoNone);
        dataUi->dataEdit->setAcceptRichText(false);
        dataUi->dataEdit->setReadOnly(true);
        dataUi->dataEdit->setUndoRedoEnabled(false);
        dataUi->dataEdit->setWordWrapMode(QTextOption::WordWrap);

        connect(dataUi->resetButton,SIGNAL(clicked()),this,SLOT(resetOutput()));
        dataUi->onBox->setChecked(true);
        connect(dataUi->onBox,SIGNAL(toggled(bool)), this, SLOT(onBoxChecked(bool)));
    }

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

void Morse::setCWMode(DEMODMODE m)
{
    cwMode = m;
}

void Morse::onBoxChecked(bool b)
{
    outputOn = b;
}

void Morse::resetOutput()
{
    dataUi->dataEdit->clear();
    rx_init(); //Reset all data
}

//Handles newOuput signal
void Morse::refreshOutput()
{
    if (!outputOn)
        return;

    outputBufMutex.lock();
    dataUi->wpmLabel->setText(QString().sprintf("%d WPM",cw_receive_speed));

    for (int i=0; i<outputBufIndex; i++) {
        dataUi->dataEdit->insertPlainText(QString(outputBuf[i])); //At cursor
        dataUi->dataEdit->moveCursor(QTextCursor::End);
    }
    outputBufIndex = 0;
    outputBufMutex.unlock();
}

void Morse::OutputData(const char* d)
{
    if (dataUi == NULL)
        return;

    dataUi->dataEdit->insertPlainText(d); //At cursor
    dataUi->dataEdit->moveCursor(QTextCursor::End);
}
void Morse::OutputData(const char c)
{
    if (dataUi == NULL)
        return;

    //Display can be accessing at same time, so we need to lock
    outputBufMutex.lock();
    if (outputBufIndex < sizeof(outputBuf)) {
        outputBuf[outputBufIndex] = c;
        outputBufIndex++;
    }
    outputBufMutex.unlock();
    emit newOutput();

    return;
}

void Morse::squelchChanged(int v)
{
    if (v == 0) {
        sqlonoff = false;
    } else {
        squelchValue = v * squelchIncrement; //Slider is in .5 increments so 2 inc = 1 squelch value
        sqlonoff = true;
    }
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
            clrRepBuf();
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
                    if (element == 0)
                        rx_rep_buf[cw_rr_current++] = CW_DOT_REPRESENTATION;
                    else
                        rx_rep_buf[cw_rr_current++] = CW_DASH_REPRESENTATION;

                    pendingChar = true;
                    pendingElement = false;
                    pendingWord = false;
                }
            }
            if (lastSpaceCount >= countsPerCharSpace) {
                //Process char
                if (pendingChar) {
                    OutputData(morseCode.rx_lookup(rx_rep_buf));
                    clrRepBuf();
                    pendingChar = false;
                    pendingWord = true;
                }
            }
            if (lastSpaceCount >= countsPerWordSpace) {
                //Process word
                if (pendingWord) {
                    OutputData(" ");
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
    return ProcessBlockFldigi(in);

    return ProcessBlockSuperRatt(in);

    bool res;
    bool isMark;
    float db;
    float power;

    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
    for (int i=0, j=0; i<numSamples; i=i+modemDecimateFactor, j++) {
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
                            OutputData("-");

                        } else {
                            OutputData(".");
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


/*
Working on understanding and implementing FLDIGI Algorithms
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
void Morse::reset_rx_filter()
{
    //If anything had changed from the defaults we started with, rest
    //if (use_fft_filter != progdefaults.CWuse_fft_filter ||
    if(	cw_speed != progdefaults.CWspeed ) {

        use_fft_filter = progdefaults.CWuse_fft_filter;
        cw_speed = cw_default_speed = progdefaults.CWspeed;

        //if (use_fft_filter) { // FFT filter
        //	cw_FFT_filter->create_lpf(progdefaults.CWspeed/(1.2 * modemSampleRate));
        //	FFTphase = 0;
        //} else { // FIR filter
            cw_FIR_filter->init_lowpass (CW_FIRLEN, DEC_RATIO , progdefaults.CWspeed/(1.2 * modemSampleRate));
            FIRphase = 0;
        //}

        cw_adaptive_receive_threshold = 2 * DOT_MAGIC / cw_speed;
        cw_noise_spike_threshold = cw_adaptive_receive_threshold / 4;
        cw_default_dot_length = DOT_MAGIC / cw_default_speed;
        cw_default_dash_length = 3 * cw_default_dot_length;

        phaseacc = 0.0;
        FFTphase = 0.0;
        FIRphase = 0.0;
        FFTvalue = 0.0;
        FIRvalue = 0.0;

        clrRepBuf();

        agc_peak = 0;
    }
#if 0
    if (lower_threshold != progdefaults.CWlower ||
        upper_threshold != progdefaults.CWupper) {
        lower_threshold = progdefaults.CWlower;
        upper_threshold = progdefaults.CWupper;
        clear_syncscope();
    }
#endif
}

//Not used in fldigi, but there for CPX samples
CPX Morse::mixer(CPX in)
{
    CPX z (cos(phaseacc), sin(phaseacc));
    z = z * in;

    phaseacc += TWOPI * modemFrequency / modemSampleRate;
    if (phaseacc > M_PI)
        phaseacc -= TWOPI;
    else if (phaseacc < M_PI)
        phaseacc += TWOPI;

    return z;
}

//My version of FIRprocess + rx_processing for Pebble
CPX * Morse::ProcessBlockFldigi(CPX *in)
{

    CPX z;

    reset_rx_filter();

    //Downconverter first mixes in place, ie changes in!  So we have to work with a copy
    CPXBuf::copy(workingBuf->Ptr(),in,numSamples);

    //We need to account for modemOffset in ReceiverWidget
    //Actual freq for CWU will be freq + modemFrequency for CWL will be freq -modemFrequency.
    //And we want actual freq to be at baseband
    if (cwMode == dmCWL)
        modemDownConvert.SetFrequency(-modemFrequency);
    else if (cwMode == dmCWU)
        modemDownConvert.SetFrequency(modemFrequency);

    int numModemSamples = modemDownConvert.ProcessData(numSamples, workingBuf->Ptr(), this->out);
    //Now at 8k modem rate
    //Verify that testbench post banpass signal looks the same, just at 8k sample rate
    global->testBench->DisplayData(numModemSamples,this->out, modemSampleRate, PROFILE_5);

    for (int i = 0; i<numModemSamples; i++) {
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
        z = out[i] * 1.95; //A little gain - same as fldigi

        //cw_FIR_filter does MAC and returns 1 result every DEC_RATIO samples
        //LowPass filter establishes the detection bandwidth (freq)
        //and Detection time (DEC_RATIO)
        //If the LP filter passes the tone, we have detection
        //We need some sort of AGC here to get consistent tone thresholds
        if (cw_FIR_filter->run ( z, z )) {

            //smpl_ctr keeps track of uSecs
            //Since cw_FIR_filter only returns every DEC_RATIO (16) samples, smpl_ctr increases in DEC_RATIO increments
            smpl_ctr += DEC_RATIO;

            // demodulate and get power in signal
            FIRvalue = z.mag(); //sqrt sum of squares
            //or
            //FIRvalue = z.sqrMag(); //just sum of squares

            //Moving average to handle jitter during CW rise and fall periods
            FIRvalue = bitfilter->run(FIRvalue);
            //Main logic for timing and character decoding
            decode_stream(FIRvalue);
        }


        //Its possible we get called right after dataUi has been been instantiated
        //since receive loop is running when CW window is opened
        if (dataUi != NULL && dataUi->dataBar!=NULL)
            dataUi->dataBar->setValue(metric); //Tuning bar
    }

    return in;
}

void Morse::rx_init()
{
    cw_receive_state = RS_IDLE;
    clrRepBuf();
    agc_peak = 0;

    usedefaultWPM = false;
}

void Morse::init()
{
    //We don't have separate waterfall like fldigi does, remove waterfall code everywhere
    trackingfilter->reset();
    cw_adaptive_receive_threshold = (long int)trackingfilter->run(2 * cw_default_dot_length);

    rx_init();
    use_paren = progdefaults.CW_use_paren;
    prosigns = progdefaults.CW_prosigns;
    stopflag = false;
}

//Called with value indicating level of tone detected
void Morse::decode_stream(double value)
{
    const char *c, *somc;
    char *cptr;

    // Compute a variable threshold value for tone detection
    // Fast attack and slow decay.
    if (value > agc_peak)
        agc_peak = decayavg(agc_peak, value, 20);
    else
        agc_peak = decayavg(agc_peak, value, 800);

    //metric is what we use to determine rising or falling status
    //Also what could be displayed on 0-100 tuning bar
    metric = clamp(agc_peak * 2e3 , 0.0, 100.0);
    //metric = clamp(agc_peak * 1000 , 0.0, 100.0); //JSPR uses 1000, where does # come from?

    global->testBench->SendDebugTxt(QString().sprintf("V: %f A: %f, M: %f",value, agc_peak, metric));

    // save correlation amplitude value for the sync scope
    // normalize if possible
    if (agc_peak)
        value /= agc_peak;
    else
        value = 0;

    //qDebug()<<"CW Value "<<value;

    //Check squelch to avoid noise errors
    if (!sqlonoff || metric > squelchValue ) {
        // Power detection using hysterisis detector
        // upward trend means tone starting
        if ((value > progdefaults.CWupper) && (cw_receive_state != RS_IN_TONE)) {
            FldigiProcessEvent(CW_KEYDOWN_EVENT, NULL);
        }
        // downward trend means tone stopping
        if ((value < progdefaults.CWlower) && (cw_receive_state == RS_IN_TONE)) {
            FldigiProcessEvent(CW_KEYUP_EVENT, NULL);
        }
    }

    //Is there a character ready to ouput
    if (FldigiProcessEvent(CW_QUERY_EVENT, &c) == CW_SUCCESS) {
        //Output character(s)
        if (strlen(c) == 1) {
            OutputData(progdefaults.rx_lowercase ? tolower(*c) : *c);
        } else while (*c) {
            OutputData(progdefaults.rx_lowercase ? tolower(*c++) : *c++);
        }
    }

}

// Compare two timestamps, and return the difference between them in usecs.

int Morse::usec_diff(unsigned int earlier, unsigned int later)
{
    // Compare the timestamps.
    // At 4 WPM, the dash length is 3*(1200000/4)=900,000 usecs, and
    // the word gap is 2,100,000 usecs.
    if (earlier >= later) {
        return 0;
    } else {
        int usec = (int) (((double) (later - earlier) * USECS_PER_SEC) / modemSampleRate);
        return usec;
    }
}

//=======================================================================
// cw_update_tracking()
// This gets called everytime we have a dot dash sequence or a dash dot
// sequence. Since we have semi validated tone durations, we can try and
// track the cw speed by adjusting the cw_adaptive_receive_threshold variable.
// This is done with moving average filters for both dot & dash.
//=======================================================================
void Morse::update_tracking(int idot, int idash)
{
    int dot, dash;
    if (idot > cw_lower_limit && idot < cw_upper_limit)
        dot = idot;
    else
        dot = cw_default_dot_length;
    if (idash > cw_lower_limit && idash < cw_upper_limit)
        dash = idash;
    else
        dash = cw_default_dash_length;

    cw_adaptive_receive_threshold = (long int)trackingfilter->run((dash + dot) / 2);

    sync_parameters();
}

// sync_parameters()
// Synchronize the dot, dash, end of element, end of character, and end
// of word timings and ranges to new values of Morse speed, or receive tolerance.
void Morse::sync_parameters()
{
    int lowerwpm, upperwpm;

    cw_default_dot_length = DOT_MAGIC / progdefaults.CWspeed;
    cw_default_dash_length = 3 * cw_default_dot_length;


    // check if user changed the tracking or the cw default speed
    if (cwTrack != progdefaults.CWtrack ||
        (cw_default_speed != progdefaults.CWspeed)) {
        trackingfilter->reset();
        cw_adaptive_receive_threshold = 2 * cw_default_dot_length;
    }
    cwTrack = progdefaults.CWtrack;
    cw_default_speed = progdefaults.CWspeed;

    // Receive parameters:
    lowerwpm = cw_default_speed - progdefaults.CWrange;
    upperwpm = cw_default_speed + progdefaults.CWrange;
    if (lowerwpm < progdefaults.CWlowerlimit)
        lowerwpm = progdefaults.CWlowerlimit;
    if (upperwpm > progdefaults.CWupperlimit)
        upperwpm = progdefaults.CWupperlimit;
    cw_lower_limit = 2 * DOT_MAGIC / upperwpm;
    cw_upper_limit = 2 * DOT_MAGIC / lowerwpm;

    if (cwTrack)
        cw_receive_speed = DOT_MAGIC / (cw_adaptive_receive_threshold / 2);
    else {
        cw_receive_speed = cw_default_speed;
        cw_adaptive_receive_threshold = 2 * cw_default_dot_length;
    }

    if (cw_receive_speed > 0)
        cw_receive_dot_length = DOT_MAGIC / cw_receive_speed;
    else
        cw_receive_dot_length = DOT_MAGIC / 5;

    cw_receive_dash_length = 3 * cw_receive_dot_length;

    cw_noise_spike_threshold = cw_receive_dot_length / 2;

}

//All the things we need to do to start a new char
void Morse::clrRepBuf()
{
    memset(rx_rep_buf, 0, sizeof(rx_rep_buf));
    cw_rr_current = 0; //Index to rx_rep_buf
    smpl_ctr = 0; //Start timer over
}

bool Morse::FldigiProcessEvent(CW_EVENT event, const char **c)
{
    static int space_sent = true;	// for word space logic
    static int last_element = 0;	// length of last dot/dash
    int element_usec;		// Time difference in usecs

    switch (event) {
        //Reset everything and start over
        case CW_RESET_EVENT:
            sync_parameters();
            cw_receive_state = RS_IDLE;
            clrRepBuf();
            break;

            break;
        //Tone filter output is trending up
        case CW_KEYDOWN_EVENT:
            if (cw_receive_state == RS_IN_TONE) {
                //We can't be processing a tone and also get a keydown event
                return false;
            }
            // first tone in idle state reset audio sample counter
            if (cw_receive_state == RS_IDLE) {
                clrRepBuf();
            }

            //Continue counting tone duration
            // save the timestamp
            cw_rr_start_timestamp = smpl_ctr;
            // Set state to indicate we are inside a tone.
            old_cw_receive_state = cw_receive_state;
            cw_receive_state = RS_IN_TONE;
            //!!What does CW_ERROR mean, no char?
            return CW_ERROR;

            break;
        //Tone filter output is trending down
        case CW_KEYUP_EVENT:
            // The receive state is expected to be inside a tone.
            if (cw_receive_state != RS_IN_TONE)
                return CW_ERROR;
            // Save the current timestamp so we can see how long tone was
            cw_rr_end_timestamp = smpl_ctr;
            element_usec = usec_diff(cw_rr_start_timestamp, cw_rr_end_timestamp);

            // make sure our timing values are up to date
            sync_parameters();

            // If the tone length is shorter than any noise cancelling
            // threshold that has been set, then ignore this tone.
            if (cw_noise_spike_threshold > 0
                && element_usec < cw_noise_spike_threshold) {
                cw_receive_state = RS_IDLE;
                return CW_ERROR;
            }

            // Set up to track speed on dot-dash or dash-dot pairs for this test to work, we need a dot dash pair or a
            // dash dot pair to validate timing from and force the speed tracking in the right direction. This method
            // is fundamentally different than the method in the unix cw project. Great ideas come from staring at the
            // screen long enough!. Its kind of simple really ... when you have no idea how fast or slow the cw is...
            // the only way to get a threshold is by having both code elements and setting the threshold between them
            // knowing that one is supposed to be 3 times longer than the other. with straight key code... this gets
            // quite variable, but with most faster cw sent with electronic keyers, this is one relationship that is
            // quite reliable. Lawrence Glaister (ve7it@shaw.ca)
            if (last_element > 0) {
                // check for dot dash sequence (current should be 3 x last)
                if ((element_usec > 2 * last_element) &&
                    (element_usec < 4 * last_element)) {
                    update_tracking(last_element, element_usec);
                }
                // check for dash dot sequence (last should be 3 x current)
                if ((last_element > 2 * element_usec) &&
                    (last_element < 4 * element_usec)) {
                    update_tracking(element_usec, last_element);
                }
            }
            last_element = element_usec;

            // ok... do we have a dit or a dah?
            // a dot is anything shorter than 2 dot times
            if (element_usec <= cw_adaptive_receive_threshold) {
                rx_rep_buf[cw_rr_current++] = CW_DOT_REPRESENTATION;
                //		printf("%d dit ", last_element/1000);  // print dot length
            } else {
                // a dash is anything longer than 2 dot times
                rx_rep_buf[cw_rr_current++] = CW_DASH_REPRESENTATION;
            }

            // We just added a representation to the receive buffer.
            // If it's full, then reset everything as it probably noise
            if (cw_rr_current == RECEIVE_CAPACITY - 1) {
                cw_receive_state = RS_IDLE;
                cw_rr_current = 0;	// reset decoding pointer
                smpl_ctr = 0;		// reset audio sample counter
                return CW_ERROR;
            } else {
                // zero terminate representation
                rx_rep_buf[cw_rr_current] = 0;
            }
            // All is well.  Move to the more normal after-tone state.
            cw_receive_state = RS_AFTER_TONE;
            return CW_ERROR;
            break; //case CW_KEYUP

        //Update timings etc
        case CW_QUERY_EVENT:

            // this should be called quite often (faster than inter-character gap) It looks after timing
            // key up intervals and determining when a character, a word space, or an error char '*' should be returned.
            // CW_SUCCESS is returned when there is a printable character. Nothing to do if we are in a tone
            if (cw_receive_state == RS_IN_TONE)
                return CW_ERROR;
            // in this call we expect a pointer to a char to be valid

            if (c == NULL) {
                // else we had no place to put character...
                cw_receive_state = RS_IDLE;
                cw_rr_current = 0;
                // reset decoding pointer
                return CW_ERROR;
            }


            // compute length of silence so far
            sync_parameters();
            element_usec = usec_diff(cw_rr_end_timestamp, smpl_ctr);
            // SHORT time since keyup... nothing to do yet
            if (element_usec < (2 * cw_receive_dot_length))
                return CW_ERROR;
            // MEDIUM time since keyup... check for character space
            // one shot through this code via receive state logic
            // FARNSWOTH MOD HERE -->
            if (element_usec >= (2 * cw_receive_dot_length) &&
                element_usec <= (4 * cw_receive_dot_length) &&
                cw_receive_state == RS_AFTER_TONE) {
                // Look up the representation
                //cout << "CW_QUERY medium time after keyup: " << rx_rep_buf;
                *c = morseCode.rx_lookup(rx_rep_buf);
                //cout <<": " << *c <<flush;
                if (*c == NULL) {
                    // invalid decode... let user see error
                    *c = "*";
                }
                cw_receive_state = RS_IDLE;
                cw_rr_current = 0;	// reset decoding pointer
                space_sent = false;

                return CW_SUCCESS;
            }

            // LONG time since keyup... check for a word space
            // FARNSWOTH MOD HERE -->
            if ((element_usec > (4 * cw_receive_dot_length)) && !space_sent) {
                *c = " ";
                space_sent = true;
                return CW_SUCCESS;
            }
            // should never get here... catch all
            return CW_ERROR;

            break; //case CW_QUERY

    }
    // should never get here... catch all
    return CW_ERROR;
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
