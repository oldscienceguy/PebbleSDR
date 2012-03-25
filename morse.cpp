#include "morse.h"

Morse::Morse(int sr, int fc) : SignalProcessing(sr,fc)
{
    //Setup Goertzel
    cwGoertzel = new Goertzel(40);
    cwGoertzel->SetFreqHz(1000,8000);

}

Morse::~Morse()
{
    if (cwGoertzel != NULL) delete cwGoertzel;

}

CPX * Morse::ProcessBlock(CPX * in)
{
    //If Goretzel is 8k and Receiver is 96k, decimate factor is 12.  Only process every 12th sample
    int decimate = sampleRate / cwGoertzel->sampleRate;
    float power;
    for (int i=0; i<NumSamples(); i=i+decimate) {
        power = cwGoertzel->FPNextSample(in[i].re);
    }
    return in;
}
