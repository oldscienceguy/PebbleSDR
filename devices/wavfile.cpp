#include "wavfile.h"
#include "QDebug"

WavFile::WavFile()
{
    fileParsed = false;
    writeMode = false;
    wavFile = NULL;
}

WavFile::~WavFile()
{
    if (wavFile != NULL)
        wavFile->close();
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
    //Make sure we have a WAV file
    qint64 len = wavFile->read((char*)&riff,sizeof(riff));
    if (len <= 0)
        return false;
    if (riff.id[0]!='R' || riff.id[1]!='I' || riff.id[2]!='F' || riff.id[3] != 'F')
        return false;
    if (riff.format[0]!='W' || riff.format[1]!='A' || riff.format[2]!='V' || riff.format[3] != 'E')
        return false;

    //Loop reading chunks until we find data
    SUB_CHUNK subChunk;
    //Min chunks needed
    bool gotFmtChunk = false;
    bool gotDataChunk = false;
    bool gotFactChunk = false;
    //Init our extended fields so we know if they've been read
    factSubChunk.loFreq = 0;
    factSubChunk.mode = 255; //zero is AM so we need to indicate not valid
    factSubChunk.spare1 = 0;

    while (!wavFile->atEnd()) {
        len = wavFile->read((char*)&subChunk,sizeof(SUB_CHUNK));

        if (subChunk.id[0]=='f' && subChunk.id[1]=='m' && subChunk.id[2]=='t' && subChunk.id[3] == ' ') {

            len = wavFile->read((char*)&fmtSubChunk,subChunk.size);
            if (len<0)
                return false;
            qDebug()<<"SR:"<<fmtSubChunk.sampleRate<<" Bits/Sample:"<<fmtSubChunk.bitsPerSample<<" Channels:"<<fmtSubChunk.channels<< "Fmt:"<<fmtSubChunk.format;

            //If compressed data, we need to read additional data
            if (fmtSubChunk.format != 1 && fmtSubChunk.format != 3) {
                quint16 extraParamSize;
                len = wavFile->read((char*)&extraParamSize,2);
                if (len)
                    wavFile->seek(wavFile->pos() + extraParamSize); //skip
            }

            gotFmtChunk = true;
        } else if (subChunk.id[0]=='d' && subChunk.id[1]=='a' && subChunk.id[2]=='t' && subChunk.id[3] == 'a') {
            gotDataChunk = true;
            break; //Out of while !file end

        } else if (subChunk.id[0]=='f' && subChunk.id[1]=='a' && subChunk.id[2]=='c' && subChunk.id[3] == 't') {
            len = wavFile->read((char*)&factSubChunk,subChunk.size);
            if (len<0)
                return false;
            qDebug()<<"WAV LO Freq:"<<factSubChunk.loFreq;
            gotFactChunk = true;

        } else {
            //Unsupported, skip and continue
            wavFile->seek(wavFile->pos() + subChunk.size);
        }

    }

    if (!gotFmtChunk && !gotDataChunk)
        return false; //Missing chunks
    //File pos is now at start of data, save for looping
    dataStart = wavFile->pos();

    fileParsed = true;

    return true;
}

bool WavFile::OpenWrite(QString fname, int sampleRate, quint32 _loFreq, quint8 mode, quint8 spare)
{
    //qDebug()<<fname;

    wavFile = new QFile(fname);
    if (wavFile == NULL) {
        return false;
    }
    //Create a new file, overwrite any existing
    //Note: Directories must exist or we get error
    bool res = wavFile->open(QFile::WriteOnly | QFile::Truncate);
    if (!res) {
        qDebug()<<"Write wav file - "<<fname;
        qDebug()<<"Write wav file error - "<< wavFile->errorString();
        return false;
    }
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

    fmtSubChunkPre.id[0] = 'f';
    fmtSubChunkPre.id[1] = 'm';
    fmtSubChunkPre.id[2] = 't';
    fmtSubChunkPre.id[3] = ' ';
    fmtSubChunkPre.size = 16;

    fmtSubChunk.format = 1; //1 = PCM, 3 = FLOAT
    fmtSubChunk.channels = 2;
    fmtSubChunk.sampleRate = sampleRate;
    fmtSubChunk.byteRate = sampleRate * 2 * (16/8);  //SampleRate * NumChannels * BitsPerSample/8
    fmtSubChunk.blockAlign = 2 * (16/8);  //NumChannels * BitsPerSample/8
    fmtSubChunk.bitsPerSample = sizeof(FMT_SUB_CHUNK);

    factSubChunkPre.id[0] = 'f';
    factSubChunkPre.id[1] = 'a';
    factSubChunkPre.id[2] = 'c';
    factSubChunkPre.id[3] = 't';
    factSubChunkPre.size = sizeof(FACT_SUB_CHUNK);

    factSubChunk.numSamples = 0; //Not used
    factSubChunk.loFreq = _loFreq;
    factSubChunk.mode = mode;
    factSubChunk.spare1 = spare;

    dataSubChunkPre.id[0] = 'd';
    dataSubChunkPre.id[1] = 'a';
    dataSubChunkPre.id[2] = 't';
    dataSubChunkPre.id[3] = 'a';
    //This will be updated after we write samples and close file
    dataSubChunkPre.size = 0; //NumSamples * NumChannels * BitsPerSample/8

    //Seek to first data byte
    //RIFF - FMT - FACT - DATA
    dataStart = sizeof(RIFF_CHUNK) + sizeof(SUB_CHUNK) + sizeof(FMT_SUB_CHUNK) + sizeof(SUB_CHUNK) + sizeof(FACT_SUB_CHUNK) + sizeof(SUB_CHUNK);

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
        dataSubChunkPre.size += bufLen;

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

    int bytesWritten;
    if (writeMode && wavFile != NULL) {
        //4 + (8 + SubChunk1Size) + (8 + SubChunk2Size) should be same as 36 + dataSubChunk.size
        riff.size = 4 + (8 + fmtSubChunkPre.size) + (8 + dataSubChunkPre.size);

        wavFile->seek(0); //Replace header data
        bytesWritten = wavFile->write((char*)&riff,sizeof(RIFF_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&fmtSubChunkPre,sizeof(SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&fmtSubChunk,sizeof(FMT_SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&factSubChunkPre,sizeof(SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&factSubChunk,sizeof(FACT_SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&dataSubChunkPre,sizeof(SUB_CHUNK));
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
