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

    //Get our widget boundaries
    QRect meterFr = this->geometry(); //relative to parent

    plotArea = QPixmap(meterFr.width(),meterFr.height());
    plotArea.fill(Qt::black);

    plotOverlay = QPixmap(meterFr.width(),meterFr.height());
    plotOverlay.fill(Qt::black);

    plotLabel = QPixmap(meterFr.width(),meterFr.height());
    plotLabel.fill(Qt::black);

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
    update();
}

void BargraphMeter::stop()
{
    currentLevel = minLevel;
    running = false;
    update();
}

void BargraphMeter::refreshMeter()
{
    if (!running)
        return;

    QPainter painter(&plotArea);
    plotArea.fill(Qt::black); //Erase previous graph

    //Scale from min to max
    //This assumes bargraph has more pixels than range between min and max
    //If rect is 100px and max-min = 10, then scale is 10px for every value
    int disp = currentLevel - minLevel; //zero base
    disp = disp < 0 ? 0 : disp; //Never let it go below zero
    float scale = (float)plotArea.width() / (float) (maxLevel - minLevel);

    //Left to Right orientation
    QRect bar = plotArea.rect();

    bar.setRight(disp * scale);
    painter.drawRect(bar);
    painter.fillRect(bar, barColor);


    if (running)
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
    QRect plotFr = this->geometry(); //relative to parent

    painter.drawPixmap(plotFr, plotArea); //Includes plotOverlay which was copied to plotArea

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
