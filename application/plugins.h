#ifndef PLUGINS_H
#define PLUGINS_H

#include "gpl.h"
#include "global.h"
#include "settings.h"
#include <QDir>
#include <QList>

#include "digital_modem_interfaces.h"

struct PluginInfo
{
    QString name;
    QString description;
    QString fileName;
    DigitalModemInterface *interface;
};

class Plugins
{
public:
    Plugins();
    //For menus
    QStringList GetPluginNames();

    DigitalModemInterface *GetInterface(QString name);

private:
    void findPlugins();
    QDir pluginsDir;
    QList<PluginInfo> pluginInfoList;
};

#endif // PLUGINS_H
