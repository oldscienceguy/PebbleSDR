#ifndef BARGRAPHMETER_H
#define BARGRAPHMETER_H

//Will eventually be used for smeter, cw tuning, etc
//See Qt LevelMeter example

#include <QWidget>

namespace Ui {
class BargraphMeter;
}

class BargraphMeter : public QWidget
{
    Q_OBJECT
    
public:
    explicit BargraphMeter(QWidget *parent = 0);
    ~BargraphMeter();
    
    void setMin(quint16 _min);
    void setMax(quint16 _max);
    void setValue(quint16 _value);
    void setRefreshRate(quint16 _rate);
    void start();
    void stop();


    void paintEvent(QPaintEvent *event);
public slots:
    void refreshMeter();

private:
    Ui::BargraphMeter *ui;

    quint16 minLevel;
    quint16 maxLevel;
    quint16 currentLevel;
    quint16 refreshRate;
    QColor backgroundColor;
    QColor barColor;
    QTimer *refreshTimer;


};

#endif // BARGRAPHMETER_H
