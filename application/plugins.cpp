#include "plugins.h"
#include <QtPlugin>
#include <QPluginLoader>

//Use this if we want to import a static plugin, ie one that is always installed
//Q_IMPORT_PLUGIN(...)

Plugins::Plugins()
{
    findPlugins();
}

QStringList Plugins::GetPluginNames()
{
    QStringList names;

    foreach(PluginInfo p, pluginInfoList)
        names.append(p.name);
    return names;
}

DigitalModemInterface *Plugins::GetInterface(QString name)
{
    foreach(PluginInfo p, pluginInfoList) {
        if (p.name == name)
            return p.interface;
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
        qDebug()<<loader.errorString();
        if (plugin) {
            DigitalModemInterface *iDigitalModem = qobject_cast<DigitalModemInterface *>(plugin);
            if (iDigitalModem) {
                //plugin supports interface
                pluginInfo.name = iDigitalModem->GetPluginName();
                pluginInfo.description = iDigitalModem->GetDescription();
                pluginInfo.fileName = fileName;
                pluginInfo.interface = iDigitalModem;
                pluginInfoList.append(pluginInfo);
            }
            //qDebug()<<plugin->Get
        }
    }
}
