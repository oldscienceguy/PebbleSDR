//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "filesdrdevice.h"
#include "QFileDialog"
#include "pebblelib_global.h"

PebbleLibGlobal *pebbleLibGlobal;

FileSDRDevice::FileSDRDevice():DeviceInterfaceBase()
{
    InitSettings("WavFileSDR");
    copyTest = false; //Write what we read
    fileName = "";
    recordingPath = "";
	pebbleLibGlobal = new PebbleLibGlobal();
}

FileSDRDevice::~FileSDRDevice()
{
    Stop();
    Disconnect();
}

bool FileSDRDevice::Initialize(cbProcessIQData _callback,
							   cbProcessBandscopeData _callbackBandscope,
							   cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	DeviceInterfaceBase::Initialize(_callback, _callbackBandscope, _callbackAudio, _framesPerBuffer);
    producerConsumer.Initialize(std::bind(&FileSDRDevice::producerWorker, this, std::placeholders::_1),
		std::bind(&FileSDRDevice::consumerWorker, this, std::placeholders::_1),50,framesPerBuffer * sizeof(CPX));

    return true;
}

bool FileSDRDevice::Connect()
{

    //Use last recording path and filename for convenience
    if (recordingPath.isEmpty()) {
		recordingPath = pebbleLibGlobal->appDirPath;
        //QT QTBUG-35779 Trailing '*' is required to workaround QT 5.2 bug where directory arg was ignored
		recordingPath += "/PebbleRecordings/*";
    }

#if 1
    //Passing NULL for dir shows current/last directory, which may be inside the mac application bundle
    //Mavericks native file open dialog doesn't respect passed directory arg, QT (Non-native) version works, but is ugly
    fileName = QFileDialog::getOpenFileName(NULL,tr("Open Wave File"), recordingPath, tr("Wave Files (*.wav)"));
    //fileName = QFileDialog::getOpenFileName(NULL,tr("Open Wave File"), recordingPath, tr("Wave Files (*.wav)"),0,QFileDialog::DontUseNativeDialog);

#else
    //Use this instead of static if we need more options
    QFileDialog fDialog;
    fDialog.setLabelText(tr("Open Wave File"));
    fDialog.setFilter(tr("Wave Files (*.wav)"));
    fDialog.setDirectory(recordingPath);
    QStringList files;
    if (fDialog.exec())
        files = fDialog.selectedFiles();

    fDialog.close();

    fileName = files[0];
#endif

    if (fileName == NULL)
        return false; //User canceled out of dialog

    //Save recordingPath for later use
    QFileInfo fi(fileName);
    recordingPath = fi.path()+"/*";

	bool res = wavFileRead.OpenRead(fileName, framesPerBuffer);
    if (!res)
        return false;

	deviceSampleRate = wavFileRead.GetSampleRate();
	//We have sample rate for file, set polling interval
	//We don't use producer thread, sampleRateTimer and producerSlot() instead
	producerConsumer.SetProducerInterval(deviceSampleRate, framesPerBuffer);
	producerConsumer.SetConsumerInterval(deviceSampleRate, framesPerBuffer);

    if (copyTest) {
		res = wavFileWrite.OpenWrite(fileName + "2", deviceSampleRate,0,0,0);
    }
    return true;
}

bool FileSDRDevice::Disconnect()
{
    wavFileRead.Close();
    wavFileWrite.Close();
    return true;
}

void FileSDRDevice::Start()
{
	//How often do we need to read samples from files to get framesPerBuffer at sampleRate
	nsPerBuffer = (1000000000.0 / deviceSampleRate) * framesPerBuffer;
	//qDebug()<<"nsPerBuffer"<<nsPerBuffer;

	producerConsumer.Start(true,true);

	elapsedTimer.start();
}

void FileSDRDevice::Stop()
{
    producerConsumer.Stop();
}

void FileSDRDevice::ReadSettings()
{

	DeviceInterfaceBase::ReadSettings();

	fileName = qSettings->value("FileName", "").toString();
	recordingPath = qSettings->value("RecordingPath", "").toString();
}

void FileSDRDevice::WriteSettings()
{
	DeviceInterfaceBase::WriteSettings();

	qSettings->setValue("FileName", fileName);
	qSettings->setValue("RecordingPath", recordingPath);
}

