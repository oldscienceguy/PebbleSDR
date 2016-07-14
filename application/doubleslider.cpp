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
  alt_handle = new DoubleSliderHandle(this);
  addAction(new QWidgetAction(alt_handle));
  alt_handle->move(this->pos().x() + this->width()- alt_handle->width(), this->pos().y() );

}

DoubleSliderHandle::DoubleSliderHandle(DoubleSlider *_parent)
  : QLabel(_parent)
{
  parent = _parent;
  filter = new DoubleSliderEventFilter(parent);

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

int DoubleSlider::alt_value()
{
  return alt_handle->value();
}

void DoubleSlider::alt_setValue(int value)
{
  alt_handle->setValue(value);
}

void DoubleSlider::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
  if (mouseEvent->button() == Qt::LeftButton)
  {
	alt_handle->show();
	alt_handle->handleActivated = false;
  }
  mouseEvent->accept();
}

void DoubleSlider::alt_update()
{
  QPoint posCursor(QCursor::pos());
  QPoint posParent(mapToParent(mapToGlobal(pos())));
  QPoint point(alt_handle->mapToParent(alt_handle->mapFromGlobal(QCursor::pos())).x(),alt_handle->y());
  int horBuffer = (alt_handle->width());
  bool lessThanMax = mapToParent(point).x() < pos().x()+ width() - horBuffer;
  bool greaterThanMin = mapToParent(point).x() > pos().x();
  if(lessThanMax && greaterThanMin)
	alt_handle->move(point);
  emit alt_valueChanged(alt_value());
}

void DoubleSliderHandle::mousePressEvent(QMouseEvent *mouseEvent)
{
  qGuiApp->installEventFilter(filter);
  parent->clearFocus();
}

bool DoubleSliderEventFilter::eventFilter(QObject* obj, QEvent* event)
{
  switch(event->type())
  {
  case QEvent::MouseButtonRelease:
	qGuiApp->removeEventFilter(this);
	return true;
	break;
  case QEvent::MouseMove:
	grandParent->alt_update();
	return true;
	break;
  default:
	return QObject::eventFilter(obj, event);
  }
  return false;
}

void DoubleSliderHandle::setValue(double value)
{
  double width = parent->width(), position = pos().x();
  double range = parent->maximum() - parent->minimum();
  int location = (value - parent->minimum())/range;
  location = location *width;
  move(y(),location);
}

int DoubleSliderHandle::value()
{
  double width = parent->width(), position = pos().x();
  double value = position/width;
  double range = parent->maximum() - parent->minimum();
  return parent->minimum() + (value * range);
}
void DoubleSlider::Reset()
{
  int horBuffer = (alt_handle->width());
  QPoint myPos = mapToGlobal(pos());
  QPoint point(myPos.x() + width() - horBuffer, myPos.y()- alt_handle->height());
  point = alt_handle->mapFromParent(point);

  alt_handle->move(point);
  alt_handle->show();
  alt_handle->raise();

}
