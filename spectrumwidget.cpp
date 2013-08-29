//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "spectrumwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

class Receiver;

SpectrumWidget::SpectrumWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);


    ui.displayBox->addItem("Spectrum");
    ui.displayBox->addItem("Waterfall");
    ui.displayBox->addItem("I/Q");
    ui.displayBox->addItem("Phase");
    ui.displayBox->addItem("Off");
    ui.displayBox->setCurrentIndex(-1);

    connect(ui.displayBox,SIGNAL(currentIndexChanged(int)),this,SLOT(displayChanged(int)));

    //Starting plot range
    plotMaxDb = -50; //!! Same as CuteSDR Save in settings
    plotMinDb = global->minDb;
    ui.maxDbBox->setMinimum(plotMinDb);
    ui.maxDbBox->setMaximum(-10);
    ui.maxDbBox->setValue(plotMaxDb);
    connect(ui.maxDbBox,SIGNAL(valueChanged(int)),this,SLOT(maxDbChanged(int)));

	spectrumMode=SignalSpectrum::SPECTRUM;

	//message = NULL;
	signalSpectrum = NULL;
	averageSpectrum = NULL;
	lastSpectrum = NULL;

	fMixer = 0;

    dbRange = abs(global->maxDb - global->minDb);

    //Spectrum power is converted to 8bit int for display
    //This array maps the colors for each power level
    //Technique from CuteSDR to get a smooth color palette across the spectrum
#if 1
    spectrumColors = new QColor[255];

    for( int i=0; i<256; i++)
    {
        if( (i<43) )
            spectrumColors[i].setRgb( 0,0, 255*(i)/43);
        if( (i>=43) && (i<87) )
            spectrumColors[i].setRgb( 0, 255*(i-43)/43, 255 );
        if( (i>=87) && (i<120) )
            spectrumColors[i].setRgb( 0,255, 255-(255*(i-87)/32));
        if( (i>=120) && (i<154) )
            spectrumColors[i].setRgb( (255*(i-120)/33), 255, 0);
        if( (i>=154) && (i<217) )
            spectrumColors[i].setRgb( 255, 255 - (255*(i-154)/62), 0);
        if( (i>=217)  )
            spectrumColors[i].setRgb( 255, 0, 128*(i-217)/38);
    }
#endif

    //Note:  Make sure that top range is less than dbRange
    //Ranges do not have to be the same size
    int range1 = 30;
    int range2 = 40;
    int range3 = 45;
    int range4 = 50;
    int range5 = 55;
    int range6 = 60;
    int range7 = 65;
    int range8 = 70;
    int range9 = 75;
    int range10 = 80; //Everything > than this will be max db color

    //Standard named colors
    //black, blue, green, cyan, yellow, magenta, red

#if 0
    //Create color map from zero relative 0db to dbRange
    spectrumColors = new QColor[dbRange];
    for (int i=0; i<dbRange; i++) {
        if (i < range1)
            spectrumColors[i].setRgb(0,0,0); //Black
        else if (i >= range1 && i < range2)
            spectrumColors[i].setRgb(0,0,100);
        else if (i >= range2 && i < range3)
            spectrumColors[i].setRgb(0,0,255); //Blue
        else if (i >= range3 && i < range4)
            spectrumColors[i].setRgb(0,100,100); //Looking for blue/green
        else if (i >= range4 && i < range5)
            spectrumColors[i].setRgb(0,100,0); //Green
        else if (i >= range5 && i < range6)
            spectrumColors[i].setRgb(0,255,255); //Cyan
        else if (i >= range6 && i < range7)
            spectrumColors[i].setRgb(255,255,0); //Yellow
        else if (i >= range7 && i < range8)
            spectrumColors[i].setRgb(255,255,0); //Looking for yellow/orange
        else if (i >= range8 && i < range9)
            spectrumColors[i].setRgb(255,170,0); //Orange
        else if (i >= range9 && i < range10)
            spectrumColors[i].setRgb(255,0,0);
        else if (i >= range10)
            spectrumColors[i].setRgb(255,0,0); // Red

    }
