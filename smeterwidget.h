#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include <QWidget>
#include <QThread>

#include "ui/ui_smeterwidget.h"
#include "signalstrength.h"

class SMeterWidgetThread;

//Derive from QFrame instead of QWidget so we get full stylesheet support
class SMeterWidget : public QFrame
{
	Q_OBJECT

public:
	SMeterWidget(QWidget *parent = 0);
	~SMeterWidget();
	
	void setSignalStrength (SignalStrength *ss);

	void Run(bool r);

private:
	Ui::SMeterWidgetClass ui;
	SignalStrength *signalStrength; //Where we get data from
	void paintEvent(QPaintEvent *e);
	bool isRunning;
	SMeterWidgetThread *smt;

	private slots:
		void updateMeter();

};
class SMeterWidgetThread:public QThread
	{
		Q_OBJECT
		public:
			SMeterWidgetThread(SMeterWidget *m) {sm = m; msSleep=100;}
			void SetRefresh(int ms) {msSleep = ms;} //Refresh rate in me
			void run() {
				for(;;) {
				//We can't trigger a paint event cross thread, Qt design
				//But we can trigger a signal which main thread will get and that can trigger repaint
				//sw->repaint();
				emit repaint();
				//Sleep for resolution
				msleep(msSleep);
				}
			}
		signals:
				void repaint();
	private:
		SMeterWidget * sm;
		int msSleep;
	};

#endif // SMeterWidget_H
