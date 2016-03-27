#include "sdrplaydevice.h"
#include "db.h"

SDRPlayDevice::SDRPlayDevice():DeviceInterfaceBase()
{
	InitSettings("SDRPlay");
	optionUi = NULL;
	packetIBuf = packetQBuf = NULL;
	samplesPerPacket = 0;
	apiVersion = 0; //Not set


}

SDRPlayDevice::~SDRPlayDevice()
{
	if (packetIBuf != NULL)
		delete[] packetIBuf;
	if (packetQBuf != NULL)
		delete[] packetQBuf;
}

bool SDRPlayDevice::Initialize(cbProcessIQData _callback,
								  cbProcessBandscopeData _callbackBandscope,
								  cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	numProducerBuffers = 100;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(CPX);
	readBufferSize = framesPerBuffer * sampleDataSize;

	packetIBuf = new short[framesPerBuffer * 2]; //2X what we need so we have overflow space
	packetQBuf = new short[framesPerBuffer * 2];
	producerIndex = 0;

	producerConsumer.Initialize(std::bind(&SDRPlayDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&SDRPlayDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	producerConsumer.SetProducerInterval(deviceSampleRate,framesPerBuffer);
	producerConsumer.SetConsumerInterval(deviceSampleRate,framesPerBuffer);

#endif

	//Other constructor like init
	sampleRateMhz = deviceSampleRate / 1000000.0; //Sample rate in Mhz, NOT Hz
	producerFreeBufPtr = NULL;
	return true;
}

bool SDRPlayDevice::errorCheck(mir_sdr_ErrT err)
{
	switch (err) {
		case mir_sdr_Success:
			return true;
			break;
		case mir_sdr_Fail:
			return false;
			break;
		case mir_sdr_InvalidParam:
			qDebug()<<"SDRPLay InvalidParam error";
			break;
		case mir_sdr_OutOfRange:
			qDebug()<<"SDRPlay OutOfRange error";
			break;
		case mir_sdr_GainUpdateError:
			qDebug()<<"SDRPlay GainUpdate error";
			break;
		case mir_sdr_RfUpdateError:
			qDebug()<<"SDRPlay RfUpdate error";
			break;
		case mir_sdr_FsUpdateError:
			qDebug()<<"SDRPlay FsUpdate error";
			break;
		case mir_sdr_HwError:
			qDebug()<<"SDRPlay Hw error";
			break;
		case mir_sdr_AliasingError:
			qDebug()<<"SDRPlay Aliasing error";
			return true; //Ignore for now
			break;
		case mir_sdr_AlreadyInitialised:
			qDebug()<<"SDRPlay AlreadyInitialized error";
			break;
		case mir_sdr_NotInitialised:
			qDebug()<<"SDRPlay NotInitialized error";
			break;
		default:
			qDebug()<<"Unknown SDRPlay error";
			break;
	}
	return false;
}

bool SDRPlayDevice::Command(DeviceInterface::STANDARD_COMMANDS _cmd, QVariant _arg)
{
	switch (_cmd) {
		case CmdConnect:
			DeviceInterfaceBase::Connect();
			//Device specific code follows
			//Check version
			if (!errorCheck(mir_sdr_ApiVersion(&apiVersion)))
				return false;
			qDebug()<<"SDRPLay Version: "<<apiVersion;

			currentBand = band0; //Initial value to force band search on first frequency check

			//Will fail if SDRPlay is not connected
			if (!reinitMirics(deviceFrequency))
				return false;
			connected = true;
			running = false;
			optionParent = NULL;

			dbFSChanged(dbFS); //Init all the AGC values
			return true;

		case CmdDisconnect:
			DeviceInterfaceBase::Disconnect();
			//Device specific code follows
			if (!errorCheck(mir_sdr_Uninit()))
				return false;
			connected = false;
			return true;

		case CmdStart:
			DeviceInterfaceBase::Start();
			//Device specific code follows
			running = true;
			producerConsumer.Start(true,true);

			return true;

		case CmdStop:
			DeviceInterfaceBase::Stop();
			//Device specific code follows
			running = false;
			producerConsumer.Stop();
			return true;

		case CmdReadSettings:
			DeviceInterfaceBase::ReadSettings();
			ReadSettings();
			return true;

		case CmdWriteSettings:
			DeviceInterfaceBase::WriteSettings();
			WriteSettings();
			return true;

		case CmdDisplayOptionUi:						//Arg is QWidget *parent
			//Use QVariant::fromValue() to pass, and value<type passed>() to get back
			this->SetupOptionUi(_arg.value<QWidget*>());
			return true;

		default:
			return false;
	}
}

QVariant SDRPlayDevice::Get(DeviceInterface::STANDARD_KEYS _key, QVariant _option)
{
	Q_UNUSED(_option);
	QStringList sl;

	switch (_key) {
		case PluginName:
			return "SDRPlay";
			break;
		case PluginDescription:
			return "SDRPlay (Mirics chips)";
			break;
		case DeviceName:
			return "SDRPlay";
		case DeviceType:
			return IQ_DEVICE;
		case DeviceSampleRates:
			//Don't return any sample rates to sdrOptions UI, we handle it all in device UI
			return sl;
			break;
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool SDRPlayDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);
	switch (_key) {
		case DeviceFrequency:
			return setFrequency(_value.toDouble());

		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);
	}
}

void SDRPlayDevice::ReadSettings()
{
	//DeviceInterfaceBase takes care of reading and writing these values
	lowFrequency = 100000;
	highFrequency = 2000000000;
	deviceFrequency = lastFreq = 10000000;
	deviceSampleRate = 2000000;
	normalizeIQGain = 1.0;
	//normalizeIQGain = DB::dbToAmplitude(-14.0);

	DeviceInterfaceBase::ReadSettings();
	dcCorrectionMode = qSettings->value("dcCorrectionMode",0).toInt(); //0 = off
	totalGainReduction = qSettings->value("totalGainReduction",60).toInt(); //60db
	bandwidthKhz = (mir_sdr_Bw_MHzT) qSettings->value("bandwidthKhz",mir_sdr_BW_1_536).toInt();
	//bandwidth can not be > sampleRate which is a problem is ini is manually edited

	IFKhz = (mir_sdr_If_kHzT) qSettings->value("IFKhz",mir_sdr_IF_Zero).toInt();

	agcEnabled = qSettings->value("agcEnabled",false).toBool();
	dbFS = qSettings->value("dbFS",-15).toInt();
}

void SDRPlayDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	qSettings->setValue("dcCorrectionMode",dcCorrectionMode);
	qSettings->setValue("totalGainReduction",totalGainReduction);
	qSettings->setValue("bandwidthKhz",bandwidthKhz);
	qSettings->setValue("IFKhz",IFKhz);
	qSettings->setValue("agcEnabled",agcEnabled);
	qSettings->setValue("dbFS",dbFS);

	qSettings->sync();
}

