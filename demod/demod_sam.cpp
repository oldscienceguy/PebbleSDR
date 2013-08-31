#include "demod_sam.h"

Demod_SAM::Demod_SAM(int _inputRate, int _numSamples) :
    Demod(_inputRate, _numSamples)
{
    //SAM config
    samLoLimit = -2000 * TWOPI/sampleRate; //PLL range
    samHiLimit = 2000 * TWOPI/sampleRate;
    samBandwidth = 300;
    samLockCurrent = 0.5;
    samLockPrevious = 1.0;
    samDc = 0.0;

    //From set demod mode
    alpha = 0.3 * samBandwidth * TWOPI/sampleRate;
    beta = alpha * alpha * 0.25;
    samLockCurrent = 0.5;
    samLockPrevious = 1.0;
    samDc = 0.0;

}

Demod_SAM::~Demod_SAM()
{

}

void Demod_SAM::ProcessBlock(  CPX * in, CPX * out, int demodSamples )
{
    CPX delay;

    for (int i = 0; i < demodSamples; i++) {
        delay = PLL(in[i],samLoLimit,samHiLimit, demodSamples);
        //Basic am demod
        samLockCurrent = 0.999f * samLockCurrent + 0.001f * fabs(delay.im);
        samLockPrevious = samLockCurrent;
        samDc = (0.9999f * samDc) + (0.0001f * delay.re);
        out[i].re = out[i].im = (delay.re - samDc) *.50 ;

    }
}
