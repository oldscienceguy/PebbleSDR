#ifndef WWVDIGITALMODEM_H
#define WWVDIGITALMODEM_H

#include "../pebblelib/digital_modem_interfaces.h"
#include "ui/ui_data-wwv.h"

class WWVDigitalModem  : public QObject, public DigitalModemInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DigitalModemInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DigitalModemInterface)

public:

    WWVDigitalModem();

    void SetSampleRate(int _sampleRate, int _sampleCount);

    //Setup demod mode etc
    void SetDemodMode(DEMODMODE _demodMode);

    //Process samples
    CPX * ProcessBlock(CPX * in);

    //Setup UI in context of parent
    void SetupDataUi(QWidget *parent);

    //Info - return plugin name for menus
    QString GetPluginName();

    //Additional info
    QString GetDescription();

private:
    Ui::dataWWV *dataUi;

};


#endif // WWVDIGITALMODEM_H
