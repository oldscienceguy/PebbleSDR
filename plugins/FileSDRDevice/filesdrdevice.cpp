//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "filesdrdevice.h"
#include "QFileDialog"


FileSDRDevice::FileSDRDevice()
{
    InitSettings("WavFileSDR");
    //ReadSettings();

    framesPerBuffer = 2048; //Temp till we pass

    copyTest = false; //Write what we read

    producerThread = new SDRProducerThread(this);
    producerThread->setRefresh(0);
    consumerThread = new SDRConsumerThread(this);
    consumerThread->setRefresh(0);

}

FileSDRDevice::~FileSDRDevice()
{
    //WriteSettings();
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
    return true;
}

bool FileSDRDevice::Connect()
{
    //WIP: Chose file or fixed internal generator

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
    WriteSettings();
    wavFileRead.Close();
    wavFileWrite.Close();
    return true;
}

void FileSDRDevice::Start()
{
    //WIP: Init to support largest data type, float and do CPX conversion in consumer thread.  Then we can read directly to buffer
    //We can read from file way faster than consumer thread can keep up
    //Unlike real time data, file data isn't lost so we don't really need to producer/consumer buffer
    //So by only creating 1 buffer, we force producer to wait for consumer to be done
    //Ignore overflow warnings in debug

    InitProducerConsumer(1,framesPerBuffer * sizeof(CPX));

    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    consumerThread->start();
    producerThread->start();
    isThreadRunning = true;

    return;

}

void FileSDRDevice::Stop()
{
    if (isThreadRunning) {
        producerThread->stop();
        consumerThread->stop();
        isThreadRunning = false;
    }
    return;

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
    //SDR::ReadSettings();
#if 0
    sStartup = qSettings->value("Startup",162450000).toDouble();
    sLow = qSettings->value("Low",60000000).toDouble();
    sHigh = qSettings->value("High",1700000000).toDouble();
    sStartupMode = qSettings->value("StartupMode",dmFMN).toInt();
    sGain = qSettings->value("Gain",0.05).toDouble();
#endif

}

void FileSDRDevice::WriteSettings()
{
    //SDR::WriteSettings();

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
    AcquireFreeBuffer();
    int samplesRead = wavFileRead.ReadSamples(((CPX **)producerBuffer)[nextProducerDataBuf],framesPerBuffer);
    IncrementProducerBuffer();

#if 0
    //Testing output - need to run in thread eventually until Stop
    for (int i=0; i<framesPerBuffer; i++){
        sample = nco->NextSample();
        inBuffer[i] = sample;
    }
#endif
    ReleaseFilledBuffer();

}
void FileSDRDevice::StopConsumerThread(){}
void FileSDRDevice::RunConsumerThread()
{
    AcquireFilledBuffer();
    CPX *buf = ((CPX **)producerBuffer)[nextConsumerDataBuf];
    if (copyTest)
        wavFileWrite.WriteSamples(buf, framesPerBuffer);
    ProcessIQData(buf,framesPerBuffer);
    IncrementConsumerBuffer();
    ReleaseFreeBuffer();
}


void FileSDRDevice::InitProducerConsumer(int _numDataBufs, int _producerBufferSize)
{
    numDataBufs = _numDataBufs;
    //2 bytes per sample, framesPerBuffer samples after decimate
    producerBufferSize = _producerBufferSize;
    if (producerBuffer != NULL) {
        for (int i=0; i<numDataBufs; i++)
            free (producerBuffer[i]);
        free (producerBuffer);
    }
    producerBuffer = new unsigned char *[numDataBufs];
    for (int i=0; i<numDataBufs; i++)
        producerBuffer[i] = new unsigned char [producerBufferSize];


    //Start out with all producer buffers available
    if (semNumFreeBuffers != NULL)
        delete semNumFreeBuffers;
    semNumFreeBuffers = new QSemaphore(numDataBufs);

    if (semNumFilledBuffers != NULL)
        delete semNumFilledBuffers;
    //Init with zero available
    semNumFilledBuffers = new QSemaphore(0);

    nextProducerDataBuf = nextConsumerDataBuf = 0;

}

bool FileSDRDevice::IsFreeBufferAvailable()
{
    if (semNumFreeBuffers == NULL)
        return false; //Not initialized yet

    //Make sure we have at least 1 data buffer available without blocking
    int freeBuf = semNumFreeBuffers->available();
    if (freeBuf == 0) {
        //qDebug()<<"No free buffer available, ignoring block.";
        return false;
    }
    return true;
}

void FileSDRDevice::AcquireFreeBuffer()
{
    if (semNumFreeBuffers == NULL)
        return; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFreeBuffers->available();
    if ( available < 5) { //Ouput when we get within 5 of overflow
        //qDebug("Limited Free buffers available %d",available);
        freeBufferOverflow = true;
    } else {
        freeBufferOverflow = false;
    }

    semNumFreeBuffers->acquire(); //Will not return until we get a free buffer, but will yield
}

void FileSDRDevice::AcquireFilledBuffer()
{
    if (semNumFilledBuffers == NULL)
        return; //Not initialized yet

    //Debugging to watch producer/consumer overflow
    //Todo:  Add back-pressure to reduce sample rate if not keeping up
    int available = semNumFilledBuffers->available();
    if ( available > (numDataBufs - 5)) { //Ouput when we get within 5 of overflow
        //qDebug("Filled buffers available %d",available);
        filledBufferOverflow = true;
    } else {
        filledBufferOverflow = false;
    }

    semNumFilledBuffers->acquire(); //Will not return until we get a filled buffer, but will yield

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

//SDRThreads
SDRProducerThread::SDRProducerThread(DeviceInterface *_sdr)
{
    sdr = _sdr;
    msSleep=5;
    doRun = false;
}
void SDRProducerThread::stop()
{
    sdr->StopProducerThread();
    doRun=false;
}
//Refresh rate in me
void SDRProducerThread::setRefresh(int ms)
{
    msSleep = ms;
}
void SDRProducerThread::run()
{
    doRun = true;
    while(doRun) {
        sdr->RunProducerThread();
        msleep(msSleep);
    }
}
SDRConsumerThread::SDRConsumerThread(DeviceInterface *_sdr)
{
    sdr = _sdr;
    msSleep=5;
    doRun = false;
}
void SDRConsumerThread::stop()
{
    sdr->StopConsumerThread();
    doRun=false;
}
//Refresh rate in me
void SDRConsumerThread::setRefresh(int ms)
{
    msSleep = ms;
}
void SDRConsumerThread::run()
{
    doRun = true;
    while(doRun) {
        sdr->RunConsumerThread();
        msleep(msSleep);
    }
}

void Sleeper::usleep(unsigned long usecs)
{
    QThread::usleep(usecs);
}
void Sleeper::msleep(unsigned long msecs)
{
    QThread::msleep(msecs);
}
void Sleeper::sleep(unsigned long secs)
{
    QThread::sleep(secs);
}
