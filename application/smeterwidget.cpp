//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "smeterwidget.h"
#include <QPainter>
#include <QStylePainter>

SMeterWidget::SMeterWidget(QWidget *parent) :QFrame(parent)
{
	ui.setupUi(this);
	signalStrength = NULL;

	units = PEAK_DB; //First item in box until we get from setings
	ui.unitBox->addItem("Peak",PEAK_DB);
	ui.unitBox->addItem("Average",AVG_DB);
	ui.unitBox->addItem("SNR",SNR);
	ui.unitBox->addItem("S-UNITS",S_UNITS);
	ui.unitBox->addItem("Ext",EXT);
	ui.unitBox->addItem("None",NONE);
	//Set current label selection
	ui.unitBox->setCurrentIndex(0); //Todo: get from settings
	connect(ui.unitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(unitBoxChanged(int)));

	ui.barGraph->setColor(Qt::cyan); //!!Make multi color? or set threshold to show red > s9
}

SMeterWidget::~SMeterWidget()
{
}

void SMeterWidget::unitBoxChanged(int item)
{
	units = (UNITS)ui.unitBox->currentData().toUInt();
	updateLabels();
}

void SMeterWidget::updateLabels()
{
	QStringList labels;
	quint32 width = this->width();
	quint32 widthTest = 225; //Should be based on char width
	switch (units) {
		case PEAK_DB:
			//Fall through
		case AVG_DB:
			//Fall through
		case SNR:
			//dB labels, 10db per tick
			ui.barGraph->setMax(DB::maxDb);
			ui.barGraph->setMin(DB::minDb);
			ui.barGraph->setValue(DB::minDb);

			//labels.append("-120"); //Assumed at end of graph
			if (width>widthTest) labels.append("-110");
			labels.append("-100");
			if (width>widthTest) labels.append("-90");
			labels.append("-80");
			if (width>widthTest) labels.append("-70");
			labels.append("-60");
			if (width>widthTest) labels.append("-50");
			labels.append("-40");
			if (width>widthTest) labels.append("-30");
			labels.append("-20");
			if (width>widthTest) labels.append("-10");
			//labels.append("0"); //Assumed at end of graph
			break;
		case S_UNITS:
			//S labels (6db per S-Unit)
			// s-unit range is S1 (-121) to S+34 (-37)
			// so we need to adjust scale accordingly, different than raw db
			ui.barGraph->setMax(-37);
			ui.barGraph->setMin(-121);
			ui.barGraph->setValue(DB::minDb);

			//S1 is left most and not labeled
			//Range is -120 to -10
			//Approx 6db per inc
			//labels.append("S1"); //-121db
			if (width>widthTest)labels.append("S2"); //-115db
			labels.append("S3"); //-109db
			if (width>widthTest) labels.append("S4"); //-103db
			labels.append("S5"); //-97db
			if (width>widthTest) labels.append("S6"); //-91db
			labels.append("S7"); //-85db
			if (width>widthTest)labels.append("S8"); //-79db
			labels.append("S9"); //-73db
			if (width>widthTest) labels.append("+6"); //-67
			labels.append("+12"); //-61
			if (width>widthTest) labels.append("+18"); //-55
			labels.append("+24"); //-49
			if (width>widthTest) labels.append("+30"); //-43
			//labels.append("+34"); // -37
			break;
		case EXT:
		case NONE:
			labels.append("");
			labels.append("");
			labels.append("");
			labels.append("");
			labels.append("");
			labels.append("");
			break;
	}

	ui.barGraph->setLabels(labels);
}


void SMeterWidget::SetSignalSpectrum(SignalSpectrum *s)
{
    //We only need to update smeter when we have new spectrum data to display
    //Use newFftData signal to trigger repaint instead of thread
    //signalSpectrum = s;
    if (s!=NULL) {
		connect(s,SIGNAL(newFftData()),this,SLOT(updateMeter()));
    }
}

void SMeterWidget::start()
{
    ui.barGraph->start();
}

void SMeterWidget::stop()
{
	ui.barGraph->stop();
}

void SMeterWidget::resizeEvent(QResizeEvent *_event)
{
	//Update bargraph labels and ticks to new size
	updateLabels();
	ui.barGraph->resizeEvent(_event);
}

void SMeterWidget::setSignalStrength(SignalStrength *ss)
{
	signalStrength = ss;
}

void SMeterWidget::updateMeter()
{
	float db = DB::minDb;
    if (signalStrength != NULL) {
		switch (units) {
			case PEAK_DB:
				db = signalStrength->peakdB(); //Peak
				break;
			case AVG_DB:
				db = signalStrength->avgdB();
				break;
			case SNR:
				db = signalStrength->snr();
				break;
			case S_UNITS:
				db = signalStrength->avgdB();
				break;
			case NONE:
				break;
			case EXT:
				db = signalStrength->extValue();
				break;
			default:
				db = DB::minDb;
				break;
		}
    }
	else {
		db = DB::minDb;
	}
	ui.barGraph->setValue(db);
	ui.value->setText(QString::number(db,'f',0));
}


