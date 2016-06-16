//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "wavfile.h"
#include "QDebug"

WavFile::WavFile()
{
    fileParsed = false;
    writeMode = false;
    wavFile = NULL;
	pcmBuf = NULL;
	floatBuf = NULL;
	monoPcmBuf = NULL;
}

WavFile::~WavFile()
{
    if (wavFile != NULL)
        wavFile->close();
}

bool WavFile::OpenRead(QString fname, quint32 _maxNumberOfSamples)
{
	maxNumberOfSamples = _maxNumberOfSamples;
	if (pcmBuf != NULL)
		delete []pcmBuf;
	pcmBuf = new PCM_DATA_2CH[maxNumberOfSamples * 2];
	if (floatBuf != NULL)
		delete []floatBuf;
	floatBuf = new FLOAT_DATA[maxNumberOfSamples * 2];
	if (monoPcmBuf != NULL)
		delete []monoPcmBuf;
	monoPcmBuf = new qint16[maxNumberOfSamples];

    wavFile = new QFile(fname);
    if (wavFile == NULL) {
        return false;
    }
    bool res = wavFile->open(QFile::ReadOnly);
    if (!res)
        return false;

    qint64 dataChunkPos = 0;

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
    loFreq = 0;
    mode = 255; //zero is AM so we need to indicate not valid

    while (!wavFile->atEnd()) {
        len = wavFile->read((char*)&subChunk,sizeof(SUB_CHUNK));

        if (strncasecmp((char *)subChunk.id,"fmt ",4) == 0) {

            len = wavFile->read((char*)&fmtSubChunk,subChunk.size);
            if (len<0)
                return false;
            qDebug()<<"SR:"<<fmtSubChunk.sampleRate<<" Bits/Sample:"<<fmtSubChunk.bitsPerSample<<" Channels:"<<fmtSubChunk.channels<< "Fmt:"<<fmtSubChunk.format;

            //If compressed data, we need to read additional data
			if (fmtSubChunk.format != PCM_FORMAT && fmtSubChunk.format != FLOAT_FORMAT) {
                quint16 extraParamSize;
                len = wavFile->read((char*)&extraParamSize,2);
                if (len)
                    wavFile->seek(wavFile->pos() + extraParamSize); //skip
            }

            fmtChannels = fmtSubChunk.channels;

            gotFmtChunk = true;
        } else if (strncasecmp((char *)subChunk.id,"data",4) == 0) {

            gotDataChunk = true;
            //There may be other chunks we need to find, mark and keep looking
            dataChunkPos = wavFile->pos();

        } else if (strncasecmp((char *)subChunk.id,"fact",4) == 0) {
            len = wavFile->read((char*)&factSubChunk,subChunk.size);
            if (len<0)
                return false;
            gotFactChunk = true;
        } else  if (strncasecmp((char *)subChunk.id,"list",4) == 0) {

            //!!Replace Pebble custom info with RIFF list/info chunks as used in Audacity.  More standard

            //subChunk.size has following bytes
            //I think this has all the Info labels and notes chunks included in size
            len = wavFile->read(chunkBuf,subChunk.size);
            //"INFO" (info chunk)
            // quint8[4] label quint length (padded to even) quint8[?] null terminated value
            int i = 0;
            char tag[5]; //4 + null
            quint8 value[256];
            quint32 valueLen;
            //First 4 bytes are subchunk, compare lowercased
            if (strncasecmp(&chunkBuf[i],"info",4) == 0) {
                //Loop after 'info' to get all tag/value pairs
                i += 4;
                while (i < len) {
                    //Process info subchunk
                    memcpy(tag, &chunkBuf[i],4);
                    tag[4] = 0x00;
                    i += 4;
                    valueLen = (quint16)chunkBuf[i];
                    if (valueLen > sizeof(value)) {
                        qWarning()<<"Wave file value len > size of data buf";
                        valueLen = sizeof(value);
                    }

                    i += 4;
                    memcpy(value, &chunkBuf[i], valueLen); //Already null terminated
                    i += valueLen;
                    //!!Compare tag with Pebble data and set variables
                    char* dontCare;
                    if (strncasecmp(tag,"lofr",4) == 0)
                        loFreq = strtol((char*)value,&dontCare,10);
                    else if (strncasecmp(tag,"mode",4) == 0)
                        mode = strtol((char*)value,&dontCare,10);

                }
            } else if(strncasecmp(&chunkBuf[i],"labl",4) == 0) {
                //Process labl subchunk
                i+=4;
            } else if (strncasecmp(&chunkBuf[i],"note",4) == 0) {
                //Process note subchunk
                i+=4;
            }


        } else {
            //Unsupported, skip and continue
            wavFile->seek(wavFile->pos() + subChunk.size);
        }

    }

    if (!gotFmtChunk && !gotDataChunk)
        return false; //Missing chunks
    wavFile->seek(dataChunkPos);
    //File pos is now at start of data, save for looping
    dataStart = dataChunkPos;

    fileParsed = true;

    return true;
}

