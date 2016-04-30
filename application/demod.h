#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include "QObject"
#include "processstep.h"
#include <iostream>
#include <QString>
#include "fir.h"
#include "iir.h"
#include "demod/rdsdecode.h"
#include "ui_data-band.h"
#include "device_interfaces.h"

class Demod_AM;
class Demod_SAM;
class Demod_WFM;
class Demod_NFM;

class Demod : public ProcessStep
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
	Demod(quint32 _sampleRate, quint32 _bufferSize);
	Demod(quint32 _sampleRate, quint32 _wfmSampleRate, quint32 _bufferSize);
	~Demod();

    void SetupDataUi(QWidget *parent);

    CPX * ProcessBlock(CPX * in, int _numSamples);
	DeviceInterface::DemodMode DemodMode() const;
    void ResetDemod(); //Resets all data decoders, called after frequency change from receiver
	void SetDemodMode(DeviceInterface::DemodMode mode, int _sourceSampleRate, int _audioSampleRate);
	static DeviceInterface::DemodMode StringToMode(QString m);
	static QString ModeToString(DeviceInterface::DemodMode dm);

    struct DemodInfo {
		DeviceInterface::DemodMode mode;
        QStringList filters;
        qint32 defaultFilter;
        qint32 lowCutMin; //Low bandpass
        qint32 highCutMax; //High bandpass
        qint32 maxOutputBandWidth; //for specified mode - used to set decimation rate in receiver.cpp
        qint16 agcSlope;
        qint16 agcKneeDb;
        qint16 agcDecayMs;
    };
    static const DemodInfo demodInfo[];


    void SetBandwidth(double bandwidth);
signals:
    void BandData(QString status, QString callSign, QString shortData, QString longData);

public slots:
    void OutputBandData(QString status, QString callSign, QString shortData, QString longData);

private:
    Demod_AM *demodAM;
    Demod_SAM *demodSAM;
    Demod_WFM *demodWFM;
    Demod_NFM *demodNFM;

	DeviceInterface::DemodMode mode;
    Ui::dataBand *dataUi;
    bool outputOn;

    CRdsDecode rdsDecode;
    char rdsBuf[256]; //Formatted RDS string for output
    char rdsCallString[10];
    char rdsString[128]; //Max RDS string
    bool rdsUpdate; //true if we need to update display

    //Used for FMW only for now
    int inputSampleRate; //Original signal coming in
    int inputWfmSampleRate; //Original signal coming in for Wfm (wider signal)

    int audioSampleRate; //Audio out

    void FMMono(CPX * in, CPX * out, int _numSamples);
    void FMStereo(CPX * in, CPX * out, int _numSamples);

	//Simple time domain demod algorithms for testing and comparison
    void SimpleUSB(CPX *in, CPX *out, int _numSamples);
    void SimpleLSB(CPX *in, CPX *out, int _numSamples);
};

