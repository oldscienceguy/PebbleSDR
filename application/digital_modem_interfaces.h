#ifndef DIGITAL_MODEM_INTERFACES_H
#define DIGITAL_MODEM_INTERFACES_H
#include "gpl.h"
#include "cpx.h"
#include "demod.h"
#include <QtCore>
#include <QtPlugin>

class DigitalModemInterface
{
public:
    //Interface must be all pure virtual functions

    virtual void SetSampleRate(int _sampleRate, int _sampleCount) = 0;

    //Setup demod mode etc
    virtual void SetDemodMode(DEMODMODE m) = 0;

    //Process samples
    virtual CPX * ProcessBlock(CPX * in) = 0;

    //Setup UI in context of parent
    virtual void SetupDataUi(QWidget *parent) = 0;

    //Info - return plugin name for menus
    virtual QString GetPluginName() = 0;

    //Additional info
    virtual QString GetDescription() = 0;
};

//How best to encode version number in interface
#define DigitalModemInterface_iid "N1DDY.Pebble.DigitalModemInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DigitalModemInterface, DigitalModemInterface_iid)

#endif // DIGITAL_MODEM_INTERFACES_H
