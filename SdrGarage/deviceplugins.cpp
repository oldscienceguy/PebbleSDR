#include "deviceplugins.h"
#include <QtPlugin>
#include <QPluginLoader>

DevicePlugins::DevicePlugins()
{
    findPlugins();
}

void DevicePlugins::findPlugins()
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
    //qDebug()<<pluginsDir;

    PluginInfo pluginInfo;

    //Load dynamic plugins
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        //qDebug()<<fileName;

        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (plugin) {
            DeviceInterface *iDeviceInterface = qobject_cast<DeviceInterface *>(plugin);
            if (iDeviceInterface) {
                //plugin supports device interface
                //plugin can support multiple similar devices, ie softrock family
                //Get # devices plugin supports
                //For each device, create plugin info and SDR with device interface and device #
                int numDevices = iDeviceInterface->GetNumDevices();
                for (int i = 0; i<numDevices; i++) {
                    pluginInfo.type = PluginInfo::DEVICE_PLUGIN;
                    pluginInfo.name = iDeviceInterface->GetPluginName(i);
                    pluginInfo.description = iDeviceInterface->GetPluginDescription(i);
                    pluginInfo.fileName = fileName;
                    pluginInfo.deviceNumber  = i;
                    pluginInfo.deviceInterface = iDeviceInterface;
                    pluginInfoList.append(pluginInfo);
                    qDebug()<<"Found plugin "<<pluginInfo.name;
                }
            }
        } else {
            //Dump error string if having problems loading, for example can't find pebblelib
            qDebug()<<loader.errorString();
        }
    }
}