void SDRPlayDevice::SetupOptionUi(QWidget *parent)
{
	int cur;

	//Arg is QWidget *parent
	if (optionUi != NULL) {
		delete optionUi;
		optionUi = NULL;
	}

	optionParent = parent;

	//Change .h and this to correct class name for ui
	optionUi = new Ui::SDRPlayOptions();
	optionUi->setupUi(parent);
	parent->setVisible(true);

	//Set combo boxes
	optionUi->sampleRate->addItem("1Mhz",1000000);
	optionUi->sampleRate->addItem("2Mhz",2000000);
	optionUi->sampleRate->addItem("3Mhz",3000000);
	optionUi->sampleRate->addItem("4Mhz",4000000);
	optionUi->sampleRate->addItem("5Mhz",5000000);
	optionUi->sampleRate->addItem("6Mhz",6000000);
	optionUi->sampleRate->addItem("7Mhz",7000000);
	optionUi->sampleRate->addItem("8Mhz",8000000);
	optionUi->sampleRate->addItem("9Mhz",9000000);
	optionUi->sampleRate->addItem("10Mhz",10000000);
	cur = optionUi->sampleRate->findData(deviceSampleRate);
	optionUi->sampleRate->setCurrentIndex(cur);
	connect(optionUi->sampleRate,SIGNAL(currentIndexChanged(int)),this,SLOT(sampleRateChanged(int)));

	matchBandwidthToSampleRate(true);
	connect(optionUi->IFBw,SIGNAL(currentIndexChanged(int)),this,SLOT(IFBandwidthChanged(int)));

	matchIFModeToSRAndBW();
	connect(optionUi->IFMode,SIGNAL(currentIndexChanged(int)),this,SLOT(IFModeChanged(int)));


	optionUi->dcCorrection->addItem("Off", 0);
	optionUi->dcCorrection->addItem("One Shot", 4);
	optionUi->dcCorrection->addItem("Continuous", 5);
	cur = optionUi->dcCorrection->findData(dcCorrectionMode);
	optionUi->dcCorrection->setCurrentIndex(cur);
	connect(optionUi->dcCorrection,SIGNAL(currentIndexChanged(int)),this,SLOT(dcCorrectionChanged(int)));

	optionUi->totalGainReduction->setValue(totalGainReduction);
	connect(optionUi->totalGainReduction, SIGNAL(valueChanged(int)), this, SLOT(totalGainReductionChanged(int)));

	optionUi->agcEnabled->setChecked(agcEnabled);
	connect(optionUi->agcEnabled,SIGNAL(clicked(bool)),this,SLOT(agcEnabledChanged(bool)));

	optionUi->dbFS->setMaximum(0);
	optionUi->dbFS->setMinimum(-50);
	optionUi->dbFS->setValue(dbFS);
	connect(optionUi->dbFS,SIGNAL(valueChanged(int)),this,SLOT(dbFSChanged(int)));

	if (apiVersion == 0)
		optionUi->apiVersion->setText("API Version");
	else
		optionUi->apiVersion->setText("API Version " + QString::number(apiVersion));

}