bool WavFile::OpenWrite(QString fname, int sampleRate, quint32 _loFreq, quint8 _mode, quint8 spare)
{
	Q_UNUSED(spare);
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

	fmtSubChunk.format = PCM_FORMAT; //1 = PCM, 3 = FLOAT
    fmtSubChunk.channels = 2;
    fmtSubChunk.sampleRate = sampleRate;
    fmtSubChunk.byteRate = sampleRate * 2 * (16/8);  //SampleRate * NumChannels * BitsPerSample/8
    fmtSubChunk.blockAlign = 2 * (16/8);  //NumChannels * BitsPerSample/8
	fmtSubChunk.bitsPerSample = 16;

#if 0
    factSubChunkPre.id[0] = 'f';
    factSubChunkPre.id[1] = 'a';
    factSubChunkPre.id[2] = 'c';
    factSubChunkPre.id[3] = 't';
    factSubChunkPre.size = sizeof(FACT_SUB_CHUNK);

    factSubChunk.numSamples = 0; //Not used
    factSubChunk.loFreq = _loFreq;
    factSubChunk.mode = _mode;
    factSubChunk.spare1 = spare;
#endif
    listSubChunkPre.id[0] = 'l';
    listSubChunkPre.id[1] = 'i';
    listSubChunkPre.id[2] = 's';
    listSubChunkPre.id[3] = 't';
    listSubChunkPre.size = sizeof(LIST_SUB_CHUNK); //Must be kept in sync if we change anything below

    listSubChunk.typeID[0] = 'i';
    listSubChunk.typeID[1] = 'n';
    listSubChunk.typeID[2] = 'f';
    listSubChunk.typeID[3] = 'o';

    int i = 0;
    char buf[256];
    quint32 len; //wav expects 4 byte len

    //'lofr'freq[12] 10 digits for 2g plus null plus 1 for even
    listSubChunk.text[i++] = 'l';
    listSubChunk.text[i++] = 'o';
    listSubChunk.text[i++] = 'f';
    listSubChunk.text[i++] = 'r';
    len = sprintf(buf,"%ul",_loFreq); // we need to know the length of the tag value
    len++; //Length does not include null terminator and we want to copy that also
    memcpy(&listSubChunk.text[i],&len,4); //Move len in internal format
    i += 4;
    memcpy(&listSubChunk.text[i],buf,len);
    i += len;


    //'mode'mode[4] 2 byte + null + 1 for even
    listSubChunk.text[i++] = 'm';
    listSubChunk.text[i++] = 'o';
    listSubChunk.text[i++] = 'd';
    listSubChunk.text[i++] = 'e';
    len = sprintf(buf,"%u",_mode); // we need to know the length of the tag value
    len++; //include Null
    memcpy(&listSubChunk.text[i],&len,4); //Move len in internal format
    i += 4;
    memcpy(&listSubChunk.text[i],buf,len);
    i += len;

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

	if (fmtSubChunk.format == PCM_FORMAT) {
        if (fmtChannels == 2) {
            bytesToRead = sizeof(PCM_DATA_2CH) * numSamples;

            bytesRead = wavFile->read((char*)pcmBuf,bytesToRead);
            if (bytesRead == -1)
                samplesRead = 0;
            else
                samplesRead = bytesRead / sizeof(PCM_DATA_2CH);

            for (int i=0; i<samplesRead; i++) {
                buf[i].real(pcmBuf[i].left/32767.0);
                buf[i].im = pcmBuf[i].right/32767.0;
            }

        } else if (fmtChannels == 1) {
            //Mono, only need 1/2 samples since re and im are the same
            bytesToRead = sizeof(qint16) * numSamples;
            bytesRead = wavFile->read((char*)monoPcmBuf,bytesToRead);
            if (bytesRead == -1)
                samplesRead = 0;
            else
                samplesRead = bytesRead / sizeof(qint16);

            //Convert to array of doubles so we can apply hilbert
            for (int i =0; i<samplesRead; i++) {
                //Testing with arrl cw files, have to reduce gain significantly
                buf[i].real(monoPcmBuf[i] / 32767.0 * .005);
                buf[i].im = 0; //This gives us mirror images pos and neg
                //Steps afer this to get back to just positive spectrum are basically hilbert transformation
                //Todo: Look at using hilbert transoformation here to get a more accurate real to cpx conversion
            }
        }

	} else if (fmtSubChunk.format == FLOAT_FORMAT) {
        bytesToRead = sizeof(FLOAT_DATA) * numSamples;
        bytesRead = wavFile->read((char*)floatBuf,bytesToRead);
        if (bytesRead == -1)
            samplesRead = 0;
        else
            samplesRead = bytesRead / sizeof(FLOAT_DATA);
        for (int i=0; i<samplesRead; i++) {
            buf[i].real(floatBuf[i].left);
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

	if (fmtSubChunk.format == PCM_FORMAT)
        len = wavFile->read((char*)&pcmData,sizeof(pcmData));
	else if (fmtSubChunk.format == FLOAT_FORMAT)
        len = wavFile->read((char*)&floatData,sizeof(floatData));
    else
        return sample; //unsupported format

    if (len <= 0) {
        return sample;
    }
	if (fmtSubChunk.format == PCM_FORMAT) {
        //Convert wav 16 bit (-32767 to +32767) int to sound card float32.  Values are -1 to +1
        sample.real(pcmData.left/32767.0);
        sample.im = pcmData.right/32767.0;
    } else {
        //Samples already float32
        sample.real(floatData.left);
        sample.im = floatData.right;
        //qDebug()<<sample.real()<<"/"<<sample.im;
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
    int bufLen = sizeof(PCM_DATA_2CH);
    for (int i=0; i<numSamples; i++) {
        //Convert float to +/- 32767
        pcmData.left = buf[i].real() * 32767;
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
        riff.size = 4 + (8 + fmtSubChunkPre.size) +
                //(8 + factSubChunkPre.size)+ //Not using
                (8 + listSubChunkPre.size)+
                (8 + dataSubChunkPre.size);

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

#if 0
        bytesWritten = wavFile->write((char*)&factSubChunkPre,sizeof(SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&factSubChunk,sizeof(FACT_SUB_CHUNK));
        if (bytesWritten == -1)
            return false;
#endif

        bytesWritten = wavFile->write((char*)&listSubChunkPre,sizeof(SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&listSubChunk,sizeof(LIST_SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        bytesWritten = wavFile->write((char*)&dataSubChunkPre,sizeof(SUB_CHUNK));
        if (bytesWritten == -1)
            return false;

        wavFile->close();

		if (pcmBuf != NULL)
			delete []pcmBuf;
		pcmBuf = NULL;
		if (floatBuf != NULL)
			delete []floatBuf;
		floatBuf = NULL;
		if (monoPcmBuf != NULL)
			delete []monoPcmBuf;
		monoPcmBuf = NULL;

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
