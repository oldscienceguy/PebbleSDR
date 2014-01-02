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
    enum PluginType {MODEM_PLUGIN, DEVICE_PLUGIN, MODEM_PSEUDO_PLUGIN, DEVICE_PSEUDO_PLUGIN};
    PluginType type;
    QString name;
    QString description;
    QString fileName;
    DigitalModemInterface *modemInterface;
    DeviceInterface *deviceInterface;
};
//Add to QT metatype so we can use QVariants with this in UI lists
//QVariant v; v.setValue(p);
//QVariant v; p = v.value<PluginInfo>()
Q_DECLARE_METATYPE(PluginInfo)

class Plugins
{
public:
    Plugins(Receiver *_receiver, Settings *_settings);
    //For menus
    QList<PluginInfo> GetModemPluginInfo();
    QList<PluginInfo> GetDevicePluginInfo();

    DigitalModemInterface *GetModemInterface(QString name);
    DeviceInterface *GetDeviceInterface(QString name);

private:
    void findPlugins();
    QDir pluginsDir;
    QList<PluginInfo> pluginInfoList;
    //Temp for internal devices
    Receiver *receiver;
    Settings *settings;
};

#endif // PLUGINS_H
