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

	connect(&sampleRateTimer,SIGNAL(timeout()),this, SLOT(producerSlot()));
	sampleRateTimer.setTimerType(Qt::PreciseTimer); //1ms resolution

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

	sampleRate = wavFileRead.GetSampleRate();
	//We have sample rate for file, set polling interval
	//We don't use producer thread, sampleRateTimer and producerSlot() instead
	//producerConsumer.SetProducerInterval(sampleRate, framesPerBuffer);
	producerConsumer.SetConsumerInterval(sampleRate, framesPerBuffer);

    if (copyTest) {
		res = wavFileWrite.OpenWrite(fileName + "2", sampleRate,0,0,0);
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
	quint16 msToFillBuffer = (1000.0 / sampleRate) * framesPerBuffer;
	qDebug()<<"msToFillBuffer"<<msToFillBuffer;
	//Note msToFillBuffer == 0 means timer will get triggered whenever there are no events
	//ie as fast as possible.  High CPU will result, but we may be able to keep up with higer SR

	producerConsumer.Start(false,true);
	sampleRateTimer.start(msToFillBuffer);
}

void FileSDRDevice::Stop()
{
	sampleRateTimer.stop();
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

QVariant FileSDRDevice::Get(DeviceInterface::STANDARD_KEYS _key, quint16 _option)
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
			return "SDRFile: " + QFileInfo(fileName).fileName() + "-" + QString::number(sampleRate);
		case DeviceType:
			return IQ_DEVICE;
		case DeviceSampleRate:
			return sampleRate;
		case StartupType:
			return DeviceInterface::DEFAULTFREQ; //Fixed, can't change freq
		case HighFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return sampleRate;
			else
				return loFreq + sampleRate / 2.0;
		}
		case LowFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return 0;
			else
				return loFreq - sampleRate / 2.0;
		}
		case StartupFrequency: {
			//If it's a pebble wav file, we should have LO freq
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return sampleRate / 2.0; //Default
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

bool FileSDRDevice::Set(STANDARD_KEYS _key, QVariant _value, quint16 _option)
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

//Called by timer to match sampleRate
void FileSDRDevice::producerSlot()
{
	//pebbleLibGlobal->perform.StartPerformance("Wav file producer");
	CPX *bufPtr;
	int samplesRead;
	//Fill one buffer with data
	if ((bufPtr = (CPX*)producerConsumer.AcquireFreeBuffer()) == NULL)
		return;
	samplesRead = wavFileRead.ReadSamples(bufPtr,framesPerBuffer);
	for (int i=0; i<framesPerBuffer; i++) {
		normalizeIQ(&bufPtr[i],bufPtr[i]);
	}
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

