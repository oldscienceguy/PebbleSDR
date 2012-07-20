//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "spectrumwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

SpectrumWidget::SpectrumWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	spectrumOffset=0;

	spectrumMode=SignalSpectrum::SPECTRUM;

	plotArea = NULL;
	//message = NULL;
	signalSpectrum = NULL;
	averageSpectrum = NULL;
	lastSpectrum = NULL;

	fMixer = 0;
	//Set focus policy so we get key strokes
	setFocusPolicy(Qt::StrongFocus); //Focus can be set by click or tab

	//Start paint thread
	st = new SpectrumThread(this);
	connect(st,SIGNAL(repaint()),this,SLOT(updateSpectrum()));
	connect(ui.offsetSlider,SIGNAL(valueChanged(int)),this,SLOT(offsetSliderChanged(int)));
	//st->start();
	isRunning = false;
}

SpectrumWidget::~SpectrumWidget()
{
	st->quit();
	if (lastSpectrum != NULL) free(lastSpectrum);
	if (averageSpectrum != NULL) free (averageSpectrum);
}
void SpectrumWidget::Run(bool r)
{
	if (r) {
		st->start();
		isRunning = true;
	}
	else {
		st->stop();
		isRunning =false;
		signalSpectrum = NULL;
		plotArea = NULL; //Start with a  clean plot every time
		update();
	}
}
void SpectrumWidget::SetMessage(QStringList s)
{
	message = s;
}
//We need to set focus when mouse is in spectrum so we get keyboard events
void SpectrumWidget::enterEvent ( QEvent * event )  
{
	setFocus();
}
void SpectrumWidget::leaveEvent(QEvent *event)
{
	clearFocus();
}

void SpectrumWidget::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();
	int m = fMixer;
	switch (key)
	{
	case Qt::Key_Up: //Bigger step
		m += upDownIncrement;
		emit mixerChanged(m);
		break;
	case Qt::Key_Right:
		m += leftRightIncrement;
		emit mixerChanged(m);
		break;
	case Qt::Key_Down:
		m -= upDownIncrement;
		emit mixerChanged(m);
		break;
	case Qt::Key_Left:
		m -= leftRightIncrement;
		emit mixerChanged(m);
		break;
	}
}
void SpectrumWidget::mousePressEvent ( QMouseEvent * event ) 
{
	if (!isRunning || signalSpectrum == NULL)
		return;

	//Is the click in plotFrame?
	int mX = event->x();
	int mY = event->y();
	QRect pf = this->ui.plotFrame->geometry();
	if (pf.contains(mX,mY))
	{
		QSize sz = pf.size();
		int m = mX - pf.x(); //make zero relative
		m = (float)sampleRate * m / sz.width();
		m -= sampleRate/2; //make +/- relative

		emit mixerChanged(m);
	}
}
void SpectrumWidget::SetMode(DEMODMODE m)
{
	demodMode = m;
}
void SpectrumWidget::SetMixer(int m, double f)
{
	fMixer = m;
	loFreq = f;
}
//Track bandpass so we can display with cursor
void SpectrumWidget::SetFilter(int lo, int hi)
{
	loFilter = lo;
	hiFilter = hi;
}

