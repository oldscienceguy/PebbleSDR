#ifndef RBDSCALLS_H
#define RBDSCALLS_H

#include <QtGlobal>

#define RDS_FREQUENCY 57000.0			//"carrier" frequency of RDS signal within FM demod output
#define RDS_BITRATE (RDS_FREQUENCY/48.0) //1187.5 bps bitrate

#define NUMBITS_CRC  10
#define NUMBITS_MSG  16
#define NUMBITS_BLOCK  (NUMBITS_CRC + NUMBITS_MSG)

//Block offset words for chkword generation
#define OFFSET_WORD_BLOCK_A  0x0FC
#define OFFSET_WORD_BLOCK_B  0x198
#define OFFSET_WORD_BLOCK_C  0x168
#define OFFSET_WORD_BLOCK_CP 0x350
#define OFFSET_WORD_BLOCK_D  0x1B4

//Block offset syndromes for chkword checking
//created multiplying Block Offsets by parity check matrix
//Takes into account that the (26,16)code is a shortened cyclic block code
//from the original (341,331) length cyclic code.
#define OFFSET_SYNDROME_BLOCK_A  0x3D8
#define OFFSET_SYNDROME_BLOCK_B  0x3D4
#define OFFSET_SYNDROME_BLOCK_C  0x25C
#define OFFSET_SYNDROME_BLOCK_CP 0x3CC
#define OFFSET_SYNDROME_BLOCK_D  0x258

#define CRC_POLY 0x5B9			//RDS crc polynomial  x^10+x^8+x^7+x^5+x^4+x^3+1

#define GROUPB_BIT 0x0800	//bit position in BlockB for determining group A or B msgs

#define BLOCK_A 0	//indexes for the four blocks
#define BLOCK_B 1
#define BLOCK_C 2
#define BLOCK_D 3

#define GROUPB_OFFSET 4		//offset into table below for type group B syndrome calcs

const int BLK_OFFSET_TBL[8] =
{
	OFFSET_SYNDROME_BLOCK_A,
	OFFSET_SYNDROME_BLOCK_B,
	OFFSET_SYNDROME_BLOCK_C,
	OFFSET_SYNDROME_BLOCK_D,
	//
	OFFSET_SYNDROME_BLOCK_A,
	OFFSET_SYNDROME_BLOCK_B,
	OFFSET_SYNDROME_BLOCK_CP,
	OFFSET_SYNDROME_BLOCK_D
};

//Generator matrix from RDS spec for creating checksum word
const quint32 CHKWORDGEN[16] =
{
	0x077,	//00 0111 0111
	0x2E7,	//10 1110 0111
	0x3AF,	//11 1010 1111
	0x30B,	//11 0000 1011
	0x359,	//11 0101 1001
	0x370,	//11 0111 0000
	0x1B8,	//01 1011 1000
	0x0DC,	//00 1101 1100
	0x06E,	//00 0110 1110
	0x037,	//00 0011 0111
	0x2C7,	//10 1100 0111
	0x3BF,	//11 1011 1111
	0x303,	//11 0000 0011
	0x35D,	//11 0101 1101
	0x372,	//11 0111 0010
	0x1B9	//01 1011 1001
};

//Parity check matrix from RDS spec for FEC
//Takes into account that the (26,16)code is a shortened cyclic block code
//from the original (341,331) length cyclic code.
const quint32 PARCKH[16] =
{
	0x2DC,	// 10 1101 1100
	0x16E,	// 01 0110 1110
	0x0B7,	// 00 1011 0111
	0x287,	// 10 1000 0111
	0x39F,	// 11 1001 1111
	0x313,	// 11 0001 0011
	0x355,	// 11 0101 0101
	0x376,	// 11 0111 0110
	0x1BB,	// 01 1011 1011
	0x201,	// 10 0000 0001
	0x3DC,	// 11 1101 1100
	0x1EE,	// 01 1110 1110
	0x0F7,	// 00 1111 0111
	0x2A7,	// 10 1010 0111
	0x38F,	// 11 1000 1111
	0x31B	// 11 0001 1011
};


typedef struct _RDS_GRPS
{
	quint16 BlockA;
	quint16 BlockB;
	quint16 BlockC;
	quint16 BlockD;
}tRDS_GROUPS;

//structure to hold PI code vs 3 letter callsigns for RBDS msgs
typedef struct
{
	quint16 pi;
	char csign[4];
}tRBDS3LET;

