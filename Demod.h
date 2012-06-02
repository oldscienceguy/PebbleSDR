#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include <iostream>
#include <QString>
#include "filters/fir.h"
#include "filters/iir.h"

enum DEMODMODE {
    dmAM,
    dmSAM,
    dmFMN,
    dmFMW,
    dmDSB,
    dmLSB,
    dmUSB,
    dmCWL,
    dmCWU,
    dmDIGL,
    dmDIGU,
	dmNONE
};

class Demod : public SignalProcessing
{
public:
    Demod(int samplerate, int size); 
	~Demod();

    CPX * ProcessBlock(CPX * in, int _numSamples);
    DEMODMODE demodMode() const;
    void setDemodMode(DEMODMODE mode, int _sourceSampleRate, int _audioSampleRate);
	static DEMODMODE StringToMode(QString m);
	static QString ModeToString(DEMODMODE dm);

private:
    DEMODMODE mode;
    
    void FMDeemphasisFilter(int _bufSize, CPX *in, CPX *out);
    float fmDeemphasisAlpha;
    static const float usDeemphasisTime = 75E-6; //Use for US & Korea FM
    static const float intlDeemphasisTime = 50E-6;  //Use for international FM

    //CFir fmMonoLPFilter;
    CIir fmMonoLPFilter;
    CFir fmAudioLPFilter;
    CIir fmPilotNotchFilter;
    CIir fmPilotBPFilter;
    CFir hilbertFilter;

    //These are CuteSDR FIR filters that decimate and filter using predefined (filtercoef.h) half-band filters to avoid aliasing
    CDecimateBy2* decBy2A;
    CDecimateBy2* decBy2B;
    CDecimateBy2* decBy2C;
    CDecimateBy2* decBy2D;


    //Used for FMW only for now
    int sourceSampleRate; //Original signal coming in
    int audioSampleRate; //Audio out
    int postFM2Dec; //From postFM1 to output output rate
    int postFM1Dec; //For 48k post FMW filters
    static const int postFMSampleRate = 48000;

    float pllPhase;
    float samLockCurrent;
    float samLockPrevious;
    float samDc;
    float fmDCOffset;
	//Previous I/Q values, used in simpleFM
	float fmIPrev;
	float fmQPrev;
    
	//PLL variables
    float pllFrequency;
	float alpha,beta;

	//Moving averages for smooth am detector
	float amDc;
	float amSmooth;

    
	//SAM config
	float samLoLimit; //PLL range
	float samHiLimit;
	float samBandwidth;

	//FMN config
	float fmLoLimit;
	float fmHiLimit;
	float fmBandwidth;

    CPX PLL(CPX sig, float loLimit, float hiLimit, int _numSamples);

    void PllSAM(CPX * in, CPX * out, int _numSamples);
    void PllFMN(CPX * in, CPX * out, int _numSamples);
    void FMMono(CPX * in, CPX * out, int _numSamples);
    void FMStereo(CPX * in, CPX * out, int _numSamples);

	//Simple time domain demod algorithms for testing and comparison
    void SimpleAM(CPX *in, CPX *out, int _numSamples);
    void SimpleAMSmooth(CPX *in, CPX *out, int _numSamples);
    void SimpleUSB(CPX *in, CPX *out, int _numSamples);
    void SimpleLSB(CPX *in, CPX *out, int _numSamples);
    void SimplePhase(CPX *in, CPX *out, int _numSamples);
    void SimpleFM(CPX *in, CPX *out, int _numSamples);
    void SimpleFM2(CPX *in, CPX *out, int _numSamples);

};

