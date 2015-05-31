#ifndef SDRPLAYDEVICE_H
#define SDRPLAYDEVICE_H

//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QObject>
#include "deviceinterfacebase.h"
#include "ui_sdrplayoptions.h"
#include "MiricsAPI/mir_sdr.h"

/*
 * Used the SDRPlay / Mirics libmir_sdr.so API library
 * Alternative open source libraries can be found at
 * git://git.osmocom.org/libmirisdr
 * https://code.google.com/p/libmirisdr-2/
 *
 * Use the mirics api evaluation tool which is installed with Window API download from SDRPlay for experiments
 */

class SDRPlayDevice : public QObject, public DeviceInterfaceBase
{
	Q_OBJECT

	//Exports, FILE is optional
	//IID must be same that caller is looking for, defined in interfaces file
	Q_PLUGIN_METADATA(IID DeviceInterface_iid)
	//Let Qt meta-object know about our interface
	Q_INTERFACES(DeviceInterface)

public:
	SDRPlayDevice();
	~SDRPlayDevice();

	//Required
	bool Initialize(cbProcessIQData _callback,
					cbProcessBandscopeData _callbackBandscope,
					cbProcessAudioData _callbackAudio,
					quint16 _framesPerBuffer);
	bool Command(STANDARD_COMMANDS _cmd, QVariant _arg);
	QVariant Get(STANDARD_KEYS _key, quint16 _option = 0);
	bool Set(STANDARD_KEYS _key, QVariant _value, quint16 _option = 0);
	void ReadSettings();
	void WriteSettings();
	//Display device option widget in settings dialog
	void SetupOptionUi(QWidget *parent);

private slots:
	void dcCorrectionChanged(int _item);
	void tunerGainReductionChanged(int _value);
	void IFBandwidthChanged(int _item);

private:
	struct band {
		double low;
		double high;
	};
	//SetRF only works if frequency is within one of these bands
	//Otherwise we have to uninit and re-init within the band
	const band band0 = {0, 0}; //Special case for initialization
	const band band1 = {100000, 60000000};
	const band band2 = {60000000, 120000000};
	const band band3 = {120000000, 245000000};
	const band band4 = {245000000, 380000000};
	const band band5 = {430000000, 1000000000};
	const band band6 = {1000000000, 2000000000};
	band currentBand;

	void producerWorker(cbProducerConsumerEvents _event);
	void consumerWorker(cbProducerConsumerEvents _event);
	//Work buffer for consumer to convert device format data to CPX Pebble format data
	CPX *consumerBuffer;
	short *producerIBuf;
	short *producerQBuf;
	quint16 producerIndex;

	Ui::SDRPlayOptions *optionUi;

	//SDRPlay data
	double sampleRateMhz;
	int samplesPerPacket; //Returned by init
	int tunerGainReduction;
	int dcCorrectionMode;

	mir_sdr_Bw_MHzT bandwidthKhz;
	mir_sdr_If_kHzT IFKhz;

	bool errorCheck(mir_sdr_ErrT err);
	bool setGainReduction(int gRdb, int abs, int syncUpdate);
	bool setFrequency(double newFrequency);
	bool setDcMode(int _dcCorrectionMode, int _speedUp);
	bool reinitMirics(double newFrequency);
};
#endif // SDRPLAYDEVICE_H
