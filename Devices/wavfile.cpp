#include "wavfile.h"
#include "QDebug"

WavFile::WavFile()
{
    fileParsed = false;
    writeMode = false;
    wavFile = NULL;
    dataStart = sizeof(RIFF_CHUNK) + sizeof(FMT_SUB_CHUNK) + sizeof(DATA_SUB_CHUNK);

}

bool WavFile::OpenRead(QString fname)
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
    qDebug()<<"SR:"<<fmtSubChunk.sampleRate<<" Bits/Sample:"<<fmtSubChunk.bitsPerSample<<" Channels:"<<fmtSubChunk.channels<< "Fmt:"<<fmtSubChunk.format;

#if 0
    //If we're reading non PCM data, we may have more to read before data
    if (fmtSubChunk.format!=1 && fmtSubChunk.extraParamSize > 0) {
        //We may get garbage, max allowed is 22, which may not be right
        len = wavFile->read(tmpBuf,22);
        if (len<0)
            return false;
    }
#endif
    len = wavFile->read((char*)&dataSubChunk,sizeof(dataSubChunk));
    if (len<0)
        return false;

#if 0
    len = wavFile->read((char*)&data,sizeof(data));
    if (len<0)
        return false;
    wavFile->close();
#endif

    fileParsed = true;

    return true;
}

bool WavFile::OpenWrite(QString fname, int sampleRate)
{
    wavFile = new QFile(fname);
    if (wavFile == NULL) {
        return false;
    }
    //Create a new file, overwrite any existing
    bool res = wavFile->open(QFile::ReadWrite); // || QFile::Truncate);
    if (!res)
        return false;
    //Write RIFF
    riff.id[0] = 'R';
    riff.id[1] = 'I';
    riff.id[2] = 'F';
    riff.id[3] = 'F';
    //Update when done
    riff.size = 0; //4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
    riff.format[0] = 'W';
    riff.format[1] = 'A';
    riff.format[2] = 'V';
    riff.format[3] = 'E';

    fmtSubChunk.id[0] = 'f';
    fmtSubChunk.id[1] = 'm';
    fmtSubChunk.id[2] = 't';
    fmtSubChunk.id[3] = ' ';
    fmtSubChunk.size = 16;
    fmtSubChunk.format = 1; //1 = PCM, 3 = FLOAT
    fmtSubChunk.channels = 2;
    fmtSubChunk.sampleRate = sampleRate;
    fmtSubChunk.byteRate = sampleRate * 2 * (16/8);  //SampleRate * NumChannels * BitsPerSample/8
    fmtSubChunk.blockAlign = 2 * (16/8);  //NumChannels * BitsPerSample/8
    fmtSubChunk.bitsPerSample = 16; //Fixed for now

    dataSubChunk.id[0] = 'd';
    dataSubChunk.id[1] = 'a';
    dataSubChunk.id[2] = 't';
    dataSubChunk.id[3] = 'a';
    //This will be updated after we write samples and close file
    dataSubChunk.size = 0; //NumSamples * NumChannels * BitsPerSample/8

    //Seek to first data byte
    wavFile->seek(dataStart);

    writeMode = true;
    return true;
}

//Returns #sample read into buf
int WavFile::ReadSamples(CPX *buf, int numSamples)
{

    if (writeMode || wavFile == NULL)
        return 0;

    //If we're at end of file, start over in continuous loop
    if (wavFile->atEnd())
        wavFile->seek(dataStart);

    int bytesRead;
    int bytesToRead;
    int samplesRead = 0;

    if (fmtSubChunk.format == 1) {
        bytesToRead = sizeof(PCM_DATA) * numSamples;
        bytesRead = wavFile->read((char*)pcmBuf,bytesToRead);
        if (bytesRead == -1)
            samplesRead = 0;
        else
            samplesRead = bytesRead / sizeof(PCM_DATA);
        for (int i=0; i<samplesRead; i++) {
            buf[i].re = pcmBuf[i].left/32767.0;
            buf[i].im = pcmBuf[i].right/32767.0;
        }

    } else if (fmtSubChunk.format == 3) {
        bytesToRead = sizeof(FLOAT_DATA) * numSamples;
        bytesRead = wavFile->read((char*)floatBuf,bytesToRead);
        if (bytesRead == -1)
            samplesRead = 0;
        else
            samplesRead = bytesRead / sizeof(FLOAT_DATA);
        for (int i=0; i<samplesRead; i++) {
            buf[i].re = floatBuf[i].left;
            buf[i].im = floatBuf[i].right;
        }

    }

    return samplesRead;

}

CPX WavFile::ReadSample()
{
    CPX sample;
    int len;

    if (writeMode || wavFile == NULL)
        return sample;

    //If we're at end of file, start over in continuous loop
    if (wavFile->atEnd())
        wavFile->seek(dataStart);

    if (fmtSubChunk.format == 1)
        len = wavFile->read((char*)&pcmData,sizeof(pcmData));
    else if (fmtSubChunk.format == 3)
        len = wavFile->read((char*)&floatData,sizeof(floatData));
    else
        return sample; //unsupported format

    if (len <= 0) {
        return sample;
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

    return sample;
}

//Writes IQ block
bool WavFile::WriteSamples(CPX* buf, int numSamples)
{
    if (!writeMode || wavFile == NULL)
        return false;

    int bytesWritten = 0;
    //Only support float for now, add PCM if we need it later
    int bufLen = sizeof(PCM_DATA);
    for (int i=0; i<numSamples; i++) {
        //Convert float to +/- 32767
        pcmData.left = buf[i].re * 32767;
        pcmData.right = buf[i].im * 32767;
        bytesWritten = wavFile->write((const char*) &pcmData, bufLen);
        if (bytesWritten != bufLen)
            return false;
        dataSubChunk.size += bufLen;

    }
    return true;
}

bool WavFile::Close()
{
    //If open for writing, update length fields
    if (!writeMode && wavFile != NULL) {
        wavFile->close();
        return true;
    }

    if (writeMode && wavFile != NULL) {
        //4 + (8 + SubChunk1Size) + (8 + SubChunk2Size) should be same as 36 + dataSubChunk.size
        riff.size = 4 + (8 + fmtSubChunk.size) + (8 + dataSubChunk.size);
        wavFile->seek(0); //Replace header data
        int bytesWritten = wavFile->write((char*)&riff,sizeof(RIFF_CHUNK));
        if (bytesWritten == -1)
            return false;
        bytesWritten = wavFile->write((char*)&fmtSubChunk,sizeof(FMT_SUB_CHUNK));
        if (bytesWritten == -1)
            return false;
        bytesWritten = wavFile->write((char*)&dataSubChunk,sizeof(DATA_SUB_CHUNK));
        if (bytesWritten == -1)
            return false;
        wavFile->close();
        return true;
    }

    return false;
}

int WavFile::GetSampleRate(){
    if (fileParsed)
        return fmtSubChunk.sampleRate;
    else
        return 96000;
}
