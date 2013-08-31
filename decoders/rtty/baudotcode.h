#ifndef BAUDOTCODE_H
#define BAUDOTCODE_H

class BaudotCode
{
public:
    //5 Bit Translation Codes
    enum TRANSLATION_CODES {INTERNAT,MILITARY,WEATHER,ARABIC,CYRILLIC,GREEK };

    //Last received shift flags
    enum SHIFT_FLAGS {LTR,FIG,NAT};

    BaudotCode();
    char asciiToBaudot(char ch, int *shift);
    char baudotToAscii(char ch, int *shift, TRANSLATION_CODES code);


private:
    static const char aStandardBaudot[];
    static const char bStandardLetter[];
    static const char bStandardFigure[];
    static const char bInternationalFigure[];
    static const char bMilitaryFigure[];
    static const char bWeatherFigure[];
    static const char bArabicLetter[];
    static const char bArabicFigure[];
    static const char bCyrillicLetter[];
    static const char bCyrillicFigure[];
    static const char bGreekLetter[];
    static const char bGreekFigure[];
    static const char bGreekNationalFigure[];

};

/* ascii codes */
#define ACR 13
#define ALF 10

/* raw baudot code contstants */
#define BNULL	0x00	/* Used as fill char */
#define BCR		0x08
#define BLF		0x02
#define BLTR 	0x1f
#define BFIG 	0x1b
#define BNAT 	0x00	/* ???? */

/* Raw tor code constants */
#define TCR		0x78		/* Carraige Return */
#define TLF		0x6c		/* Line Feed */
#define TFIG 	0x36
#define TLTR 	0x5a
#define TPS1 	0x0f		/* Mode B phase signal 1 */
#define TPS2 	0x66		/* Mode B phase signal 2 */
#define TRQ	 	0x66
#define TALPHA 0x0f
#define TBETA  0x33
#define TCS1	0x65		/* Control Signal 1 */
#define TCS2	0x6a
#define TCS3	0x59
#define TPLUS	0x63		/* +? */
#define TQUES	0x72


#endif // BAUDOTCODE_H
