#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include "QObject"
#include "signalprocessing.h"
#include <iostream>
#include <QString>
#include "filters/fir.h"
#include "filters/iir.h"
#include "demod/wfmdemod.h"
#include "demod/rdsdecode.h"
#include "ui/ui_data-band.h"

enum DEMODMODE {
    dmAM,
    dmSAM,
    dmFMN,
    dmFMMono,
    dmFMStereo,
    dmDSB,
    dmLSB,
    dmUSB,
    dmCWL,
    dmCWU,
    dmDIGL,
    dmDIGU,
	dmNONE
};

class Demod_AM;

class Demod : public SignalProcessing
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod(int _inputRate, int _numSamples);
    Demod(int _inputRate, int _inputWfmRate, int size);
	~Demod();

    void SetupDataUi(QWidget *parent);

    CPX * ProcessBlock(CPX * in, int _numSamples);
    DEMODMODE DemodMode() const;
    void ResetDemod(); //Resets all data decoders, called after frequency change from receiver
    void SetDemodMode(DEMODMODE mode, int _sourceSampleRate, int _audioSampleRate);
	static DEMODMODE StringToMode(QString m);
	static QString ModeToString(DEMODMODE dm);

    struct DemodInfo {
        DEMODMODE mode;
        qint32 lowCutMin; //Low bandpass
        qint32 highCutMax; //High bandpass
        qint32 maxBandWidth; //for specified mode
        //CuteSDR has other settings for variable bp filter, agc settings, etc which we don't use yet
    };
    //Must be in same order as DEMODMODE
    //Verified with CuteSDR values
    const DemodInfo demodInfo[13] = {
        {dmAM,          -10000,     10000,      48000},
        {dmSAM,         -10000,     10000,      48000},
        {dmFMN,         -15000,     15000,      48000}, //Check FM
        {dmFMMono,      -15000,     15000,      48000},
        {dmFMStereo,    -100000,    100000,     100000},
        {dmDSB,         -10000,     10000,      48000},
        {dmLSB,         -20000,     0,          20000},
        {dmUSB,         0,          20000,      20000},
        {dmCWL,         -1000,      1000,       1000}, //Check CW
        {dmCWU,         -1000,      1000,       1000},
        {dmDIGL,        -20000,     0,          20000},
        {dmDIGU,        0,          20000,      20000},
        {dmNONE,        0,          0,          0}

    };


    void SetBandwidth(double bandwidth);
signals:
    void BandData(char *status, char *callSign, char *callSignData);

private slots:
    void OutputBandData(char *status, char *callSign, char *callSignData);

private:
    Demod_AM *demodAM;

    DEMODMODE mode;
    Ui::dataBand *dataUi;
    bool outputOn;

    //Testing
    CWFmDemod *wfmDemod;
    CRdsDecode rdsDecode;
    char rdsBuf[256]; //Formatted RDS string for output
    char rdsCallString[10];
    char rdsString[128]; //Max RDS string
    bool rdsUpdate; //true if we need to update display


    void FMDeemphasisFilter(int _bufSize, CPX *in, CPX *out);
    float fmDeemphasisAlpha;
    static const float usDeemphasisTime; //Use for US & Korea FM
    static const float intlDeemphasisTime;  //Use for international FM

    //CFir fmMonoLPFilter;
    CIir fmMonoLPFilter;
    CFir fmAudioLPFilter;
    CIir fmPilotNotchFilter;
    CIir fmPilotBPFilter;
    CFir hilbertFilter;

    //Used for FMW only for now
    int inputSampleRate; //Original signal coming in
    int inputWfmSampleRate; //Original signal coming in for Wfm (wider signal)

    int audioSampleRate; //Audio out

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

