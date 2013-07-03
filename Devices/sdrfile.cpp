#include "sdrfile.h"
#include "Receiver.h"
#include "Settings.h"
#include "Demod.h"
#include "QFileDialog"

SDRFile::SDRFile(Receiver *_receiver,SDRDEVICE dev,Settings *_settings): SDR(_receiver, dev,_settings)
{
    settings=_settings;
    if (!settings)
        return; //No init

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif

    qSettings = new QSettings(path+"/PebbleData/sdrfile.ini",QSettings::IniFormat);
    ReadSettings();

    framesPerBuffer = settings->framesPerBuffer;
    inBuffer = CPXBuf::malloc(framesPerBuffer);
    outBuffer = CPXBuf::malloc(framesPerBuffer);

#if 0
    nco = new NCO(settings->sampleRate,framesPerBuffer);
    nco->SetFrequency(100);
#endif

    producerThread = new SDRProducerThread(this);
    producerThread->setRefresh(0); //Semaphores will block and wait, no extra delay needed
    //consumerThread = new SDRConsumerThread(this);
    //consumerThread->setRefresh(0);

}
SDRFile::~SDRFile(void)
{
    if (!settings)
        return;
}

bool SDRFile::Connect()
{
    //WIP: Chose file or fixed internal generator
    //Passing NULL for dir shows current/last directory, which may be inside the mac application bundle
    fileName = QFileDialog::getOpenFileName(NULL,tr("Open Wave File"), NULL, tr("Wave Files (*.wav)"));
    bool res = wavFile.OpenRead(fileName);
    if (!res)
        return false;

    return true;
}
bool SDRFile::Disconnect()
{
    wavFile.Close();
    return true;
}
double SDRFile::SetFrequency(double fRequested,double fCurrent)
{
    return fRequested;
}

void SDRFile::ShowOptions() {return;}

void SDRFile::Start()
{
    //WIP: We don't need both threads, just the producer.  TBD
    //We want the consumer thread to start first, it will block waiting for data from the SDR thread
    //consumerThread->start();
    producerThread->start();
    isThreadRunning = true;

    return;
}
void SDRFile::Stop()
{
    if (isThreadRunning) {
        producerThread->stop();
        //consumerThread->stop();
        isThreadRunning = false;
    }
    return;
}

void SDRFile::ReadSettings()
{
    SDR::ReadSettings(qSettings);
#if 0
    sStartup = qSettings->value("Startup",162450000).toDouble();
    sLow = qSettings->value("Low",60000000).toDouble();
    sHigh = qSettings->value("High",1700000000).toDouble();
    sStartupMode = qSettings->value("StartupMode",dmFMN).toInt();
    sGain = qSettings->value("Gain",0.05).toDouble();
#endif
}

void SDRFile::WriteSettings()
{

}

double SDRFile::GetStartupFrequency()
{
    return GetSampleRate() / 2.0;
}

int SDRFile::GetStartupMode()
{
    return dmLSB;
}
double SDRFile::GetHighLimit()
{
    return GetSampleRate();
}
double SDRFile::GetLowLimit()
{
    return 0;
}
//WIP: We really need an adjustable gain
//For internally generated data, use .5
double SDRFile::GetGain() {return 0.50;}
QString SDRFile::GetDeviceName()
{
    return "SDRFile: " + QFileInfo(fileName).fileName() + "-" + QString::number(GetSampleRate());
}

int SDRFile::GetSampleRate()
{
    return wavFile.GetSampleRate();
}

#if 0
Add test tone ie, freq (700) and offset(10000)
Add optional noise generator

/** Generates a tone of the specified frequency
*  Gotten from: http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&oe=UTF-8&safe=off&selm=3c641e%243jn%40uicsl.csl.uiuc.edu
*/
float *makeTone(int samplerate, float frequency, int length, float gain=1.0) {
    //y(n) = 2 * cos(A) * y(n-1) - y(n-2)
    //A= (frequency of interest) * 2 * PI / (sampling frequency)
    //A is in radians.
    // frequency of interest MUST be <= 1/2 the sampling frequency.
    float *tone = new float[length];
    float A = frequency*2*PI/samplerate;

    for (int i=0; i<length; i++) {
    if (i > 1) tone[i]= 2*cos(A)*tone[i-1] - tone[i-2];
    else if (i > 0) tone[i] = 2*cos(A)*tone[i-1] - (cos(A));
    else tone[i] = 2*cos(A)*cos(A) - cos(2*A);
    }

    for (int i=0; i<length; i++) tone[i] = tone[i]*gain;

    return tone;
}

/** adds whitenoise to a sample */
void *addNoise(float *sample, int length, float gain=1.0) {
    for (int i=0; i<length; i++) sample[i] += (2*(rand()/(float)RAND_MAX)-1)*gain;
}



#endif



void SDRFile::StopProducerThread(){}
void SDRFile::RunProducerThread()
{

    for (int i=0; i<framesPerBuffer; i++)
    {
        inBuffer[i] = wavFile.ReadData();

    }

#if 0
    //Testing output - need to run in thread eventually until Stop
    for (int i=0; i<framesPerBuffer; i++){
        sample = nco->NextSample();
        inBuffer[i] = sample;
    }
#endif
    receiver->ProcessBlock(inBuffer,outBuffer,framesPerBuffer);

}
void SDRFile::StopConsumerThread(){}
void SDRFile::RunConsumerThread(){}


