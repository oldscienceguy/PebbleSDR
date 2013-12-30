#ifndef PEBBLEII_H
#define PEBBLEII_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "ui_pebbleii.h"
#include "receiver.h"
#include "settings.h"

class PebbleII : public QMainWindow
{
	Q_OBJECT

public:
    PebbleII(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~PebbleII();
	public slots:
		//none
private:
	Ui::PebbleIIClass ui;
	Receiver *receiver;
	void closeEvent(QCloseEvent *event);

};

#endif // PEBBLEII_H
