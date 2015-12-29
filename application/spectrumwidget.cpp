//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "spectrumwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWindow>
#include "device_interfaces.h"
#include "cmath" //For std::abs() that supports float.  Include last so it overrides abs(int)

class Receiver;

SpectrumWidget::SpectrumWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    ui.zoomLabelFrame->setVisible(false);
    ui.zoomPlotFrame->setVisible(false);

    ui.displayBox->addItem("Spectrum",SignalSpectrum::SPECTRUM);
    ui.displayBox->addItem("Waterfall",SignalSpectrum::WATERFALL);
    //ui.displayBox->addItem("I/Q",SignalSpectrum::IQ);
    //ui.displayBox->addItem("Phase",SignalSpectrum::PHASE);
    ui.displayBox->addItem("No Display",SignalSpectrum::NODISPLAY);
    ui.displayBox->setCurrentIndex(-1);

    connect(ui.displayBox,SIGNAL(currentIndexChanged(int)),this,SLOT(displayChanged(int)));

    //Starting plot range
	//CuteSDR defaults to -50
	plotMaxDb = global->settings->fullScaleDb;
	ui.maxDbBox->setMinimum(-50); //Makes no sense to set to anything below this or no useful information
	ui.maxDbBox->setMaximum(DB::maxDb);
    ui.maxDbBox->setValue(plotMaxDb);
	ui.maxDbBox->setSingleStep(5);
    connect(ui.maxDbBox,SIGNAL(valueChanged(int)),this,SLOT(maxDbChanged(int)));

	plotMinDb = global->settings->baseScaleDb;
	ui.minDbBox->setMinimum(DB::minDb);
	ui.minDbBox->setMaximum(ui.maxDbBox->minimum() - minMaxDbDelta);
	ui.minDbBox->setValue(plotMinDb);
	ui.minDbBox->setSingleStep(5);
	connect(ui.minDbBox,SIGNAL(valueChanged(int)),this,SLOT(minDbChanged(int)));


	ui.updatesPerSec->setMinimum(0); //Freezes display
	ui.updatesPerSec->setMaximum(30); //20 x/sec is very fast
	ui.updatesPerSec->setValue(global->settings->updatesPerSecond);
	ui.updatesPerSec->setSingleStep(2);;
	connect(ui.updatesPerSec,SIGNAL(valueChanged(int)),this,SLOT(updatesPerSecChanged(int)));
	spectrumMode=SignalSpectrum::SPECTRUM;

	//message = NULL;
	signalSpectrum = NULL;
	averageSpectrum = NULL;
	lastSpectrum = NULL;

	fMixer = 0;

	dbRange = std::abs(DB::maxDb - DB::minDb);

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

	//Used as a geometric scale 2^0 to 2^4 or 1x to 16x, 2^6= 1 to 64x
    //Should range change with sample rate?
    //With SDR-IP sample rates and 2048 FFT
    //   62,500 / 2048 samples =  30hz/bin, 64x =   976hz span
    //  250,000 / 2048 samples = 122hz/bin, 64x =  3906hz span
    //2,000,000 / 2048 samples = 976hz/bin, 64x = 31250hz span
    //Ideally we'd like a max 2k span to be able to see details of cw, rtty, wwv signals
	ui.zoomSlider->setRange(0,400);
    ui.zoomSlider->setSingleStep(1);
    ui.zoomSlider->setValue(0); //Left end of scale
    connect(ui.zoomSlider,SIGNAL(valueChanged(int)),this,SLOT(zoomChanged(int)));
    zoom = 1;

	ui.zoomSelector->addItem("Off",ZOOM_MODE::Off);
	ui.zoomSelector->addItem("Spect",ZOOM_MODE::Spectrum);
	ui.zoomSelector->addItem("HiRes",ZOOM_MODE::HiResolution);
	ui.zoomSelector->setCurrentIndex(0);

	connect(ui.zoomSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(zoomSelectorChanged(int)));

    modeOffset = 0;

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

    QRect zoomPlotFr = ui.zoomPlotFrame->geometry();
    zoomPlotArea = QPixmap(zoomPlotFr.width(),zoomPlotFr.height());
    zoomPlotArea.fill(Qt::black);
    zoomPlotOverlay = QPixmap(zoomPlotFr.width(), zoomPlotFr.height());
    QRect zoomPlotLabelFr = ui.zoomLabelFrame->geometry();
    zoomPlotLabel = QPixmap(zoomPlotLabelFr.width(),zoomPlotLabelFr.height());
    zoomPlotLabel.fill(Qt::black);

	if (r) {
		ui.displayBox->setCurrentIndex(global->sdr->Get(DeviceInterface::LastSpectrumMode).toInt()); //Initial display mode
		if (zoomMode != Off)
			updateZoomFrame(Off, true);
		ui.zoomLabel->setText(QString().sprintf("S: %.0f kHz",sampleRate/1000.0));
		isRunning = true;
	}
	else {
		ui.displayBox->blockSignals(true);
		ui.displayBox->setCurrentIndex(-1);
		ui.displayBox->blockSignals(false);
		ui.zoomLabel->setText(QString().sprintf(""));
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

void SpectrumWidget::resizeFrames()
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

    QRect zoomPlotFr = ui.zoomPlotFrame->geometry();
    zoomPlotArea = QPixmap(zoomPlotFr.width(),zoomPlotFr.height());
    zoomPlotArea.fill(Qt::black);
    zoomPlotOverlay = QPixmap(zoomPlotFr.width(), zoomPlotFr.height());

    QRect zoomPlotLabelFr = ui.zoomLabelFrame->geometry();
    zoomPlotLabel = QPixmap(zoomPlotLabelFr.width(),zoomPlotLabelFr.height());
    zoomPlotLabel.fill(Qt::black);

    DrawOverlay(false); //plotArea size changed
	if (zoomMode != Off)
        DrawOverlay(true);
}

