#ifndef BARGRAPHMETER_H
#define BARGRAPHMETER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

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

	void resizePixmaps();
public:
    explicit BargraphMeter(QWidget *parent = 0);
    ~BargraphMeter();
    
    void setMin(qint16 _min);
    void setMax(qint16 _max);
    void setValue(qint16 _value);
    void start();
    void stop();
    void setNumTicks(quint16 _ticks);
    void setColor(QColor _color);
    void setLabels(QStringList _labels);

	void resizeEvent(QResizeEvent *_event);
    void paintEvent(QPaintEvent *event);
    void drawOverlay();
    void drawLabels();
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
    QStringList labels; //Optional labels

    bool showLabelArea;
    bool showLabels;
    bool showTicks;

    //Switching to pixmaps
    QPixmap plotArea;
    QPixmap plotOverlay;
    QPixmap plotLabel;

    quint16 numTicks;


};

#endif // BARGRAPHMETER_H
