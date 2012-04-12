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

    ui.sourceBox->addItem("Inst");
    ui.sourceBox->addItem("Avg");
    ui.sourceBox->addItem("CW");
    //Get from settings
    QFont medFont("Lucida Grande",9);
    ui.sourceBox->setFont(medFont);

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