#define NUMENTRIES 72
const tRBDS3LET CALL3TABLE[NUMENTRIES] =
{
	{0x99A5, "KBW",},
	{0x9992, "KOY",},
	{0x9978, "WHO",},
	{0x99A6, "KCY",},
	{0x9993, "KPQ",},
	{0x999C, "WHP",},
	{0x9990, "KDB",},
	{0x9964, "KQV",},
	{0x999D, "WIL",},
	{0x99A7, "KDF",},
	{0x9994, "KSD",},
	{0x997A, "WIP",},
	{0x9950, "KEX",},
	{0x9965, "KSL",},
	{0x99B3, "WIS",},
	{0x9951, "KFH",},
	{0x9966, "KUJ",},
	{0x997B, "WJR",},
	{0x9952, "KFI",},
	{0x9995, "KUT",},
	{0x99B4, "WJW",},
	{0x9953, "KGA",},
	{0x9967, "KVI",},
	{0x99B5, "WJZ",},
	{0x9991, "KGB",},
	{0x9968, "KWG",},
	{0x997C, "WKY",},
	{0x9954, "KGO",},
	{0x9996, "KXL",},
	{0x997D, "WLS",},
	{0x9955, "KGU",},
	{0x9997, "KXO",},
	{0x997E, "WLW",},
	{0x9956, "KGW",},
	{0x996B, "KYW",},
	{0x999E, "WMC",},
	{0x9957, "KGY",},
	{0x9999, "WBT",},
	{0x999F, "WMT",},
	{0x99AA, "KHQ",},
	{0x996D, "WBZ",},
	{0x9981, "WOC",},
	{0x9958, "KID",},
	{0x996E, "WDZ",},
	{0x99A0, "WOI",},
	{0x9959, "KIT",},
	{0x996F, "WEW",},
	{0x9983, "WOL",},
	{0x995A, "KJR",},
	{0x999A, "WGH",},
	{0x9984, "WOR",},
	{0x995B, "KLO",},
	{0x9971, "WGL",},
	{0x99A1, "WOW",},
	{0x995C, "KLZ",},
	{0x9972, "WGN",},
	{0x99B9, "WRC",},
	{0x995D, "KMA",},
	{0x9973, "WGR",},
	{0x99A2, "WRR",},
	{0x995E, "KMJ",},
	{0x999B, "WGY",},
	{0x99A3, "WSB",},
	{0x995F, "KNX",},
	{0x9975, "WHA",},
	{0x99A4, "WSM",},
	{0x9960, "KOA",},
	{0x9976, "WHB",},
	{0x9988, "WWJ",},
	{0x99AB, "KOB",},
	{0x9977, "WHK",},
	{0x9989, "WWL"},
};

const char PTYPETABLERBDS[32][17] =
{	//16 char strings
//   1234567890123456
	{"                "},	//0
	{"     News       "},	//1
	{"  Information   "},	//2
	{"    Sports      "},	//3
	{"     Talk       "},	//4
	{"     Rock       "},	//5
	{"  Classic Rock  "},	//6
	{"   Adult Hits   "},	//7
	{"   Soft Rock    "},	//8
	{"    Top 40      "},	//9
	{"    Country     "},	//10
	{"    Oldies      "},	//11
	{"     Soft       "},	//12
	{"   Nostalgia    "},	//13
	{"     Jazz       "},	//14
	{"   Classical    "},	//15

	{"Rhythm and Blues"},	//16
	{"  Soft R & B    "},	//17
	{"Foreign Language"},	//18
	{"Religious Music "},	//19
	{" Religious Talk "},	//20
	{" Personality    "},	//21
	{"    Public      "},	//22
	{"    College     "},	//23
	{" Hablar_Espanol "},	//24
	{" Musica_Espanol "},	//25
	{"   Hip Hop      "},	//26
	{"                "},	//27
	{"                "},	//28
	{"   Weather      "},	//29
	{"Emergency_Test  "},	//30
	{"ALERT! ALERT!   "}	//31
};

const char PTYPETABLERDS[32][20] =
{	//16 char strings
//   1234567890123456
	{"                   "},	//0
	{"       News        "},	//1
	{"  Current Affairs  "},	//2
	{"    Information    "},	//3
	{"       Sport       "},	//4
	{"    Education      "},	//5
	{"      Drama        "},	//6
	{"     Culture       "},	//7
	{"     Science       "},	//8
	{"      Varied       "},	//9
	{"     Pop Music     "},	//10
	{"    Rock Music     "},	//11
	{"    M.O.R Music    "},	//12
	{"  Light Classical  "},	//13
	{" Serious Classical "},	//14
	{"   Other Music     "},	//15

	{"     Weather       "},	//16
	{"     Finance       "},	//17
	{"Chidren's Programs "},	//18
	{"  Social Affairs   "},	//19
	{"     Religion      "},	//20
	{"     Phone In      "},	//21
	{"      Travel       "},	//22
	{"     Leisure       "},	//23
	{"    Jazz Music     "},	//24
	{"  Country Music    "},	//25
	{"  National Music   "},	//26
	{"   Oldies Music    "},	//27
	{"    Folk Music     "},	//28
	{"   Documentary     "},	//29
	{"   Alarm Test      "},	//30
	{" ALARM! ALARM!     "}		//31
};

// List of Group Type Codes
#define GRPTYPE_0A 0
#define GRPTYPE_0B 1
#define GRPTYPE_1A 2
#define GRPTYPE_1B 3
#define GRPTYPE_2A 4	//radio text
#define GRPTYPE_2B 5
#define GRPTYPE_3A 6
#define GRPTYPE_3B 7
#define GRPTYPE_4A 8
#define GRPTYPE_4B 9
#define GRPTYPE_5A 10
#define GRPTYPE_5B 11
#define GRPTYPE_6A 12
#define GRPTYPE_6B 13
#define GRPTYPE_7A 14
#define GRPTYPE_7B 15
#define GRPTYPE_8A 16
#define GRPTYPE_8B 17
#define GRPTYPE_9A 18
#define GRPTYPE_9B 19
#define GRPTYPE_10A 20
#define GRPTYPE_10B 21
#define GRPTYPE_11A 22
#define GRPTYPE_11B 23
#define GRPTYPE_12A 24
#define GRPTYPE_12B 25
#define GRPTYPE_13A 26
#define GRPTYPE_13B 27
#define GRPTYPE_14A 28
#define GRPTYPE_14B 29
#define GRPTYPE_15A 30
#define GRPTYPE_15B 31

#define A_B_BIT 0x10	//bit toggles if need to reset radiotext


#endif // RBDSCALLS_H
