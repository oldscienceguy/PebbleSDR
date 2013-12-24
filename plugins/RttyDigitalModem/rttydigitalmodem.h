#ifndef RTTYDIGITALMODEM_H
#define RTTYDIGITALMODEM_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "../pebblelib/digital_modem_interfaces.h"
//#include "ui/ui_data-example.h"

class RttyDigitalModem  : public QObject, public DigitalModemInterface
{
    Q_OBJECT

    //Exports, FILE is optional
    //IID must be same that caller is looking for, defined in interfaces file
    Q_PLUGIN_METADATA(IID DigitalModemInterface_iid)
    //Let Qt meta-object know about our interface
    Q_INTERFACES(DigitalModemInterface)

public:

    RttyDigitalModem();

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

    QObject *asQObject() {return (QObject *)this;}

signals:
    void Testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
    void Testbench(int _length, double* _buf, double _sampleRate, int _profile);
    bool AddProfile(QString profileName, int profileNumber); //false if profilenumber already exists
    void RemoveProfile(quint16 profileNumber);

private:
    //Ui::dataExample *dataUi;

};

#endif // RTTYDIGITALMODEM_H