void SDRPlayDevice::dcCorrectionChanged(int _item)
{
	int cur = _item;
	dcCorrectionMode = optionUi->dcCorrection->itemData(cur).toUInt();
	setDcMode(dcCorrectionMode, 1);
	WriteSettings();
}

void SDRPlayDevice::totalGainReductionChanged(int _value)
{
	if (agcEnabled)
		return; //AGC managing

	setGainReduction(_value, 1, 0);
	WriteSettings();
}

void SDRPlayDevice::IFBandwidthChanged(int _item)
{
	int cur = _item;
	bandwidthKhz = (mir_sdr_Bw_MHzT)optionUi->IFBw->itemData(cur).toUInt();
	matchIFModeToSRAndBW();
	WriteSettings();
	reinitMirics(deviceFrequency);
}

void SDRPlayDevice::sampleRateChanged(int _item)
{
	deviceSampleRate = (quint32)optionUi->sampleRate->itemData(_item).toUInt();
	//Update dependent bandwidth options
	matchBandwidthToSampleRate(false);
	matchIFModeToSRAndBW();
	WriteSettings();
}

void SDRPlayDevice::agcEnabledChanged(bool _b)
{
	agcEnabled = _b;
}

void SDRPlayDevice::dbFSChanged(int _value)
{
	//We get value from - to zero from UI
	dbFS = _value;
	//Convert dBfs to power
	//pow(10, db/10.0); //From numerous references in DB class
	agcPwrSetpoint = DB::dBToPower(dbFS);
	//Mirics AGC note
	//power = Antilog(dbFS/10)
	//Antilog = log(n)^10
	//double dbFSPower = pow(10, log10(dbFS/10); //reduces to just dbFS/10

	//Define agc window
	agcPwrSetpointHigh = DB::dBToPower(dbFS + 2);
	agcPwrSetpointLow = DB::dBToPower(dbFS - 2);
}

void SDRPlayDevice::IFModeChanged(int _item)
{
	//We know that IF is valid for SR because we limit the choices whenever SR or BW is changed
	IFKhz = (mir_sdr_If_kHzT)optionUi->IFMode->itemData(_item).toUInt();
	WriteSettings();
	reinitMirics(deviceFrequency);  //IF mode only changed by init()
}