#endif
    //Turns on mouse events without requiring button press
    //This does not work if plotFrame is set in code or if both plotFrame and it's parent widgetFrame are set in code
    //But if I set in the designer poperty editor (all in hierarchy have to be set), we get events
    //Maybe a Qt bug related to where they get set
    //ui.widgetFrame->setMouseTracking(true);
    //ui.plotFrame->setMouseTracking(true);

    //Set focus policy so we get key strokes
    setFocusPolicy(Qt::StrongFocus); //Focus can be set by click or tab

    //Used as a logrithmic scale 2^0 to 2^4 or 1x to 16x
    ui.zoomSlider->setRange(0,400);
    ui.zoomSlider->setSingleStep(1);
    ui.zoomSlider->setValue(0); //Left end of scale
    connect(ui.zoomSlider,SIGNAL(valueChanged(int)),this,SLOT(zoomChanged(int)));
    zoom = 1;

	isRunning = false;
}

SpectrumWidget::~SpectrumWidget()
{
	if (lastSpectrum != NULL) free(lastSpectrum);
	if (averageSpectrum != NULL) free (averageSpectrum);
}
void SpectrumWidget::Run(bool r)
{
    //Global is not initialized in constructor
    ui.displayBox->setFont(global->settings->medFont);
    ui.maxDbBox->setFont(global->settings->medFont);
    ui.cursorLabel->setFont(global->settings->medFont);

    QRect plotFr = ui.plotFrame->geometry(); //relative to parent

    //plotArea has to be created after layout has done it's work, not in constructor
    plotArea = QPixmap(plotFr.width(),plotFr.height());
    plotArea.fill(Qt::black);
    plotOverlay = QPixmap(plotFr.width(),plotFr.height());
    plotOverlay.fill(Qt::black);
    QRect plotLabelFr = ui.labelFrame->geometry();
    plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
    plotLabel.fill(Qt::black);

	if (r) {
        ui.displayBox->setCurrentIndex(global->sdr->lastDisplayMode); //Initial display mode
        zoomChanged(0); //Display initial value
		isRunning = true;
	}
	else {
        ui.displayBox->setCurrentIndex(-1);
		isRunning =false;
		signalSpectrum = NULL;
        plotArea.fill(Qt::black); //Start with a  clean plot every time
        plotOverlay.fill(Qt::black); //Start with a  clean plot every time
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
    //This forces focus if another ui element has it, ie frequency direct entry
    //Seems to affect entire widget however, not just plot frame
    ui.plotFrame->setFocus();
    event->accept();
}
void SpectrumWidget::leaveEvent(QEvent *event)
{
    ui.plotFrame->clearFocus();
    event->accept();
}

//Todo: Move resize logic out of paint and capture here
void SpectrumWidget::resizeEvent(QResizeEvent *event)
{
    QRect plotFr = ui.plotFrame->geometry(); //relative to parent

    //Window resized
    plotArea = QPixmap(plotFr.width(),plotFr.height());
    plotArea.fill(Qt::black);
    plotOverlay = QPixmap(plotFr.width(),plotFr.height());
    plotOverlay.fill(Qt::black);

    QRect plotLabelFr = ui.labelFrame->geometry();
    plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
    plotLabel.fill(Qt::black);

    DrawOverlay(); //plotArea size changed

    event->accept(); //We don't handle
}

//Returns +/- freq from LO based on where mouse cursor is
//Called from paint
double SpectrumWidget::GetMouseFreq()
{
    QPoint mp = QCursor::pos(); //Current mouse position in global coordinates
    mp = mapFromGlobal(mp); //Convert to widget relative
    QRect pf = this->ui.plotFrame->geometry();
    QRect lf = this->ui.labelFrame->geometry();
    if (!pf.contains(mp) && !lf.contains(mp))
        return 0.0;
    return GetXYFreq(mp.x(),mp.y());
}
double SpectrumWidget::GetXYFreq(int x, int y)
{
    QRect pf = this->ui.plotFrame->geometry();

    //Find freq at cursor
    int hzPerPixel = sampleRate / pf.width() * zoom;
    //Convert to +/- relative to center
    int m = x - pf.center().x();
    //And conver to freq
    m *= hzPerPixel;
    return m;
}

int SpectrumWidget::GetMouseDb()
{
    QPoint mp = QCursor::pos(); //Current mouse position in global coordinates
    mp = mapFromGlobal(mp); //Convert to widget relative
    QRect pf = this->ui.plotFrame->geometry();
    if (!pf.contains(mp))
        return plotMinDb;

    //Find db at cursor
    int db = (pf.height() - mp.y()); //Num pixels
    double scale = (double)(plotMaxDb - plotMinDb)/pf.height();
    db *= scale; //Scale to get db
    //Convert back to minDb relative
    db += plotMinDb;
    return db;
}

//Track cursor and display freq in cursorLabel
//Here for reference.  Abandoned and replaced with direct access to current cursor position in paint
void SpectrumWidget::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

//Todo: Implement scrolling frequency changes
void SpectrumWidget::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event);  //Suppress warnings, use everywhere!
    //Only in plotFrame
    QRect pf = this->ui.plotFrame->geometry();
    if (!pf.contains(event->pos()))
            return;

    QPoint angleDelta = event->angleDelta();
    int freq = fMixer;

    if (angleDelta.ry() == 0) {
        if (angleDelta.rx() > 0) {
            //Scroll Right
            freq+= 100;
            emit mixerChanged(freq);
        } else if (angleDelta.rx() < 0) {
            //Scroll Left
            freq-= 100;
            emit mixerChanged(freq);

        }
    } else if (angleDelta.rx() == 0) {
        if (angleDelta.ry() > 0) {
            //Scroll Down
            freq-= 1000;
            emit mixerChanged(freq);

        } else if (angleDelta.ry() < 0) {
            //Scroll Up
            freq+= 1000;
            emit mixerChanged(freq);
        }
    }
    // neg==left pos==right
    //qDebug("Angle delta rx %d",event->angleDelta().rx());  //Left to right
    // neg==up pos==down
    //qDebug("Angle delta ry %d",event->angleDelta().ry());  //Up Down

    //Buttons held during scroll
    //if (event->buttons()==Qt::RightButton)
    //event->angleDelta on all platforms
    //event->pos widget relative mouse coordinates
    //event->ignore(); //Let someone else handle it
    event->accept(); //We handled it
}

