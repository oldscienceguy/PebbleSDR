#ifndef MORSE_H
#define MORSE_H

#include "SignalProcessing.h"
#include "goertzel.h"
#include "qframe"
#include "QTextEdit"

//We need to inherit from QObject so we can filter events.
//Remove if we eventually create a separate 'morse widget' object for UI
class Morse : public SignalProcessing, QObject
{

public:
    Morse(int sr, int fc);
    ~Morse();

    CPX * ProcessBlock(CPX * in);

    void ConnectToUI(QFrame *_meter,QTextEdit *edit);


    //Updated by ProcessBlock and holds power levels.
    //Note this is smaller buffer than frameCount because of decimation
    float *powerBuf;  //Not sure if we need this

    bool *toneBuf; //true if goertzel returns tone present

    //Returns tcw in ms for any given WPM
    int WpmToTcw(int w);

protected:
    //Temp so we can filter events
    bool eventFilter(QObject *o, QEvent *e);
    QFrame *uiMeter;
    QTextEdit *uiTextEdit;

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

};

#endif // MORSE_H
