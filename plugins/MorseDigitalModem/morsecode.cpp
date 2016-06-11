//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "morsecode.h"

//Main table used to generate fast lookup tables
//DotDash must be unique for each entry
//7 Elements max
MorseSymbol MorseCode::m_morseTable[] = {
	//Prosigns are 2 letters with no interletter spacing
	//http://en.wikipedia.org/wiki/Prosigns_for_Morse_Code
    //Special abreviations http://en.wikipedia.org/wiki/Morse_code_abbreviations
    //Supposed to have inter-letter spacing, but often sent as single 'letter'
    //Non English extensions (TBD from Wikipedia)

	//ASCII codes must be unique, no duplicates allowed in table

	//Ascii  Display   Dot/Dash	Token
	{0x0a,	"\n",		".-.-",		0	}, // For text files. lf same as '\n'
	{0x0d,	"\n",		".-.-",		0	}, // For text files. Same as '~' <AA> CR/LF
	{0x21,	"!",		"-.-.--",	0	}, // exclamation
	{0x22,   "<CT>",	"-.-.-",	0	}, // '"' Attention or Start copying (same as KA)
	{0x23,   "<DO>",	"-..---",	0	},  // '#' Shift to Wabun code, also <NJ>
	{0x24,	"$",		"...-..-",	0	},
	{0x25,	"<SK>",		"...-.-",	0	}, // '%' End of QSO or End of Work
	{0x26,	"<INT>",	"..-.-",	0	}, // '&'
	{0x27,	"'",		".----.",	0	}, // apostrophe
	{0x28,	"(",		"-.--.",	0	}, // open paren. MARS may use this to send/receive KN instead of +
	{0x29,	")",		"-.--.-",	0	}, // close paren. Sometimes used for both open and closed paren
	{0x2a,   "<SN>",	"...-.",	0	}, //Understood
	//'(' and <KN> use same code.  Defer to '('
	//{0x2b,	"<KN>",		"-.--.",	0	}, // '+' Invitation to a specific station to transmit, compared to K which is general
	{0x2c,	",",		"--..--",	0	}, // comma
	{0x2d,	"-",		"-....-",	0	}, // dash
	{0x2e,	".",		".-.-.-",	0	}, // period
	{0x2f,	"/",		"-..-.",	0	}, // forward slash

	{0x30,	"0",		"-----",	0	},
	{0x31,	"1",		".----",	0	},
	{0x32,	"2",		"..---",	0	},
	{0x33,	"3",		"...--",	0	},
	{0x34,	"4",		"....-",	0	},
	{0x35,	"5",		".....",	0	},
	{0x36,	"6",		"-....",	0	},
	{0x37,	"7",		"--...",	0	},
	{0x38,	"8",		"---..",	0	},
	{0x39,	"9",		"----.",	0	},
	{0x3a,	":",		"---...",	0	}, // colon
	{0x3b,	";",		"-.-.-.",	0	}, // semi colon
	//{0x3c,	"&",		".-...",	0	}, // '<' Please Wait 10 secondsor sometimes used as'&'
	{0x3c,	"<AS>",		".-...",	0	}, // '<' Please Wait 10 secondsor '&'
	//{0x3d,	"<BT>",		"-...-",	0	}, // '='  <BT>2 LF or new paragraph
	//{0x3d,	"=",		"-...-",	0	}, // '='  <BT>2 LF or new paragraph
	{0x3d,	"\n\n",		"-...-",	0	}, // '='  <BT>2 LF or new paragraph
	//Three choices for Stop (end of message).  Choose 1 or configure
	//{0x3e,	"<AR>",		".-.-.",	0	}, // '>' <AR> New Page, End of Message
	//{0x3e,	"+",		".-.-.",	0	}, // '>' <AR> New Page, End of Message
	{0x3e,	"\n\n\n",	".-.-.",	0	}, // '>' <AR> New Page, End of Message
	{0x3f,	"?",		"..--..",	0	}, // question
	{0x40,	"@",		".--.-.",	0	}, // At sign (Added to ITU standard in 2004)

	//ASCII letters (always converted to UC for typing, 'a' same as 'A'
	{0x41,	"A",		".-",		0	},
	{0x42,	"B",		"-...",		0	},
	{0x43,	"C",		"-.-.",		0	},
	{0x44,	"D",		"-..",		0	},
	{0x45,	"E",		".",		0	},
	{0x46,	"F",		"..-.",		0	},
	{0x47,	"G",		"--.",		0	},
	{0x48,	"H",		"....",		0	},
	{0x49,	"I",		"..",		0	},
	{0x4a,	"J",		".---",		0	},
	{0x4b,	"K",		"-.-",		0	}, //Also <K>
	{0x4c,	"L",		".-..",		0	},
	{0x4d,	"M",		"--",		0	},
	{0x4e,	"N",		"-.",		0	},
	{0x4f,	"O",		"---",		0	},
	{0x50,	"P",		".--.",		0	},
	{0x51,	"Q",		"--.-",		0	},
	{0x52,	"R",		".-.",		0	},
	{0x53,	"S",		"...",		0	},
	{0x54,	"T",		"-",		0	},
	{0x55,	"U",		"..-",		0	},
	{0x56,	"V",		"...-",		0	},
	{0x57,	"W",		".--",		0	},
	{0x58,	"X",		"-..-",		0	},
	{0x59,	"Y",		"-.--",		0	},
	{0x5a,	"Z",		"--..",		0	},
	{0x5b,  "<BK>",		"-...-.-",	0	}, // '[' Break (Bk)
	{0x5c,	"\\",		".-..-.",	0	}, // '\' back slash
	//{0x5d,  "<ERROR>",		"........",	0	}, // ']' <HH> Error
	{0x5d,  "????",		"........",	0	}, // ']' <HH> Error
	{0x5e,  "<CL>",		"-.-..-..",	0	}, // '^' Going off air
	{0x5f,	"_",		"..--.-",	0	}, // '_' underscore

	{0x7b,	"<HM>",		"....--",	0	}, // '{'
	//{0x7c,  "<SOS>",	"···---···",	0}, // '|' SOS (9 bits too long)
	{0x7d,	"<VE>",		"...-.",	0	}, // '}' Same as <SN>?Not sure where this came from
	{0x7e,	"\n",		".-.-",		0	}, // '~' <AA> CR/LF

	//International characters
	//Todo: update display so characters are readable in morse window

	//WARNING: There are duplicate morse codes which must be handled
	//Lookup by ascii will work, but init() will skip duplicate tokens to lookup isn't broken
	//Typing the ascii code will send the right morse
	//But decoding will print the first symbol found

	//These map to extended ascii
	//Duplicate morse is allowed, but only 1 display value
	{0x80,	"Ç",		"-.-..",	0	}, //(1) C cedila ISO 8859-1
	{0x85,	"À",		".--.-",	0	}, //(1) A grave
	{0x8a,	"È",		".-..-",	0	}, //(1) E grave ISO 8859-1
	{0x8e,	"Ä",		".-.-",		0	}, //(1) A diaeresis ISO 8859-1
	{0x8f,	"Ȧ",		".--.-",	0	}, //A dot ISO 8859-1 also a acute
	{0x90,	"É",		"..-..",	0	}, //(1) E acute ISO 8859-1
	{0x92,	"Æ",		".-.-",		0	}, //AE
	{0x99,	"Ö",		"---.",		0	}, //(1) O diaeresis ISO 8859-1
	{0x9a,	"Ü",		"..--",		0	}, //(1) U diaeresis ISO 8859-1
	{0xa2,	"Ó",		"---.",		0	}, //O acute
	{0xa5,	"Ñ",		"--.--",	0	}, //(1) N tilde ISO 8859-1

	//These don't map to any ascii key, just assign sequential codes for now until we can figure out typeing codes
	{0xb0,	"Ą",		".-.-",		0	}, //A cedila
	{0xb1,	"Ć",		"-.-..",	0		}, //C acute
	{0xb2,	"Ĉ",		"-.-..",	0	}, //C circumflex
	{0xb3,	"(Ch)",		"----",		0	}, //Ch not printable?
	{0xb4,	"Đ",		"..-..",	0	}, //D with stroke
	{0xb5,	"Ð",		"..--.",	0	}, //Eth
	{0xb6,	"Ȩ",		"..-..",	0	}, //E cedila
	{0xb7,	"Ĝ",		"--.-.",	0	}, //G circumflex
	{0xb8,	"Ĥ",		"----",		0	}, //H circumflex
	{0xb9,	"Ĵ",		".---.",	0	}, //J circumflex
	{0xba,	"Ł",		".-..-",	0	}, //L stroke
	{0xbb,	"Ń",		"--.--",	0	}, //N acute
	{0xbc,	"Ø",		"---.",		0	}, //O stroke
	{0xbd,	"Ś",		"...-...",	0	}, //S acute
	{0xbe,	"Ŝ",		"...-.",	0	}, //S circumflex, also prosign for understood
	{0xbf,	"Š",		"----",		0	}, //S caron ISO 8859-2
	{0xc0,	"Ᵽ",		".--..",	0	}, //P stroke?
	{0xc1,	"Ǔ",		"..--",		0	}, //U caron
	{0xc2,	"Ź",		"--..-.",	0	}, //Z acute
	{0xc3,	"Ż",		"--..-",	0	}, //Z dot ISO 8859-2

	{0x00,		NULL,	NULL,		0	} //Must terminate table
};

