#include "digitalmodemexample.h"
#include "gpl.h"

void DigitalModemExample::SetDemodMode(DEMODMODE m)
{
    qDebug()<<"Demod mode = "<<m; //Demod::ModeToString(m);
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
    return "Hello World";
}

QString DigitalModemExample::GetDescription()
{
    return "Example of digial modem plugin for Pebble SDR";
}
