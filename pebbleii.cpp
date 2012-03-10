//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "pebbleii.h"
#include "global.h"

Global *global;

//Qt::FramelessWindowHint is interesting, but requires us to create our own close, resize, etc
PebbleII::PebbleII(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent,flags)
{
	ui.setupUi(this);
	global = new Global();
	receiver = new Receiver(ui.receiverUI, this);
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

