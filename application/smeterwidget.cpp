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


void SMeterWidget::SetSignalSpectrum(SignalSpectrum *s)
{
    //We only need to update smeter when we have new spectrum data to display
    //Use newFftData signal to trigger repaint instead of thread
    //signalSpectrum = s;
    if (s!=NULL) {
        connect(s,SIGNAL(newFftData()),this,SLOT(updateMeter()));
        //We're going to adj max so we get even dist across scale
        //If we assume 18db per tic (3 s-units), then range should be evenly divisible by 18
		int dbRange = global->db.maxDb - global->db.minDb;
        dbRange = (dbRange / 18) * 18; //Now even multiple
		ui.barGraph->setMax(global->db.minDb + dbRange);
        //Scale starts at S1, not S0 so add 6db to min
		ui.barGraph->setMin(global->db.minDb - 6);
		ui.barGraph->setValue(global->db.minDb);
        ui.barGraph->setColor(Qt::cyan); //!!Make multi color? or set threshold to show red > s9
        QStringList labels;
        //S1 is left most and not labeled
        //+60 is right most and not labeled
        //Range is -120 to -10
        //So middle must equate to -55 to show correct values unless we adjust
        //6db per inc
        //labels.append("S1"); //-121db
        //labels.append("S2"); //-115db
        labels.append("S3"); //-109db
        //labels.append("S4"); //-103db
        //labels.append("S5"); //-97db
        labels.append("S6"); //-91db
        //labels.append("S7"); //-85db
        //labels.append("S8"); //-79db
        labels.append("S9"); //-73db
        labels.append("+20"); // -55 (-53 to be exact)
        labels.append("+40"); // -37 (-33 to be exact)
        //labels.append("+60"); // -19db (-13 to be exact

        ui.barGraph->setLabels(labels);
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
        instFValue = -127;
    ui.barGraph->setValue(instFValue);
}

#if 0
void SMeterWidget::paintEvent(QPaintEvent *e)
{
	//Don't need painter.begin and painter.end with this constructor
    QPainter painter(this);

    QRect pa = ui.plotArea->frameGeometry(); //relative to parent, including frame
    QRect sa = ui.scaleArea->frameGeometry(); //relative to parent, including frame
    int paX = pa.x(); //Upper left corder, relative to widget, ie 0,0
    int paY = pa.y()+8; //Adj
	int mid = pa.width()/2;

	QPen pen;
	pen.setStyle(Qt::SolidLine);
	//Range is -127 to -13db, or a span of 114dB
	//Draw scale just below plot area
    painter.setFont(QFont("Ludida Grande", 8)); //!!Should get this from settings
    pen.setColor(Qt::black);;
    painter.setPen(pen);
    painter.drawText(sa.bottomLeft(),"S0");
    painter.drawText(sa.right()/2,sa.bottom(), "S9");
    painter.drawText(sa.right()-15,sa.bottom(),"+60");

    //What percentate is instValue of span
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
		instFValue = -127;
	//gcc int/int = int regardless of var type.
	float percent = abs (instFValue + 127) / 114.0;
    if (!running)
		percent = 0;

	int plotX = percent * pa.width();

	if (plotX <= paX)
		return; //nothing to draw

    pen.setColor(Qt::blue);
    pen.setWidth(pa.height());
    painter.setPen(pen);

	if (percent < .5 )
	{
		//Just draw blue
		//Scale to width of meter
        painter.drawLine(paX,paY,paX + plotX, paY);
	} else {
		//draw blue
		int tmp = paX+mid;
		painter.drawLine(paX,paY,tmp, paY);
		pen.setColor("Red");
		painter.setPen(pen);
		//then anything over 50% (S9) in red
		painter.drawLine(tmp+1,paY,paX + plotX, paY);
	}

} 

#endif


