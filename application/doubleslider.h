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
	void mouseReleaseEvent(QMouseEvent *event);

	//Equivalent to QSlider methods, just with 2 added
	int value2();

	void setValue2(int value);

	void reset();

	//Used to update the position of the alternate handle through the use of an event filter
	void update2();

signals:
	void value2Changed(int);

private:
	DoubleSliderHandle *m_handle2; //2nd slider handle

};

class DoubleSliderEventFilter : public QObject
{
public:
	DoubleSliderEventFilter(DoubleSlider *_grandParent);

protected:
	//Overloaded function from QSlider
	bool eventFilter(QObject* obj, QEvent* event);

private:
	DoubleSlider *m_grandParent;
};

class DoubleSliderHandle: public QLabel
{
	Q_OBJECT
public:
	DoubleSliderHandle(DoubleSlider *m_parent = 0);

	//An overloaded mousePressevent so that we can start grabbing the cursor and using it's position for the value
	void mousePressEvent(QMouseEvent *event);

	//Returns the value of this handle with respect to the slider
	int value();

	// Maps mouse coordinates to slider values
	int mapValue();

	bool activated() {return m_handleActivated;}
	void setActivated(bool b) {m_handleActivated = b;}

public slots:
   //Sets the value of the handle with respect to the slider
   void setValue(double value);

private:
	//Store the filter for installation on the qguiapp
	DoubleSliderEventFilter *m_filter;

	//Store the parent as a slider so that you don't have to keep casting it
	DoubleSlider *m_parent;

	//Store a bool to determine if the alternate handle has been activated
	bool m_handleActivated;

};
#endif // DOUBLESLIDER_H
