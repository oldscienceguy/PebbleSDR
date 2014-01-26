#ifndef DEVICEPLUGINS_H
#define DEVICEPLUGINS_H
#include "device_interfaces.h"

#include <QDir>


struct PluginInfo
{
    enum PluginType {MODEM_PLUGIN, DEVICE_PLUGIN, MODEM_PSEUDO_PLUGIN, DEVICE_PSEUDO_PLUGIN};
    PluginType type;
    QString name;
    QString description;
    QString fileName;
    quint16 deviceNumber; //Combined with filename to reference multiple devices in one plugin
    DeviceInterface *deviceInterface;
};

class DevicePlugins
{
public:
    DevicePlugins();
    void findPlugins();

private:
    QDir pluginsDir;
    QList<PluginInfo> pluginInfoList;

};

#endif // DEVICEPLUGINS_H