void SpectrumWidget::hoverEnter(QHoverEvent *event)
{
    event->ignore(); //Let someone else handle it

}
void SpectrumWidget::hoverLeave(QHoverEvent *event)
{
    event->ignore();
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
    case Qt::Key_Enter:
        emit mixerChanged(m, true); //Change LO to current mixer freq
        break;
	}
    event->accept();
}
void SpectrumWidget::mousePressEvent ( QMouseEvent * event ) 
{
	if (!isRunning || signalSpectrum == NULL)
		return;
    QRect pf = this->ui.plotFrame->geometry();
    QRect lf = this->ui.labelFrame->geometry();

    if (!pf.contains(event->pos()) && !lf.contains(event->pos()))
            return;

    Qt::MouseButton button = event->button();
    // Mac: Qt::ControlModifier == Command Key
    // Mac: Qt::AltModifer == Option(Alt) Key
    // Mac: Qt::MetaModifier == Control Key

    Qt::KeyboardModifiers modifiers = event->modifiers(); //Keyboard modifiers
    double deltaFreq = GetMouseFreq();

    if (button == Qt::LeftButton) {
        if( modifiers == Qt::NoModifier)
            emit mixerChanged(deltaFreq, false); //Mixer mode
        else if (modifiers == Qt::AltModifier)
            emit mixerChanged(deltaFreq, true); //Mac Option same as Right Click

    } else if (button == Qt::RightButton) {
        if (modifiers == Qt::NoModifier)
            emit mixerChanged(deltaFreq, true); //LO mode
    }
    event->accept();
}
void SpectrumWidget::SetMode(DEMODMODE m)
{
	demodMode = m;
}
void SpectrumWidget::SetMixer(int m, double f)
{
	fMixer = m;
	loFreq = f;
    DrawOverlay();
}
//Track bandpass so we can display with cursor
void SpectrumWidget::SetFilter(int lo, int hi)
{
	loFilter = lo;
	hiFilter = hi;
    DrawOverlay();
}

