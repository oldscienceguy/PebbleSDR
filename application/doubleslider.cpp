#include "doubleslider.h"


//Qt
#include <QMouseEvent>
#include "qmimedata.h"
#include "qdrag.h"
#include "qwidgetaction.h"
#include "qapplication.h"
#include "qpixmap.h"
#include "qcursor.h"
#include "qguiapplication.h"
#include "qdir.h"
#include <QProxyStyle>
#include <QPainter>
#include <QStyleOptionSlider>
#include <QDebug>


//Todo: clicking in slider sets slider1 pos, but is not defined for slider2.  Disable?
//Todo: option to designate slider1 high and slider2 low.  UI to not allow them to cross
//Each slider is independent and can move over the full range of values from min to max
//Should handles be allowed to overlap?  Original handle can point down, our handle can point up, so they never overlap

//See qstyle.h for style options
class SliderProxy : public QProxyStyle
{
public:
int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const
{
	switch(metric) {
	//case PM_SliderThickness  : return 25;
	//case PM_SliderLength     : return 25;
	default                  : return (QProxyStyle::pixelMetric(metric,option,widget));
	}
  }
};

DoubleSlider::DoubleSlider(QWidget *parent)
  : QSlider(parent)
{
	//styling
	setOrientation(Qt::Horizontal);
	setAcceptDrops(true);
	SliderProxy *aSliderProxy = new SliderProxy();

#if 0
	//If we want to use image as slider
	QString path = QDir::fromNativeSeparators(ImagesPath("handle.png"));
	setStyleSheet("QSlider::handle { image: url(" + path + "); }");
#endif
	setStyle(aSliderProxy); //All style requests go to our proxy so we pick up QSlider styles

	//setting up the alternate handle
	m_handle2 = new DoubleSliderHandle(this);
	//What does adding an action give us in terms of capabilities?
	addAction(new QWidgetAction(m_handle2));
	//Set initial positions of lower and upper sliders, assume handle2 is the upper
	int x = this->pos().x() + this->width()- m_handle2->width();
	int y = this->pos().y();
	m_handle2->move(x,y);
	//Todo: just set default values in constructor and then position sliders after layout is complete (where?)
	//Layout is not complete at this point and width is wrong
	//m_handle2->setValue(maximum()); //Doesn't work here

	m_tracking2Enabled = true; //Emit value changed while slider is moving
	m_value2 = 0;
}

void DoubleSlider::mousePressEvent(QMouseEvent *ev)
{
	QStyle::SubControl pressedControl;
	//Derived from QSlider source code
	QStyleOptionSlider opt;
	initStyleOption(&opt);
	//Returns the sub control at the given position, matching style
	pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, ev->pos(), this);

	if (pressedControl == QStyle::SC_SliderGroove) {
		//Don't allow clicks in SliderGroove, ambiguous as to high/low
		ev->accept(); //Eat it
		return;
	}
	//Let base class handle everything else
	QSlider::mousePressEvent(ev);
}

void DoubleSlider::mouseReleaseEvent(QMouseEvent *ev)
{
	QStyle::SubControl pressedControl;
	//Derived from QSlider source code
	QStyleOptionSlider opt;
	initStyleOption(&opt);
	//Returns the sub control at the given position, matching style
	pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, ev->pos(), this);

	if (pressedControl == QStyle::SC_SliderGroove) {
		//Don't allow clicks in SliderGroove, ambiguous as to high/low
		ev->accept(); //Eat it
		return;
	}
	//Let base class handle everything else
	QSlider::mouseReleaseEvent(ev);
}

void DoubleSlider::mouseMoveEvent(QMouseEvent *ev)
{
#if 0
	QStyle::SubControl pressedControl;
	//Derived from QSlider source code
	QStyleOptionSlider opt;
	initStyleOption(&opt);
	//Returns the sub control at the given position, matching style
	pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, ev->pos(), this);

	if (pressedControl == QStyle::SC_SliderGroove) {
		//Don't allow clicks and moves in SliderGroove, ambiguous as to high/low
		ev->accept(); //Eat it
		return;
	}
#endif
	//Let base class handle everything else
	QSlider::mouseMoveEvent(ev);

}

int DoubleSlider::value2()
{
	return m_value2; //value is continually updated during mouse moves
}

