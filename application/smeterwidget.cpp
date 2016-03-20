//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "smeterwidget.h"
#include <QPainter>
#include <QStylePainter>

SMeterWidget::SMeterWidget(QWidget *parent) :QFrame(parent)
{
	ui.setupUi(this);
	signalStrength = NULL;

    instButtonClicked();
    connect(ui.instButton,SIGNAL(clicked()),this,SLOT(instButtonClicked()));
    connect(ui.avgButton,SIGNAL(clicked()),this,SLOT(avgButtonClicked()));

	units = DB;
	ui.unitBox->addItem("dB",DB);
	ui.unitBox->addItem("S",S_UNITS);
	ui.unitBox->addItem("SNR",SNR);
	ui.unitBox->addItem("None",NONE);
	//Set current label selection
	ui.unitBox->setCurrentIndex(0); //Todo: get from settings
	connect(ui.unitBox,SIGNAL(currentIndexChanged(int)),this,SLOT(unitBoxChanged(int)));

	ui.barGraph->setColor(Qt::cyan); //!!Make multi color? or set threshold to show red > s9
}

SMeterWidget::~SMeterWidget()
{
}
void SMeterWidget::instButtonClicked()
{
    src = 0;
    ui.instButton->setFlat(false);
    ui.avgButton->setFlat(true);

}
void SMeterWidget::avgButtonClicked()
{
    src = 1;
    ui.instButton->setFlat(true);
	ui.avgButton->setFlat(false);
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
	if (units == DB) {
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
	} else if (units == S_UNITS) {
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

	} else if (units == SNR) {
		//SNR labels
		labels.append("");
		labels.append("");
		labels.append("");
		labels.append("");
		labels.append("");
		labels.append("");
	} else {
		//No labels
		labels.append("");
		labels.append("");
		labels.append("");
		labels.append("");
		labels.append("");
		labels.append("");
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
    float instFValue;
    if (signalStrength != NULL) {
        if (src == 0)
            instFValue = signalStrength->instFValue();
        else if (src == 1)
            instFValue = signalStrength->avgFValue();
        else
            instFValue = signalStrength->extFValue();
    }
    else
		instFValue = DB::minDb;
    ui.barGraph->setValue(instFValue);
	ui.value->setText(QString::number(instFValue,'f',0));
}


