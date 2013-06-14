#ifndef OREGONSCIENTIFICSENSORDECODE_H
#define OREGONSCIENTIFICSENSORDECODE_H

#include "cpx.h"
#include "QtCore"

class OregonScientificSensorDecode
{
public:
    OregonScientificSensorDecode();

    enum Direction {
        Rising,
        Falling
    };

    enum States {
        Wait,
        Preamble,
        Sync,
        Data
    };

    class Element {
        Direction dir;
        bool complete;
        quint16 count;
    };

    States state;
    const double sensorFreq; //Sensors transmit on 433.9MHz
    //The RTL-SDR has more noise near the center frequency, so we tune to the side
    //and then shift the frequency in the low-pass filter.
    const double sensorFreqOffset;

    //Threshold for OOK HIGH level
    double threshold;

    bool ProcessData();
};

#endif // OREGONSCIENTIFICSENSORDECODE_H