QVariant FileSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case PluginName:
			return "WAV File SDR";
			break;
		case PluginDescription:
			return "Plays back I/Q WAV file";
			break;
		case DeviceName:
			return "SDRFile: " + QFileInfo(fileName).fileName() + "-" + QString::number(deviceSampleRate);
		case DeviceType:
			return IQ_DEVICE;
		case StartupType:
			return DeviceInterface::DEFAULTFREQ; //Fixed, can't change freq
		case HighFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return deviceSampleRate;
			else
				return loFreq + deviceSampleRate / 2.0;
		}
		case LowFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return 0;
			else
				return loFreq - deviceSampleRate / 2.0;
		}
		case StartupFrequency: {
			//If it's a pebble wav file, we should have LO freq
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return deviceSampleRate / 2.0; //Default
			else
				return loFreq;
			break;
		}
		case StartupDemodMode: {
			int startupMode =  wavFileRead.GetMode();
			if (startupMode < 255)
				return startupMode;
			else
				return lastDemodMode;

			break;
		}
		default:
			//If we don't handle it, let default grab it
			return DeviceInterfaceBase::Get(_key, _option);
			break;

	}
}

bool FileSDRDevice::Set(STANDARD_KEYS _key, QVariant _value, QVariant _option)
{
	Q_UNUSED(_option);

	switch (_key) {
		case DeviceFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return Get(DeviceInterface::StartupFrequency).toDouble();
			else
				return loFreq;
		}
		default:
			return DeviceInterfaceBase::Set(_key, _value, _option);

	}
}

void FileSDRDevice::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);

	//Nothing to do
}

//Producer is not currently used, here for reference
void FileSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
	switch (_event) {
		case cbProducerConsumerEvents::Start:
			break;

		case cbProducerConsumerEvents::Run:
			producerSlot();
			break;

		case cbProducerConsumerEvents::Stop:
			break;
	}

}

//Called by timer at fastest rate, we use timer to make sure events are processed between samples
void FileSDRDevice::producerSlot()
{
	//qDebug()<<elapsedTimer.restart();

	timespec req, rem;
	//Note: timer(0) averages 30,000ns between calls to this slot
	//Govern output rate to match sampleRate, same as if actual device was outputing samples
	qint64 nsRemaining = nsPerBuffer - elapsedTimer.nsecsElapsed();
	if (nsRemaining > 0) {
		req.tv_sec = 0;
		//We want to get close to exact time, but not go over
		//So we sleep for 1/2 of the remaining time each sequence
		req.tv_nsec = nsRemaining;
		if (nanosleep(&req,&rem) < 0) {
			qDebug()<<"nanosleep failed";
		}
	}
	elapsedTimer.start(); //Restart elapsed timer


	//pebbleLibGlobal->perform.StartPerformance("Wav file producer");
	int samplesRead;

	//Fill one buffer with data
	if ((producerBuf = (CPX*)producerConsumer.AcquireFreeBuffer()) == NULL)
		return;

	samplesRead = wavFileRead.ReadSamples(producerBuf,framesPerBuffer);
	for (int i=0; i<framesPerBuffer; i++) {
		normalizeIQ(&producerBuf[i],producerBuf[i]);
	}

	//ProcessIQData(producerBuf,framesPerBuffer);

	producerConsumer.ReleaseFilledBuffer();
	//pebbleLibGlobal->perform.StopPerformance(1000);
}

void FileSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
    CPX *bufPtr;
    switch (_event) {
        case cbProducerConsumerEvents::Start:
            break;

        case cbProducerConsumerEvents::Run:

			//We always want to consume everything we have, producer will eventually block if we're not consuming fast enough
			while (producerConsumer.GetNumFilledBufs() > 0) {

				if ((bufPtr = (CPX*)producerConsumer.AcquireFilledBuffer()) == NULL)
					return;

				if (copyTest)
					wavFileWrite.WriteSamples(bufPtr, framesPerBuffer);

				ProcessIQData(bufPtr,framesPerBuffer);
				producerConsumer.ReleaseFreeBuffer();
			}
            break;

        case cbProducerConsumerEvents::Stop:
            break;
    }

}