void SpectrumWidget::resizeEvent(QResizeEvent *event)
{
    resizeFrames();
	event->accept(); //We handle
}

//Returns +/- freq from LO based on where mouse cursor is
//Called from paint & mouse click
double SpectrumWidget::GetMouseFreq()
{
    QRect pf = this->ui.plotFrame->geometry();
    QRect lf = this->ui.labelFrame->geometry();
    QRect zpf = this->ui.zoomPlotFrame->geometry();
    QRect zlf = this->ui.zoomLabelFrame->geometry();

    QPoint mp = QCursor::pos(); //Current mouse position in global coordinates
    mp = mapFromGlobal(mp); //Convert to widget relative

    if (pf.contains(mp) || lf.contains(mp)) {
        //Find freq at cursor
        float hzPerPixel = sampleRate / pf.width();
        //Convert to +/- relative to center
        int m = mp.x() - pf.center().x();
        //And conver to freq
        m *= hzPerPixel;
        return m;

    } else if (zpf.contains(mp) || zlf.contains(mp)) {
        //Find freq at cursor
        float hzPerPixel;
		if (zoomMode == Spectrum)
            hzPerPixel= sampleRate / zpf.width() * zoom;
        else
            hzPerPixel= signalSpectrum->getZoomedSampleRate() / zpf.width() * zoom;

        //Convert to +/- relative to center
        int m = mp.x() - zpf.center().x();
        //And conver to freq
        m *= hzPerPixel;
        //and factor in mixer
        m += fMixer;
        return m;

    } else {
        //Not in our area
        return 0.0;
    }

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

	if (spectrumMode == SignalSpectrum::NODISPLAY)
		return;

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
		event->accept();
		return;
	case Qt::Key_Right:
		m += leftRightIncrement;
		emit mixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Down:
		m -= upDownIncrement;
		emit mixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Left:
		m -= leftRightIncrement;
		emit mixerChanged(m);
		event->accept();
		return;
    case Qt::Key_Enter:
        emit mixerChanged(m, true); //Change LO to current mixer freq
		event->accept();
		return;
	}
	event->ignore();
}
void SpectrumWidget::mousePressEvent ( QMouseEvent * event ) 
{
	if (!isRunning || signalSpectrum == NULL)
		return;
	if (spectrumMode == SignalSpectrum::NODISPLAY)
		return;

    Qt::MouseButton button = event->button();
    // Mac: Qt::ControlModifier == Command Key
    // Mac: Qt::AltModifer == Option(Alt) Key
    // Mac: Qt::MetaModifier == Control Key

    Qt::KeyboardModifiers modifiers = event->modifiers(); //Keyboard modifiers

    double deltaFreq = GetMouseFreq();
    if (deltaFreq == 0)
        return; //not in our area or no mixer

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
void SpectrumWidget::SetMode(DeviceInterface::DEMODMODE m, int _modeOffset)
{
	demodMode = m;
    modeOffset = _modeOffset;
}
void SpectrumWidget::SetMixer(int m, double f)
{
	fMixer = m;
	loFreq = f;
    DrawOverlay(false);
	if (zoomMode != Off)
        DrawOverlay(true);
}
//Track bandpass so we can display with cursor
void SpectrumWidget::SetFilter(int lo, int hi)
{
	loFilter = lo;
	hiFilter = hi;
    DrawOverlay(false);
	if (zoomMode != Off)
        DrawOverlay(true);
}

void SpectrumWidget::plotSelectionChanged(SignalSpectrum::DISPLAYMODE mode)
{
    if (mode == SignalSpectrum::NODISPLAY) {
        ui.labelFrame->setVisible(false);
        ui.plotFrame->setVisible(false);
        ui.zoomLabelFrame->setVisible(false);
        ui.zoomPlotFrame->setVisible(false);
        ui.maxDbBox->setVisible(false);
        ui.zoomSlider->setVisible(false);
        ui.zoomLabel->setVisible(false);
		ui.zoomSelector->setVisible(false);
		ui.maxDbBox->setVisible(false);
		ui.minDbBox->setVisible(false);
		ui.updatesPerSec->setVisible(false);
        //Turn zoom off
        zoom = 1;
		zoomMode = Off;
        ui.zoomSlider->setValue(0);
		//Bug: Doesn't collapse hidden frames
		adjustSize();

#if 0
		//Old code, not sure why we did this
        QWidget *parent = ui.controlFrame->parentWidget();
        while (parent) {
            //Warning, adj size will use all layout hints, including h/v spacer sizing.  So overall size may change
            parent->adjustSize();
            parent = parent->parentWidget();
        }
#endif

    } else {
        ui.labelFrame->setVisible(true);
        ui.plotFrame->setVisible(true);
        //ui.zoomLabelFrame->setVisible(false);
        //ui.zoomPlotFrame->setVisible(false);
        ui.maxDbBox->setVisible(true);
        ui.zoomSlider->setVisible(true);
        ui.zoomLabel->setVisible(true);
		ui.zoomSelector->setVisible(true);
		ui.maxDbBox->setVisible(true);
		ui.minDbBox->setVisible(true);
		ui.updatesPerSec->setVisible(true);
	}

    spectrumMode = mode;
    if (signalSpectrum != NULL) {
		signalSpectrum->SetDisplayMode(spectrumMode, zoomMode == HiResolution);
    }


    if (spectrumMode != SignalSpectrum::NODISPLAY) {
        plotArea.fill(Qt::black);
        zoomPlotArea.fill(Qt::black);

        DrawOverlay(false);
		if (zoomMode != Off)
            DrawOverlay(true);
    }
	update();
}

void SpectrumWidget::updatesPerSecChanged(int item)
{
	if (signalSpectrum == NULL)
		return;
	signalSpectrum->SetUpdatesPerSec(ui.updatesPerSec->value());
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
void SpectrumWidget::DrawCursor(QPainter *painter, QRect plotFr, bool isZoomed, QColor color)
{
    if (!isRunning)
        return;

    int plotWidth = plotFr.width();
    int plotHeight = plotFr.height();

    int hzPerPixel = sampleRate / plotWidth;
    if (isZoomed) {
		if (zoomMode == Spectrum)
            hzPerPixel = sampleRate * zoom / plotWidth;
        else
            hzPerPixel = signalSpectrum->getZoomedSampleRate() * zoom / plotWidth;
    }

    //Show mixer cursor, fMixer varies from -f..0..+f relative to LO
    //Map to coordinates
    int x1;
    if (spectrumMode == SignalSpectrum::SPECTRUM || spectrumMode == SignalSpectrum::WATERFALL) {
        if (isZoomed) {
            x1 = 0.5 * plotWidth; //Zoom is always centered
        } else {
            x1 = fMixer / hzPerPixel;  //Convert fMixer to pixels
            x1 = x1 + plotFr.center().x(); //Make relative to center
        }
    } else {
        x1 = 0.5 * plotWidth; //Cursor doesn't move in other modes, keep centered
    }


    //Show filter range
    //This doesn't work for CW because we have to take into account offset
    int xLo, xHi;
    int filterWidth = hiFilter - loFilter; //Could be pos or neg
	if (demodMode == DeviceInterface::dmCWU) {
        //loFilter = 500 hiFilter = 1500 modeOffset = 1000
        //Filter should be displayed 0 to 1000 or -1000 to 0 relative to cursor
        xLo = x1; //At the cursor
        xHi = x1 + filterWidth / hzPerPixel;
	} else if (demodMode == DeviceInterface::dmCWL) {
        xLo = x1 - filterWidth / hzPerPixel;
        xHi = x1;
    } else {
        xLo = x1 + loFilter/hzPerPixel;
        xHi = x1 + hiFilter/hzPerPixel;
    }

    //Shade filter area
    painter->setBrush(Qt::SolidPattern);
    painter->setOpacity(0.50);
    painter->fillRect(xLo, 0,x1 - xLo - 1, plotHeight, Qt::gray);
    painter->fillRect(x1+2, 0,xHi - x1 - 2, plotHeight, Qt::gray);

    //Show filter boundaries
    painter->setPen(QPen(Qt::cyan, 1,Qt::SolidLine));
    painter->drawLine(xLo,0,xLo, plotFr.height()); //Extend line into label frame
    painter->drawLine(xHi,0,xHi, plotFr.height()); //Extend line into label frame

    //Main cursor, draw last so it's on top
    painter->setOpacity(1.0);
    painter->setPen(QPen(color, 1,Qt::SolidLine));
    painter->drawLine(x1,0,x1, plotFr.height()); //Extend line into label frame

}

void SpectrumWidget::paintEvent(QPaintEvent *e)
{

    QPainter painter(this);
    QRect plotFr = ui.plotFrame->geometry(); //relative to parent
    int plotHeight = plotFr.height(); //Plot area height
    QRect plotLabelFr = ui.labelFrame->geometry();
    QRect zoomPlotFr = ui.zoomPlotFrame->geometry(); //relative to parent
    int zoomPlotHeight = zoomPlotFr.height(); //Plot area height
    QRect zoomPlotLabelFr = ui.zoomLabelFrame->geometry();


	//Make all painter coordinates 0,0 relative to plotFrame
	//0,0 will be translated to 4,4 etc
    //painter.translate(plotFr.x(),plotFr.y());

	if (!isRunning)
	{
		if (true)
		{
            painter.setFont(global->settings->medFont);
			for (int i=0; i<message.count(); i++)
			{
                painter.drawText(20, 15 + (i*12) , message[i]);
			}
		}
	return;
	}

#if 0
    //Here for testing.  See SignalSpectrum for rest
    signalSpectrum->emitFftCounter--;
    if (signalSpectrum->emitFftCounter < 0) {
        qDebug()<<"More paints than FFTs"<<":"<<signalSpectrum->emitFftCounter;
        signalSpectrum->emitFftCounter = 0;
        //We may have garbage in spectrum, but if we skip we get flash of blank screen
    } else if (signalSpectrum->emitFftCounter > 0) {
        qDebug()<<"More FFTs than patings"<<":"<<signalSpectrum->emitFftCounter;
        signalSpectrum->emitFftCounter = 0;
        //We know we have valid spectrum, we just lost a previous one, so go ahead and plot
    }
#endif

    if (spectrumMode == SignalSpectrum::NODISPLAY || signalSpectrum == NULL)
        return;


    //Cursor tracking of freq and db
    QString label;
    mouseFreq = GetMouseFreq() + loFreq;
    int mouseDb = GetMouseDb();
    if (mouseFreq > 0)
		//label.sprintf("%.3f kHz @ %ddB",mouseFreq / 1000.0,mouseDb);
		label.sprintf("%.3f kHz",mouseFreq / 1000.0);
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
		if (zoomMode != Off) {
            painter.drawPixmap(zoomPlotFr, zoomPlotArea); //Includes plotOverlay which was copied to plotArea
            painter.drawPixmap(zoomPlotLabelFr,zoomPlotLabel);
        }
    } else if (spectrumMode == SignalSpectrum::WATERFALL) {
        painter.drawPixmap(plotFr, plotArea);
        painter.drawPixmap(plotLabelFr,plotLabel);
		if (zoomMode != Off) {
            painter.drawPixmap(zoomPlotFr, zoomPlotArea); //Includes plotOverlay which was copied to plotArea
            painter.drawPixmap(zoomPlotLabelFr,zoomPlotLabel);
        }
    }
	else
	{
        painter.eraseRect(plotFr);
	}

    //Tell spectrum generator it ok to give us another one
    signalSpectrum->displayUpdateComplete = true;
}

void SpectrumWidget::displayChanged(int s)
{
    //Get mode from itemData
    SignalSpectrum::DISPLAYMODE displayMode = (SignalSpectrum::DISPLAYMODE)ui.displayBox->itemData(s).toInt();
	//Save in device ini
	global->sdr->Set(DeviceInterface::LastSpectrumMode,displayMode);
    plotSelectionChanged(displayMode);
}

void SpectrumWidget::maxDbChanged(int s)
{
    plotMaxDb = ui.maxDbBox->value();
	global->settings->fullScaleDb = plotMaxDb;
	global->settings->WriteSettings(); //save
	DrawOverlay(false);
	if (zoomMode != Off)
		DrawOverlay(true); //plotArea db range changed
	update();
}

void SpectrumWidget::minDbChanged(int t)
{
	plotMinDb = ui.minDbBox->value();
	global->settings->baseScaleDb = plotMinDb;
	global->settings->WriteSettings(); //save
	DrawOverlay(false);
	if (zoomMode != Off)
		DrawOverlay(true); //plotArea db range changed
	update();

}

void SpectrumWidget::zoomChanged(int item)
{
	double newZoom;

	//Zoom is a geometric progression so it feels smooth
    //Item goes from 10 to 200 (or anything), so zoom goes from 10/10(1) to 10/100(0.1) to 10/200(0.5)
	//Eventually we should calculate zoom factors using sample rate and bin size to make sure we can't
	//over-zoom
	if (zoomMode == Spectrum)
		//Highest zoom factor is with item==400, .0625
		newZoom = 1.0 / pow(2.0,item/100.0);
	else if (zoomMode == HiResolution)
		//Since SR is lower (48k or 64k typically) max zoom at 400 is only .15749
		newZoom = 1.0 / pow(2.0,item/150.0);
	else
		newZoom = 1.0;

	if (zoomMode == Off && item > 0) {
		//Turn on zoom
		updateZoomFrame(Spectrum,false);
	}
#if 0
	//Turning zoom off automatically casues user problems when adjusting zoom at lower levels
	//Too easy to turn it off by mistake
	else if (item == 0) {
		//Turn off zoom
		updateZoomFrame(Off,false);
	}
#endif

    //There are 2 possible behaviors
    //We could just keep LO and zoom around it.  But if zoom puts mixer off screen, we won't see signal
    //So we assume user want's to keep signal in view and set the LO to the mixer, then zoom
    //But when zooming out, we expect LO to not change
    //if (fMixer != 0 && newZoom < zoom)
    //    emit mixerChanged(fMixer, true); //Change LO to current mixer freq

    zoom = newZoom;

    //In zoom mode, hi/low mixer boundaries need to be updated so we always keep frequency on screen
    //emit mixerLimitsChanged(range, - range);

	//Update display with new labels
    DrawOverlay(false);
	if (zoomMode != Off)
        DrawOverlay(true);
    update();
}

//New Fft data is ready for display, update screen if last update is finished
void SpectrumWidget::newFftData()
{
    if (!isRunning)
        return;

    double startFreq =  - (sampleRate/2); //Relative to 0
    double endFreq = sampleRate/2;
    //Zoom is always centered on fMixer
    //Start by assuming we have full +/- samplerate and make fMixer zero centered
    double zoomStartFreq =  startFreq * zoom;
    double zoomEndFreq = endFreq * zoom ;
    zoomStartFreq += fMixer;
    zoomEndFreq += fMixer;
    //Now handle overflow, we want fMixer in center regardless of where we are
    double adj = 0;
    if (zoomStartFreq <= startFreq) {
        adj = zoomStartFreq - startFreq;
        zoomStartFreq = startFreq;
        zoomEndFreq -= adj;
    } else if (zoomEndFreq >= endFreq) {
        adj = zoomEndFreq - endFreq;
        zoomEndFreq = endFreq;
        zoomStartFreq += adj;
    }

    //SpectrumGain of 1
	qint16 maxDbDisplayed = DB::maxDb;
	qint16 minDbDisplayed = DB::minDb;
    //Apply offset and gain options from UI
    minDbDisplayed = plotMinDb; //minDbDisplayed - spectrumOffset;
    maxDbDisplayed = plotMaxDb; //minDbDisplayed - (minDbDisplayed / spectrumGain);

    int plotWidth = plotArea.width();
    int plotHeight = plotArea.height();

    if (spectrumMode == SignalSpectrum::SPECTRUM) {
		//Gradient is used for fill
		QLinearGradient gradient;
		//Relative to bounding rectanble of polygon since polygon changes every update
		//Upper right (0,0) and Lower left (1,1)
		gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
		gradient.setStart(0.5, 0); //Midway top
		//Semi-transparent green.  Solid green Pen will show spectrum line
		gradient.setColorAt(0.20,QColor(0, 200, 0, 127));
		//Semi-transparent white, alpha = 127  alpha = 0 is fully transparenet, 255 is fully opaque
		gradient.setColorAt(1.0, QColor(255, 255, 255, 127));
		gradient.setFinalStop(0.5,1); //Midway bottom

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

		//Define a line or a polygon (if filled) that represents the spectrum power levels
		quint16 numPoints = 0;
		//Start at lower left corner (upper left is (0,0) in Qt coordinate system
		LineBuf[numPoints].setX(0);
		LineBuf[numPoints].setY(plotHeight);
		numPoints++;
		for (int i=1; i< plotWidth; i++)
        {
			LineBuf[numPoints].setX(i);
			LineBuf[numPoints].setY(fftMap[i]);
			numPoints++;
            //Keep track of peak values for optional display
            //if(fftbuf[i] < m_FftPkBuf[i])
            //    m_FftPkBuf[i] = fftMap[i];
        }
		//MiterJoin gives us sharper peaks
		QPen pen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
		plotPainter.setPen(pen);
		//Add points to complete the polygon
		LineBuf[numPoints].setX(plotWidth);
		LineBuf[numPoints].setY(plotHeight);

		numPoints++;
		//Connect back to origin
		LineBuf[numPoints].setX(0);
		LineBuf[numPoints].setY(plotHeight);

#if 0
		//Just draw the spectrum line, no fill
		plotPainter.drawPolyline(LineBuf, numPoints);
#else
		//Draw the filled polygon.  Note this may take slightly more CPUs
		QBrush tmpBrush = QBrush(gradient);
		plotPainter.setBrush(tmpBrush);
		plotPainter.drawPolygon(LineBuf,numPoints);

#endif
		if (zoomMode != Off) {
            plotWidth = zoomPlotArea.width();
            plotHeight = zoomPlotArea.height();

			//first copy into 2Dbitmap the overlay bitmap.
			zoomPlotArea = zoomPlotOverlay.copy(0,0,plotWidth,plotHeight);
			//Painter has to be created AFTER we copy overlay since plotArea is being replaced each time
			QPainter zoomPlotPainter(&zoomPlotArea);

			if (zoomMode == Spectrum) {
				signalSpectrum->MapFFTToScreen(
					plotHeight,
					plotWidth,
					//These are same as testbench
					maxDbDisplayed,      //FFT dB level  corresponding to output value == MaxHeight
					minDbDisplayed,   //FFT dB level corresponding to output value == 0
					zoomStartFreq, //-sampleRate/2, //Low frequency
					zoomEndFreq, //sampleRate/2, //High frequency
					zoomedFftMap );

			} else {
				//Convert to plot area coordinates
				//This gets passed straight through to FFT MapFFTToScreen
				signalSpectrum->MapFFTZoomedToScreen(
					plotHeight,
					plotWidth,
					//These are same as testbench
					maxDbDisplayed,      //FFT dB level  corresponding to output value == MaxHeight
					minDbDisplayed,   //FFT dB level corresponding to output value == 0
					zoom,
					modeOffset,
					zoomedFftMap );
			}

			//Start at lower left corner (upper left is (0,0) in Qt coordinate system
			numPoints = 0;
			LineBuf[numPoints].setX(0);
			LineBuf[numPoints].setY(plotHeight);
			numPoints++;
			for (int i=1; i< plotWidth; i++)
			{
				LineBuf[numPoints].setX(i);
				LineBuf[numPoints].setY(zoomedFftMap[i]);
				numPoints++;
				//Keep track of peak values for optional display
				//if(fftbuf[i] < m_FftPkBuf[i])
				//    m_FftPkBuf[i] = fftMap[i];
			}
			//MiterJoin gives us sharper peaks
			QPen pen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
			zoomPlotPainter.setPen(pen);
			//Add points to complete the polygon
			LineBuf[numPoints].setX(plotWidth);
			LineBuf[numPoints].setY(plotHeight);

			numPoints++;
			//Connect back to origin
			LineBuf[numPoints].setX(0);
			LineBuf[numPoints].setY(plotHeight);

#if 0
			//Just connect the dots in LineBuf!
			//Just draw the spectrum line, no fill
			zoomPlotPainter.drawPolyline(LineBuf, numPoints);
#else
			//Draw the filled polygon.  Note this may take slightly more CPUs
			QBrush tmpBrush = QBrush(gradient);
			zoomPlotPainter.setBrush(tmpBrush);
			zoomPlotPainter.drawPolygon(LineBuf,numPoints);

#endif

        }
        //We can only display offscreen pixmap in paint() event, so call it to update
        update();

    } else if (spectrumMode == SignalSpectrum::WATERFALL) {
        //Color code each bin and draw line
        QPainter plotPainter(&plotArea);
        QPainter zoomPlotPainter(&zoomPlotArea);

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

        //Testng vertical spacing in zoom mode
        int vspace = 1;
		if (zoomMode != Off) {
            //Waterfall
            //Scroll fr rect 1 pixel to create new line
            QRect rect = zoomPlotArea.rect();
            //rect.setBottom(rect.bottom() - 10); //Reserve the last 10 pix for labels
            zoomPlotArea.scroll(0,vspace,rect);

			if (zoomMode == Spectrum) {
                signalSpectrum->MapFFTToScreen(
                    255, //Equates to spectrumColor array
                    plotWidth,
                    //These are same as testbench
                    maxDbDisplayed, //FFT dB level  corresponding to output value == MaxHeight
                    minDbDisplayed, //FFT dB level corresponding to output value == 0
                    zoomStartFreq, //Low frequency
                    zoomEndFreq, //High frequency
                    fftMap );

            } else {
                //Instead of plot area coordinates we convert to screen color array
                //Width is unchanged, but height is # colors we have for each db
                signalSpectrum->MapFFTZoomedToScreen(
                    255, //Equates to spectrumColor array
                    plotWidth,
                    //These are same as testbench
                    maxDbDisplayed, //FFT dB level  corresponding to output value == MaxHeight
                    minDbDisplayed, //FFT dB level corresponding to output value == 0
                    zoom,
                    modeOffset,
                    fftMap );
            }
            for (int i=0; i<plotArea.width(); i++)
            {
                plotColor = spectrumColors[255 - fftMap[i]];
                zoomPlotPainter.setPen(plotColor);
                zoomPlotPainter.drawPoint(i,0);
            }

        }

        update();
	}
}

void SpectrumWidget::zoomSelectorChanged(int item)
{
	Q_UNUSED(item);

	updateZoomFrame( (ZOOM_MODE)ui.zoomSelector->currentData().toInt(), true);
}

void SpectrumWidget::updateZoomFrame(ZOOM_MODE newMode, bool updateSlider)
{
	//Make sure zoomSelector is changed if this is not called from zoomSelector event
	ui.zoomSelector->blockSignals(true);
	ui.zoomSelector->setCurrentIndex(newMode);
	ui.zoomSelector->blockSignals(false);

	//setValue will automatically signal value changed and call connected SLOT unless we block signals
	switch (newMode) {
		case Off: //No zoom
			ui.zoomLabelFrame->setVisible(false);
			ui.zoomPlotFrame->setVisible(false);
			//We apparently need to call adjustSize() when we hide windows, but not when we make visible
			adjustSize();  //If we don't call this, grid and labels aren't painted correctly
			resizeFrames();
			zoomMode = Off;
			if (updateSlider)
				ui.zoomSlider->setValue(0);
			break;
		case Spectrum: //Expanded unprocessed spectrum
			if (zoomMode == Off) {
				//Set zoom to middle of range for now, eventually base on bin width
				ui.zoomLabelFrame->setVisible(true);
				ui.zoomPlotFrame->setVisible(true);
				//adjustSize(); //Why do we have to call this when we hide frames, but not when we show them?
				resizeFrames();
			}
			zoomMode = Spectrum;
			if (updateSlider)
				ui.zoomSlider->setValue((ui.zoomSlider->maximum() - ui.zoomSlider->minimum()) / 4);
			break;
		case HiResolution: //HiRes pre-demod
			if (zoomMode == Off) {
				ui.zoomLabelFrame->setVisible(true);
				ui.zoomPlotFrame->setVisible(true);
				//adjustSize(); //Causes problems
				resizeFrames();
			}
			zoomMode = HiResolution;
			if (updateSlider)
				ui.zoomSlider->setValue((ui.zoomSlider->maximum() - ui.zoomSlider->minimum()) / 8);
			break;
	}

	//Let spectrum know if it should process HiRes FFT
	signalSpectrum->SetDisplayMode(spectrumMode, zoomMode == HiResolution);
}

void SpectrumWidget::DrawSpectrum()
{

}

void SpectrumWidget::DrawWaterfall()
{

}

void SpectrumWidget::DrawScale(QPainter *labelPainter, double centerFreq, bool drawZoomed)
{
    if (!isRunning)
        return;

    //Draw the frequency scale
    float pixPerHdiv;
	pixPerHdiv = (float)plotLabel.width() / (float)numXDivs;
    int x,y;
    QRect rect;
    int plotLabelHeight = plotLabel.height();

    //Show actual hi/lo frequency range
    QString horizLabels[maxHDivs];
    //zoom is fractional
    //100,000 sps * 1.00 / 10 = 10,000 hz per division
    //100,000 sps * 0.10 / 10 = 1000 hz per division
    //Bug with 16bit at 2msps rate
	quint32 hzPerhDiv = sampleRate / numXDivs;
	if (drawZoomed) {
		if (zoomMode == Spectrum)
			hzPerhDiv = sampleRate * zoom / numXDivs;
        else
			hzPerhDiv = signalSpectrum->getZoomedSampleRate() * zoom / numXDivs;
    }

    //horizDivs must be even number so middle is 0
    //So for 10 divs we start out with [0] at low and [9] at high
	int center = numXDivs / 2;
	if (center < 1000000000)
		horizLabels[center] = QString::number(centerFreq/1000.0,'f',1)+"k";
	else
		horizLabels[center] = QString::number(centerFreq/1000000.0,'f',1)+"m";

	int tick,left,right;
    for (int i=0; i <= center; i++) {
        tick = i * hzPerhDiv;
        left = centerFreq - tick;
        //Never let left label go negative, possible if center < sampleRate/2
        if (left < 0)
            horizLabels[center - i] = "---";
		else if (left < 1000000000)
			horizLabels[center - i] = QString::number(left/1000.0,'f',1)+"k";
		else
			horizLabels[center - i] = QString::number(left/1000000.0,'f',1)+"m";


        right = centerFreq + tick;
		if (right < 1000000000)
			horizLabels[center + i] = QString::number(right/1000.0,'f',1)+"k";
		else
			horizLabels[center + i] = QString::number(right/1000000.0,'f',1)+"m";

    }

    QFont overlayFont("Arial");
    overlayFont.setPointSize(10);
    overlayFont.setWeight(QFont::Normal);
    labelPainter->setFont(overlayFont);

    labelPainter->setPen(QPen(Qt::cyan,1,Qt::SolidLine));

	for( int i=0; i <= numXDivs; i++) {
        if (i==0) {
            //Left justify
            x = (int)( (float) i * pixPerHdiv);
            rect.setRect(x ,0, (int)pixPerHdiv, plotLabelHeight);
            labelPainter->drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, horizLabels[i]);

		} else if (i == numXDivs) {
            //Right justify
            x = (int)( (float)i*pixPerHdiv - pixPerHdiv);
            rect.setRect(x ,0, (int)pixPerHdiv, plotLabelHeight);
            labelPainter->drawText(rect, Qt::AlignRight|Qt::AlignVCenter, horizLabels[i]);

        } else {
            //Center justify
            x = (int)( (float)i*pixPerHdiv - pixPerHdiv/2);
            rect.setRect(x ,0, (int)pixPerHdiv, plotLabelHeight);
            labelPainter->drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, horizLabels[i]);

        }

    }

}