//Called with SR or BW changes to make sure IF mode options are within range
void SDRPlayDevice::matchIFModeToSRAndBW()
{
	mir_sdr_If_kHzT newMode;
	quint32 sampleRateKhz = deviceSampleRate / 1000;

	optionUi->IFMode->blockSignals(true);
	optionUi->IFMode->clear();
	optionUi->IFMode->addItem("IF Zero",mir_sdr_IF_Zero);
	//Make sure SR and Bandwidth are sufficient (not in API spec, discovered by mir_sdr_api log
	//Min sample rate = bandwidth + (2 * IF mode)
	//9.096 = 5.000 + 2 * 2.048

	newMode = IFKhz;
	if (sampleRateKhz < bandwidthKhz + 2 * mir_sdr_IF_0_450) {
		newMode = mir_sdr_IF_Zero;
	} else if (sampleRateKhz >= bandwidthKhz + 2 * mir_sdr_IF_0_450 &&
			   sampleRateKhz < bandwidthKhz + 2 * mir_sdr_IF_1_620) {
		optionUi->IFMode->addItem("IF 450",mir_sdr_IF_0_450);
		if (IFKhz > mir_sdr_IF_0_450)
			newMode = mir_sdr_IF_0_450; //Lower IF
	} else if (sampleRateKhz >= bandwidthKhz + 2 * mir_sdr_IF_1_620 &&
			   sampleRateKhz < bandwidthKhz + 2 * mir_sdr_IF_2_048) {
		optionUi->IFMode->addItem("IF 450",mir_sdr_IF_0_450);
		optionUi->IFMode->addItem("IF 1.620",mir_sdr_IF_1_620);
		if (IFKhz > mir_sdr_IF_1_620)
			newMode = mir_sdr_IF_1_620;
	} else if (sampleRateKhz >= bandwidthKhz + 2 * mir_sdr_IF_2_048) {
		optionUi->IFMode->addItem("IF 450",mir_sdr_IF_0_450);
		optionUi->IFMode->addItem("IF 1.620",mir_sdr_IF_1_620);
		optionUi->IFMode->addItem("IF 2.048",mir_sdr_IF_2_048);
		//All IF modes are valid, so don't constrain
	}
	IFKhz = newMode;
	int cur = optionUi->IFMode->findData(IFKhz);
	optionUi->IFMode->setCurrentIndex(cur);

	optionUi->IFMode->blockSignals(false);

}

