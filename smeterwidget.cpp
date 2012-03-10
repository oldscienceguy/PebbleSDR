//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SMeterWidget.h"
#include <QPainter>
#include <QStylePainter>

SMeterWidget::SMeterWidget(QWidget *parent)
	: QFrame(parent)
{
	ui.setupUi(this);
	isRunning = false;
	signalStrength = NULL;

	//Trying different fixes to prevent background color from hiding our painter
	//this->setBackgroundRole(QPalette::Base); //
	//this->setAttribute(Qt::WA_NoBackground);
	//this->setAttribute(Qt::WA_OpaquePaintEvent);
	//ui.plotArea->setAutoFillBackground(false);


	//Start paint thread
	smt = new SMeterWidgetThread(this);
	connect(smt,SIGNAL(repaint()),this,SLOT(updateMeter()));
	smt->start();
}

SMeterWidget::~SMeterWidget()
{
	smt->quit();
}
void SMeterWidget::setSignalStrength(SignalStrength *ss)
{
	signalStrength = ss;
}
void SMeterWidget::Run(bool r)
{
	isRunning = r;
}
void SMeterWidget::updateMeter()
{
	repaint();
}

void SMeterWidget::paintEvent(QPaintEvent *e)
{
	//Don't need painter.begin and painter.end with this constructor
	QPainter painter(this);

	QRect pa = ui.plotArea->frameGeometry(); //relative to parent, including frame
	int paX = pa.x(); //Upper left corder, relative to widget, ie 0,0
	int paY = pa.y();
	int mid = pa.width()/2;

	QPen pen;
	pen.setStyle(Qt::SolidLine);
	pen.setColor("Blue");
	pen.setWidth(pa.height());
	painter.setPen(pen);
	//Range is -127 to -13db, or a span of 114dB
	//Draw scale just below plot area
	painter.setFont(QFont("Arial", 6));
	int bottom = pa.bottom() + 4;
	painter.drawText(QPoint(paX,bottom),"S0");
	painter.drawText(QPoint(paX+mid-4,bottom), "S9");
	painter.drawText(QPoint(pa.right()-9,bottom),"+60");

	//What percentate is instValue of span
	float instFValue;
	if (signalStrength != NULL)
		instFValue = signalStrength->instFValue();
	else
		instFValue = -127;
	//gcc int/int = int regardless of var type.
	float percent = abs (instFValue + 127) / 114.0;
	if (!isRunning)
		percent = 0;

	int plotX = percent * pa.width();

	if (plotX <= paX)
		return; //nothing to draw

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





