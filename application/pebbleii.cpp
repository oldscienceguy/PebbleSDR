//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebbleii.h"
#include "global.h"
#include "testbench.h"
#include <QtPlugin>
#include "digital_modem_interfaces.h"

//Use this if we want to import a static plugin, ie one that is always installed
//Q_IMPORT_PLUGIN(...)



Global *global;

//Qt::FramelessWindowHint is interesting, but requires us to create our own close, resize, etc
PebbleII::PebbleII(QWidget *parent, Qt::WindowFlags flags)
: QMainWindow(parent,flags)
{
    global = new Global(); //We need globals in constructors, so must be first thing we do
    //Setup test bench here, not in global constructor or we get circular use with global constants
    global->testBench = new CTestBench();
    global->testBench->Init();

    //Load plugins next because receiver will need them for UI
    loadPlugins();

    ui.setupUi(this);
	receiver = new Receiver(ui.receiverUI, this);
    global->receiver = receiver; //Many classes need access

	QCoreApplication::setApplicationVersion("0.001");
	QCoreApplication::setApplicationName("Pebble");
	QCoreApplication::setOrganizationName("N1DDY");

	//Test
	//QString dir = QCoreApplication::applicationDirPath();
}

PebbleII::~PebbleII()
{
	delete global;
}
 void PebbleII::closeEvent(QCloseEvent *event)
 {
	 receiver->Close();
	 delete receiver; //Trigger shutdown via destructors
 }

 void PebbleII::loadPlugins()
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
     //QStringList flist = pluginsDir.entryList(QDir::Files);

     //Load dynamic plugins
     foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
         //qDebug()<<fileName;

         QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
         QObject *plugin = loader.instance();
         if (plugin) {
             //populateMenus(plugin);
             //qDebug()<<plugin->objectName();
             //Test interface
             DigitalModemInterface *iDigitalModem = qobject_cast<DigitalModemInterface *>(plugin);
             if (iDigitalModem) {
                 //plugin supports interface
                 qDebug()<<"Calling plugin"<<iDigitalModem->GetPluginName();
             }

             //qDebug()<<plugin->Get
             pluginFileNames += fileName;
         }
     }
 }
