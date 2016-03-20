//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "bargraphmeter.h"
#include "ui_bargraphmeter.h"
#include <QPainter>
#include <QTimer>

BargraphMeter::BargraphMeter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BargraphMeter)
{
    ui->setupUi(this);
    minLevel = 0;
    maxLevel = 100;
    currentLevel = 0;
    barColor = Qt::red;
    running = false;
    showLabelArea = false;
    numTicks = 0;
    showLabels = true;
    showTicks = true;

	//Initial display
	resizeEvent(NULL);
    //Thread safe display update
    connect(this, SIGNAL(newData()), this, SLOT(refreshMeter()));


}

BargraphMeter::~BargraphMeter()
{
    delete ui;
}

void BargraphMeter::start()
{
    currentLevel = minLevel;
    running = true;

    refreshMeter();
}

void BargraphMeter::stop()
{
    currentLevel = minLevel;
    refreshMeter();
    running = false;
}

void BargraphMeter::setColor(QColor _color)
{
    barColor = _color;
    update();
}

//Either setTicks or setLabels, not both
void BargraphMeter::setLabels(QStringList _labels)
{
    //Ticks and labels must be same size
    numTicks = _labels.size();
	labels = _labels;
}

//See spectrumwidget.cpp for similar logic
void BargraphMeter::resizePixmaps()
{
	QRect plotFr = ui->plotFrame->rect(); //We just need width and height
	QRect plotLabelFr = ui->labelFrame->rect();
	plotArea = QPixmap(plotFr.width(),plotFr.height());
	plotArea.fill(Qt::black);

	plotOverlay = QPixmap(plotFr.width(),plotFr.height());
	plotOverlay.fill(Qt::black);

	plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
	plotLabel.fill(Qt::black);
}

//Redraw scale on re-size
void BargraphMeter::resizeEvent(QResizeEvent * _event)
{
	Q_UNUSED(_event);
	//Reset pixmap sizes
	resizePixmaps();
	refreshMeter();
}

void BargraphMeter::setNumTicks(quint16 _ticks)
{
    if (numTicks != _ticks) {
        //Only update if change
        //labels must match #ticks
        labels = QStringList();
        numTicks = _ticks;
    }
}

void BargraphMeter::drawOverlay()
{
    //Draw anything that only changes when resized
    //Start clean
    plotOverlay.fill(Qt::black);
}

void BargraphMeter::drawLabels()
{
    if (labels.size() == 0) {
        return;
    }
    plotLabel.fill((Qt::black));
	if (numTicks == 0)
		return;
    QPainter painter(&plotLabel);
    painter.setPen(QPen(Qt::white, 1,Qt::SolidLine));

	//quint16 plotHeight = plotLabel.height();
    quint16 plotWidth = plotLabel.width();

    //Evenly split ticks
    //numTicks are the visible ticks, each with a potential label
	quint16 tickInc = plotWidth / (numTicks + 1);
	//quint16 tickPix = plotHeight * 0.20;

	QFont overlayFont("Arial");
	//overlayFont.setPointSize(8);
	overlayFont.setPixelSize(10);
	//overlayFont.setFixedPitch(true);
	painter.setFont(overlayFont);

    QFontMetrics metrics(overlayFont);

    int x;
    int ctr;
    //don't draw at x==0, first tickInc on
    for (int i=0; i<numTicks; i++) {
        x = (i+1) * tickInc; //3px inc at 2 (0,1,2) 5(3,4,5) etc
        x -= 1; //0 relative
        //Draw tick from top to 25%, leaving middle clear
        //painter.drawLine(i,0,i,tickPix);
        painter.drawLine(x,0,x,2); //Draw up into plot area
        //Draw tick from botton to 25%, leaving middle clear
        //painter.drawLine(i,plotHeight,i,plotHeight - tickPix);
        //Draw label text if specified
        //adjust x to account for width so text is centered. will depend on hpix per char
        if (i < labels.size()) {
            ctr = x - (labels[i].length() * metrics.averageCharWidth() / 2);  //Offset left 1/2 label pix width
			painter.drawText(ctr, metrics.height() + 2,labels[i]);
        }

    }
}

void BargraphMeter::refreshMeter()
{
    if (!running)
        return;

    drawOverlay();
    drawLabels();
    plotArea = plotOverlay.copy(); //copy whole image and we'll draw on top of it


    QPainter painter(&plotArea);

    //Scale from min to max
    //This assumes bargraph has more pixels than range between min and max
    //If rect is 100px and max-min = 10, then scale is 10px for every value
    int dbRange = maxLevel - minLevel;
    int disp = currentLevel - minLevel; //zero base
    disp = disp < 0 ? 0 : disp; //Never let it go below zero
    float scale = (float)disp / (float) (dbRange);

    //Left to Right orientation
    QRect bar = plotArea.rect();

    bar.setRight(plotArea.width() * scale);
    painter.drawRect(bar);
    //pattern brush lets overlay show through, solid will cover it up
    painter.fillRect(bar, QBrush(barColor,Qt::SolidPattern));

    update();

}

void BargraphMeter::setMax(qint16 _max)
{
    maxLevel = _max;
}
void BargraphMeter::setMin(qint16 _min)
{
    minLevel = _min;
}

//Its possible (likely) that setValue will be called from another thread, so we can't update display directly
//Emit new signal instead
void BargraphMeter::setValue(qint16 _value)
{
    currentLevel = _value;
    emit newData();
}

void BargraphMeter::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QRect plotFr = ui->plotFrame->geometry();
    QRect labelFr = ui->labelFrame->geometry();

    painter.drawPixmap(plotFr, plotArea); //Includes plotOverlay which was copied to plotArea
	painter.drawPixmap(labelFr, plotLabel); //Includes plotOverlay which was copied to plotArea

#if 0
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect bar = rect();

    //Scale from min to max
    //This assumes bargraph has more pixels than range between min and max
    //If rect is 100px and max-min = 10, then scale is 10px for every value
    int disp = currentLevel - minLevel; //zero base
    disp = disp < 0 ? 0 : disp; //Never let it go below zero
    int scale = bar.width() / (maxLevel - minLevel);
    //Left to Right orientation
    bar.setRight(disp * scale);
#endif
#if 0
    bar.setTop(rect().top() + (1.0 - m_peakHoldLevel) * rect().height());
    bar.setBottom(bar.top() + 5);
    painter.fillRect(bar, m_rmsColor);
    bar.setBottom(rect().bottom());

    bar.setTop(rect().top() + (1.0 - m_decayedPeakLevel) * rect().height());
    painter.fillRect(bar, m_peakColor);

    bar.setTop(rect().top() + (1.0 - m_rmsLevel) * rect().height());
#endif

    //painter.fillRect(bar, barColor);

}