//Called when SR changes to make sure BW options are within range
//If called when SR changes in UI, then reset bw (preserveBandwidth == false)
//If called when setting up UI, then just check to make sure bw is within range (preserveBandwidth = true)
void SDRPlayDevice::matchBandwidthToSampleRate(bool preserveBandwidth)
{
	mir_sdr_Bw_MHzT newBandwidth;

	optionUi->IFBw->blockSignals(true);
	optionUi->IFBw->clear();

	optionUi->IFBw->addItem("0.200 Mhz",mir_sdr_BW_0_200);
	optionUi->IFBw->addItem("0.300 Mhz",mir_sdr_BW_0_300);
	optionUi->IFBw->addItem("0.600 Mhz",mir_sdr_BW_0_600);
	//Only allow BW selections that are <= sampleRate
	newBandwidth = bandwidthKhz;
	if (deviceSampleRate < 1536000) {
		newBandwidth = mir_sdr_BW_0_600;
	}
	if (deviceSampleRate >= 1536000 && deviceSampleRate < 5000000) {
		optionUi->IFBw->addItem("1.536 Mhz",mir_sdr_BW_1_536);
		if (!preserveBandwidth) {
			//Always set new bw to max for SR when SR changes
			newBandwidth = mir_sdr_BW_1_536;
		} else if (bandwidthKhz > mir_sdr_BW_1_536) {
			//else only reset if out of range
			newBandwidth = mir_sdr_BW_1_536;
		}
	}
	if (deviceSampleRate >= 5000000 && deviceSampleRate < 6000000) {
		optionUi->IFBw->addItem("1.536 Mhz",mir_sdr_BW_1_536);
		optionUi->IFBw->addItem("5.000 Mhz",mir_sdr_BW_5_000);
		if (!preserveBandwidth) {
			//Always set new bw to max for SR when SR changes
			newBandwidth = mir_sdr_BW_5_000;
		} else if (bandwidthKhz > mir_sdr_BW_5_000) {
			//else only reset if out of range
			newBandwidth = mir_sdr_BW_5_000;
		}
	}
	if (deviceSampleRate >= 6000000 && deviceSampleRate < 7000000) {
		optionUi->IFBw->addItem("1.536 Mhz",mir_sdr_BW_1_536);
		optionUi->IFBw->addItem("5.000 Mhz",mir_sdr_BW_5_000);
		optionUi->IFBw->addItem("6.000 Mhz",mir_sdr_BW_6_000);
		if (!preserveBandwidth) {
			//Always set new bw to max for SR when SR changes
			newBandwidth = mir_sdr_BW_6_000;
		} else if (bandwidthKhz > mir_sdr_BW_6_000) {
			//else only reset if out of range
			newBandwidth = mir_sdr_BW_6_000;
		}
	}
	if (deviceSampleRate >= 7000000 && deviceSampleRate < 8000000) {
		optionUi->IFBw->addItem("1.536 Mhz",mir_sdr_BW_1_536);
		optionUi->IFBw->addItem("5.000 Mhz",mir_sdr_BW_5_000);
		optionUi->IFBw->addItem("6.000 Mhz",mir_sdr_BW_6_000);
		optionUi->IFBw->addItem("7.000 Mhz",mir_sdr_BW_7_000);
		if (!preserveBandwidth) {
			//Always set new bw to max for SR when SR changes
			newBandwidth = mir_sdr_BW_7_000;
		} else if (bandwidthKhz > mir_sdr_BW_7_000) {
			//else only reset if out of range
			newBandwidth = mir_sdr_BW_7_000;
		}
	}
	if (deviceSampleRate >= 8000000) {
		optionUi->IFBw->addItem("1.536 Mhz",mir_sdr_BW_1_536);
		optionUi->IFBw->addItem("5.000 Mhz",mir_sdr_BW_5_000);
		optionUi->IFBw->addItem("6.000 Mhz",mir_sdr_BW_6_000);
		optionUi->IFBw->addItem("7.000 Mhz",mir_sdr_BW_7_000);
		optionUi->IFBw->addItem("8.000 Mhz",mir_sdr_BW_8_000);
		if (!preserveBandwidth) {
			//Always set new bw to max for SR when SR changes
			newBandwidth = mir_sdr_BW_8_000;
		} else if (bandwidthKhz > mir_sdr_BW_8_000) {
			//else only reset if out of range
			newBandwidth = mir_sdr_BW_8_000;
		}
	}
	bandwidthKhz = newBandwidth;

	//We know bw is within range because of checks above
	int cur = optionUi->IFBw->findData(bandwidthKhz);
	optionUi->IFBw->setCurrentIndex(cur);

	optionUi->IFBw->blockSignals(false);
}

//Initializes the mirics chips set
//Used initially and any time frequency changes to a new band
bool SDRPlayDevice::reinitMirics(double newFrequency)
{
	initInProgress.lock(); //Pause producer/consumer
	if (running) {
		//Unitinialize first
		if (!errorCheck(mir_sdr_Uninit())) {
			initInProgress.unlock();
			return false;
		}
	}
	if (!errorCheck(mir_sdr_Init(totalGainReduction, sampleRateMhz, newFrequency / 1000000.0, bandwidthKhz ,IFKhz , &samplesPerPacket))) {
		initInProgress.unlock();
		return false;
	}
	//Whenever we initialize, we also need to reset key values
	setDcMode(dcCorrectionMode, 1);

	initInProgress.unlock(); //re-start producer/consumer
	return true;
}

//gRdb = gain reduction in db (0 to 102 depending on band)
//abs = 0 Offset from current gr, abs = 1 Absolute
//syncUpdate = 0 Immedate, syncUpdate = 1 synchronous
bool SDRPlayDevice::setGainReduction(int gRdb, int abs, int syncUpdate)
{
	int newGainReduction;
	if (abs == 1)
		//Abs
		newGainReduction = gRdb;
	else
		//Offset
		newGainReduction = totalGainReduction + gRdb;

	//If didn't change
	if (newGainReduction == totalGainReduction)
		return true; //Nothing to do
	totalGainReduction = newGainReduction;
	Q_ASSERT(totalGainReduction < 100);
	if (errorCheck(mir_sdr_SetGr(gRdb, abs, syncUpdate))) {
		pendingGainReduction = true;
		//Update UI with new AGC values
		if (running && agcEnabled && optionParent != NULL && optionParent->isVisible()) {
			optionUi->totalGainReduction->setValue(totalGainReduction);
		}

		return true;
	} else {
		return false;
	}
}