void SpectrumWidget::DrawOverlay(bool drawZoomed)
{
    if (!isRunning)
        return;

    //!!! 1/11/14 This never seems to get called with isRunning == true

    QPainter *painter = new QPainter();
    QPainter *labelPainter = new QPainter();

    int overlayWidth;
    int overlayHeight;
    QRect plotFr;
	quint16 dbLabelInterval;

	//Update span label
	qint32 range = sampleRate;
	qint32 zoomRange = signalSpectrum->getZoomedSampleRate();
	if (zoomMode == Off || zoomMode == Spectrum) {
		ui.zoomLabel->setText(QString().sprintf(" %.0f kHz",range/1000 * zoom));
	} else {
		//HiRes
		ui.zoomLabel->setText(QString().sprintf(" %.0f kHz",zoomRange/1000 * zoom));
	}


	if (!drawZoomed) {
        if (plotOverlay.isNull())
            return; //Not created yet

        plotOverlay.fill(Qt::black); //Clear every time because we are update cursor and other info here also
        plotLabel.fill(Qt::black);
        overlayWidth = plotOverlay.width();
        overlayHeight = plotOverlay.height();
        plotFr = ui.plotFrame->geometry(); //relative to parent
        painter->begin(&plotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&plotLabel);

    } else {
        if (zoomPlotOverlay.isNull())
            return;

        zoomPlotOverlay.fill(Qt::black); //Clear every time because we are update cursor and other info here also
        zoomPlotLabel.fill(Qt::black);
        overlayWidth = zoomPlotOverlay.width();
        overlayHeight = zoomPlotOverlay.height();
        plotFr = ui.zoomPlotFrame->geometry(); //relative to parent
        painter->begin(&zoomPlotOverlay);  //Because we use pointer, automatic with instance
        labelPainter->begin(&zoomPlotLabel);
    }

	numXDivs = (overlayWidth / 100) * 2; //x2 to make sure always even number
	if (overlayHeight < 100) {
		dbLabelInterval = 20; //20db each div
	} else {
		dbLabelInterval = 10; //10db each div
	}
	numYDivs = (plotMaxDb - plotMinDb) / dbLabelInterval; //Range / db per div

    int x,y;
    float pixPerHdiv;
	pixPerHdiv = (float)overlayWidth / (float)numXDivs;

    float pixPerVdiv;
	pixPerVdiv = (float)overlayHeight / (float)numYDivs;

	QFont overlayFont("Arial");
	overlayFont.setPointSize(10);
	overlayFont.setWeight(QFont::Normal);
	painter->setFont(overlayFont);

    if (spectrumMode == SignalSpectrum::SPECTRUM) {
		//draw X grids
        //y = overlayHeight - overlayHeight/vertDivs;
		for( int i = 1; i < numXDivs; i++)
        {
            x = (int)( (float)i*pixPerHdiv );
			if(i==numXDivs/2)
                painter->setPen(QPen(Qt::red, 1,Qt::DotLine));
            else
                painter->setPen(QPen(Qt::white, 1,Qt::DotLine));
            painter->drawLine(x, 0, x , overlayHeight);
            //painter.drawLine(x, overlayHeight-5, x , overlayHeight);
        }

		//draw Y grids
		painter->setPen(QPen(Qt::white, 1,Qt::DotLine));
		for( int i = 1; i<numYDivs; i++)
        {
            y = (int)( (float)i*pixPerVdiv );
            painter->drawLine(0, y, overlayWidth, y);
        }
		//draw Y scale
		painter->setPen(QPen(Qt::white));

		for (int i=1; i<numYDivs; i++) {
			y = (int)( (float)i*pixPerVdiv );
			//Draw slightly above line (y) for visibility
			//Start with plotMaxDB (neg value) and work to minDB
			painter->drawText(0, y-2, "-"+QString::number(abs(plotMaxDb) + (i * dbLabelInterval)));
		}

		DrawCursor(painter, plotFr, drawZoomed, Qt::white);
		DrawCursor(labelPainter, plotFr, drawZoomed, Qt::white); //Continues into label area


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
		DrawCursor(labelPainter,plotFr, drawZoomed, Qt::white);
    }


	if (!drawZoomed)
        DrawScale(labelPainter, loFreq, false);
    else
        DrawScale(labelPainter, loFreq + fMixer, true);


    //If we're not running, display overlay as plotArea
    if (!isRunning) {
        plotArea = plotOverlay.copy(0,0,overlayWidth,overlayHeight);
        update();
    }

    painter->end();  //Because we use pointer, automatic with instance
    labelPainter->end();
    delete painter;
    delete labelPainter;

}