void SpectrumWidget::plotSelectionChanged(SignalSpectrum::DISPLAYMODE mode)
{
    if (mode == SignalSpectrum::NODISPLAY) {
        ui.labelFrame->setVisible(false);
        ui.plotFrame->setVisible(false);
    } else {
        ui.labelFrame->setVisible(true);
        ui.plotFrame->setVisible(true);
    }

	if (signalSpectrum != NULL) {
		signalSpectrum->SetDisplayMode(mode);
	}
    if (global->sdr != NULL)
        global->sdr->lastDisplayMode = mode;

	spectrumMode = mode;
    DrawOverlay();
    update();
}
void SpectrumWidget::SetSignalSpectrum(SignalSpectrum *s) 
{
	signalSpectrum = s;	
	if (s!=NULL) {
        connect(signalSpectrum,SIGNAL(newFftData()),this,SLOT(newFftData()));

		sampleRate = s->SampleRate();
		upDownIncrement = s->settings->upDownIncrement;
		leftRightIncrement = s->settings->leftRightIncrement;
	}
}
// Diplays frequency cursor and filter range
//Now called from DrawOverlay
void SpectrumWidget::DrawCursor(QPainter &painter, QColor color)
{
    if (!isRunning)
        return;

    QRect plotFr = ui.plotFrame->geometry(); //relative to parent

    int plotWidth = plotFr.width();
    int plotHeight = plotFr.height();

    int hzPerPixel = sampleRate / plotWidth * zoom;

    //Show mixer cursor, fMixer varies from -f..0..+f relative to LO
    //Map to coordinates
    int x1;
    if (spectrumMode == SignalSpectrum::SPECTRUM || spectrumMode == SignalSpectrum::WATERFALL) {
        x1 = fMixer / hzPerPixel;  //Convert fMixer to pixels
        x1 = x1 + plotFr.center().x(); //Make relative to center
    } else {
        x1 = 0.5 * plotWidth; //Cursor doesn't move in other modes, keep centered
    }


    //Show filter range
    //This doesn't work for CW because we have to take into account offset
    int xLo, xHi;
    int modeOffset = global->settings->modeOffset; //Move to constructor
    int filterWidth = hiFilter - loFilter; //Could be pos or neg
    if (demodMode == dmCWU) {
        //loFilter = 500 hiFilter = 1500 modeOffset = 1000
        //Filter should be displayed 0 to 1000 or -1000 to 0 relative to cursor
        xLo = x1; //At the cursor
        xHi = x1 + filterWidth / hzPerPixel;
    } else if (demodMode == dmCWL) {
        xLo = x1 - filterWidth / hzPerPixel;
        xHi = x1;
    } else {
        xLo = x1 + loFilter/hzPerPixel;
        xHi = x1 + hiFilter/hzPerPixel;
    }

    //Shade filter area
    painter.setBrush(Qt::SolidPattern);
    painter.setOpacity(0.50);
    painter.fillRect(xLo, 0,x1 - xLo - 1, plotHeight, Qt::gray);
    painter.fillRect(x1+2, 0,xHi - x1 - 2, plotHeight, Qt::gray);

    //Show filter boundaries
    painter.setPen(QPen(Qt::cyan, 1,Qt::SolidLine));
    painter.drawLine(xLo,0,xLo, plotFr.height()); //Extend line into label frame
    painter.drawLine(xHi,0,xHi, plotFr.height()); //Extend line into label frame

    //Main cursor, draw last so it's on top
    painter.setOpacity(1.0);
    painter.setPen(QPen(color, 1,Qt::SolidLine));
    painter.drawLine(x1,0,x1, plotFr.height()); //Extend line into label frame

}

