//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rttydigitalmodem.h"

RttyDigitalModem::RttyDigitalModem()
{
}

//Implement pure virtual destructor from interface, otherwise we don't link
DigitalModemInterface::~DigitalModemInterface()
{

}

RttyDigitalModem::~RttyDigitalModem()
{

}

void RttyDigitalModem::setSampleRate(int _sampleRate, int _sampleCount)
{
    Q_UNUSED(_sampleRate);
    Q_UNUSED(_sampleCount);
}

void RttyDigitalModem::setDemodMode(DeviceInterface::DEMODMODE _demodMode)
{
	Q_UNUSED(_demodMode);
    //qDebug()<<"Demod mode = "<<m; //Demod::ModeToString(m);
}

CPX *RttyDigitalModem::processBlock(CPX *in)
{
    return in;
}

void RttyDigitalModem::setupDataUi(QWidget *parent)
{
	Q_UNUSED(parent);
    return;
#if 0
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
#endif
}

QString RttyDigitalModem::getPluginName()
{
    return "RTTY";
}

QString RttyDigitalModem::getPluginDescription()
{
    return "Rtty digital modem";
}
