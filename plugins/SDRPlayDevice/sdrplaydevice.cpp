#include "sdrplaydevice.h"

SDRPlayDevice::SDRPlayDevice():DeviceInterfaceBase()
{
	InitSettings("SDRPlay");
	optionUi = NULL;
	producerIBuf = producerQBuf = NULL;
	consumerBuffer = NULL;
	samplesPerPacket = 0;
}

SDRPlayDevice::~SDRPlayDevice()
{
	if (producerIBuf != NULL)
		delete[] producerIBuf;
	if (producerQBuf != NULL)
		delete[] producerQBuf;
	if (consumerBuffer != NULL)
		delete consumerBuffer;
}

bool SDRPlayDevice::Initialize(cbProcessIQData _callback,
								  cbProcessBandscopeData _callbackBandscope,
								  cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
	numProducerBuffers = 50;
#if 1
	//Remove if producer/consumer buffers are not used
	//This is set so we always get framesPerBuffer samples (factor in any necessary decimation)
	//ProducerConsumer allocates as array of bytes, so factor in size of sample data
	quint16 sampleDataSize = sizeof(short); //SDRPlay returns shorts
	readBufferSize = framesPerBuffer * sampleDataSize * 2; //2 samples per frame (I/Q)

	producerIBuf = new short[framesPerBuffer * 2]; //2X what we need so we have overflow space
	producerQBuf = new short[framesPerBuffer * 2];
	producerIndex = 0;

	consumerBuffer = CPXBuf::malloc(framesPerBuffer);

	producerConsumer.Initialize(std::bind(&SDRPlayDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&SDRPlayDevice::consumerWorker, this, std::placeholders::_1),numProducerBuffers, readBufferSize);
	//Must be called after Initialize
	producerConsumer.SetProducerInterval(sampleRate,readBufferSize);
	producerConsumer.SetConsumerInterval(sampleRate,readBufferSize);

#endif

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
	//
	//sampleRateMhz must be >= bandwidth
	sampleRateMhz = sampleRate / 1000000.0; //Sample rate in Mhz, NOT Hz
	//Receiver bandwidth, sampleRate must be >=
	bandwidthMhz = mir_sdr_BW_1_536;
	IFKhz = mir_sdr_IF_Zero;
	switch (_cmd) {
		case CmdConnect:
			DeviceInterfaceBase::Connect();
			//Device specific code follows
			//Check version
			float ver;
			if (!errorCheck(mir_sdr_ApiVersion(&ver)))
				return false;
			qDebug()<<"SDRPLay Version: "<<ver;

			currentBand = band0; //Initial value to force band search on first frequency check

			//setFrequency does an init if currentBand == band0.
			//Will fail if SDRPlay is not connected
			if (!setFrequency(10000000)) //Test, get startup freq to fix
				return false;
			connected = true;
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

QVariant SDRPlayDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
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
			//These correspond to SDRPlay IF Bandwidth options
			sl<<"200000";
			sl<<"300000";
			sl<<"600000";
			sl<<"1536000";
			sl<<"5000000";
			sl<<"6000000";
			sl<<"7000000";
			sl<<"8000000";
			return sl;
			break;
		case HighFrequency:
			return 2000000000;
		case LowFrequency:
			return 100000;
		default:
			return DeviceInterfaceBase::Get(_key, _option);
	}
}

bool SDRPlayDevice::Set(DeviceInterface::STANDARD_KEYS _key, QVariant _value, quint16 _option)
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
	DeviceInterfaceBase::ReadSettings();
	dcCorrectionMode = qSettings->value("dcCorrectionMode",0).toInt(); //0 = off
	tunerGainReduction = qSettings->value("tunerGainReduction",60).toInt(); //60db

}

void SDRPlayDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();
	qSettings->setValue("dcCorrectionMode",dcCorrectionMode);
	qSettings->setValue("tunerGainReduction",tunerGainReduction);

}

void SDRPlayDevice::SetupOptionUi(QWidget *parent)
{
	//Arg is QWidget *parent
	if (optionUi != NULL)
		delete optionUi;

	//Change .h and this to correct class name for ui
	optionUi = new Ui::SDRPlayOptions();
	optionUi->setupUi(parent);
	parent->setVisible(true);

	//Set combo boxes
	optionUi->IFMode->addItem("Zero",mir_sdr_IF_Zero);
	optionUi->IFMode->addItem("450 Khz",mir_sdr_IF_0_450);
	optionUi->IFMode->addItem("1620 Khz",mir_sdr_IF_1_620);
	optionUi->IFMode->addItem("2048 Khz",mir_sdr_IF_2_048);

	optionUi->IFBw->addItem("0.200 Mhz",mir_sdr_BW_0_200);
	optionUi->IFBw->addItem("0.300 Mhz",mir_sdr_BW_0_300);
	optionUi->IFBw->addItem("0.600 Mhz",mir_sdr_BW_0_600);
	optionUi->IFBw->addItem("1.536 Mhz",mir_sdr_BW_1_536);
	optionUi->IFBw->addItem("5.000 Mhz",mir_sdr_BW_5_000);
	optionUi->IFBw->addItem("6.000 Mhz",mir_sdr_BW_6_000);
	optionUi->IFBw->addItem("7.000 Mhz",mir_sdr_BW_7_000);
	optionUi->IFBw->addItem("8.000 Mhz",mir_sdr_BW_8_000);

	optionUi->dcCorrection->addItem("Off", 0);
	optionUi->dcCorrection->addItem("One Shot", 4);
	optionUi->dcCorrection->addItem("Continuous", 5);
	int cur = optionUi->dcCorrection->findData(dcCorrectionMode);
	optionUi->dcCorrection->setCurrentIndex(cur);
	connect(optionUi->dcCorrection,SIGNAL(currentIndexChanged(int)),this,SLOT(dcCorrectionChanged(int)));

	optionUi->tunerGainReduction->setValue(tunerGainReduction);
	connect(optionUi->tunerGainReduction, SIGNAL(valueChanged(int)), this, SLOT(tunerGainReductionChanged(int)));

}

