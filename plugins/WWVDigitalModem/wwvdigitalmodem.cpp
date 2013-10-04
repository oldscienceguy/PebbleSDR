#include "wwvdigitalmodem.h"
#include "gpl.h"
#include <QDebug>

WWVDigitalModem::WWVDigitalModem()
{
    dataUi = NULL;
}

void WWVDigitalModem::SetSampleRate(int _sampleRate, int _sampleCount)
{
    Q_UNUSED(_sampleRate);
    Q_UNUSED(_sampleCount);
}

void WWVDigitalModem::SetDemodMode(DEMODMODE _demodMode)
{
}

CPX *WWVDigitalModem::ProcessBlock(CPX *in)
{
    return in;
}

void WWVDigitalModem::SetupDataUi(QWidget *parent)
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
        dataUi = new Ui::dataWWV();
        dataUi->setupUi(parent);
    }
}

QString WWVDigitalModem::GetPluginName()
{
    return "WWV";
}

QString WWVDigitalModem::GetDescription()
{
    return "Example of digial modem plugin for Pebble SDR";
}