void SpectrumWidget::updateSpectrum()
{
	repaint();
}
void SpectrumWidget::offsetSliderChanged(int v)
{
	spectrumOffset=v;
	repaint();
}
void SpectrumWidget::plotSelectionChanged(SignalSpectrum::DISPLAYMODE mode)
{
	if (signalSpectrum != NULL) {
		signalSpectrum->SetDisplayMode(mode);
	}
	//We may want different refresh rates for different modes
	if (mode==SignalSpectrum::WATERFALL)
		st->SetRefresh(50);//Fast enough so we don't perceive flicker
	else
		st->SetRefresh(100);//

	spectrumMode = mode;
	repaint();
}
void SpectrumWidget::SetSignalSpectrum(SignalSpectrum *s) 
{
	signalSpectrum = s;	
	if (s!=NULL) {
		sampleRate = s->SampleRate();
		upDownIncrement = s->settings->upDownIncrement;
		leftRightIncrement = s->settings->leftRightIncrement;
	}
}
//
void SpectrumWidget::paintCursor(int x1, int y1, QPainter &painter, QColor color)
{
	QPen pen;
	pen.setStyle(Qt::SolidLine);
	pen.setWidth(1);
	pen.setColor(color);
	painter.setPen(pen);
	painter.drawLine(x1,0,x1, y1);

	//Show bandpass filter range
	QRect fr = ui.plotFrame->geometry(); //relative to parent
	int hzPerPixel = sampleRate / fr.width();
	int xLo = x1 + loFilter/hzPerPixel;
	int xHi = x1 + hiFilter/hzPerPixel;
	int y2 = y1+3;
	pen.setWidth(4);
	pen.setColor(Qt::black);
	//pen.setBrush(Qt::Dense5Pattern);
	painter.setPen(pen); //If we don't set pen again, changes have no effect
	painter.drawLine(xLo,y2,xHi,y2);
}

void SpectrumWidget::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);
	QRect fr = ui.plotFrame->geometry(); //relative to parent
	int wHeight = fr.height(); //Plot area height
	int wWidth = fr.width()-2;
	//Never got window/viewport to work right, use translate instead.  Simpler
	//painter.setWindow(10,10,fr.width(),fr.height());
	//painter.setViewport(fr.x(),fr.y(),fr.width(),fr.height());

	//Make all painter coordinates 0,0 relative to plotFrame
	//0,0 will be translated to 4,4 etc
	//Correction to deal with raised frame?  Shouldn't have to do this, but ...
	painter.translate(fr.x(),fr.y()-2);

	if (!isRunning)
	{
		if (true)
		{
            painter.setFont(global->settings->medFont);
			for (int i=0; i<message.count(); i++)
			{
				painter.drawText(20, i*15 + 15 , message[i]);
			}
		}
	return;
	}
	if (spectrumMode == SignalSpectrum::NODISPLAY || signalSpectrum == NULL)
		return;


	painter.setFont(QFont("Arial", 7,QFont::Normal));
	painter.setPen(Qt::red);
	if (signalSpectrum->inBufferOverflowCount > 0)
		painter.drawText(wWidth-20,10,"over");
	if (signalSpectrum->inBufferUnderflowCount > 0)
		painter.drawText(0,10,"under");
	painter.setPen(Qt::black);

	//QPointF p1,p2;
	//p1.setX(0);p1.setY(0);
	//p2.setX(20);p2.setY(20);

	//Draw scale
#if(0)
	//Show simple +/- range
	QString range = QString::number((sampleRate/2000),'f',0);
	painter.drawText(0,wHeight+12, "-"+range+"k");
	painter.drawText(wWidth-20,wHeight+12, "+"+range+"k");
#else
	//Show actual frequency range
	double loRange = (loFreq - sampleRate/2)/1000;
	double midRange = loFreq/1000;
	double hiRange = (loFreq + sampleRate/2)/1000;
	QString loText = QString::number(loRange,'f',0);
	QString midText = QString::number(midRange,'f',0);
	QString hiText = QString::number(hiRange,'f',0);
	painter.setFont(QFont("Arial", 7,QFont::Bold));
	painter.drawText(-2,wHeight+12, loText+"k");
	painter.drawText(wWidth/2 - 15,wHeight+12, midText+"k");
	painter.drawText(wWidth - 15,wHeight+12, hiText+"k");
