//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rttydigitalmodem.h"

RttyDigitalModem::RttyDigitalModem()
{
}

void RttyDigitalModem::SetSampleRate(int _sampleRate, int _sampleCount)
{
    Q_UNUSED(_sampleRate);
    Q_UNUSED(_sampleCount);
}

void RttyDigitalModem::SetDemodMode(DEMODMODE _demodMode)
{
    //qDebug()<<"Demod mode = "<<m; //Demod::ModeToString(m);
}

CPX *RttyDigitalModem::ProcessBlock(CPX *in)
{
    return in;
}

void RttyDigitalModem::SetupDataUi(QWidget *parent)
{
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

QString RttyDigitalModem::GetPluginName()
{
    return "RTTY";
}

QString RttyDigitalModem::GetPluginDescription()
{
    return "Rtty digital modem";
}
