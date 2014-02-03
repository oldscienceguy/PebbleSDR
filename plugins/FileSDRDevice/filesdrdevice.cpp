//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "filesdrdevice.h"
#include "QFileDialog"


FileSDRDevice::FileSDRDevice()
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

QString FileSDRDevice::GetPluginName(int _devNum)
{
    return "WAV File SDR";
}

QString FileSDRDevice::GetPluginDescription(int _devNum)
{
    return "Plays back I/Q WAV file";
}

bool FileSDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
    ProcessIQData = _callback;
    framesPerBuffer = _framesPerBuffer;
    producerConsumer.Initialize(this,1,framesPerBuffer * sizeof(CPX));

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

double FileSDRDevice::SetFrequency(double fRequested,double fCurrent)
{
    quint32 loFreq = wavFileRead.GetLoFreq();
    if (loFreq == 0)
        return GetStartupFrequency();
    else
        return loFreq;
}

void FileSDRDevice::ShowOptions()
{

}

void FileSDRDevice::ReadSettings()
{
    QSettings *qs = GetQSettings();

    //These are common settings for every device, variables are defined in DeviceInterface
    startup = (STARTUP)qs->value("Startup", DEFAULTFREQ).toInt();
    freqToSet = qs->value("StartupFreq", 10000000).toDouble();
    inputDeviceName = qs->value("InputDeviceName", "").toString();
    outputDeviceName = qs->value("OutputDeviceName", "").toString();
    sampleRate = qs->value("SampleRate", 48000).toInt();
    iqGain = qs->value("iqGain",1).toDouble();
    iqOrder = (IQORDER)qs->value("IQOrder", IQ).toInt();
    iqBalanceGain = qs->value("iqBalanceGain",1).toDouble();
    iqBalancePhase = qs->value("iqBalancePhase",0).toDouble();
    iqBalanceEnable = qs->value("iqBalanceEnable",false).toBool();
    lastFreq = qs->value("LastFreq", 10000000).toDouble();
    lastMode = qs->value("LastMode",0).toInt();
    lastDisplayMode = qs->value("LastDisplayMode",0).toInt();
    isTestBenchChecked = qs->value("TestBench",false).toBool();

    fileName = qs->value("FileName", "").toString();
    recordingPath = qs->value("RecordingPath", "").toString();
}

void FileSDRDevice::WriteSettings()
{
    QSettings *qs = GetQSettings();

    qs->setValue("Startup",startup);
    qs->setValue("StartupFreq",freqToSet);
    qs->setValue("InputDeviceName", inputDeviceName);
    qs->setValue("OutputDeviceName", outputDeviceName);
    qs->setValue("SampleRate",sampleRate);
    qs->setValue("iqGain",iqGain);
    qs->setValue("IQOrder", iqOrder);
    qs->setValue("iqBalanceGain", iqBalanceGain);
    qs->setValue("iqBalancePhase", iqBalancePhase);
    qs->setValue("iqBalanceEnable", iqBalanceEnable);
    qs->setValue("LastFreq",lastFreq);
    qs->setValue("LastMode",lastMode);
    qs->setValue("LastDisplayMode",lastDisplayMode);
    qs->setValue("TestBench",isTestBenchChecked);

    qs->setValue("FileName", fileName);
    qs->setValue("RecordingPath", recordingPath);
}

double FileSDRDevice::GetStartupFrequency()
{
    //If it's a pebble wav file, we should have LO freq
    quint32 loFreq = wavFileRead.GetLoFreq();
    if (loFreq == 0)
        return GetSampleRate() / 2.0; //Default
    else
        return loFreq;
}

int FileSDRDevice::GetStartupMode()
{
    int startupMode =  wavFileRead.GetMode();
    if (startupMode < 255)
        return startupMode;
    else
        return lastMode;
}

double FileSDRDevice::GetHighLimit()
{
    quint32 loFreq = wavFileRead.GetLoFreq();
    if (loFreq == 0)
        return GetSampleRate();
    else
        return loFreq + GetSampleRate() / 2.0;
}

double FileSDRDevice::GetLowLimit()
{
    quint32 loFreq = wavFileRead.GetLoFreq();
    if (loFreq == 0)
        return 0;
    else
        return loFreq - GetSampleRate() / 2.0;
}

double FileSDRDevice::GetGain()
{
    return 0.5;
}

QString FileSDRDevice::GetDeviceName()
{
    return "SDRFile: " + QFileInfo(fileName).fileName() + "-" + QString::number(GetSampleRate());
}

int FileSDRDevice::GetSampleRate()
{
    return wavFileRead.GetSampleRate();
}

int *FileSDRDevice::GetSampleRates(int &len)
{
    len = 0;
    return NULL;
}

bool FileSDRDevice::UsesAudioInput()
{
    return false;
}

bool FileSDRDevice::GetTestBenchChecked()
{
    return isTestBenchChecked;
}

void FileSDRDevice::SetupOptionUi(QWidget *parent)
{
    //Nothing to do
}

void FileSDRDevice::StopProducerThread(){}
void FileSDRDevice::RunProducerThread()
{
    CPX *bufPtr;
    if ((bufPtr = (CPX*)producerConsumer.AcquireFreeBuffer()) == NULL)
        return;

    int samplesRead = wavFileRead.ReadSamples(bufPtr,framesPerBuffer);
    producerConsumer.ReleaseFilledBuffer();

}
void FileSDRDevice::StopConsumerThread(){}
void FileSDRDevice::RunConsumerThread()
{
    CPX *bufPtr;

    if ((bufPtr = (CPX*)producerConsumer.AcquireFilledBuffer()) == NULL)
        return;

    if (copyTest)
        wavFileWrite.WriteSamples(bufPtr, framesPerBuffer);
    ProcessIQData(bufPtr,framesPerBuffer);
    producerConsumer.ReleaseFreeBuffer();
}

//These need to be moved to a DeviceInterface implementation class, equivalent to SDR
//Must be called from derived class constructor to work correctly
void FileSDRDevice::InitSettings(QString fname)
{
    //Use ini files to avoid any registry problems or install/uninstall
    //Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
    //Scope::SystemScope puts file c:\ProgramData\n1ddy

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    // RegExp for /Pebble.app/contents/macos
    int pos = path.lastIndexOf(QRegExp("/.+\\.app/contents/macos$",Qt::CaseInsensitive));
    if (pos > 0)
        path.truncate(pos);
#endif
    qSettings = new QSettings(path + "/PebbleData/" + fname +".ini",QSettings::IniFormat);

}

