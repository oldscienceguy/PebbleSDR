#ifndef DIGITAL_MODEM_INTERFACES_H
#define DIGITAL_MODEM_INTERFACES_H

class DigitalModemInterface
{
    //Interface must be all pure virtual functions

    //Setup demod mode etc
    //ProcessBlock
    //SetupDataUi
    //Info - return plugin name for menu, other factoids
    //About dialog
};

//How best to encode version number in interface
#define DigitalModemInterface_iid "Pebble.DigitalModemInterface.V1"

Q_DECLARE_INTERFACE(DigitalModemInterface, DigitalModemInterface_iid)

#endif // DIGITAL_MODEM_INTERFACES_H