/*
	Return a token value, in the range 2-512, for a lookup table representation.
	The routine returns 0 if no valid token could be made from the string.

	This token algorithm is designed ONLY for valid CW representations; that is,
	strings composed of only '.' and '-', and in this case, strings shorter than
	8 characters.  The algorithm simply turns the representation into a
	'bitmask', based on occurrences of '.' and '-'.  The first bit set in the
	mask indicates the start of data.
	The bitmask is limited to 9 bits, 1 'start bit' and 8 data bits.
	So the largest bitmask is 1 1111 1111, 0x1ff, or 511.  This is used to lookup tokens in token order.
	The 8 data bits give us a range of 0-255 which is used to look up tokens in character order
*/
quint16 MorseCode::tokenizeDotDash(const char *dotDash)
{
	//token starts with 1 to set hob as flag start of token
	quint16 token = 1;	//Return token value
	const char *sptr;	// Pointer through string

	//Max 8 bit symbol plus 1 hob flag
	if (strlen(dotDash) > 8 || strlen(dotDash) < 1) {
		qDebug()<<"Morse token too long or empty:"<<dotDash;
        return 0;
	}

	for (sptr = dotDash; *sptr != 0; sptr++) {
        token <<= 1;
		//Set the next bit to 1 if dash or leave as 0 if dot
		if (*sptr == c_dashChar)
            token |= 1;
		else if (*sptr != c_dotChar) {
			qDebug()<<"Symbol contains characters other than '.' or '-':"<<dotDash;
			return 0; //Error
		}
    }

    return token;
}

