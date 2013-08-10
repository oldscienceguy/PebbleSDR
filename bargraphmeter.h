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
    
    void setMin(qint16 _min);
    void setMax(qint16 _max);
    void setValue(qint16 _value);
    void start();
    void stop();


    void paintEvent(QPaintEvent *event);
public slots:
    void refreshMeter();

signals:
    void newData();

protected:
    Ui::BargraphMeter *ui;
    bool running;
    //scale can be neg and pos
    qint16 minLevel;
    qint16 maxLevel;
    qint16 currentLevel;
    QColor backgroundColor;
    QColor barColor;

    bool showLabelArea;

    //Switching to pixmaps
    QPixmap plotArea;
    QPixmap plotOverlay;
    QPixmap plotLabel;


};

#endif // BARGRAPHMETER_H