bool SDRPlayDevice::setDcMode(int _dcCorrectionMode, int _speedUp)
{
	dcCorrectionMode = _dcCorrectionMode;
	if (errorCheck(mir_sdr_SetDcMode(dcCorrectionMode, _speedUp))) { //Speed up disabled (what is speed up?)
		return errorCheck(mir_sdr_SetDcTrackTime(63)); //Max Todo: review what this should be.  User adjustable?
	}
	return false;
}

bool SDRPlayDevice::setFrequency(double newFrequency)
{
	if (deviceFrequency == newFrequency || newFrequency == 0)
		return true;

	band newBand;
	quint16 setRFMode = 1; //0=apply freq as offset, 1=apply freq absolute
	quint16 syncUpdate = 0; //0=apply freq change immediately, 1=apply synchronously

#if 1
	//Bug?  With Mac API, I can't change freq within a band like 245-380 with absolute
	//If the new frequency is outside the current band, then we have to uninit and reinit in the new band
	if (newFrequency < currentBand.low || newFrequency > currentBand.high) {
		//Find new band
		if (newFrequency >= band1.low && newFrequency <= band1.high)
			newBand = band1;
		else if (newFrequency >= band2.low && newFrequency <= band2.high)
			newBand = band2;
		else if (newFrequency >= band3.low && newFrequency <= band3.high)
			newBand = band3;
		else if (newFrequency >= band4.low && newFrequency <= band4.high)
			newBand = band4;
		else if (newFrequency >= band5.low && newFrequency <= band5.high)
			newBand = band5;
		else if (newFrequency >= band6.low && newFrequency <= band6.high)
			newBand = band6;
		else {
			qDebug()<<"Frequency outside of bands";
			return false;
		}
		//Re-init with new band
		//Init takes freq in mhz
		if (!reinitMirics(newFrequency))
			return false;
		currentBand = newBand;

	} else {
		//SetRf takes freq in hz
		if (!errorCheck(mir_sdr_SetRf(newFrequency,setRFMode,syncUpdate))) {
			//Sometimes we get an error that previous update timed out, reset and try again
			mir_sdr_ResetUpdateFlags(false,true,false);
			if (!errorCheck(mir_sdr_SetRf(newFrequency,setRFMode,syncUpdate)))
				return false;
		}
	}
#else
	//Try offset logic from osmosdr
	//Higher freq = positive offset
	double delta = newFrequency - deviceFrequency;
	if (fabs(delta) < 10000.0) {
		if (!errorCheck(mir_sdr_SetRf(delta,0,syncUpdate)))
			return false;

	} else {
		if (!reinitMirics(newFrequency / 1000000.0))
			return false;
	}

#endif
	deviceFrequency = newFrequency;
	lastFreq = deviceFrequency;
	return true;

}


