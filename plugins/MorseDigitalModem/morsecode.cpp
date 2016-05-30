//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "morsecode.h"

//Main table used to generate fast lookup tables
//DotDash must be unique for each entry
//7 Elements max
MorseSymbol MorseCode::m_morseTable[] = {
    //Prosigns, no interletter spacing http://en.wikipedia.org/wiki/Prosigns_for_Morse_Code
    //Special abreviations http://en.wikipedia.org/wiki/Morse_code_abbreviations
    //Supposed to have inter-letter spacing, but often sent as single 'letter'
    //Non English extensions (TBD from Wikipedia)

    //I don't think the char matters because we're not supporting send, can use '*' for everything
	//Char  Display   Dot/Dash
    {'=',	"<BT>",   "-...-"	}, // 2 LF or new paragraph
    {'~',	"<AA>",   ".-.-"	}, // CR/LF
    {'<',	"<AS>",   ".-..."	}, // Please Wait
    {'>',	"<AR>",   ".-.-."	}, // End of Message, sometimes shown as '+'
    {'%',	"<SK>",   "...-.-"	}, // End of QSO or End of Work
    {'+',	"<KN>",   "-.--."	}, // Invitation to a specific station to transmit, compared to K which is general
    {'&',	"<INT>",  "..-.-"	}, //
    {'{',	"<HM>",   "....--"	}, //
    {'}',	"<VE>",   "...-."	}, //
    //{'*',   "(ER)",   "·······."},  //Error (8 elements, too long)
    {'*',   "(SN)",   "···-·"},    //Understood
    //{'*',   "(CL)",   "-·-··-··"}, //Going off air (8 elements, too long)
    {'*',   "(CT)",   "-·-·-"},   //Start copying (same as KA)
    {'*',   "(DT)",   "-··---"},  //Shift to Wabun code
    //{'*',   "(SOS)",  "···---···"}, //SOS (Too long)
    {'*',   "(BK)",   "-...-.-"}, //Break (Bk)

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
    {0, NULL, NULL} //Must terminate table
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
quint8 MorseCode::tokenizeSymbol(const char *symbol)
{
    unsigned int token;	/* Return token value */
    const char *sptr;	/* Pointer through string */

    /*
     * Our algorithm can handle only 6 characters of representation.
     * And we insist on there being at least one character, too.
     */
	if (strlen(symbol) > 6 || strlen(symbol) < 1)
        return 0;

    /*
     * Build up the token value based on the dots and dashes.  Start the
     * token at 1 - the sentinel (start) bit.
     */
	for (sptr = symbol, token = 1; *sptr != 0; sptr++) {
        /*
         * Left-shift the sentinel (start) bit.
         */
        token <<= 1;

        /*
         * If the next element is a dash, OR in another bit.  If it is
         * not a dash or a dot, then there is an error in the repres-
         * entation string.
         */
		if (*sptr == c_dashChar)
            token |= 1;
		else if (*sptr != c_dotChar)
            return 0;
    }

    /* Return the value resulting from our tokenization of the string. */
    return token;
}

/* ---------------------------------------------------------------------- */

MorseCode::MorseCode() {
	init();
}

MorseCode::~MorseCode() {
}

void MorseCode::init()
{
	MorseSymbol *cw;	/* Pointer to table entry */
    long code;
    int len;
	quint8 token;

    // Clear the RX & TX tables
	for (quint32 i = 0; i < c_morseTableSize; i++) {
		m_txLookup[i].code = 0x04; //0000 0100
		m_txLookup[i].prt = 0;
		m_rxLookup[i] = 0;
    }

    // For each main table entry, create a token entry.
	for (cw = m_morseTable; cw->chr != 0; cw++) {
		if ((cw->chr == '(') && !c_useParen) continue;
		if ((cw->chr == '<') && c_useParen) continue;
		token = tokenizeSymbol(cw->dotDash);
		if (token != 0)
			m_rxLookup[token] = cw;
    }
    // Build TX table
	for (cw = m_morseTable; cw->chr != 0; cw++) {
		if ((cw->chr == '(') && !c_useParen) continue;
		if ((cw->chr == '<') && c_useParen) continue;
        len = strlen(cw->dotDash);
        code = 0x04;
        while (len-- > 0) {
			if (cw->dotDash[len] == c_dashChar) {
                code = (code << 1) | 1;
                code = (code << 1) | 1;
                code = (code << 1) | 1;
            } else
                code = (code << 1) | 1;
            code <<= 1;
        }
		m_txLookup[(int)cw->chr].code = code;
		m_txLookup[(int)cw->chr].prt = cw->display;
    }
}

//Return CW_TABLE element so caller can access anything from it, not just display
MorseSymbol *MorseCode::rxLookup(char *r)
{
    int			token;
	MorseSymbol *cw;

	if ((token = tokenizeSymbol(r)) == 0)
        return NULL;

	if ((cw = m_rxLookup[token]) == NULL)
        return NULL;

    return cw;
}

const char *MorseCode::txPrint(char c)
{
	if (m_txLookup[toupper(c)].prt)
		return m_txLookup[toupper(c)].prt;
    else
		return "";
}

quint32 MorseCode::wpmToTcwMs(quint32 wpm)
{
	//quint32 tcw = 60.0 / (wpm * 50) * 1000;
	quint32 tcw = c_mSecDotMagic / wpm;
	return tcw;
}

quint32 MorseCode::tcwMsToWpm(quint32 msTcw)
{
	quint32 wpm = c_mSecDotMagic / msTcw;
	return wpm;
}

// Usec = DM / WPM
// DM = WPM * Usec
// WPM = DM / Usec
quint32 MorseCode::wpmToTcwUsec(quint32 wpm)
{
	return c_uSecDotMagic / wpm;
}

quint32  MorseCode::tcwUsecToWpm(quint32 tcwUsec)
{
	return c_uSecDotMagic / tcwUsec;
}


unsigned long MorseCode::txLookup(char c)
{
	return m_txLookup[toupper(c)].code;
}


