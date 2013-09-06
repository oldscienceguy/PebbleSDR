#ifndef DIGITALMODEMEXAMPLE_H
#define DIGITALMODEMEXAMPLE_H

#include "../application/digital_modem_interfaces.h"
#include "ui/ui_data-example.h"

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
    //Setup demod mode etc
    void SetDemodMode(DEMODMODE m);

    //Process samples
    CPX * ProcessBlock(CPX * in);

    //Setup UI in context of parent
    void SetupDataUi(QWidget *parent);

    //Info - return plugin name for menus
    QString GetPluginName();

    //Additional info
    QString GetDescription();

private:
    Ui::dataExample *dataUi;

};


#endif // DIGITALMODEMEXAMPLE_H
