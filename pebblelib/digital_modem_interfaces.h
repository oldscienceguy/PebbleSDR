#ifndef DIGITAL_MODEM_INTERFACES_H
#define DIGITAL_MODEM_INTERFACES_H
#include "gpl.h"
#include <QWidget>
#include "pebblelib_global.h"

class CPX;
class PEBBLELIBSHARED_EXPORT DigitalModemInterface
{
public:
    //Interface must be all pure virtual functions

    virtual void SetSampleRate(int _sampleRate, int _sampleCount) = 0;

    //Setup demod mode etc
    virtual void SetDemodMode(DEMODMODE _demodMode) = 0;

    //Process samples
    virtual CPX * ProcessBlock(CPX * in) = 0;

    //Setup UI in context of parent
    virtual void SetupDataUi(QWidget *parent) = 0;

    //Info - return plugin name for menus
    virtual QString GetPluginName() = 0;

    //Additional info
    virtual QString GetDescription() = 0;

    //Any plugins that need to output to Testbench should declare this signal
    //void Testbench(quint16 _length, CPX* _buf, double _sampleRate, quint16 _profile);
    //To connect to this signal, we will need a QObject pointer where connection is established
    //We can't just have interface inherit from QObject (lots of ambiguous reference errors)
    //so we ask the plugin itself to return it's QObject pointer
    virtual QObject* asQObject() = 0;

};

//How best to encode version number in interface
#define DigitalModemInterface_iid "N1DDY.Pebble.DigitalModemInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DigitalModemInterface, DigitalModemInterface_iid)

#endif // DIGITAL_MODEM_INTERFACES_H
