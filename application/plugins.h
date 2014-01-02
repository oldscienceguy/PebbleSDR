#ifndef PLUGINS_H
#define PLUGINS_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "gpl.h"
#include "global.h"
#include "settings.h"
#include <QDir>
#include <QList>

#include "../pebblelib/digital_modem_interfaces.h"
#include "../pebblelib/device_interfaces.h"

struct PluginInfo
{
    enum PluginType {MODEM_PLUGIN, DEVICE_PLUGIN};
    PluginType type;
    QString name;
    QString description;
    QString fileName;
    DigitalModemInterface *modemInterface;
    DeviceInterface *deviceInterface;
};

class Plugins
{
public:
    Plugins();
    //For menus
    QStringList GetModemPluginNames();
    QStringList GetDevicePluginNames();

    DigitalModemInterface *GetModemInterface(QString name);
    DeviceInterface *GetDeviceInterface(QString name);

private:
    void findPlugins();
    QDir pluginsDir;
    QList<PluginInfo> pluginInfoList;
};

#endif // PLUGINS_H
