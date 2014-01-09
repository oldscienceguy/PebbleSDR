//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "plugins.h"
#include <QtPlugin>
#include <QPluginLoader>
//Temp for internal devices, will be deleted when everything is a plugin
#include "devices/softrock.h"
#include "devices/elektorsdr.h"
#include "devices/sdr_iq.h"
#include "devices/hpsdr.h"
#include "devices/funcube.h"
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
    p.name = "SR Ensemble";
    p.fileName = "SR_ENSEMBLE";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_ENSEMBLE,settings);
    info.append(p);

    p.name = "SR Ensemble 2M";
    p.fileName = "SR_ENSEMBLE_2M";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_ENSEMBLE_2M,settings);
    info.append(p);

    p.name = "SR Ensemble 4M";
    p.fileName = "SR_ENSEMBLE_4M";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_ENSEMBLE_4M,settings);
    info.append(p);

    p.name = "SR Ensemble 6M";
    p.fileName = "SR_ENSEMBLE_6M";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_ENSEMBLE_6M,settings);
    info.append(p);

    p.name = "SR V9-ABPF";
    p.fileName = "SR_V9";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_V9,settings);
    info.append(p);

    p.name = "SR LITE II";
    p.fileName = "SR_LITE";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_LITE,settings);
    info.append(p);

 #if 0
    p.name = "SR Ensemble LF";
    p.fileName = "SR_ENSEMBLE_LF";
    p.deviceInterface = new SoftRock(receiver, SDR::SR_ENSEMBLE_LF,settings);
    info.append(p);
#endif

    p.name = "FiFi";
    p.fileName = "FiFi";
    p.deviceInterface = new SoftRock(receiver, SDR::FiFi,settings);
    info.append(p);

    p.name = "Elektor SDR";
    p.fileName = "ELEKTOR";
    p.deviceInterface = new ElektorSDR(receiver, SDR::ELEKTOR,settings);
    info.append(p);

#if 0
    p.name = "Elektor SDR + Preamp";
    p.fileName = "ELEKTOR_PA";
    p.deviceInterface = new ElektorSDR(receiver, SDR::ELEKTOR_PA,settings);
    info.append(p);
#endif

    p.name = "RFSpace SDR-IQ";
    p.fileName = "SDR_IQ_USB";
    p.deviceInterface = new SDR_IQ(receiver, SDR::SDR_IQ_USB,settings);
    info.append(p);

    p.name = "RFSpace SDR-IP";
    p.fileName = "SDR_IP_TCP";
    p.deviceInterface = new SDR_IP(receiver, SDR::SDR_IP_TCP,settings);
    info.append(p);

    p.name = "HPSDR USB";
    p.fileName = "HPSDR_USB";
    p.deviceInterface = new HPSDR(receiver, SDR::HPSDR_USB,settings);
    info.append(p);

    //p.name = "HPSDR TCP";
    //p.fileName = "HPSDR_TCP";
    //info.append(p);

    p.name = "FUNcube Pro";
    p.fileName = "FUNCUBE";
    p.deviceInterface = new FunCube(receiver, SDR::FUNCUBE,settings);
    info.append(p);

    p.name = "FUNcube Pro+";
    p.fileName = "FUNCUBE_PLUS";
    p.deviceInterface = new FunCube(receiver, SDR::FUNCUBE_PLUS,settings);
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
                int numDevices = iDeviceInterface->GetNumDevices();
                for (int i = 0; i<numDevices; i++) {
                    pluginInfo.type = PluginInfo::DEVICE_PLUGIN;
                    pluginInfo.name = iDeviceInterface->GetPluginName(i);
                    pluginInfo.description = iDeviceInterface->GetPluginDescription(i);
                    pluginInfo.fileName = fileName;
                    pluginInfo.deviceNumber  = i;
                    pluginInfo.deviceInterface = new SDR(iDeviceInterface, i);
                    pluginInfo.modemInterface = NULL;
                    pluginInfoList.append(pluginInfo);
                }
            }
            //qDebug()<<plugin->Get
        }
    }
}
