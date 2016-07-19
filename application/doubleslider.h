#ifndef DOUBLESLIDER_H
#define DOUBLESLIDER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Derived from http://stackoverflow.com/questions/17361885/range-slider-in-qt-two-handles-in-a-qslider
//Modified for general use in Pebble by Richard Landsman

#include <QObject>
#include <QSlider>
#include <QLabel>

class DoubleSliderHandle;

class DoubleSlider: public QSlider
{
	Q_OBJECT
public:
	DoubleSlider(QWidget *parent = 0);

	//Overloaded
	void mousePressEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev); //Not used
	void mouseMoveEvent(QMouseEvent *ev);

	//Equivalent to QSlider methods, just with 2 added
	int value2();
	void setValue2(int value);

	void setTracking2(bool b);
	bool tracking2() {return m_tracking2Enabled;}

	void reset();

	//Used to update the position of the alternate handle through the use of an event filter
	void update2(bool emitSignal);

signals:
	//Same signals as QSlider
	//Use setTracking2(bool) to enable/disable valueChanged signals while slider is moving
	void value2Changed(int);
	void slider2Pressed();
	void slider2Released();
	void slider2Moved();

private:
	DoubleSliderHandle *m_handle2; //2nd slider handle
	bool m_tracking2Enabled;
	int m_value2;

};

class DoubleSliderHandle: public QLabel
{
	Q_OBJECT
public:
	DoubleSliderHandle(DoubleSlider *m_parent = 0);

	//An overloaded mousePressEvent so that we can start grabbing the cursor and using it's position for the value
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

private:
	//Store the parent as a slider so that you don't have to keep casting it
	DoubleSlider *m_parent;

};
#endif // DOUBLESLIDER_H
