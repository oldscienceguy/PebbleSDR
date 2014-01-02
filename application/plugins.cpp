//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "plugins.h"
#include <QtPlugin>
#include <QPluginLoader>

//Use this if we want to import a static plugin, ie one that is always installed
//Q_IMPORT_PLUGIN(...)

Plugins::Plugins()
{
    findPlugins();
}

QStringList Plugins::GetModemPluginNames()
{
    QStringList names;

    foreach(PluginInfo p, pluginInfoList)
        if (p.type == PluginInfo::MODEM_PLUGIN)
            names.append(p.name);
    return names;
}

QStringList Plugins::GetDevicePluginNames()
{
    QStringList names;

    foreach(PluginInfo p, pluginInfoList)
        if (p.type == PluginInfo::DEVICE_PLUGIN)
            names.append(p.name);
    return names;

}

DigitalModemInterface *Plugins::GetModemInterface(QString name)
{
    foreach(PluginInfo p, pluginInfoList) {
        if (p.name == name && p.type == PluginInfo::MODEM_PLUGIN)
            return p.modemInterface;
    }
    return NULL;
}

DeviceInterface *Plugins::GetDeviceInterface(QString name)
{
    foreach(PluginInfo p, pluginInfoList) {
        if (p.name == name && p.type == PluginInfo::DEVICE_PLUGIN)
            return p.deviceInterface;
    }
    return NULL;
}

void Plugins::findPlugins()
{
    //Load static plugins
    foreach (QObject *plugin, QPluginLoader::staticInstances())
        qDebug()<<plugin->objectName();
        //populateMenus(plugin);

    pluginsDir = QDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp(); //Root dir where app is located
    }
#endif
    pluginsDir.cd("plugins");

    PluginInfo pluginInfo;

    //Load dynamic plugins
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        //qDebug()<<fileName;

        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        //Dump error string if having problems loading, for example can't find pebblelib
        //qDebug()<<loader.errorString();
        if (plugin) {
            DigitalModemInterface *iDigitalModem = qobject_cast<DigitalModemInterface *>(plugin);
            DeviceInterface *iDeviceInterface = qobject_cast<DeviceInterface *>(plugin);
            if (iDigitalModem) {
                //plugin supports interface
                pluginInfo.type = PluginInfo::MODEM_PLUGIN;
                pluginInfo.name = iDigitalModem->GetPluginName();
                pluginInfo.description = iDigitalModem->GetDescription();
                pluginInfo.fileName = fileName;
                pluginInfo.modemInterface = iDigitalModem;
                pluginInfo.deviceInterface = NULL;
                pluginInfoList.append(pluginInfo);
            }
            if (iDigitalModem) {
                //plugin supports interface
                pluginInfo.type = PluginInfo::DEVICE_PLUGIN;
                pluginInfo.name = iDigitalModem->GetPluginName();
                pluginInfo.description = iDigitalModem->GetDescription();
                pluginInfo.fileName = fileName;
                pluginInfo.deviceInterface = iDeviceInterface;
                pluginInfo.modemInterface = NULL;
                pluginInfoList.append(pluginInfo);
            }
            //qDebug()<<plugin->Get
        }
    }
}
