//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebbleii.h"
#include "global.h"
#include "testbench.h"

Global *global;

//Qt::FramelessWindowHint is interesting, but requires us to create our own close, resize, etc
PebbleII::PebbleII(QWidget *parent, Qt::WindowFlags flags)
: QMainWindow(parent,flags)
{
    global = new Global(); //We need globals in constructors, so must be first thing we do
    //Setup test bench here, not in global constructor or we get circular use with global constants
    global->testBench = new CTestBench();
    global->testBench->Init();

	global->mainWindow = this; //mainWindow is used in Settings()
	global->settings = new Settings(); //We need these early in startup

    ui.setupUi(this);
	receiver = new Receiver(ui.receiverUI, this);
    global->receiver = receiver; //Many classes need access

    QCoreApplication::setApplicationVersion("2.0.0");
    QCoreApplication::setApplicationName("Pebble II");
	QCoreApplication::setOrganizationName("N1DDY");

	//Test
	//QString dir = QCoreApplication::applicationDirPath();
}

PebbleII::~PebbleII()
{
	delete receiver; //Trigger shutdown via destructors
	delete global->testBench;
	delete global->settings;
	delete global;
}
 void PebbleII::closeEvent(QCloseEvent *event)
 {
	 receiver->Close();
 }

