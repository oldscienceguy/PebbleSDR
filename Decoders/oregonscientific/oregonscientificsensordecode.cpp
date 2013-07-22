#include "oregonscientificsensordecode.h"

/*
 * Experiments based on article by Kevin Mehall https://github.com/kevinmehall/rtlsdr-433m-sensor
 * https://code.google.com/p/thn128receiver/downloads/detail?name=RXOSv1.zip&can=2&q=
 *
 * Best: http://wmrx00.sourceforge.net/Arduino/OregonScientific-RF-Protocols.pdf
 *
 * Manchester Decoding http://en.wikipedia.org/wiki/Manchester_encoding
 *
 * Need to normalize sample rate to time in lieu of an explicit interupt clock like other examples
 * V1   Clock 342hz
 * V2/3 CLock 1024hz
 * SamplesPerClock = SampleRate/ClockRate 48000/342 = 140.35 samples / clock tick
 * How do we keep from slowly drifting?  Fractal Resample to even clock rate?
 */
OregonScientificSensorDecode::OregonScientificSensorDecode() :
    sensorFreq(433.8e6),
    sensorFreqOffset(100e3),
    threshold(-0.35)
{

}

bool OregonScientificSensorDecode::ProcessData()
{
    switch (state) {
        case Wait:
            break;
        case Preamble:
            break;
        case Sync:
            break;
        case Data:
            break;
    }

}

class OOKElement
{
public:
    OOKElement() {Reset();}

    void Reset() {state=false;count=0;complete=false;}

    bool state; //Hi or low
    quint16 count; //How long was this element in sample units
    bool complete; //True if state change was detected and count has real length
};

OOKElement curElement;
OOKElement lastElement;

//Looks for next on/off (OOK) transition in stream of samples
//Returns #samples at current state == timing
//Share with Morse decoder?
bool DetectTransition(CPX *samples, quint16 numSamples, float threshold=-0.35)
{
    bool curState = curElement.state;
    bool state;
    for (int i=0; i<numSamples; i++) {
        //Just test re for now
        state =  samples[i].re > threshold;

        if (state != curState) {
            //We have a transition
            //Return new state, sampleCount
            curElement.count += i;
            curElement.complete = true; //Being returned as a result of state change
            lastElement = curElement;
            curElement.Reset();
            curElement.state = state;
            return true; //New element
        }
    }
    //No state changes, just inc count and keep going
    curElement.count += numSamples;
    curElement.complete = false; //No state change, count may be continued in next call
    return false; //Same element
}
