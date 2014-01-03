//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "filesdrdevice.h"
#include "QFileDialog"


FileSDRDevice::FileSDRDevice()
{
    InitSettings("WavFileSDR");

    framesPerBuffer = 2048; //Temp till we pass

    copyTest = false; //Write what we read
}

FileSDRDevice::~FileSDRDevice()
{
    Stop();
    Disconnect();
}

QString FileSDRDevice::GetPluginName()
{
    return "WAV File SDR";
}

QString FileSDRDevice::GetPluginDescription()
{
    return "Plays back I/Q WAV file";
}

bool FileSDRDevice::Initialize(cbProcessIQData _callback)
{
    ProcessIQData = _callback;
    producerConsumer.Initialize(this,1,framesPerBuffer * sizeof(CPX),0);

    return true;
}

bool FileSDRDevice::Connect()
{

    //TODO: Get directly from receiver or settings so set in one place
    QString recordingPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        recordingPath.chop(25);
#endif
        recordingPath += "PebbleRecordings/";

#if 1
    //Passing NULL for dir shows current/last directory, which may be inside the mac application bundle
    fileName = QFileDialog::getOpenFileName(NULL,tr("Open Wave File"), recordingPath, tr("Wave Files (*.wav)"));
    //If we try to debug write after getOpenFileName the dialog is still open and obscures QT!
    //Hack to let event loop purge which lets close message get handled - ughh!
    QEventLoop loop; while (loop.processEvents()) /* nothing */;

#else
    //Use this instead of static if we need more options
    QFileDialog fDialog;
    //fDialog.setLabelText(tr("Open Wave File"));
    //fDialog.setFilter(tr("Wave Files (*.wav)"));
    QStringList files;
    if (fDialog.exec())
        files = fDialog.selectedFiles();

    fDialog.close();

    fileName = files[0];
#endif

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
    //No device specific settings
}

void FileSDRDevice::WriteSettings()
{

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

void FileSDRDevice::SetupOptionUi(QWidget *parent)
{
    //Nothing to do
}

void FileSDRDevice::StopProducerThread(){}
void FileSDRDevice::RunProducerThread()
{
    producerConsumer.AcquireFreeBuffer();
    int samplesRead = wavFileRead.ReadSamples(producerConsumer.GetNextProducerBuffer(),framesPerBuffer);
    producerConsumer.SupplyProducerBuffer();
    producerConsumer.ReleaseFilledBuffer();

}
void FileSDRDevice::StopConsumerThread(){}
void FileSDRDevice::RunConsumerThread()
{
    producerConsumer.AcquireFilledBuffer();
    CPX *buf = producerConsumer.GetNextConsumerBuffer();
    if (copyTest)
        wavFileWrite.WriteSamples(buf, framesPerBuffer);
    ProcessIQData(buf,framesPerBuffer);
    producerConsumer.SupplyConsumerBuffer();
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
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = new QSettings(path + "/PebbleData/" + fname +".ini",QSettings::IniFormat);

}

