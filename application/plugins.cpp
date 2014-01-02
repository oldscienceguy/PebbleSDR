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

QList<PluginInfo> Plugins::GetModemPluginInfo()
{
    QList<PluginInfo> info;
    //Add transition non-plugin modems
    PluginInfo p;
    p.name = "No Data";
    p.type = PluginInfo::MODEM_PSEUDO_PLUGIN;
    p.oldEnum = 0;
    info.append(p);

    p.name = "Band Data";
    p.oldEnum = 1;
    info.append(p);

    foreach(PluginInfo p, pluginInfoList)
        if (p.type == PluginInfo::MODEM_PLUGIN) {
            p.oldEnum = 2;
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
    p.name = "SR Ensemble";
    p.oldEnum = SDR::SR_ENSEMBLE;
    info.append(p);
    p.name = "SR Ensemble 2M";
    p.oldEnum = SDR::SR_ENSEMBLE_2M;
    info.append(p);
    p.name = "SR Ensemble 4M";
    p.oldEnum = SDR::SR_ENSEMBLE_4M;
    info.append(p);
    p.name = "SR Ensemble 6M";
    p.oldEnum = SDR::SR_ENSEMBLE_6M;
    info.append(p);
    p.name = "SR V9-ABPF";
    p.oldEnum = SDR::SR_V9;
    info.append(p);
    p.name = "SR LITE II";
    p.oldEnum = SDR::SR_LITE;
    info.append(p);
    p.name = "FiFi";
    p.oldEnum = SDR::FiFi;
    info.append(p);
    p.name = "Elektor SDR";
    p.oldEnum = SDR::ELEKTOR;
    info.append(p);
    p.name = "RFSpace SDR-IQ";
    p.oldEnum = SDR::SDR_IQ_USB;
    info.append(p);
    p.name = "RFSpace SDR-IP";
    p.oldEnum = SDR::SDR_IP_TCP;
    info.append(p);
    p.name = "HPSDR USB";
    p.oldEnum = SDR::HPSDR_USB;
    info.append(p);
    //p.name = "HPSDR TCP";
    //p.oldEnum = SDR::HPSDR_TCP;
    //info.append(p);
    p.name = "FUNcube Pro";
    p.oldEnum = SDR::FUNCUBE;
    info.append(p);
    p.name = "FUNcube Pro+";
    p.oldEnum = SDR::FUNCUBE_PLUS;
    info.append(p);
    p.name = "File";
    p.oldEnum = SDR::FILE;
    info.append(p);
    p.name = "RTL2832 Family";
    p.oldEnum = SDR::DVB_T;
    info.append(p);

    foreach(PluginInfo p, pluginInfoList)
        if (p.type == PluginInfo::DEVICE_PLUGIN) {
            p.oldEnum = 0;
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
        //Dump error string if having problems loading, for example can't find pebblelib
        //qDebug()<<loader.errorString();
        if (plugin) {
            DigitalModemInterface *iDigitalModem = qobject_cast<DigitalModemInterface *>(plugin);
            DeviceInterface *iDeviceInterface = qobject_cast<DeviceInterface *>(plugin);
            if (iDigitalModem) {
                //plugin supports interface
                pluginInfo.type = PluginInfo::MODEM_PLUGIN;
                pluginInfo.name = iDigitalModem->GetPluginName();
                pluginInfo.description = iDigitalModem->GetPluginDescription();
                pluginInfo.fileName = fileName;
                pluginInfo.modemInterface = iDigitalModem;
                pluginInfo.deviceInterface = NULL;
                pluginInfoList.append(pluginInfo);
            }
            if (iDeviceInterface) {
                //plugin supports interface
                pluginInfo.type = PluginInfo::DEVICE_PLUGIN;
                pluginInfo.name = iDeviceInterface->GetPluginName();
                pluginInfo.description = iDeviceInterface->GetPluginDescription();
                pluginInfo.fileName = fileName;
                pluginInfo.deviceInterface = iDeviceInterface;
                pluginInfo.modemInterface = NULL;
                pluginInfoList.append(pluginInfo);
            }
            //qDebug()<<plugin->Get
        }
    }
}
