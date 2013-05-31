#ifndef SDRFILE_H
#define SDRFILE_H

#include "SDR.h"
#include "QFile"
#include "NCO.h"

/*
https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
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
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some
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
typedef struct RIFF_CHUNK
{
    quint8 id[4];
    quint32 size;
    quint8 format[4];

}RIFF_CHUNK;

typedef struct FMT_SUB_CHUNK
{
    quint8 id[4];
    quint32 size;
    quint16 format;
    quint16 channels;
    quint32 sampleRate;
    quint32 byteRate;
    quint16 blockAlign;
    quint16 bitsPerSample;
    quint16 extraParamSize; //If not 0, then we have extra to read

}FMT_SUB_CHUNK;
typedef struct DATA_SUB_CHUNK
{
    quint8 id[4];
    quint32 size;
    //Data follows

}DATA_SUB_CHUNK;
//assumes 16bits per sample
typedef struct WAV_DATA
{
    qint16 left; //+/- 32767
    qint16 right;
}WAV_DATA;

#pragma pack()


class SDRFile : public SDR
{
public:
    SDRFile(Receiver *receiver, SDRDEVICE dev, Settings *_settings);
    ~SDRFile(void);

    bool Connect();
    bool Disconnect();
    double SetFrequency(double fRequested,double fCurrent);
    void ShowOptions();
    void Start();
    void Stop();
    void ReadSettings();
    void WriteSettings();

    double GetStartupFrequency();
    int GetStartupMode();
    double GetHighLimit();
    double GetLowLimit();
    double GetGain();
    QString GetDeviceName();
    int GetSampleRate();

protected:
    QSettings *qSettings;

    CPX *inBuffer;
    CPX *outBuffer;
    int framesPerBuffer;
    virtual void StopProducerThread();
    virtual void RunProducerThread();
    virtual void StopConsumerThread();
    virtual void RunConsumerThread();

    bool OpenWavFile(QString fname);
    QFile *wavFile;
    //wav file data structs
    RIFF_CHUNK riff;
    FMT_SUB_CHUNK fmtSubChunk;
    DATA_SUB_CHUNK dataSubChunk;
    WAV_DATA data;

    //Testing NCO
    NCO *nco;


};

#endif // SDRFILE_H
