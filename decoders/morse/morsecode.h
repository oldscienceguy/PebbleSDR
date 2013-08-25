#ifndef MORSECODE_H
#define MORSECODE_H

#include "global.h"

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
};



#endif // MORSECODE_H
