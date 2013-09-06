#ifndef DIGITAL_MODEM_INTERFACES_H
#define DIGITAL_MODEM_INTERFACES_H

#include <QtCore>
#include <QtPlugin>

class DigitalModemInterface
{
public:
    //Interface must be all pure virtual functions

    //Setup demod mode etc
    //ProcessBlock
    //SetupDataUi
    //Info - return plugin name for menu, other factoids
    virtual QString GetPluginName() = 0;
    //About dialog
};

//How best to encode version number in interface
#define DigitalModemInterface_iid "Pebble.DigitalModemInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DigitalModemInterface, DigitalModemInterface_iid)

#endif // DIGITAL_MODEM_INTERFACES_H
