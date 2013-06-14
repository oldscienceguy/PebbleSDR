#include "sdrfile.h"
#include "Receiver.h"
#include "Settings.h"
#include "Demod.h"
#include "QFileDialog"

void ReadWavTest();

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

    wavFile = NULL;

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
    bool res = OpenWavFile(fileName);
    if (!res)
        return false;

    return true;
}
bool SDRFile::Disconnect()
{
    if (wavFile != NULL)
        wavFile->close();
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
    if (wavFile != NULL)
        //We've successfully opened and parsed chunks
        return fmtSubChunk.sampleRate;
    else
        return 96000;
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
    CPX sample;
    int len;
    if (wavFile != NULL) {
        //If we're at end of file, start over in continuous loop
        if (wavFile->atEnd())
            wavFile->seek(dataStart);

        for (int i=0; i<framesPerBuffer; i++)
        {
            if (fmtSubChunk.format == 1)
                len = wavFile->read((char*)&pcmData,sizeof(pcmData));
            else if (fmtSubChunk.format == 3)
                len = wavFile->read((char*)&floatData,sizeof(floatData));
            else
                return; //unsupported format

            if (len <= 0) {
                sample.re = 0;
                sample.im = 0;
                inBuffer[i] = sample;
            }
            if (fmtSubChunk.format == 1) {
                //Convert wav 16 bit (-32767 to +32767) int to sound card float32.  Values are -1 to +1
                sample.re = pcmData.left/32767.0;
                sample.im = pcmData.right/32767.0;
            } else {
                //Samples already float32
                sample.re = floatData.left;
                sample.im = floatData.right;
                //qDebug()<<sample.re<<"/"<<sample.im;
            }
            inBuffer[i] = sample;

        }
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


bool SDRFile::OpenWavFile(QString fname)
{
    wavFile = new QFile(fname);
    if (wavFile == NULL) {
        return false;
    }
    bool res = wavFile->open(QFile::ReadOnly);
    if (!res)
        return false;
    qint64 len = wavFile->read((char*)&riff,sizeof(riff));
    if (len <=0 )
        return false;
    len = wavFile->read((char*)&fmtSubChunk,sizeof(fmtSubChunk));
    if (len<0)
        return false;
    qDebug()<<"SR:"<<fmtSubChunk.sampleRate<<" Bits/Sample:"<<fmtSubChunk.bitsPerSample<<" Channels:"<<fmtSubChunk.channels<< "Fmt:"<<fmtSubChunk.format<<" Extra:"<<fmtSubChunk.extraParamSize;

    //If we're reading non PCM data, we may have more to read before data
    if (fmtSubChunk.format!=1 && fmtSubChunk.extraParamSize > 0) {
        //We may get garbage, max allowed is 22, which may not be right
        len = wavFile->read(tmpBuf,22);
        if (len<0)
            return false;
    }

    len = wavFile->read((char*)&dataSubChunk,sizeof(dataSubChunk));
    if (len<0)
        return false;
    dataStart = wavFile->pos(); //So we can loop
#if 0
    len = wavFile->read((char*)&data,sizeof(data));
    if (len<0)
        return false;
    wavFile->close();
#endif
    return true;
}
