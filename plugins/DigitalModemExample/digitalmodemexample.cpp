//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "digitalmodemexample.h"
#include "gpl.h"
#include <QDebug>

DigitalModemExample::DigitalModemExample()
{
    dataUi = NULL;
}

void DigitalModemExample::SetSampleRate(int _sampleRate, int _sampleCount)
{
    Q_UNUSED(_sampleRate);
    Q_UNUSED(_sampleCount);
}

void DigitalModemExample::SetDemodMode(DeviceInterface::DEMODMODE _demodMode)
{
	Q_UNUSED(_demodMode);
}

CPX *DigitalModemExample::ProcessBlock(CPX *in)
{
    return in;
}

void DigitalModemExample::SetupDataUi(QWidget *parent)
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

QString DigitalModemExample::GetPluginName()
{
    return "Example";
}

QString DigitalModemExample::GetPluginDescription()
{
    return "Example of digial modem plugin for Pebble SDR";
}