void SpectrumWidget::paintEvent(QPaintEvent *e)
{

    QPainter painter(this);
    QRect plotFr = ui.plotFrame->geometry(); //relative to parent
    int plotHeight = plotFr.height(); //Plot area height
    QRect plotLabelFr = ui.labelFrame->geometry();


	//Make all painter coordinates 0,0 relative to plotFrame
	//0,0 will be translated to 4,4 etc
    painter.translate(plotFr.x(),plotFr.y());

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


    //!!Not working yet, where to display
    QString label;
    mouseFreq = GetMouseFreq() + loFreq;
    int mouseDb = GetMouseDb();
    if (mouseFreq > 0)
        label.sprintf("%.3f kHz @ %ddB",mouseFreq / 1000.0,mouseDb);
    else
        label = "";

    QRect freqFr = ui.cursorLabel->geometry(); //Relative to controlFrame (parent)
    QRect ctrlFr = ui.controlFrame->geometry(); //Relateive to spectrum widget (parent)
    painter.setFont(global->settings->medFont);
    //x is from freq frame, y is from ctrlFr (relative to spectrumWidget)
    painter.drawText(freqFr.left(), ctrlFr.top() + (ctrlFr.height() * 0.7),label);

	if (spectrumMode == SignalSpectrum::SPECTRUM)
	{
        painter.drawPixmap(plotFr, plotArea); //Includes plotOverlay which was copied to plotArea
        painter.drawPixmap(plotLabelFr,plotLabel);
    } else if (spectrumMode == SignalSpectrum::WATERFALL) {
        painter.drawPixmap(plotFr, plotArea);
        painter.drawPixmap(plotLabelFr,plotLabel);
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
        int gain = 1; //(spectrumGain * 1000);

		//int offset = -25; //hack
        int offset = plotHeight * -0.5;

		int zoom = 1;
		//rawIQ[0].re = rawIQ[0].re * gain - offset;
		//rawIQ[0].im = rawIQ[0].im * gain - offset;
		CPX last = rawIQ[0];
		int nxtSample = 1;
		float ISamp,QSamp;
        for (int i=zoom; i<plotFr.width()-1; i += zoom)
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
        painter.eraseRect(plotFr);
	}

    //Tell spectrum generator it ok to give us another one
    signalSpectrum->displayUpdateComplete = true;
}

void SpectrumWidget::displayChanged(int item)
{
    switch (item)
    {
    case 0:
        plotSelectionChanged(SignalSpectrum::SPECTRUM);
        break;
    case 1:
        plotSelectionChanged(SignalSpectrum::WATERFALL);
        break;
    case 2:
        plotSelectionChanged(SignalSpectrum::IQ);
        break;
    case 3:
        plotSelectionChanged(SignalSpectrum::PHASE);
        break;
    case 4:
        plotSelectionChanged(SignalSpectrum::NODISPLAY);
        break;
    }
}

void SpectrumWidget::maxDbChanged(int s)
{
    plotMaxDb = ui.maxDbBox->value();
    update();
}

void SpectrumWidget::zoomChanged(int item)
{
    double newZoom;
    //Item goes from 10 to 200 (or anything), so zoom goes from 10/10(1) to 10/100(0.1) to 10/200(0.5)
    newZoom = 1.0 / pow(2.0,item/100.0);
    //There are 2 possible behaviors
    //We could just keep LO and zoom around it.  But if zoom puts mixer off screen, we won't see signal
    //So we assume user want's to keep signal in view and set the LO to the mixer, then zoom
    //But when zooming out, we expect LO to not change
    if (fMixer != 0 && newZoom < zoom)
        emit mixerChanged(fMixer, true); //Change LO to current mixer freq
    zoom = newZoom;
    //ui.zoomLabel->setText(QString().sprintf("Zoom: %.0f X",1.0 / zoom));
    ui.zoomLabel->setText(QString().sprintf("Span: %.0f kHz",sampleRate/1000 * zoom));

    //In zoom mode, hi/low mixer boundaries need to be updated so we always keep frequency on screen
    int range = sampleRate / 2 * zoom;
    emit mixerLimitsChanged(range, - range);

    //Update display
    DrawOverlay();
    update();
}