MorseCode::MorseCode() {
	init();
}

MorseCode::~MorseCode() {
}

void MorseCode::init()
{
	MorseSymbol *symbol;
	quint16 token;

    // Clear the RX & TX tables
	for (quint32 i = 0; i < c_morseTableSize; i++) {
		MorseCode::m_asciiOrderTable[i] = 0;
    }
	for (quint32 i = 0; i < c_tokenTableSize; i++) {
		MorseCode::m_tokenOrderTable[i] = 0; //Flag invalid entry
	}

	// For each main table entry, create a pointer to symbol in rxLookup in token order
	//Then we can do fast lookup by morse token in rxLookup
	for (symbol = m_morseTable; symbol->ascii != 0; symbol++) {
		//Option for special handling for possible MARS use
		//'(' used to send/receive <KN> in this option
#if 0
		if ((symbol->ascii == '(') && !c_marsMode)
			continue;
		if ((symbol->ascii == '<') && c_marsMode)
			continue;
#endif
		//tokens start with a 1 bit followed by 1's for dash and 0's for dots
		token = tokenizeDotDash(symbol->dotDash);
		if (token != 0) {
			symbol->token = token;
			//In token order
			if (MorseCode::m_tokenOrderTable[token] != 0) {
				//We already have an entry, skip this one and output warning
				//qWarning()<<"Duplicate token in morse table for -"<<symbol->ascii<<" "<<symbol->display;
			} else {
				MorseCode::m_tokenOrderTable[token] = symbol;
			}
			//In character order
			if (MorseCode::m_asciiOrderTable[symbol->ascii] != 0) {
				//We already have an entry, skip this one and output warning
				//qWarning()<<"Duplicate ascii in morse table for -"<<symbol->ascii<<" "<<symbol->display;
			} else {
				MorseCode::m_asciiOrderTable[symbol->ascii] = symbol;
			}
		}
    }
}

//Return CW_TABLE element so caller can access anything from it, not just display
MorseSymbol *MorseCode::tokenLookup(const char *r)
{
	quint32	token;
	MorseSymbol *cw;

	if ((token = tokenizeDotDash(r)) == 0)
        return NULL;

	if ((cw = MorseCode::m_tokenOrderTable[token]) == NULL)
        return NULL;

    return cw;
}

