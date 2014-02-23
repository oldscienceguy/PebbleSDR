//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "plugins.h"
#include <QtPlugin>
#include <QPluginLoader>
//Temp for internal devices, will be deleted when everything is a plugin
#include "devices/sdr_iq.h"
#include "devices/sdr_ip.h"

//Use this if we want to import a static plugin, ie one that is always installed
//Q_IMPORT_PLUGIN(...)

Plugins::Plugins(Receiver *_receiver, Settings *_settings)
{
    receiver = _receiver;
    settings = _settings;
    findPlugins();
}

QList<PluginInfo> Plugins::GetModemPluginInfo()
{
    QList<PluginInfo> info;
    //Add transition non-plugin modems
    PluginInfo p;
    p.name = "No Data";
    p.fileName = "No_Data";
    p.type = PluginInfo::MODEM_PSEUDO_PLUGIN;
    info.append(p);

    p.name = "Band Data";
    p.fileName = "Band_Data";
    info.append(p);

    foreach(PluginInfo p, pluginInfoList)
        if (p.type == PluginInfo::MODEM_PLUGIN) {
            info.append(p);
        }
    return info;

}

QList<PluginInfo> Plugins::GetDevicePluginInfo()
{
    QList<PluginInfo> info;
    //Add transition non-plugin devices
    PluginInfo p;
    p.type = PluginInfo::DEVICE_PSEUDO_PLUGIN;

    p.name = "RFSpace SDR-IQ";
    p.fileName = "SDR_IQ_USB";
    p.deviceInterface = new SDR_IQ(receiver, SDR::SDR_IQ_USB,settings);
    info.append(p);

    p.name = "RFSpace SDR-IP";
    p.fileName = "SDR_IP_TCP";
    p.deviceInterface = new SDR_IP(receiver, SDR::SDR_IP_TCP,settings);
    info.append(p);

    foreach(PluginInfo p, pluginInfoList)
        if (p.type == PluginInfo::DEVICE_PLUGIN) {
            info.append(p);
        }
    return info;


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
        if (plugin == NULL) {
            //Dump error string if having problems loading, for example can't find pebblelib
            qDebug()<<loader.errorString();
        } else {
            DigitalModemInterface *iDigitalModem = qobject_cast<DigitalModemInterface *>(plugin);
			DeviceInterface *iDeviceInterface = qobject_cast<DeviceInterface *>(plugin);
            if (iDigitalModem) {
                //plugin supports interface
                pluginInfo.type = PluginInfo::MODEM_PLUGIN;
                pluginInfo.name = iDigitalModem->GetPluginName();
                pluginInfo.description = iDigitalModem->GetPluginDescription();
                pluginInfo.fileName = fileName;
                pluginInfo.deviceNumber  = 0;
                pluginInfo.modemInterface = iDigitalModem;
                pluginInfo.deviceInterface = NULL;
                pluginInfoList.append(pluginInfo);
            }
            if (iDeviceInterface) {
                //plugin supports device interface
                //plugin can support multiple similar devices, ie softrock family
                //Get # devices plugin supports
                //For each device, create plugin info and SDR with device interface and device #
				int numDevices = iDeviceInterface->Get(DeviceInterface::PluginNumDevices).toInt();
                for (int i = 0; i<numDevices; i++) {
                    pluginInfo.type = PluginInfo::DEVICE_PLUGIN;
					pluginInfo.name = iDeviceInterface->Get(DeviceInterface::PluginName,i).toString();
					pluginInfo.description = iDeviceInterface->Get(DeviceInterface::PluginDescription,i).toString();
                    pluginInfo.fileName = fileName;
                    pluginInfo.deviceNumber  = i;
					pluginInfo.deviceInterface = new SDR(iDeviceInterface, i, receiver, settings);
                    pluginInfo.modemInterface = NULL;
                    pluginInfoList.append(pluginInfo);
                }
            }
        }
    }
}
