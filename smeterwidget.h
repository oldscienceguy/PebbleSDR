#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "global.h"
#include <QWidget>
#include <QThread>

#include "ui/ui_smeterwidget.h"
#include "signalstrength.h"
#include "signalspectrum.h"

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

    void SetSignalSpectrum(SignalSpectrum *s);
private:
	Ui::SMeterWidgetClass ui;
	SignalStrength *signalStrength; //Where we get data from
	void paintEvent(QPaintEvent *e);
	bool isRunning;
    int src; //Inst, Average, External

	private slots:
		void updateMeter();
        void srcSelectionChanged(QString);

};

#endif // SMeterWidget_H
