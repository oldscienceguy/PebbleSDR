//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "filesdrdevice.h"
#include "QFileDialog"


FileSDRDevice::FileSDRDevice():DeviceInterfaceBase()
{
    InitSettings("WavFileSDR");
    copyTest = false; //Write what we read
    fileName = "";
    recordingPath = "";
}

FileSDRDevice::~FileSDRDevice()
{
    Stop();
    Disconnect();
}

bool FileSDRDevice::Initialize(cbProcessIQData _callback,
							   cbProcessSpectrumIQData _callbackSpectrum,
							   cbProcessAudioData _callbackAudio, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callbackSpectrum);
	Q_UNUSED(_callbackAudio);
	DeviceInterfaceBase::Initialize(_callback, _callbackSpectrum, _callbackAudio, _framesPerBuffer);
    ProcessIQData = _callback;
    framesPerBuffer = _framesPerBuffer;
    producerConsumer.Initialize(std::bind(&FileSDRDevice::producerWorker, this, std::placeholders::_1),
        std::bind(&FileSDRDevice::consumerWorker, this, std::placeholders::_1),1,framesPerBuffer * sizeof(CPX));

    return true;
}

bool FileSDRDevice::Connect()
{

    //Use last recording path and filename for convenience
    if (recordingPath.isEmpty()) {
        recordingPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        recordingPath.chop(25);
#endif
        //QT QTBUG-35779 Trailing '*' is required to workaround QT 5.2 bug where directory arg was ignored
        recordingPath += "PebbleRecordings/*";
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

    bool res = wavFileRead.OpenRead(fileName);
    if (!res)
        return false;

	//We have sample rate for file, set polling interval
	producerConsumer.SetProducerInterval(wavFileRead.GetSampleRate(), framesPerBuffer);
	producerConsumer.SetConsumerInterval(wavFileRead.GetSampleRate(), framesPerBuffer);

    if (copyTest) {
        res = wavFileWrite.OpenWrite(fileName + "2", wavFileRead.GetSampleRate(),0,0,0);
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
    producerConsumer.Start();
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
			return "SDRFile: " + QFileInfo(fileName).fileName() + "-" + QString::number(wavFileRead.GetSampleRate());
		case DeviceType:
			return INTERNAL_IQ;
		case DeviceSampleRate:
			return wavFileRead.GetSampleRate();
		case StartupType:
			return DeviceInterface::DEFAULTFREQ; //Fixed, can't change freq
		case HighFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return wavFileRead.GetSampleRate();
			else
				return loFreq + wavFileRead.GetSampleRate() / 2.0;
		}
		case LowFrequency: {
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return 0;
			else
				return loFreq - wavFileRead.GetSampleRate() / 2.0;
		}
		case StartupFrequency: {
			//If it's a pebble wav file, we should have LO freq
			quint32 loFreq = wavFileRead.GetLoFreq();
			if (loFreq == 0)
				return wavFileRead.GetSampleRate() / 2.0; //Default
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
		case IQGain:
			return 0.5;
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

void FileSDRDevice::producerWorker(cbProducerConsumerEvents _event)
{
    CPX *bufPtr;
    int samplesRead;
    switch (_event) {
        case cbProducerConsumerEvents::Start:
            break;

        case cbProducerConsumerEvents::Run:
            if ((bufPtr = (CPX*)producerConsumer.AcquireFreeBuffer()) == NULL)
                return;

            samplesRead = wavFileRead.ReadSamples(bufPtr,framesPerBuffer);
            producerConsumer.ReleaseFilledBuffer();

            break;

        case cbProducerConsumerEvents::Stop:
            break;
    }
}
void FileSDRDevice::consumerWorker(cbProducerConsumerEvents _event)
{
    CPX *bufPtr;
    switch (_event) {
        case cbProducerConsumerEvents::Start:
            break;

        case cbProducerConsumerEvents::Run:

            if ((bufPtr = (CPX*)producerConsumer.AcquireFilledBuffer()) == NULL)
                return;

            if (copyTest)
                wavFileWrite.WriteSamples(bufPtr, framesPerBuffer);

            ProcessIQData(bufPtr,framesPerBuffer);
            producerConsumer.ReleaseFreeBuffer();

            break;

        case cbProducerConsumerEvents::Stop:
            break;
    }

}