//New Fft data is ready for display, update screen if last update is finished
void SpectrumWidget::newFftData()
{
    if (!isRunning)
        return;

    qint32 fftMap[2048]; //!!Fix to dynamic max screen width

    double startFreq =  - (sampleRate/2); //Relative to 0
    double endFreq = sampleRate/2;
    startFreq = startFreq * zoom; //2x = zoom of 0.5
    endFreq = endFreq * zoom;

    //SpectrumGain of 1
    qint16 maxDbDisplayed = global->maxDb;
    qint16 minDbDisplayed = global->minDb;
    //Apply offset and gain options from UI
    minDbDisplayed = plotMinDb; //minDbDisplayed - spectrumOffset;
    maxDbDisplayed = plotMaxDb; //minDbDisplayed - (minDbDisplayed / spectrumGain);

    int plotWidth = plotArea.width();
    int plotHeight = plotArea.height();

    if (spectrumMode == SignalSpectrum::SPECTRUM) {
        //These shouldn't be allocated every call
        QPoint LineBuf[2048]; //[TB_MAX_SCREENSIZE];

        //!!Draw frequency overlay
        //first copy into 2Dbitmap the overlay bitmap.
        plotArea = plotOverlay.copy(0,0,plotWidth,plotHeight);
        //Painter has to be created AFTER we copy overlay since plotArea is being replaced each time
        QPainter plotPainter(&plotArea);

        //Convert to plot area coordinates
        //This gets passed straight through to FFT MapFFTToScreen
        signalSpectrum->MapFFTToScreen(
            plotHeight,
            plotWidth,
            //These are same as testbench
            maxDbDisplayed,      //FFT dB level  corresponding to output value == MaxHeight
            minDbDisplayed,   //FFT dB level corresponding to output value == 0
            startFreq, //-sampleRate/2, //Low frequency
            endFreq, //sampleRate/2, //High frequency
            fftMap );

        for (int i=0; i< plotArea.width(); i++)
        {
            LineBuf[i].setX(i);
            LineBuf[i].setY(fftMap[i]);
            //Keep track of peak values for optional display
            //if(fftbuf[i] < m_FftPkBuf[i])
            //    m_FftPkBuf[i] = fftMap[i];
        }
        plotPainter.setPen( Qt::green );
        //Just connect the dots in LineBuf!
        plotPainter.drawPolyline(LineBuf, plotWidth);

        //We can only display offscreen pixmap in paint() event, so call it to update
        update();

    } else if (spectrumMode == SignalSpectrum::WATERFALL) {
        //Color code each bin and draw line
        QPainter plotPainter(&plotArea);

        QColor plotColor;

        //Waterfall
        //Scroll fr rect 1 pixel to create new line
        QRect rect = plotArea.rect();
        //rect.setBottom(rect.bottom() - 10); //Reserve the last 10 pix for labels
        plotArea.scroll(0,1,rect);

        //Instead of plot area coordinates we convert to screen color array
        //Width is unchanged, but height is # colors we have for each db
        signalSpectrum->MapFFTToScreen(
            255, //Equates to spectrumColor array
            plotWidth,
            //These are same as testbench
            maxDbDisplayed, //FFT dB level  corresponding to output value == MaxHeight
            minDbDisplayed, //FFT dB level corresponding to output value == 0
            startFreq, //Low frequency
            endFreq, //High frequency
            fftMap );

        for (int i=0; i<plotArea.width(); i++)
        {
            plotColor = spectrumColors[255 - fftMap[i]];
            plotPainter.setPen(plotColor);
            plotPainter.drawPoint(i,0);

        }

        update();
    }
}

void SpectrumWidget::DrawSpectrum()
{

}

void SpectrumWidget::DrawWaterfall()
{

}

