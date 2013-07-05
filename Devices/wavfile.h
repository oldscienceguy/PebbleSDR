#ifndef WAVFILE_H
#define WAVFILE_H

#include "QFile"
#include "cpx.h"

/*
https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
http://www.sonicspot.com/guide/wavefiles.html More complete spec
WAV files have an optional INFO chunk that is often left out of spec

See http://www.johnloomis.org/cpe102/asgn/asgn1/riff.html for full RIFF spec
http://www.tactilemedia.com/info/MCI_Control_Info.html

The canonical WAVE format starts with the RIFF header:

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk
                               following this number.  This is the size of the
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      1 = PCM (i.e. Linear quantization)
                               3 = IEEE Float
                               Values other than 1 & 3 indicate some
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this
                               number.
44        *   Data             The actual sound data.


*/
//Cannonical wav file format
#pragma pack(1)
//Every chunk/sub-chunk starts with id and size
typedef struct SUB_CHUNK
{
    quint8 id[4];
    quint32 size;

}SUB_CHUNK;

typedef struct RIFF_CHUNK
{
    quint8 id[4];
    quint32 size;
    quint8 format[4];

}RIFF_CHUNK;

typedef struct FMT_SUB_CHUNK
{
    //sub_chunk header here
    quint16 format;
    quint16 channels;
    quint32 sampleRate;
    quint32 byteRate;
    quint16 blockAlign;
    quint16 bitsPerSample;
}FMT_SUB_CHUNK;

typedef struct OPTIONAL_DATA
{
    quint16 extraParamSize; //Only exists for non PCM data, ie float
}OPTIONAL_DATA;

typedef struct DATA_SUB_CHUNK
{
    //sub_chunk header here
    //Data follows

}DATA_SUB_CHUNK;

//Fact chunk is only used for compressed data, but we use it to store extended data also
typedef struct FACT_SUB_CHUNK
{
    //sub_chunk header here
    quint32 numSamples; //This is the only defined FACT item
    //Extensions follow, non SDR files won't include this and subchunk length will be shorter
    quint32 loFreq;

}FACT_SUB_CHUNK;

//assumes 16bits per sample
typedef struct PCM_DATA
{
    qint16 left; //+/- 32767
    qint16 right;
}PCM_DATA;

typedef struct FLOAT_DATA
{
    float left; //32 bits per sample
    float right;
}FLOAT_DATA;

#pragma pack()

class WavFile
{
public:
    WavFile();
    ~WavFile();
    bool OpenRead(QString fname);
    bool OpenWrite(QString fname, int sampleRate, quint32 loFreq);
    CPX ReadSample();
    int ReadSamples(CPX *buf, int numSamples);
    bool WriteSamples(CPX *buf, int numSamples);
    bool Close();

    int GetSampleRate();

protected:
    bool writeMode;
    bool fileParsed;
    quint16 dataStart; //Offset in file where data starts, allows us to loop continuously
    char tmpBuf[256];
    PCM_DATA pcmBuf[4096]; //Max samples per read is 4096
    FLOAT_DATA floatBuf[4096];

    QFile *wavFile;
    //wav file data structs
    SUB_CHUNK subChunk;
    RIFF_CHUNK riff;
    SUB_CHUNK fmtSubChunkPre;
    FMT_SUB_CHUNK fmtSubChunk;
    SUB_CHUNK dataSubChunkPre;
    SUB_CHUNK factSubChunkPre;
    FACT_SUB_CHUNK factSubChunk;
    PCM_DATA pcmData;
    FLOAT_DATA floatData;

};

#endif // WAVFILE_H
