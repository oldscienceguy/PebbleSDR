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
	enum UNITS {PEAK_DB, AVG_DB, S_UNITS, SNR, FLOOR, EXT, NONE};

	SMeterWidget(QWidget *parent = 0);
	~SMeterWidget();
	
    void start();
    void stop();

	//Called by ReceiverWidget when new data is available
	void newSignalStrength(double peakDb, double avgDb, double snrDb, double floorDb, double extValue);

private:
	Ui::SMeterWidgetClass ui;
	void resizeEvent(QResizeEvent * _event);
	UNITS units;

	void updateLabels();

private slots:
		void unitBoxChanged(int item);
};

#endif // SMeterWidget_H