void DoubleSlider::setValue2(int value)
{
	if (value < minimum() || value > maximum())
		return;
	if (value <= this->value())
		//High value can't be same or lower than low value
		return;
	m_value2 = value;
	//Position handle
	int handleWidth = (m_handle2->width());
	double effectiveSlotWidth = width() - handleWidth;
	double range = maximum() - minimum();
	QPoint newHandleLoc = m_handle2->pos(); //Keep y same, just change x
	if (m_value2 == minimum()) {
		newHandleLoc.setX(0);
	} else if (m_value2 == maximum()) {
		newHandleLoc.setX(width() - handleWidth);
	} else {
		value = value - minimum();
		double percentage = value / range;
		newHandleLoc.setX(effectiveSlotWidth * percentage);
	}
	m_handle2->move(newHandleLoc);
}

void DoubleSlider::setTracking2(bool b)
{
	m_tracking2Enabled = b;
}

//Updates handle2, should just set flag and move to overloaded paint()

// The center of each handle is the indicator, ie handleWidth / 2
// Center of handle is positioned on cursor after move
// |   |-|           Handle width                    |-|
// |   |-------------Groove width----------------------|
// |-----------------Control width--------------------------|

void DoubleSlider::update2(bool emitSignal)
{
	QPoint cursor(QCursor::pos()); //Global coordinates
	QPoint widgetCursor = mapFromGlobal(cursor); //Widget coordinates
	int handleWidth = (m_handle2->width());
	double effectiveSlotWidth = width() - handleWidth;
	double range = maximum() - minimum();
	QPoint newHandleLoc(widgetCursor.x(), m_handle2->y());
	//Not taking into account slot borders yet
	int xSlotLow = 0; //x coord of beginning of slot
	int xSlotHigh = xSlotLow + width() - handleWidth; //x coord of end of slot
	double newValue;
	if (newHandleLoc.x() < xSlotLow) {
		//Off left end of scale
		newHandleLoc.setX(xSlotLow);
		newValue = minimum();
	} else if (newHandleLoc.x() > xSlotHigh) {
		//Off the right end of scale
		newHandleLoc.setX(xSlotHigh);
		newValue = maximum();
	} else {
		//Calc new value2
		//Handle doesn't move over entire width of slot due to handle width
		//Left side of handle can go to zero at low end
		//but only width - handleWidth at the high end
		double percentage = newHandleLoc.x() / effectiveSlotWidth;
		newValue = minimum() + (range * percentage);
	}
	//handles can touch but not overlap
	//limit high slider to low value + whatever value handlewidth represents
	int minValue = this->value() + ((handleWidth / effectiveSlotWidth) * range);
	if (newValue > minValue) {
		//Don't let high value be lower than low value
		m_value2 = newValue;
		m_handle2->move(newHandleLoc);
		if (m_tracking2Enabled || emitSignal) {
			emit value2Changed(m_value2);
		}
	}
}

void DoubleSlider::reset()
{
	int horBuffer = (m_handle2->width());
	QPoint myPos = mapToGlobal(pos());
	QPoint point(myPos.x() + width() - horBuffer, myPos.y()- m_handle2->height());
	point = m_handle2->mapFromParent(point);

	m_handle2->move(point);
	m_handle2->show();
	m_handle2->raise();
}

DoubleSliderHandle::DoubleSliderHandle(DoubleSlider *_parent)
  : QLabel(_parent)
{
	m_parent = _parent;

	//styling
	setAcceptDrops(true);
#if 0
	//hard coded path to image :/ sorry
	QPixmap pix = QPixmap(ImagesPath("handle.png"));
	pix =  pix.scaled(25, 25, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#else
	//We won't pick up pebble.qss styles here, so hard code equivalent for now
	//Width is 3px + 1 px border
	//Height is control height minus margin
	quint32 width = 5;
	setMinimumWidth(width);
	setMaximumWidth(width);
	quint32 h = this->height(); //typically 30px
	QPixmap pix(width, h-12); //Fudge
	QRect rect = pix.rect();
	QPainter painter(&pix);
	painter.fillRect(rect, QColor(0x708090)); //From pebble.qss darkSteelGrey
	painter.setPen(QColor(0x5c5c5c));
	painter.drawRect(rect);
#endif
	setPixmap(pix); //We subclass Label to make it easy to use Pixmaps

}

void DoubleSliderHandle::mousePressEvent(QMouseEvent *event)
{
	m_parent->mousePressEvent(event);
	event->accept();
}

void DoubleSliderHandle::mouseReleaseEvent(QMouseEvent *event)
{
	m_parent->update2(true); //always emits changed signal
	event->accept();
}

void DoubleSliderHandle::mouseMoveEvent(QMouseEvent *event)
{
	m_parent->update2(false); //only emits changed signal if tracking is enabled
	event->accept();
}

