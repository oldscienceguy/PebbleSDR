#ifndef DIGITAL_MODEM_INTERFACES_H
#define DIGITAL_MODEM_INTERFACES_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QWidget>
#include "device_interfaces.h"
#include "pebblelib_global.h"
#include "cpx.h"

class PEBBLELIBSHARED_EXPORT DigitalModemInterface
{
public:
    //Interface must be all pure virtual functions
	virtual ~DigitalModemInterface() = 0;

	virtual void setSampleRate(int _sampleRate, int _sampleCount) = 0;

    //Setup demod mode etc
	virtual void setDemodMode(DeviceInterface::DemodMode _demodMode) = 0;

    //Process samples
	virtual CPX * processBlock(CPX * in) = 0;

    //Setup UI in context of parent
	virtual void setupDataUi(QWidget *parent) = 0;

    //Info - return plugin name for menus
	virtual QString getPluginName() = 0;

    //Additional info
	virtual QString getPluginDescription() = 0;

    //Any plugins that need to output to Testbench should declare this signal
    //void Testbench(quint16 _length, CPX* _buf, double _sampleRate, quint16 _profile);
    //To connect to this signal, we will need a QObject pointer where connection is established
    //We can't just have interface inherit from QObject (lots of ambiguous reference errors)
    //so we ask the plugin itself to return it's QObject pointer
    virtual QObject* asQObject() = 0;

signals:
	void testbench(int _length, CPX* _buf, double _sampleRate, int _profile);
	void testbench(int _length, double* _buf, double _sampleRate, int _profile);
	bool addProfile(QString profileName, int profileNumber); //false if profilenumber already exists
	void removeProfile(quint16 profileNumber);


};

//How best to encode version number in interface
#define DigitalModemInterface_iid "N1DDY.Pebble.DigitalModemInterface.V1"

//Creates cast macro for interface ie qobject_cast<DigitalModemInterface *>(plugin);
Q_DECLARE_INTERFACE(DigitalModemInterface, DigitalModemInterface_iid)

#endif // DIGITAL_MODEM_INTERFACES_H
