#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
/*
These are time domain filters, see FFT filters for frequency domain
The algorithm and values can be checked at http://www-users.cs.york.ac.uk/~fisher/mkfilter/
Typical ButterWorth filter examples
	Broadcast 15Khz 20 hz -15 khz
	Speech 3Khz 300hz to 3khz
	RTTY 250-1000hz
	CW 200-500hz
	PSK31 100hz

See https://ccrma.stanford.edu/software/snd/snd/sndlib.html for c examples of other filters and audio code
See http://uazu.net/fiview/
*/
/*
Lo/High setting for each mode/filter combination
Spectrum	-freq	+freq All other modes not specified below
CWL			-freq	-50
CWU			50		freq
LSB			-freq	-50
USB			50		freq
DIGL		-freq	-200
DIGU		200		freq
DRM			1200-freq/2		1200+freq/2
*/

#include "cpx.h"

//Standard filter options (freq)
//int filterOptions[] = { 50,100,250,500,1000,1800,2000,2200,2400,2600,2800,3000,4000,5000,6000,9000,
//	10000,20000,50000,250000};

class Butterworth
{
public:
	Butterworth(int p,int z,int fc,int fc2,int fs,float g,int t[],float y[]);
	~Butterworth(void);
	float Filter(float input);
private:
	//Defines a complete filter with values from AJ Fisher calculator
    int nPoles;
    int nZeros;
    int corner1;  //Freq in hz, fc
    int corner2;
    int sampleRate;
    int gain;
    int *xTbl;
    float *yTbl;
	float *xv;
	float *yv;
};
