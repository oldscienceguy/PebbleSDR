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


//Todo: clicking in slider sets slider1 pos, but is not defined for slider2.  Disable?
//Todo: option to designate slider1 high and slider2 low.  UI to not allow them to cross
//Each slider is independent and can move over the full range of values from min to max

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
	addAction(new QWidgetAction(m_handle2));
	m_handle2->move(this->pos().x() + this->width()- m_handle2->width(), this->pos().y() );

}

DoubleSliderHandle::DoubleSliderHandle(DoubleSlider *_parent)
  : QLabel(_parent)
{
	m_parent = _parent;
	m_filter = new DoubleSliderEventFilter(m_parent);

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
	quint32 h = this->height(); //typically 30px
	QPixmap pix(5,h-12);
	QRect rect = pix.rect();
	QPainter painter(&pix);
	painter.fillRect(rect, QColor(0x708090)); //From pebble.qss darkSteelGrey
	painter.setPen(QColor(0x5c5c5c));
	painter.drawRect(rect);
#endif
	setPixmap(pix);

}

int DoubleSlider::value2()
{
	return m_handle2->value();
}

void DoubleSlider::setValue2(int value)
{
	m_handle2->setValue(value);
}

void DoubleSlider::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
	if (mouseEvent->button() == Qt::LeftButton) {
		m_handle2->show();
		m_handle2->setActivated(false);
	}
	mouseEvent->accept();
}

void DoubleSlider::update2()
{
	//QPoint posCursor(QCursor::pos());
	//QPoint posParent(mapToParent(mapToGlobal(pos())));
	QPoint point(m_handle2->mapToParent(m_handle2->mapFromGlobal(QCursor::pos())).x(),m_handle2->y());
	int horBuffer = (m_handle2->width());
	bool lessThanMax = mapToParent(point).x() < pos().x()+ width() - horBuffer;
	bool greaterThanMin = mapToParent(point).x() > pos().x();
	if(lessThanMax && greaterThanMin)
		m_handle2->move(point);
	emit value2Changed(value2());
}

void DoubleSliderHandle::mousePressEvent(QMouseEvent *mouseEvent)
{
	Q_UNUSED(mouseEvent);
	qGuiApp->installEventFilter(m_filter);
	m_parent->clearFocus();
}

DoubleSliderEventFilter::DoubleSliderEventFilter(DoubleSlider *_grandParent)
{
	m_grandParent = _grandParent;
}

bool DoubleSliderEventFilter::eventFilter(QObject* obj, QEvent* event)
{
	switch(event->type()) {
		case QEvent::MouseButtonRelease:
			qGuiApp->removeEventFilter(this);
			return true;
			break;
		case QEvent::MouseMove:
			m_grandParent->update2();
			return true;
			break;
		default:
			return QObject::eventFilter(obj, event);
	}
  return false;
}

void DoubleSliderHandle::setValue(double value)
{
	double width = m_parent->width();
	//double position = pos().x();
	double range = m_parent->maximum() - m_parent->minimum();
	int location = (value - m_parent->minimum())/range;
	location = location *width;
	move(y(),location);
}

int DoubleSliderHandle::value()
{
	double width = m_parent->width(), position = pos().x();
	double value = position/width;
	double range = m_parent->maximum() - m_parent->minimum();
	return m_parent->minimum() + (value * range);
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
