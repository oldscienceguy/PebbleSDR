#ifndef MORSE_H
#define MORSE_H

#include "SignalProcessing.h"
#include "goertzel.h"
#include "qframe"
#include "QTextEdit"

class Receiver;

//We need to inherit from QObject so we can filter events.
//Remove if we eventually create a separate 'morse widget' object for UI
class Morse : public SignalProcessing, QObject
{

public:
    Morse(int sr, int fc);
    ~Morse();

    CPX * ProcessBlock(CPX * in);

    void SetReceiver(Receiver *_rcv);


    //Updated by ProcessBlock and holds power levels.
    //Note this is smaller buffer than frameCount because of decimation
    float *powerBuf;  //Not sure if we need this

    bool *toneBuf; //true if goertzel returns tone present

    //Returns tcw in ms for any given WPM
    int WpmToTcw(int w);

    const char * MorseToAscii(quint16 morse);
    const char * MorseToDotDash(quint16 morse);


protected:
    Receiver *rcv;

    Goertzel *cwGoertzel;
    int goertzelFreq; //CW tone we're looking for, normally 700hz
    int goertzelSampleRate; //Goertzel filter only needs 8k sample rate.  Bigger means more bins and slower
    int decimateFactor; //receiver sample rate decmiate factor to get to goertzel sample rate
    int powerBufSize;
    int toneBufSize;

    int toneBufCounter;

    //Temp for painting
    char *outString;
    bool outTone;

    int maxWPM; //Determines bin resolution
    bool fixedWPM;
    int wpm;
    int numResultsPerTcw;

    //Are we counting to see if we have a dot/dash mark or a element/word space
    enum {NOT_COUNTING, MARK_COUNTING, SPACE_COUNTING} countingState;
    enum {LETTER,WORD,SPACE} elementState;
    int markCount;
    int spaceCount;


};

#endif // MORSE_H
