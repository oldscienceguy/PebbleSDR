#ifndef BARGRAPHMETER_H
#define BARGRAPHMETER_H

//Will eventually be used for smeter, cw tuning, etc
//See Qt LevelMeter example
#include "global.h"
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
    void start();
    void stop();


    void paintEvent(QPaintEvent *event);
public slots:
    void refreshMeter();

signals:
    void newData();

private:
    Ui::BargraphMeter *ui;
    bool running;
    quint16 minLevel;
    quint16 maxLevel;
    quint16 currentLevel;
    QColor backgroundColor;
    QColor barColor;


};

#endif // BARGRAPHMETER_H
