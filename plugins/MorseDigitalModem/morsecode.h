#ifndef MORSECODE_H
#define MORSECODE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "global.h"
#pragma clang diagnostic ignored "-Wc++11-extensions"

struct MorseSymbol {
	quint8 ascii;			//ASCII character used to send
	const char *display;	//Printable string
	const char *dotDash;	//Dot-Dash string
	quint8 token;			//Leading 1 followed by 1 for dash and 0 for dot
};

class MorseCode {
public:
	enum MorseSymbolTypes {DOT, DASH, EL_SPACE, CH_SPACE, WORD_SPACE};

	MorseCode();
	~MorseCode();
    void init();
	//Returns a table entry for the dot/dash representation
	MorseSymbol *rxLookup(char *r);
	quint8 txLookup(char c);
	//Used to iterate through the entire table for testing and possible future use
	MorseSymbol *tableLookup(quint32 index);

	const char *txPrint(char c);

	//Returns Tcw in ms for wpm
	static quint32 wpmToTcwMs(quint32 wpm);
	//Returns wpm for Tcw ms
	static quint32 tcwMsToWpm(quint32 msTcw);

	static quint32 wpmToTcwUsec(quint32 wpm);
	static quint32 tcwUsecToWpm(quint32 tcwUsec);

	//Max dots and dashes in a morse symbol
	static const int c_maxMorseLen = 7;
	//Max number of Tcw's we can see in a symbol
	// 7 dashes (21), with 7 interelement spaces(7), with 1 char space(7)
	static const quint32 c_maxTcwPerSymbol = 35;

	//Use DOT_MAGIC to convert WPM to/from usec is 1,200,000
	//c_uSecDotMagic / WPM = usec per TCW
	//c_mSecDotMagic / WPM = msec per TCW
	//c_secDotMagic / WPM = sec per TCW
	static const quint32 c_uSecDotMagic = 1200000; //units are micro-seconts
	static const quint32 c_mSecDotMagic = 1200; //units are milli-seconds
	static constexpr double c_secDotMagic = 1.2; //units are seconds

	//Relative lengths of elements in Tcw terms
	static const quint32 c_tcwDot = 1;
	static const quint32 c_tcwDash = 3;
	static const quint32 c_tcwCharSpace = 3;
	static const quint32 c_tcwElementSpace = 1;
	static const quint32 c_tcwWordSpace = 7;

	static const quint8 c_dotChar = '.';
	static const quint8 c_dashChar = '-';

private:

	static const quint32 c_morseTableSize = 256;
	//Main table used to generate fast lookup tables for receive and transmit
	static MorseSymbol m_morseTable[c_morseTableSize];

	//Array of pointers to cw_table dot/dash tokenized order
	MorseSymbol *m_rxLookup[c_morseTableSize];

	//In character order
	MorseSymbol *m_txLookup[c_morseTableSize];

	quint8 	tokenizeSymbol(const char *symbol);
	const bool c_useParen = false; //Use open paren character; typically used in MARS ops
};



#endif // MORSECODE_H
