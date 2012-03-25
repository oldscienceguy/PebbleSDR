#ifndef MORSE_H
#define MORSE_H

#include "SignalProcessing.h"
#include "goertzel.h"

class Morse : public SignalProcessing
{
public:
    Morse(int sr, int fc);
    ~Morse();

    CPX * ProcessBlock(CPX * in);

protected:
    Goertzel *cwGoertzel;
};

#endif // MORSE_H
