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
	~RttyDigitalModem();

    void setSampleRate(int _sampleRate, int _sampleCount);

    //Setup demod mode etc
    void setDemodMode(DeviceInterface::DEMODMODE _demodMode);

    //Process samples
    CPX * processBlock(CPX * in);

    //Setup UI in context of parent
    void setupDataUi(QWidget *parent);

    //Info - return plugin name for menus
    QString getPluginName();

    //Additional info
    QString getPluginDescription();

    QObject *asQObject() {return (QObject *)this;}

signals:
    void testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
    void testbench(int _length, double* _buf, double _sampleRate, int _profile);
    bool addProfile(QString profileName, int profileNumber); //false if profilenumber already exists
    void removeProfile(quint16 profileNumber);

private:
    //Ui::dataExample *dataUi;

};

#endif // RTTYDIGITALMODEM_H
