#ifndef MORSECODE_H
#define MORSECODE_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "global.h"
#pragma clang diagnostic ignored "-Wc++11-extensions"

struct CW_TABLE {
    char chr;	/* The character(s) represented */
    const char *display;	/* The printable representation of the character */
    const char *dotDash;	/* Dot-dash shape of the character */
};

struct CW_XMT_TABLE {
    unsigned long code;
    const    char *prt;
};

#define	CW_DOT_REPRESENTATION	'.'
#define	CW_DASH_REPRESENTATION	'-'

#define	MorseTableSize	256

class MorseCode {
public:
	MorseCode();
	~MorseCode();
    void init();
    CW_TABLE *rx_lookup(char *r);
    unsigned long	tx_lookup(int c);
    const char *tx_print(int c);

	//Returns Tcw in ms for wpm
	static quint32 wpmToTcwMs(quint32 wpm);
	//Returns wpm for Tcw ms
	static quint32 tcwMsToWpm(quint32 msTcw);

	static quint32 wpmToTcwUsec(quint32 wpm);
	static quint32 tcwUsecToWpm(quint32 tcwUsec);

	//Max dots and dashes in a morse symbol
	static const int c_maxMorseLen = 7;

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

private:

	CW_TABLE 		*m_rxLookup[256];
	CW_XMT_TABLE 	m_txLookup[256];
    unsigned int 	tokenize_representation(const char *representation);
	const bool c_useParen = false; //Use open paren character; typically used in MARS ops
};



#endif // MORSECODE_H