void SDRPlayDevice::producerWorker(cbProducerConsumerEvents _event)
{
	unsigned firstSampleNumber;
	//0 = no change, 1 = changed
	int gainReductionChanged;
	int rfFreqChanged;
	int sampleFreqChanged;
	bool reverseIQ = false; //Normally reversed from normal

#if 0
	static short maxSample = 0;
	static short minSample = 0;
#endif

	static quint32 lastSampleNumber = 0;
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;

		case cbProducerConsumerEvents::Run:
			if (!connected || !running)
				return;

			//Returns one packet (samplesPerPacket) of data int I and Q buffers
			//We want to read enough data to fill producerIbuf and producerQbuf with framesPerBuffer
			while(running) {
				//If init is in progress (locked), wait for it to complete
				initInProgress.lock();

				//Read all the I's into 1st part of buffer, and Q's into 2nd part
				if (!errorCheck(mir_sdr_ReadPacket(packetIBuf, packetQBuf, &firstSampleNumber, &gainReductionChanged,
					&rfFreqChanged, &sampleFreqChanged))) {
					initInProgress.unlock();
					qDebug()<<"Error: Partial read";
					return; //Handle error
				}
				initInProgress.unlock();

				if (lastSampleNumber != 0 && firstSampleNumber > lastSampleNumber &&
						firstSampleNumber != lastSampleNumber + samplesPerPacket) {
					qDebug()<<"Lost samples "<< lastSampleNumber<<" "<<firstSampleNumber<<" "<<samplesPerPacket;
				}
				lastSampleNumber = firstSampleNumber;

				if (gainReductionChanged) {
					pendingGainReduction = false;
				}
#if 0
				if (rfFreqChanged) {
					//If center freq changed since last packet, throw this one away and get next one
					//Should make frequency changes instant, regardless of packet size
					continue;
				}
#endif
				if (producerFreeBufPtr == NULL) {
					if ((producerFreeBufPtr = (CPX *)producerConsumer.AcquireFreeBuffer()) == NULL) {
						qDebug()<<"No free buffers available.  producerIndex = "<<producerIndex <<
								  "samplesPerPacket = "<<samplesPerPacket;
						return;
					}
					producerIndex = 0;
					totalPwrInPacket = 0;
				}
				//Save in producerBuffer (sized to handle overflow
				//Make sure samplesPerPacket is initialized before producer starts

				qint32 samplesNeeded = framesPerBuffer - producerIndex;
				samplesNeeded = samplesPerPacket <= samplesNeeded ? samplesPerPacket : samplesNeeded;
				qint32 samplesExtra = samplesPerPacket - samplesNeeded;

				normalizeIQ(&producerFreeBufPtr[producerIndex], packetIBuf, packetQBuf, samplesNeeded, reverseIQ);
				producerIndex += samplesNeeded;

				if (producerIndex >= framesPerBuffer) {
#if 0
					double avgPwrInPacket = 0;
					//AGC Logic
					//Todo: AGC not working, review logic
					if (agcEnabled && !pendingGainReduction) {
						totalPwrInPacket = 0;
						for (int i=0; i<framesPerBuffer; i++) {
							//I^2 + Q^2
							totalPwrInPacket += producerFreeBufPtr[i].sqrMag();
						}
						avgPwrInPacket = totalPwrInPacket / framesPerBuffer;
						if (avgPwrInPacket < agcPwrSetpointLow || avgPwrInPacket > agcPwrSetpointHigh) {
							//Adjust gain reduction to get measured power into setpoint range
							double pwrDelta = agcPwrSetpoint - avgPwrInPacket;
							double dbDelta;
							if (pwrDelta < 0)
								//less power = more gain reduction
								dbDelta = DB::powerToDb(fabs(pwrDelta));
							else
								//More power = less gain reduction
								dbDelta = -DB::powerToDb(pwrDelta);

#if 0
							qDebug()<<"AGC low: "<<agcPwrSetpointLow<<
									  "AGC high: "<<agcPwrSetpointHigh<<
									  "AGC Avg: "<<avgPwrInPacket<<
									  "AGC GR: "<<newGainReduction;
#endif
							//Set in offset mode, driver keeps track of what prev setting was and will reduce GR accordingly
							setGainReduction(dbDelta,0,0);
						}
					}
#endif
					//Process any remaining samples in packet if any
					producerIndex = 0;
					producerFreeBufPtr = NULL;
					producerConsumer.ReleaseFilledBuffer();

					if (samplesExtra > 0) {
						//Start a new producer buffer
						if ((producerFreeBufPtr = (CPX *)producerConsumer.AcquireFreeBuffer()) == NULL) {
							qDebug()<<"No free buffers available.  producerIndex = "<<producerIndex <<
									  "samplesPerPacket = "<<samplesPerPacket;
							return;
						}
						normalizeIQ(&producerFreeBufPtr[producerIndex], &packetIBuf[samplesNeeded],
									&packetQBuf[samplesNeeded], samplesExtra, reverseIQ);
						producerIndex += samplesExtra;
					}
				}

			} //while(running)
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void SDRPlayDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	CPX *consumerFilledBufferPtr;

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			if (!connected || !running)
				return;

			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (running && producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((consumerFilledBufferPtr = (CPX *)producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//perform.StartPerformance("ProcessIQ");
				ProcessIQData(consumerFilledBufferPtr,framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				producerConsumer.ReleaseFreeBuffer();

			}
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}
