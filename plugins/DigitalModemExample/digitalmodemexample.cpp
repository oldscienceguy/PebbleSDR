//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "digitalmodemexample.h"
#include "gpl.h"
#include <QDebug>

DigitalModemExample::DigitalModemExample()
{
	dataUi = NULL;
}

//Implement pure virtual destructor from interface, otherwise we don't link
DigitalModemInterface::~DigitalModemInterface()
{

}

DigitalModemExample::~DigitalModemExample()
{

}

void DigitalModemExample::setSampleRate(int _sampleRate, int _sampleCount)
{
    Q_UNUSED(_sampleRate);
    Q_UNUSED(_sampleCount);
}

void DigitalModemExample::setDemodMode(DeviceInterface::DemodMode _demodMode)
{
	Q_UNUSED(_demodMode);
}

CPX *DigitalModemExample::processBlock(CPX *in)
{
    return in;
}

void DigitalModemExample::setupDataUi(QWidget *parent)
{
    if (parent == NULL) {
        //We want to delete
        if (dataUi != NULL) {
            delete dataUi;
        }
        dataUi = NULL;
        return;
    } else if (dataUi == NULL) {
        //Create new one
        dataUi = new Ui::dataExample();
        dataUi->setupUi(parent);
    }
}

QString DigitalModemExample::getPluginName()
{
    return "Example";
}

QString DigitalModemExample::getPluginDescription()
{
    return "Example of digial modem plugin for Pebble SDR";
}