void SDRPlayDevice::dcCorrectionChanged(int _item)
{
	int cur = _item;
	dcCorrectionMode = optionUi->dcCorrection->itemData(cur).toUInt();
	setDcMode(dcCorrectionMode, 1);
	WriteSettings();
}

void SDRPlayDevice::tunerGainReductionChanged(int _value)
{
	tunerGainReduction = _value;
	setGainReduction(tunerGainReduction, 1, 0);
	WriteSettings();
}

//gRdb = gain reduction in db
//abs = 0 Offset from current gr, abs = 1 Absolute
//syncUpdate = 0 Immedate, syncUpdate = 1 synchronous
bool SDRPlayDevice::setGainReduction(int gRdb, int abs, int syncUpdate)
{
	return (errorCheck(mir_sdr_SetGr(gRdb, abs, syncUpdate)));
}

bool SDRPlayDevice::setDcMode(int _dcCorrectionMode, int _speedUp)
{
	dcCorrectionMode = _dcCorrectionMode;
	return errorCheck(mir_sdr_SetDcMode(dcCorrectionMode, _speedUp)); //Speed up disabled (what is speed up?)

}

bool SDRPlayDevice::setFrequency(double newFrequency)
{
	band newBand;
	quint16 setRFMode = 1; //0=apply freq as offset, 1=apply freq absolute
	quint16 syncUpdate = 0; //0=apply freq change immediately, 1=apply synchronously

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
		//1st time through we haven't done init yet
		if (currentBand.low > 0 && !errorCheck(mir_sdr_Uninit()))
			return false;

		//Re-init with new band
		//Init takes freq in mhz
		if (!errorCheck(mir_sdr_Init(tunerGainReduction, sampleRateMhz, newFrequency / 1000000.0, bandwidthMhz ,IFKhz , &samplesPerPacket)))
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

	short *producerFreeBufPtr; //Treat as array of shorts

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;

		case cbProducerConsumerEvents::Run:
			if (!connected || !running)
				return;

			//Returns one packet (samplesPerPacket) of data int I and Q buffers
			//We want to read enough data to fill producerIbuf and producerQbuf with framesPerBuffer
			while(running) {
				//Read all the I's into 1st part of buffer, and Q's into 2nd part
				if (!errorCheck(mir_sdr_ReadPacket(&producerIBuf[producerIndex], &producerQBuf[producerIndex], &firstSampleNumber, &gainReductionChanged,
					&rfFreqChanged, &sampleFreqChanged))) {
					return; //Handle error
				}
				if (rfFreqChanged) {
					//If center freq changed since last packet, throw this one away and get next one
					//Should make frequency changes instant, regardless of packet size
					continue;
				}
				producerIndex += samplesPerPacket;
				if (producerIndex >= framesPerBuffer) {
					if ((producerFreeBufPtr = (short *)producerConsumer.AcquireFreeBuffer()) == NULL)
						return;
					for (int i=0, j=0; i<framesPerBuffer; i++, j+=2) {
						//Store in alternating I/Q
						producerFreeBufPtr[j] = producerIBuf[i];
						producerFreeBufPtr[j+1] = producerQBuf[i];
					}
					//Start new buffer, but producerIndex is not even multiple of framesPerBuffer
					//ie if samplesPerPacket = 252, then we will have some samples left over
					//after we process a frame.
					//So don't set to 0, just decrement what we handled
					producerIndex -= framesPerBuffer;
					producerConsumer.ReleaseFilledBuffer();
					//qDebug()<<"Released filled buffer";
					return;
				}
			}
			return;

			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}

void SDRPlayDevice::consumerWorker(cbProducerConsumerEvents _event)
{
	short *consumerFilledBufferPtr;
	double re;
	double im;

	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;
		case cbProducerConsumerEvents::Run:
			if (!connected || !running)
				return;

			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (running && producerConsumer.GetNumFilledBufs() > 0) {
				//Wait for data to be available from producer
				if ((consumerFilledBufferPtr = (short *)producerConsumer.AcquireFilledBuffer()) == NULL) {
					//qDebug()<<"No filled buffer available";
					return;
				}

				//Process data in filled buffer and convert to Pebble format in consumerBuffer
				//Filled buffers always have framesPerBuffer I/Q samples
				for (int i=0,j=0; i<framesPerBuffer; i++, j+=2) {
					//Fill consumerBuffer with normalized -1 to +1 data
					//SDRPlay I/Q data is 16bit integers 0 - 65535
					re = (consumerFilledBufferPtr[j] - 32767) / 32767.0;
					consumerBuffer[i].re = re;
					im = (consumerFilledBufferPtr[j+1] - 32767) / 32767.0;
					consumerBuffer[i].im = im;
					//qDebug()<<re<<" "<<im;
				}

				//perform.StartPerformance("ProcessIQ");
				ProcessIQData(consumerBuffer,framesPerBuffer);
				//perform.StopPerformance(1000);
				//We don't release a free buffer until ProcessIQData returns because that would also allow inBuffer to be reused
				producerConsumer.ReleaseFreeBuffer();

			}
			break;
		case cbProducerConsumerEvents::Stop:
			break;
	}
}
