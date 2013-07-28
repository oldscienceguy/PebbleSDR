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
    currentLevel = 50;
    barColor = Qt::red;

    //Set refresh interval
    refreshRate = 5;
    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refreshMeter()));

}

BargraphMeter::~BargraphMeter()
{
    delete ui;
}

void BargraphMeter::start()
{
    refreshTimer->start(refreshRate);
}

void BargraphMeter::stop()
{
    refreshTimer->stop();
}

void BargraphMeter::setRefreshRate(quint16 _rate)
{
    refreshRate = _rate;
}

void BargraphMeter::refreshMeter()
{
    update();

}

void BargraphMeter::setMax(quint16 _max)
{
    maxLevel = _max;
}
void BargraphMeter::setMin(quint16 _min)
{
    minLevel = _min;
}
void BargraphMeter::setValue(quint16 _value)
{
    currentLevel = _value;
}

void BargraphMeter::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect bar = rect();

    //Left to Right orientation
    bar.setRight(currentLevel);

#if 0
    bar.setTop(rect().top() + (1.0 - m_peakHoldLevel) * rect().height());
    bar.setBottom(bar.top() + 5);
    painter.fillRect(bar, m_rmsColor);
    bar.setBottom(rect().bottom());

    bar.setTop(rect().top() + (1.0 - m_decayedPeakLevel) * rect().height());
    painter.fillRect(bar, m_peakColor);

    bar.setTop(rect().top() + (1.0 - m_rmsLevel) * rect().height());
#endif

    painter.fillRect(bar, barColor);

}