#endif
	int inc;
	//Tics
	for (int i=0; i<=10; i++){
		inc = wWidth * i * 0.05;
		painter.drawLine(inc,wHeight+3,inc, wHeight+2);
		painter.drawLine(wWidth - inc,wHeight+3,wWidth - inc, wHeight+2);

	}

	//In case we try to paint beyond plotFrame, set this AFTER area outside plotFrame has been painted
	painter.setClipRegion(fr);
	//scale samples to # pixels we have available
	double fStep = (double)signalSpectrum->BinCount() / wWidth;

	//Calculate y scaling
	float dbMax = -13; //+60db
	float dbMin = -127; //S0=-127
	float dbRange = fabs(dbMax - dbMin);
	//As offset increases, so should scale so we see details
	float y_scale = (float)(wHeight) / dbRange;

	int db = 0;
	int plotX = 0;
	int plotY = 0;
	float *spectrum = NULL;
	if (spectrumMode == SignalSpectrum::SPECTRUM || spectrumMode == SignalSpectrum::WATERFALL)
		spectrum = signalSpectrum->Unprocessed();
	else if (spectrumMode == SignalSpectrum::POSTMIXER)
		spectrum = signalSpectrum->PostMixer();
	else if (spectrumMode == SignalSpectrum::POSTBANDPASS)
		spectrum = signalSpectrum->PostBandPass();

	//Experiments in averaging to smooth spectrum and waterfall
	//Too much and we get mush
	bool smoothing = true;
        if (demodMode == dmCWL|| demodMode == dmCWU)
		smoothing = false; //We want crisp cw in waterfall
	int binCount = signalSpectrum->BinCount();
	if (spectrum != NULL && smoothing) {
		if (averageSpectrum == NULL) {
			averageSpectrum = new float[binCount];
			lastSpectrum = new float[binCount];
			memcpy(averageSpectrum, spectrum, binCount * sizeof(float));
		} else {
			for (int i = 0; i < binCount; i++)
				averageSpectrum[i] = (lastSpectrum[i] + spectrum[i]) / 2;
		}
		//Save this spectrum for next iteration
		memcpy(lastSpectrum, spectrum, binCount * sizeof(float));
		spectrum = averageSpectrum;
	}

	//Show mixer cursor, fMixer varies from -f..0..+f, make it zero based and scale
	int cursor = fMixer+(sampleRate/2);
	if (spectrumMode == SignalSpectrum::SPECTRUM || spectrumMode == SignalSpectrum::WATERFALL) 
		cursor = ((float)cursor/sampleRate) * wWidth; //Percentate * width
	else
		cursor = 0.5 * wWidth; //Cursor doesn't move in other modes, keep centered

	if (spectrumMode == SignalSpectrum::SPECTRUM)
	{
		//Paint cursor
		paintCursor(cursor,wHeight, painter, Qt::black);

		painter.setPen(Qt::blue);
		for (int i=0; i<fr.width(); i++)
		{
			plotX = i;
			db=0;

			//Taking average results in spectrum not being displayed correctly due to 'chunks'
			//Just use every Nth sample and throw away the rest
			db = spectrum[(int)(i * fStep)];

			//Make relative to min_dBm
			db -= dbMin;
			//scale to fit our smaller Y axis
			db *= y_scale;
			//apply gain 0-50 -> 1.0 to 2.0
			db *= 1 + (float)(spectrumOffset / 25.0) ; //Rename spectrumGain

			//and subtract noise floor (user adj)
			//db += spectrumOffset;
			//Vertical line from base of widget to height in db
			db = wHeight - db;
			if (db > wHeight) db = wHeight;
			painter.drawLine(plotX,wHeight,plotX,db);
		}
	} else if (spectrumMode == SignalSpectrum::WATERFALL ||
			   spectrumMode == SignalSpectrum::POSTMIXER  ||
			   spectrumMode == SignalSpectrum::POSTBANDPASS){
		//Waterfall
		//plotArea has to be created after layout has done it's work, not in constructor
		//Todo: Find a post layout event and more
		if (plotArea == NULL) {
			//QRect pf = this->ui.plotFrame->frameGeometry();
			plotArea = new QImage(fr.width(),fr.height(),QImage::Format_RGB16);
			plotArea->fill(0);
		} else if (plotArea->width() != fr.width() || plotArea->height() != fr.height()) {
			//Window resized
			delete plotArea;
			plotArea = new QImage(fr.width(),fr.height(),QImage::Format_RGB16);
			plotArea->fill(0);
		}
		//Scroll to create new line
		//for (int i = wHeight-1; i > 0; i--)
		//	memcpy((QRgb*)plotArea->scanLine(i),(QRgb*)plotArea->scanLine(i-1),plotArea->bytesPerLine());
		//Lets try 1 move instead of line by line
		//WARNING: memcpy is not guaranteed to handle overlapped regions and in fact fails with some gcc options
		memmove((QRgb*)plotArea->scanLine(1),(QRgb*)plotArea->scanLine(0),plotArea->bytesPerLine()*(plotArea->height()-1));

		//Color code each bin and draw line
		QColor plotColor;
		for (int i=0; i<plotArea->width(); i++)
		{
			plotX = i;
			plotY = 0;
			db=0;

			db = spectrum[(int)(i * fStep)];

			//Make relative to min_dBm
			db -= dbMin;
			//apply gain 0-50 -> 1.0 to 2.0
			db *= 1 + (float)(spectrumOffset / 25.0) ; //Rename spectrumGain

			//Select color
			if (db >= 35)
				plotColor.setRgb(255,0,0); 
			else if (db >= 30 && db < 35)
				plotColor.setRgb(0,255,0);
			else if (db >= 25 && db < 30)
				plotColor.setRgb(0,0,255);
			else 
				plotColor.setRgb(0,0,0);
			//painter.drawPoint(plotX,plotY);
			//We might be able to use a scanLine here if this is too slow
			plotArea->setPixel(plotX,plotY,plotColor.rgb());
		}
		painter.drawImage(0,0,*plotArea);
		//Draw cursor on top of image
		paintCursor(cursor,wHeight, painter, Qt::white);
	}
	else if (spectrumMode == SignalSpectrum::IQ || spectrumMode == SignalSpectrum::PHASE)
	{
		CPX *rawIQ = signalSpectrum->RawIQ();
		if (rawIQ == NULL)
			return;
		//Experiments in raw I/Q
		QPen IPen,QPen;
		IPen.setColor(Qt::red);
		QPen.setColor(Qt::blue);
		//apply gain 0-50
		int gain = 5000 + (float)(spectrumOffset * 1000) ; //Rename spectrumGain

		//int offset = -25; //hack
		int offset = wHeight * -0.5;

		int zoom = 1;
		//rawIQ[0].re = rawIQ[0].re * gain - offset;
		//rawIQ[0].im = rawIQ[0].im * gain - offset;
		CPX last = rawIQ[0];
		int nxtSample = 1;
		float ISamp,QSamp;
		for (int i=zoom; i<fr.width()-1; i += zoom)
		{
			ISamp = rawIQ[nxtSample].re * gain - offset;
			QSamp = rawIQ[nxtSample].im * gain - offset;
			//painter.drawPoint(rawIQ[i].re*mult,rawIQ[i].im*mult);
			//painter.drawPoint(i,rawIQ[i].re*mult);
			if (spectrumMode==SignalSpectrum::IQ)
			{
				painter.setPen(IPen);
				painter.drawLine(i-zoom,last.re,i,ISamp);
				painter.setPen(QPen);
				painter.drawLine(i-zoom,last.im,i,QSamp);
			} 
			else if (spectrumMode == SignalSpectrum::PHASE)
			{
				//Phase plot
				painter.setPen(IPen);
				//Move plot more to center, ie x+50
				painter.drawPoint(ISamp+50,QSamp);
			}
			last = CPX(ISamp,QSamp);
			nxtSample++;
		}
	} 
	else
	{
		painter.eraseRect(fr);
	}
}