void SpectrumWidget::DrawOverlay()
{
    if(plotOverlay.isNull())
        return; //We haven't been created yet, empty

    plotOverlay.fill(Qt::black); //Clear every time because we are update cursor and other info here also
    plotLabel.fill(Qt::black);

    int overlayWidth = plotOverlay.width();
    int overlayHeight = plotOverlay.height();

    if (overlayHeight < 60) {
        horizDivs = 10;
        vertDivs = 5; //Too crowded with small spectrum
    } else {
        horizDivs = 10;
        vertDivs = 10;
    }

    int x,y;
    float pixPerHdiv;
    pixPerHdiv = (float)overlayWidth / (float)horizDivs;

    float pixPerVdiv;
    pixPerVdiv = (float)overlayHeight / (float)vertDivs;

    QRect rect;
    QPainter painter(&plotOverlay);
    painter.initFrom(this);
    QPainter labelPainter(&plotLabel);
    labelPainter.initFrom(this);

    int plotLabelHeight = plotLabel.height();

    if (spectrumMode == SignalSpectrum::SPECTRUM) {
        //draw vertical grids
        //y = overlayHeight - overlayHeight/vertDivs;
        for( int i = 1; i < horizDivs; i++)
        {
            x = (int)( (float)i*pixPerHdiv );
            if(i==horizDivs/2)
                painter.setPen(QPen(Qt::red, 1,Qt::DotLine));
            else
                painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
            painter.drawLine(x, 0, x , overlayHeight);
            //painter.drawLine(x, overlayHeight-5, x , overlayHeight);
        }

        //draw horizontal grids
        painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
        for( int i = 1; i<vertDivs; i++)
        {
            y = (int)( (float)i*pixPerVdiv );
            painter.drawLine(0, y, overlayWidth, y);
        }

        DrawCursor(painter, Qt::white);
        DrawCursor(labelPainter,Qt::white); //Continues into label area


    } else if (spectrumMode == SignalSpectrum::WATERFALL) {
#if 0
        //Draw spectrum colors centered at bottom of label pane
        for (int i=0; i<dbRange; i++) {
            labelPainter.setPen(spectrumColors[i]);
            labelPainter.drawPoint(overlayWidth/2 - 80 + i, plotLabelHeight);
            labelPainter.drawPoint(overlayWidth/2 - 80 + i, plotLabelHeight);
        }
#endif
        //We can't draw a cursor in scrolling waterfall unless we get into layers
        //This just displays it in label area
        DrawCursor(labelPainter,Qt::black);

    }


    //Draw the frequency scale

    //Show actual hi/lo frequency range
    quint32 horizLabels[horizDivs];
    //zoom is fractional
    //100,000 sps * 1.00 / 10 = 10,000 hz per division
    //100,000 sps * 0.10 / 10 = 1000 hz per division
    //Bug with 16bit at 2msps rate
    quint32 hzPerhDiv = sampleRate * zoom / horizDivs;

    //horizDivs must be even number so middle is 0
    //So for 10 divs we start out with [0] at low and [9] at high
    int center = horizDivs / 2;
    horizLabels[center] = loFreq/1000;
    for (int i=1; i <horizDivs; i++) {
        horizLabels[center + i] = (loFreq + (i * hzPerhDiv ))/1000;
        horizLabels[center - i] = (loFreq - (i * hzPerhDiv ))/1000;
    }

    QFont overlayFont("Arial");
    overlayFont.setPointSize(10);
    overlayFont.setWeight(QFont::Normal);
    labelPainter.setFont(overlayFont);

    labelPainter.setPen(QPen(Qt::cyan,1,Qt::SolidLine));

    for( int i=0; i <= horizDivs; i++) {
        if (i==0) {
            //Left justify
            x = (int)( (float) i * pixPerHdiv);
            rect.setRect(x ,0, (int)pixPerHdiv, plotLabelHeight);
            labelPainter.drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, QString::number(horizLabels[i],'f',0)+"k");

        } else if (i == horizDivs) {
            //Right justify
            x = (int)( (float)i*pixPerHdiv - pixPerHdiv);
            rect.setRect(x ,0, (int)pixPerHdiv, plotLabelHeight);
            labelPainter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, QString::number(horizLabels[i],'f',0)+"k");

        } else {
            //Center justify
            x = (int)( (float)i*pixPerHdiv - pixPerHdiv/2);
            rect.setRect(x ,0, (int)pixPerHdiv, plotLabelHeight);
            labelPainter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, QString::number(horizLabels[i],'f',0)+"k");

        }

    }


    //If we're not running, display overlay as plotArea
    if (!isRunning) {
        plotArea = plotOverlay.copy(0,0,overlayWidth,overlayHeight);
        update();
    }
}