quint16 MorseCode::asciiLookup(quint8 c)
{
	MorseSymbol *symbol = m_asciiOrderTable[toupper(c)];
	if (symbol != NULL)
		return symbol->token;
	else
		return 0;
}

MorseSymbol *MorseCode::tableLookup(quint32 index)
{
	if (index >= c_morseTableSize)
		return NULL;
	MorseSymbol *symbol = &m_morseTable[index];
	return symbol;
}

QString MorseCode::txPrint(quint8 c)
{
	MorseSymbol *symbol = m_asciiOrderTable[toupper(c)];
	if (symbol != NULL)
		return symbol->display;
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


/*
   100 Top morse words from http://lcwo.net/forum/563
   Possible use in training or error correction
*/
const QString MorseCode::c_commonWords[] = {
	"73",
	"/AR",
	"/AS",
	"/BK",
	"/KN",
	"/SK",
	"A",
	"ABOUT",
	"AGN",
	"ALL",
	"AND",
	"ANT",
	"ARE",
	"AS",
	"AT",
	"BUT",
	"BY",
	"CAN",
	"CQ",
	"DO",
	"DON'T",
	"DX",
	"ES",
	"FB",
	"FOR",
	"FROM",
	"GA",
	"GE",
	"HAD",
	"HAVE",
	"HE",
	"HI",
	"HIS",
	"HR",
	"HW",
	"I",
	"IF",
	"IN",
	"IS",
	"IT",
	"IT'S",
	"JUST",
	"KNOW",
	"LIKE",
	"MEAN",
	"MY",
	"NAME",
	"NOT",
	"NR",
	"OF",
	"OH",
	"OM",
	"ON",
	"ONE",
	"OR",
	"OTHER",
	"OUT",
	"PSE",
	"PWR",
	"QRL",
	"QRM",
	"QRN",
	"QRQ",
	"QRS",
	"QRZ",
	"QSB",
	"QSY",
	"QTH",
	"R",
	"REALLY",
	"RIGHT",
	"RST",
	"RTU",
	"SO",
	"SOME",
	"THAT'S",
	"THAT",
	"THE",
	"THEM",
	"THERE",
	"THEY",
	"THINK",
	"TNX",
	"TO",
	"TU",
	"UH",
	"UP",
	"WAS",
	"WE",
	"WELL",
	"WERE",
	"WHAT",
	"WHEN",
	"WHERE",
	"WITH",
	"WX",
	"YEAH",
	"YOU",
	"YOUR"
};

//Must come after static initialization
const quint32 MorseCode::c_commonWordsSize = sizeof(MorseCode::c_commonWords)/sizeof(MorseCode::c_commonWords[0]);

//Abbreviations, assume letter space between char or not?
const QString MorseCode::c_abbreviations[] = {
"AA", //	All after (used after question mark to request a repetition)
"AB", //	All before (similarly)
"ARRL", //	American Radio Relay League
"ABT", //	About
"ADR", //	Address
"AGN", //	Again
"ANT", //	Antenna
"ARND", //	Around
"BCI", //	Broadcast interference
"BK", //	Break (to pause transmission of a message, say)
"BN", //	All between
"BTR", //	Better
"BUG", //	Semiautomatic mechanical key
"BURO", //	Bureau (usually used in the phrase PLS QSL VIA BURO, "Please send QSL card via my local/national QSL bureau")
"B4", //	Before
"C", //	Yes; correct
"CBA", //	Callbook address
"CFM", //	Confirm
"CK", //	Check
"CL", //	Clear (I am closing my station)
"CLG", //	Calling
"CQ", //	Calling any station
"CQD", //	Original International Distress Call, fell out of use before 1915
"CS", //	Callsign
"CTL", //	Control
"CUD", //	Could
"CUL", //	See you later
"CUZ", //	Because
"CW", //	Continuous wave (i.e., radiotelegraph)
"CX", //	Conditions
"DE", //	From (or "this is")
"DN", //	Down
"DR", //	Dear
"DSW", //	Goodbye (Russian: до свидания [Do svidanya])
"DX", //	Distance (sometimes refers to long distance contact), foreign countries
"EMRG", //	Emergency
"ENUF", //	Enough
"ES", //	And
"FB", //	Fine business (Analogous to "OK")
"FCC", //	Federal Communications Commission
"FER", //	For
"FM", //	From
"FREQ", //	Frequency
"FWD", //	Forward
"GA", //	Good afternoon or Go ahead (depending on context)
"GE", //	Good evening
"GG", //	Going
"GL", //	Good luck
"GM", //	Good morning
"GN", //	Good night
"GND", //	Ground (ground potential)
"GUD", //	Good
"GX", //	Ground
"HEE", //	Humour intended (often repeated, e.g. HEE HEE)
"HI", //	Humour intended (possibly derived from HEE)
"HR", //	Here, hear
"HV", //	Have
"HW", //	How
"II", //	I say again
"IMP", //	Impedance
"K", //	Over
"KN", //	Over; only the station named should respond (e.g. W7PTH DE W1AW KN)
"LID", //	Poor operator
"MILS", //	Milliamperes
"MNI", //	Many
"MSG", //	Message
"N", //	No; nine
"NIL", //	Nothing
"NM", //	Name
"NR", //	Number
"NW", //	Now
"NX", //	Noise; noisy
"OB", //	Old boy
"OC", //	Old chap
"OM", //	Old man (any male amateur radio operator is an OM regardless of age)
"OO", //	Official observer
"OP", //	Operator
"OT", //	Old timer
"OTC", //	Old timers club (ARRL-sponsored organization for radio amateurs first licensed 20 or more years ago)
"OOTC", //	Old old timers club (organization for those whose first two-way radio contact occurred 40 or more years ago; separate from OTC and ARRL)
"PLS", //	Please
"PSE", //	Please
"PWR", //	Power
"PX", //	Prefix
"QCWA", //	Quarter Century Wireless Association (organization for radio amateurs who have been licensed 25 or more years)
"R", //	Are; received as transmitted (origin of "Roger"), or decimal point (depending on context)
"RCVR", //	Receiver (radio)
"RFI", //	Radio Frequency Interference
"RIG", //	Radio apparatus
"RPT", //	Repeat or report (depending on context)
"RPRT", //	Report
"RST", //	Signal report format (Readability-Signal Strength-Tone)
"RTTY", //	Radioteletype
"RX", //	Receiver, radio
"SAE", //	Self-addressed envelope
"SASE", //	Self-addressed, stamped envelope
"SED", //	Said
"SEZ", //	Says
"SFR", //	So far (proword)
"SIG", //	Signal or signature
"SIGS", //	Signals
"SK", //	Out (proword), end of contact
"SK", //	Silent Key (a deceased radio amateur)
"SKED", //	Schedule
"SMS", //	Short message service
"SN", //	Soon
"SNR", //	Signal-to-noise ratio
"SRI", //	Sorry
"SSB", //	Single sideband
"STN", //	Station
"T", //	Zero (usually an elongated dah)
"TEMP", //	Temperature
"TFC", //	Traffic
"TKS", //	Thanks
"TMW", //	Tomorrow
"TNX", //	Thanks
"TT", //	That
"TU", //	Thank you
"TVI", //	Television interference
"TX", //	Transmit, transmitter
"TXT", //	Text
"U", //	You
"UR", //	Your or You're (depending on context)
"URS", //	Yours
"VX", //	Voice; phone
"VY", //	Very
"W", //	Watts
"WA", //	Word after
"WB", //	Word before
"WC", //	Wilco
"WDS", //	Words
"WID", //	With
"WKD", //	Worked
"WKG", //	Working
"WL", //	Will
"WUD", //	Would
"WTC", //	Whats the craic? (Irish Language: [Conas atá tú?])
"WX", //	Weather
"XCVR", //	Transceiver
"XMTR", //	Transmitter
"XYL", //	Wife (ex-YL)
"YF", //	Wife
"YL", //	Young lady (originally an unmarried female operator, now used for any female)
"ZX", //	Zero beat
"73", //	Best regards
"88", //	Love and kisses
};
//Must come after static initialization
const quint32 MorseCode::c_abbreviationsSize = sizeof(MorseCode::c_abbreviations)/sizeof(MorseCode::c_abbreviations[0]);

//Array of pointers to cw_table dot/dash tokenized order
MorseSymbol *MorseCode::m_tokenOrderTable[c_tokenTableSize];

//In ascii character order
MorseSymbol *MorseCode::m_asciiOrderTable[c_morseTableSize];
