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

	void setupDataUi(QWidget *parent);

	CPX * processBlock(CPX * in, int _numSamples);
	DeviceInterface::DemodMode demodMode() const;
	void resetDemod(); //Resets all data decoders, called after frequency change from receiver
	void setDemodMode(DeviceInterface::DemodMode m_mode, int _sourceSampleRate, int _audioSampleRate);
	static DeviceInterface::DemodMode stringToMode(QString m);
	static QString modeToString(DeviceInterface::DemodMode dm);

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


	void setBandwidth(double bandwidth);
signals:
	void bandData(QString status, QString callSign, QString shortData, QString longData);

public slots:
	void outputBandData(QString status, QString callSign, QString shortData, QString longData);

private:
	Demod_AM *m_demodAM;
	Demod_SAM *m_demodSAM;
	Demod_WFM *m_demodWFM;
	Demod_NFM *m_demodNFM;

	DeviceInterface::DemodMode m_mode;
	Ui::dataBand *m_dataUi;
	bool m_outputOn;

	CRdsDecode m_rdsDecode;
	char m_rdsBuf[256]; //Formatted RDS string for output
	char m_rdsCallString[10];
	char m_rdsString[128]; //Max RDS string
	bool m_rdsUpdate; //true if we need to update display

    //Used for FMW only for now
	int m_inputSampleRate; //Original signal coming in
	int m_inputWfmSampleRate; //Original signal coming in for Wfm (wider signal)

	int m_audioSampleRate; //Audio out

	void fmMono(CPX * in, CPX * out, int _numSamples);
	void fmStereo(CPX * in, CPX * out, int _numSamples);

	//Simple time domain demod algorithms for testing and comparison
	void simpleUSB(CPX *in, CPX *out, int _numSamples);
	void simpleLSB(CPX *in, CPX *out, int _numSamples);
};

