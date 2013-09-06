#ifndef DIGITALMODEMEXAMPLE_H
#define DIGITALMODEMEXAMPLE_H

#include "digital_modem_interfaces.h"

class DigitalModemExample  : public QObject, public DigitalModemInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DigitalModemInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DigitalModemInterface)

public:
    //No constructor for plugins
    QString GetPluginName();
};


#endif // DIGITALMODEMEXAMPLE_H
