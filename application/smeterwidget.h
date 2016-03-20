#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "global.h"
#include <QWidget>

#include "ui_smeterwidget.h"
#include "signalstrength.h"
#include "signalspectrum.h"
#include "bargraphmeter.h"

//Derive from QFrame instead of QWidget so we get full stylesheet support
class SMeterWidget : public QFrame
{
	Q_OBJECT

public:
	enum UNITS {DB, S_UNITS, SNR, NONE};

	SMeterWidget(QWidget *parent = 0);
	~SMeterWidget();
	
	void setSignalStrength (SignalStrength *ss);

    void SetSignalSpectrum(SignalSpectrum *s);
    void start();
    void stop();

private:
	Ui::SMeterWidgetClass ui;
	SignalStrength *signalStrength; //Where we get data from
	void resizeEvent(QResizeEvent * _event);
    int src; //Inst, Average, External
	UNITS units;

	void updateLabels();
private slots:
		void updateMeter();
        void instButtonClicked();
        void avgButtonClicked();
		void unitBoxChanged(int item);
};

#endif // SMeterWidget_H
