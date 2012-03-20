#include "sdrfile.h"
#include "Receiver.h"
#include "Settings.h"
#include "Demod.h"
#include "QFileDialog"

void ReadWavTest();

SDRFile::SDRFile(Receiver *_receiver,SDRDEVICE dev,Settings *_settings): SDR(_receiver, dev,_settings)
{

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
}

bool SDRFile::Connect()
{
    //WIP: Chose file or fixed internal generator
    QString fileName;
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
double SDRFile::GetGain() {return 0.010;}
QString SDRFile::GetDeviceName() {return "SDRFile";}
int SDRFile::GetSampleRate()
{
    //WIP: This should return the sample rate from the wav file
    if (wavFile != NULL)
        //We've successfully opened and parsed chunks
        return fmtSubChunk.sampleRate;
    else
        return 96000;
}

void SDRFile::StopProducerThread(){}
void SDRFile::RunProducerThread()
{
    CPX sample;
    int len;
    if (wavFile != NULL) {
        for (int i=0; i<framesPerBuffer; i++)
        {
            len = wavFile->read((char*)&data,sizeof(data));
            if (len <= 0) {
                sample.re = 0;
                sample.im = 0;
                inBuffer[i] = sample;
            } else {
                //Convert wav 16 bit (-32767 to +32767) int to sound card float32.  Values are -1 to +1
                sample.re = data.left/32767.0;
                sample.im = data.right/32767.0;
                inBuffer[i] = sample;
            }

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
    len = wavFile->read((char*)&dataSubChunk,sizeof(dataSubChunk));
    if (len<0)
        return false;
#if 0
    len = wavFile->read((char*)&data,sizeof(data));
    if (len<0)
        return false;
    wavFile->close();
#endif
    return true;
}
