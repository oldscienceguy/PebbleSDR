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

	ui.topLabelFrame->setVisible(false);
	ui.topPlotFrame->setVisible(false);
	ui.topFrameSplitter->setVisible(false);

	ui.displayBox->addItem("Spectrum",SPECTRUM);
	ui.displayBox->addItem("Waterfall",WATERFALL);
	ui.displayBox->addItem("Spectrum/Spectrum",SPECTRUM_SPECTRUM);
	ui.displayBox->addItem("Spectrum/Waterfall",SPECTRUM_WATERFALL);
	ui.displayBox->addItem("Waterfall/Waterfall",WATERFALL_WATERFALL);
	//ui.displayBox->addItem("I/Q",IQ);
	//ui.displayBox->addItem("Phase",PHASE);
	ui.displayBox->addItem("No Display",NODISPLAY);
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
	spectrumMode=SPECTRUM;

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

    modeOffset = 0;

	//Size arrays to be able to handle full scree, whatever the resolution
	quint32 screenWidth = global->primaryScreen->availableSize().width();
	fftMap = new qint32[screenWidth];
	topPanelFftMap = new qint32[screenWidth];
	lineBuf = new QPoint[screenWidth];

	//We need to resize objects as frames change sizes
	connect(ui.splitter,SIGNAL(splitterMoved(int,int)),this,SLOT(splitterMoved(int,int)));

	connect(ui.hiResButton,SIGNAL(clicked(bool)),this,SLOT(hiResClicked(bool)));
	topPanelHighResolution = false;
	isRunning = false;
}

SpectrumWidget::~SpectrumWidget()
{
	if (lastSpectrum != NULL) free (lastSpectrum);
	if (averageSpectrum != NULL) free (averageSpectrum);
	delete[] fftMap;
	delete[] topPanelFftMap;
	delete[] lineBuf;
}
void SpectrumWidget::Run(bool r)
{
    //Global is not initialized in constructor
    ui.displayBox->setFont(global->settings->medFont);
    ui.maxDbBox->setFont(global->settings->medFont);

    QRect plotFr = ui.plotFrame->geometry(); //relative to parent

    //plotArea has to be created after layout has done it's work, not in constructor
    plotArea = QPixmap(plotFr.width(),plotFr.height());
    plotArea.fill(Qt::black);
    plotOverlay = QPixmap(plotFr.width(),plotFr.height());
    plotOverlay.fill(Qt::black);
    QRect plotLabelFr = ui.labelFrame->geometry();
    plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
    plotLabel.fill(Qt::black);

	QRect topPlotFr = ui.topPlotFrame->geometry();
	topPanelPlotArea = QPixmap(topPlotFr.width(),topPlotFr.height());
	topPanelPlotArea.fill(Qt::black);
	topPanelPlotOverlay = QPixmap(topPlotFr.width(), topPlotFr.height());
	QRect zoomPlotLabelFr = ui.topLabelFrame->geometry();
	topPanelPlotLabel = QPixmap(zoomPlotLabelFr.width(),zoomPlotLabelFr.height());
	topPanelPlotLabel.fill(Qt::black);

	if (r) {
		spectrumMode = (DISPLAYMODE)global->sdr->Get(DeviceInterface::LastSpectrumMode).toInt();
		//Triggers connection slot to set current mode
		ui.displayBox->setCurrentIndex(ui.displayBox->findData(spectrumMode)); //Initial display mode
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

//Resizes pixmaps to match size of plot frames
void SpectrumWidget::resizePixmaps(bool scale)
{
	QRect plotFr = ui.plotFrame->rect(); //We just need width and height
	QRect plotLabelFr = ui.labelFrame->rect();
	QRect topPlotFr = ui.topPlotFrame->rect();
	QRect topPlotLabelFr = ui.topLabelFrame->rect();

	//Scale the plot areas first, then copy to new plot area that is resized
	//Result should be that we see smooth resize without blanking, especially in waterfall
	if (!plotArea.isNull() && scale) {
		//Warnings if we scale a null pixmap
		plotArea = plotArea.scaled(plotFr.width(),plotFr.height());
	} else {
		plotArea = QPixmap(plotFr.width(),plotFr.height());
		plotArea.fill(Qt::black);
	}

	if (!plotOverlay.isNull() && scale) {
		plotOverlay = plotOverlay.scaled(plotFr.width(),plotFr.height());
	} else {
		plotOverlay = QPixmap(plotFr.width(),plotFr.height());
		plotOverlay.fill(Qt::black);
	}

    plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
    plotLabel.fill(Qt::black);

	if (!topPanelPlotArea.isNull() && scale) {
		topPanelPlotArea = topPanelPlotArea.scaled(topPlotFr.width(),topPlotFr.height());
	} else {
		topPanelPlotArea = QPixmap(topPlotFr.width(),topPlotFr.height());
		topPanelPlotArea.fill(Qt::black);
	}

	if (!topPanelPlotOverlay.isNull() && scale) {
		topPanelPlotOverlay = topPanelPlotOverlay.scaled(topPlotFr.width(), topPlotFr.height());
	} else {
		topPanelPlotOverlay = QPixmap(topPlotFr.width(), topPlotFr.height());
		topPanelPlotOverlay.fill(Qt::black);
	}

	topPanelPlotLabel = QPixmap(topPlotLabelFr.width(),topPlotLabelFr.height());
	topPanelPlotLabel.fill(Qt::black);
}

void SpectrumWidget::resizeEvent(QResizeEvent *event)
{
	resizePixmaps(true); //Scale instead of blacking screen
	drawOverlay();
	event->accept(); //We handle
}

//Returns +/- freq from LO based on where mouse cursor is
//Called from paint & mouse click
double SpectrumWidget::getMouseFreq()
{
	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	plotFr.adjust(-1,0,+1,0);

	//QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect topPlotFr = mapFrameToWidget(ui.topPlotFrame);
	topPlotFr.adjust(-1,0,+1,0);

	//QRect zoomPlotLabelFr = mapFrameToWidget(ui.zoomLabelFrame);

    QPoint mp = QCursor::pos(); //Current mouse position in global coordinates
    mp = mapFromGlobal(mp); //Convert to widget relative

	//qDebug()<<mp<<" "<<plotFr<<" "<<topPlotFr;

	if (plotFr.contains(mp)) {
        //Find freq at cursor
		float hzPerPixel = sampleRate / plotFr.width();
        //Convert to +/- relative to center
		qint32 m = mp.x() - plotFr.center().x();
        //And conver to freq
        m *= hzPerPixel;
        return m;

	} else if (topPlotFr.contains(mp)) {
        //Find freq at cursor
        float hzPerPixel;
		if (!topPanelHighResolution)
			hzPerPixel= sampleRate / topPlotFr.width() * zoom;
        else
			hzPerPixel= signalSpectrum->getHiResSampleRate() / topPlotFr.width() * zoom;

        //Convert to +/- relative to center
		qint32 m = mp.x() - topPlotFr.center().x();
        //And conver to freq
        m *= hzPerPixel;
        //and factor in mixer
        m += fMixer;
        return m;

    } else {
		//Not in our area, return current frequency
		return loFreq * fMixer;
    }

}

int SpectrumWidget::getMouseDb()
{
	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	plotFr.adjust(-1,0,+1,0);

	//QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect zoomPlotFr = mapFrameToWidget(ui.topPlotFrame);
	zoomPlotFr.adjust(-1,0,+1,0);

	//QRect zoomPlotLabelFr = mapFrameToWidget(ui.zoomLabelFrame);

    QPoint mp = QCursor::pos(); //Current mouse position in global coordinates
	mp = mapFromGlobal(mp); //Convert to widget relative, not frame relative

	if (plotFr.contains(mp)) {
		//Make cursor pos relative to plotFrame so 0,0 is top left frame (not widget)
		mp -= plotFr.topLeft();

		int db = (plotFr.height() - mp.y()); //Num pixels
		double scale = (double)(plotMaxDb - plotMinDb)/plotFr.height();
		db *= scale; //Scale to get db
		//Convert back to minDb relative
		db += plotMinDb;
		return db;
	} else if (zoomPlotFr.contains(mp)) {
		mp -= zoomPlotFr.topLeft();

		int db = (zoomPlotFr.height() - mp.y()); //Num pixels
		double scale = (double)(plotMaxDb - plotMinDb)/zoomPlotFr.height();
		db *= scale; //Scale to get db
		//Convert back to minDb relative
		db += plotMinDb;
		return db;

	} else {
		return plotMinDb;
	}
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

	if (spectrumMode == NODISPLAY)
		return;

	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	plotFr.adjust(-1,0,+1,0);

	//QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect topPlotFr = mapFrameToWidget(ui.topPlotFrame);
	topPlotFr.adjust(-1,0,+1,0);

	if (!plotFr.contains(event->pos()) && !topPlotFr.contains(event->pos()))
		return;

    QPoint angleDelta = event->angleDelta();
	qint32 freq = fMixer;

    if (angleDelta.ry() == 0) {
        if (angleDelta.rx() > 0) {
            //Scroll Right
            freq+= 100;
			emit spectrumMixerChanged(freq);
        } else if (angleDelta.rx() < 0) {
            //Scroll Left
            freq-= 100;
			emit spectrumMixerChanged(freq);

        }
    } else if (angleDelta.rx() == 0) {
        if (angleDelta.ry() > 0) {
            //Scroll Down
            freq-= 1000;
			emit spectrumMixerChanged(freq);

        } else if (angleDelta.ry() < 0) {
            //Scroll Up
            freq+= 1000;
			emit spectrumMixerChanged(freq);
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
	qint32 m = fMixer;
	switch (key)
	{
	case Qt::Key_Up: //Bigger step
		m += upDownIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Right:
		m += leftRightIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Down:
		m -= upDownIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Left:
		m -= leftRightIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
    case Qt::Key_Enter:
		emit spectrumMixerChanged(m, true); //Change LO to current mixer freq
		event->accept();
		return;
	}
	event->ignore();
}
void SpectrumWidget::mousePressEvent ( QMouseEvent * event ) 
{
	if (!isRunning || signalSpectrum == NULL)
		return;
	if (spectrumMode == NODISPLAY)
		return;

    Qt::MouseButton button = event->button();
    // Mac: Qt::ControlModifier == Command Key
    // Mac: Qt::AltModifer == Option(Alt) Key
    // Mac: Qt::MetaModifier == Control Key

    Qt::KeyboardModifiers modifiers = event->modifiers(); //Keyboard modifiers

	double deltaFreq = getMouseFreq();
	if (deltaFreq < -sampleRate / 2 || deltaFreq > sampleRate/2) {
		//Invalid range
		event->accept();
		return;
	}
    if (button == Qt::LeftButton) {
        if( modifiers == Qt::NoModifier)
			emit spectrumMixerChanged(deltaFreq, false); //Mixer mode
        else if (modifiers == Qt::AltModifier)
			emit spectrumMixerChanged(deltaFreq, true); //Mac Option same as Right Click

    } else if (button == Qt::RightButton) {
        if (modifiers == Qt::NoModifier)
			emit spectrumMixerChanged(deltaFreq, true); //LO mode
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
	drawOverlay();
}
//Track bandpass so we can display with cursor
void SpectrumWidget::SetFilter(int lo, int hi)
{
	loFilter = lo;
	hiFilter = hi;
	drawOverlay();
}

//NOTE: This gets called when UI selection changes, as well as when we need to change modes from code
void SpectrumWidget::plotSelectionChanged(DISPLAYMODE _mode)
{
	DISPLAYMODE oldMode = spectrumMode; //Save
	spectrumMode = _mode;
#if 0
	if (signalSpectrum != NULL) {
		signalSpectrum->SetDisplayMode(spectrumMode, topPanelHighResolution);
	}
#endif

	if (oldMode == NODISPLAY) {
		//Changing from no display to a display
		ui.labelFrame->setVisible(true);
		ui.plotFrame->setVisible(true);
		//Zoom was turned off when display was set to off
		ui.topLabelFrame->setVisible(false);
		ui.topPlotFrame->setVisible(false);
		ui.topFrameSplitter->setVisible(false);

		ui.maxDbBox->setVisible(true);
		ui.zoomSlider->setVisible(true);
		ui.zoomLabel->setVisible(true);
		ui.maxDbBox->setVisible(true);
		ui.minDbBox->setVisible(true);
		ui.updatesPerSec->setVisible(true);
	}

	switch (spectrumMode) {
		case NODISPLAY:
			//Changing from spectrum or WF to Off
			//Hide everything except the plot selector so we can switch back on
			//Turn zoom off
			zoom = 1;
			topPanelHighResolution = false;

			ui.zoomSlider->setValue(0);

			ui.labelFrame->setVisible(false);
			ui.plotFrame->setVisible(false);
			ui.topLabelFrame->setVisible(false);
			ui.topPlotFrame->setVisible(false);
			ui.topFrameSplitter->setVisible(false);

			ui.maxDbBox->setVisible(false);
			ui.zoomSlider->setVisible(false);
			ui.zoomLabel->setVisible(false);
			ui.maxDbBox->setVisible(false);
			ui.minDbBox->setVisible(false);
			ui.updatesPerSec->setVisible(false);
			ui.hiResButton->setVisible(false);

			update(); //Schedule a paint in the event loop.  repaint() triggers immediate
			break;

		case SPECTRUM:
			//switching between Waterfall, Spectrum, etc clear the old display
			//UpdateTopPanel first to turn on/off top panel modes
			updateTopPanel(NODISPLAY,false);
			break;
		case WATERFALL:
			//switching between Waterfall, Spectrum, etc clear the old display
			updateTopPanel(NODISPLAY,false);
			break;
		case SPECTRUM_WATERFALL:
			//switching between Waterfall, Spectrum, etc clear the old display
			updateTopPanel(SPECTRUM,false);
			break;
		case SPECTRUM_SPECTRUM:
			//switching between Waterfall, Spectrum, etc clear the old display
			updateTopPanel(SPECTRUM,false);
			break;
		case WATERFALL_WATERFALL:
			//switching between Waterfall, Spectrum, etc clear the old display
			updateTopPanel(WATERFALL,false);
			break;
		default:
			qDebug()<<"Invalid display mode";
			break;
	}

	drawOverlay();
	update(); //Schedule a paint in the event loop.  repaint() triggers immediate
}

void SpectrumWidget::updatesPerSecChanged(int item)
{
	if (signalSpectrum == NULL)
		return;
	signalSpectrum->setUpdatesPerSec(ui.updatesPerSec->value());
}

void SpectrumWidget::splitterMoved(int x, int y)
{
	resizePixmaps(true); //Scale so display isnt' blanked
	drawOverlay();
}

void SpectrumWidget::hiResClicked(bool _b)
{
	signalSpectrum->setHiRes(_b); //Tell spectrum to start collecting hires data
	topPanelHighResolution = _b;
	ui.zoomSlider->blockSignals(true);
	if (_b) {
		ui.zoomSlider->setValue(10);
		zoom = calcZoom(10); //Set new low zoom rate
	} else {
		ui.zoomSlider->setValue(0);
		zoom = calcZoom(0); //Set new low zoom rate
	}
	ui.zoomSlider->blockSignals(false);
	drawOverlay();
	update(); //Redraw scale
}

void SpectrumWidget::SetSignalSpectrum(SignalSpectrum *s) 
{
	signalSpectrum = s;	
	if (s!=NULL) {
        connect(signalSpectrum,SIGNAL(newFftData()),this,SLOT(newFftData()));

		sampleRate = s->getSampleRate();
		upDownIncrement = global->settings->upDownIncrement;
		leftRightIncrement = global->settings->leftRightIncrement;
	}
}
// Diplays frequency cursor and filter range
//Now called from DrawOverlay
void SpectrumWidget::drawCursor(QPainter *painter, QRect plotFr, bool isZoomed, QColor color)
{
    if (!isRunning)
        return;

    int plotWidth = plotFr.width();
    int plotHeight = plotFr.height();

    int hzPerPixel = sampleRate / plotWidth;
    if (isZoomed) {
		if (!topPanelHighResolution)
            hzPerPixel = sampleRate * zoom / plotWidth;
        else
			hzPerPixel = signalSpectrum->getHiResSampleRate() * zoom / plotWidth;
    }

    //Show mixer cursor, fMixer varies from -f..0..+f relative to LO
    //Map to coordinates
	qint32 x1=0;
	if (spectrumMode != NODISPLAY) {
        if (isZoomed) {
            x1 = 0.5 * plotWidth; //Zoom is always centered
        } else {
            x1 = fMixer / hzPerPixel;  //Convert fMixer to pixels
            x1 = x1 + plotFr.center().x(); //Make relative to center
        }
	}
#if 0
	else {
        x1 = 0.5 * plotWidth; //Cursor doesn't move in other modes, keep centered
    }
#endif


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

//Returns QRect in coordinates relative to widget
//Used to paint spectrumWidget frames
QRect SpectrumWidget::mapFrameToWidget(QFrame *frame)
{
	//0,0 origin, translate to widget relative coordinates
	QRect mapped = frame->rect(); //May need to try Geometry() or FrameGeometry() if borders not correct
	//Map 0,0 in frame relative to 0,0 in SpectrumWidget (this)
	mapped.translate(frame->mapTo(this,QPoint(0,0)));
	//Hack to move left and right edges out 1px for 1px frame border
	//Otherwise we still see left plot border
	//mapped.adjust(-1,0,+1,0);
	return mapped;
}

//Called by paint event to display spectrum in top or bottom pane
void SpectrumWidget::paintSpectrum(bool paintTopPanel, QPainter *painter)
{
	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect topPanelFr = mapFrameToWidget(ui.topPlotFrame);
	QRect topPanelLabelFr = mapFrameToWidget(ui.topLabelFrame);

	QPoint cursorPos = QWidget::mapFromGlobal(QCursor::pos());
	cursorPos.setX(cursorPos.x() + 4);

	//Cursor tracking of freq and db
	QString freqLabel;
	QString dbLabel;
	mouseFreq = getMouseFreq() + loFreq;
	int mouseDb = getMouseDb();
	if (mouseFreq > 0)
		freqLabel.sprintf("%.3f kHz",mouseFreq / 1000.0);
	else
		freqLabel = "";
	dbLabel.sprintf("%d db",mouseDb);

	painter->setFont(global->settings->medFont);
	//How many pixels do we need to display label with specified font
	QFontMetrics metrics(global->settings->medFont);
	QRect rect = metrics.boundingRect(freqLabel);
	if (cursorPos.x() + rect.width() > plotFr.width())
		cursorPos.setX(cursorPos.x() - rect.width()); //left of cursor

	//We don't want to paint the left and right pixmap borders, just the spectrum data
	//So we expand plotFr so the left border is off screen (-1) same with right border (+1)
	if (paintTopPanel) {
		topPanelFr.adjust(-1,0,+1,0);
		painter->drawPixmap(topPanelFr, topPanelPlotArea); //Includes plotOverlay which was copied to plotArea
		painter->drawPixmap(topPanelLabelFr,topPanelPlotLabel);
	} else {
		plotFr.adjust(-1,0,+1,0);
		painter->drawPixmap(plotFr, plotArea); //Includes plotOverlay which was copied to plotArea
		painter->drawPixmap(plotLabelFr,plotLabel);
	}
	//Draw cursor label on top of pixmap, but not control frame
	if (plotFr.contains(cursorPos) || topPanelFr.contains(cursorPos)) {
		painter->setPen(Qt::white);
		painter->drawText(cursorPos,freqLabel);
		//db label below freq
		cursorPos.setY(cursorPos.y() + metrics.height());
		painter->drawText(cursorPos,dbLabel);
	}
	//Draw overload indicator
	if (!paintTopPanel && signalSpectrum->getOverload()) {
		cursorPos.setX(plotFr.width() - 50);
		cursorPos.setY(10);
		painter->setPen(Qt::red);
		painter->drawText(cursorPos,"Overload");
	}
}

void SpectrumWidget::paintEvent(QPaintEvent *e)
{

    QPainter painter(this);
	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect topPanelFr = mapFrameToWidget(ui.topPlotFrame);
	QRect topPanelLabelFr = mapFrameToWidget(ui.topLabelFrame);

	QPoint cursorPos = QWidget::mapFromGlobal(QCursor::pos());
	cursorPos.setX(cursorPos.x() + 4);

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

	if (spectrumMode == NODISPLAY || signalSpectrum == NULL)
        return;


    //Cursor tracking of freq and db
	QString freqLabel;
	QString dbLabel;
	mouseFreq = getMouseFreq() + loFreq;
	int mouseDb = getMouseDb();
    if (mouseFreq > 0)
		freqLabel.sprintf("%.3f kHz",mouseFreq / 1000.0);
	else
		freqLabel = "";
	dbLabel.sprintf("%d db",mouseDb);

	painter.setFont(global->settings->medFont);
	//How many pixels do we need to display label with specified font
	QFontMetrics metrics(global->settings->medFont);
	QRect rect = metrics.boundingRect(freqLabel);
	if (cursorPos.x() + rect.width() > plotFr.width())
		cursorPos.setX(cursorPos.x() - rect.width()); //left of cursor

	switch (spectrumMode) {
		case SPECTRUM:
			paintSpectrum(false,&painter); //Main frame
			break;
		case SPECTRUM_SPECTRUM:
			paintSpectrum(true,&painter); //Top frame
			paintSpectrum(false,&painter); //Main frame
			break;
		case SPECTRUM_WATERFALL:
			paintSpectrum(true,&painter); //Top frame

			painter.drawPixmap(plotFr, plotArea);
			painter.drawPixmap(plotLabelFr,plotLabel);

			if (plotFr.contains(cursorPos)) {
				painter.setPen(Qt::black);
				painter.drawText(cursorPos,freqLabel);
				//No db label for waterfall
				//cursorPos.setY(cursorPos.y() + metrics.height());
				//painter.drawText(cursorPos,dbLabel);
			}
			break;

		case WATERFALL:
			painter.drawPixmap(plotFr, plotArea);
			painter.drawPixmap(plotLabelFr,plotLabel);

			if (plotFr.contains(cursorPos) || topPanelFr.contains(cursorPos)) {
				painter.setPen(Qt::black);
				painter.drawText(cursorPos,freqLabel);
				//No db label for waterfall
				//cursorPos.setY(cursorPos.y() + metrics.height());
				//painter.drawText(cursorPos,dbLabel);
			}
			break;
		case WATERFALL_WATERFALL:
			painter.drawPixmap(plotFr, plotArea);
			painter.drawPixmap(plotLabelFr,plotLabel);

			painter.drawPixmap(topPanelFr, topPanelPlotArea); //Includes plotOverlay which was copied to plotArea
			painter.drawPixmap(topPanelLabelFr,topPanelPlotLabel);

			if (plotFr.contains(cursorPos) || topPanelFr.contains(cursorPos)) {
				painter.setPen(Qt::black);
				painter.drawText(cursorPos,freqLabel);
				//No db label for waterfall
				//cursorPos.setY(cursorPos.y() + metrics.height());
				//painter.drawText(cursorPos,dbLabel);
			}
			break;
		default:
			painter.eraseRect(plotFr);
			break;
	}

    //Tell spectrum generator it ok to give us another one
    signalSpectrum->m_displayUpdateComplete = true;
}

void SpectrumWidget::displayChanged(int s)
{
    //Get mode from itemData
	DISPLAYMODE displayMode = (DISPLAYMODE)ui.displayBox->itemData(s).toInt();
	//Save in device ini
	global->sdr->Set(DeviceInterface::LastSpectrumMode,displayMode);
    plotSelectionChanged(displayMode);
}

void SpectrumWidget::maxDbChanged(int s)
{
    plotMaxDb = ui.maxDbBox->value();
	global->settings->fullScaleDb = plotMaxDb;
	global->settings->WriteSettings(); //save
	drawOverlay();
	update();
}

void SpectrumWidget::minDbChanged(int t)
{
	plotMinDb = ui.minDbBox->value();
	global->settings->baseScaleDb = plotMinDb;
	global->settings->WriteSettings(); //save
	drawOverlay();
	update();

}

double SpectrumWidget::calcZoom(int item)
{
	//Zoom is a geometric progression so it feels smooth
	//Item goes from 10 to 200 (or anything), so zoom goes from 10/10(1) to 10/100(0.1) to 10/200(0.5)
	//Eventually we should calculate zoom factors using sample rate and bin size to make sure we can't
	//over-zoom
	if (!topPanelHighResolution)
		//Highest zoom factor is with item==400, .0625
		return 1.0 / pow(2.0,item/100.0);
	else
		//Since SR is lower (48k or 64k typically) max zoom at 400 is only .15749
		return 1.0 / pow(2.0,item/150.0);
}

void SpectrumWidget::zoomChanged(int item)
{
	zoom = calcZoom(item);

	if (spectrumMode == SPECTRUM  && item > 0)
		//Top panel is off, Turn on as if user clicked on selection
		ui.displayBox->setCurrentIndex(ui.displayBox->findData(SPECTRUM_SPECTRUM));
	else if (spectrumMode == WATERFALL && item > 0)
		ui.displayBox->setCurrentIndex(ui.displayBox->findData(WATERFALL_WATERFALL));

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

    //In zoom mode, hi/low mixer boundaries need to be updated so we always keep frequency on screen
    //emit mixerLimitsChanged(range, - range);

	//Update display with new labels
	drawOverlay();
    update();
}

void SpectrumWidget::drawSpectrum(QPixmap &_pixMap, QPixmap &_pixOverlayMap, qint32 *_fftMap)
{
	int plotWidth = _pixMap.width();
	int plotHeight = _pixMap.height();

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
	_pixMap = _pixOverlayMap.copy(0,0,plotWidth,plotHeight);
	//Painter has to be created AFTER we copy overlay since plotArea is being replaced each time
	QPainter plotPainter(&_pixMap);

	//Define a line or a polygon (if filled) that represents the spectrum power levels
	quint16 numPoints = 0;
	quint16 lastFftMap;
	//1px left, right and bottom borders will not be painted so we don't see lines
	//Spectrum data starts and x=1 and ends at plotWidth
	//Start at lower left corner (upper left is (0,0) in Qt coordinate system
	lineBuf[numPoints].setX(0);
	lineBuf[numPoints].setY(_fftMap[0]);
	numPoints++;
	//Plot data
	for (int i=0; i< plotWidth; i++)
	{
		lineBuf[numPoints].setX(i+1); //Adjust for hidden border
		lineBuf[numPoints].setY(_fftMap[i]);
		lastFftMap = _fftMap[i];  //Keep for right border in polygon
		numPoints++;
		//Keep track of peak values for optional display
	}
	//MiterJoin gives us sharper peaks
	//Set style to Qt::NoPen for no borders
	QPen pen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
	plotPainter.setPen(pen);
	//Add points to complete the polygon
	//Top Right (border)
	lineBuf[numPoints].setX(plotWidth);
	lineBuf[numPoints].setY(lastFftMap);
	numPoints++;
	//Bottom Right
	lineBuf[numPoints].setX(plotWidth);
	lineBuf[numPoints].setY(plotHeight);

	numPoints++;
	//Bottom Left
	lineBuf[numPoints].setX(0);
	lineBuf[numPoints].setY(plotHeight);

	numPoints++;
	//Top fftMap[0]
	lineBuf[numPoints].setX(0);
	lineBuf[numPoints].setY(_fftMap[0]);

#if 0
	//Just draw the spectrum line, no fill
	plotPainter.drawPolyline(LineBuf, numPoints);
#else
	//Draw the filled polygon.  Note this may take slightly more CPUs
	QBrush tmpBrush = QBrush(gradient);
	plotPainter.setBrush(tmpBrush);
	plotPainter.drawPolygon(lineBuf,numPoints);

#endif
}

void SpectrumWidget::drawWaterfall(QPixmap &_pixMap, QPixmap &_pixOverlayMap, qint32 *_fftMap)
{
	quint16 wfPixPerLine = 1; //Testing vertical zoom
	//Color code each bin and draw line
	QPainter plotPainter(&_pixMap);

	QColor plotColor;

	//Waterfall
	//Scroll fr rect 1 pixel to create new line
	QRect rect = _pixMap.rect();
	_pixMap.scroll(0,wfPixPerLine,rect);

	for (int i=0; i<_pixMap.width(); i++) {
		for (int j=0; j<wfPixPerLine; j++) {
			plotColor = spectrumColors[255 - _fftMap[i]];
			plotPainter.setPen(plotColor);
			plotPainter.drawPoint(i,j);
		}

	}

#if 0
	//Testng vertical spacing in zoom mode
	quint16 wfZoomPixPerLine = 1;
	if (twoPanel) {
		//Waterfall
		//Scroll fr rect 1 pixel to create new line
		QRect rect = topPanelPlotArea.rect();
		//rect.setBottom(rect.bottom() - 10); //Reserve the last 10 pix for labels
		topPanelPlotArea.scroll(0,wfZoomPixPerLine,rect);

		if (topPanelDisplayMode == SPECTRUM) {
			signalSpectrum->MapFFTToScreen(
				255, //Equates to spectrumColor array
				topPanelPlotArea.width(),
				//These are same as testbench
				plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb, //FFT dB level corresponding to output value == 0
				zoomStartFreq, //Low frequency
				zoomEndFreq, //High frequency
				fftMap );

		} else {
			//Instead of plot area coordinates we convert to screen color array
			//Width is unchanged, but height is # colors we have for each db
			signalSpectrum->MapFFTZoomedToScreen(
				255, //Equates to spectrumColor array
				topPanelPlotArea.width(),
				//These are same as testbench
				plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb, //FFT dB level corresponding to output value == 0
				zoom,
				modeOffset,
				fftMap );
		}
		for (int i=0; i<plotArea.width(); i++) {
			for (int j=0; j<wfZoomPixPerLine; j++) {
				plotColor = spectrumColors[255 - fftMap[i]];
				zoomPlotPainter.setPen(plotColor);
				zoomPlotPainter.drawPoint(i,j);
			}
		}

	}
#endif

	update();
}

//New Fft data is ready for display, update screen if last update is finished
void SpectrumWidget::newFftData()
{
    if (!isRunning)
        return;

	//Safety check to make sure pixmaps are always sized correctly
	if (topPanelPlotArea.size() != ui.topPlotFrame->size() ||
			plotArea.size() != ui.plotFrame->size()) {
		//qDebug()<<"Resize pixmap";
		resizePixmaps(false); //Don't scale, we may have switched modes
		drawOverlay();
	}

    double startFreq =  - (sampleRate/2); //Relative to 0
    double endFreq = sampleRate/2;
	//Top panel is always centered on fMixer
    //Start by assuming we have full +/- samplerate and make fMixer zero centered
	double topPanelStartFreq =  startFreq * zoom;
	double topPanelEndFreq = endFreq * zoom ;
	topPanelStartFreq += fMixer;
	topPanelEndFreq += fMixer;

	switch (spectrumMode) {
		case NODISPLAY:
			//Do nothing
			break;

		case SPECTRUM:
			//Convert to plot area coordinates
			//This gets passed straight through to FFT MapFFTToScreen
			signalSpectrum->mapFFTToScreen(
				plotArea.height(),
				plotArea.width(),
				//These are same as testbench
				plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb,   //FFT dB level corresponding to output value == 0
				startFreq, //-sampleRate/2, //Low frequency
				endFreq, //sampleRate/2, //High frequency
				fftMap );
			drawSpectrum(plotArea, plotOverlay, fftMap);
			//We can only display offscreen pixmap in paint() event, so call it to update
			update();
			break;
		case SPECTRUM_SPECTRUM:
			//Draw top
			if (!topPanelHighResolution) {
				//Convert to plot area coordinates
				//This gets passed straight through to FFT MapFFTToScreen
				signalSpectrum->mapFFTToScreen(
					topPanelPlotArea.height(),
					topPanelPlotArea.width(),
					//These are same as testbench
					plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					plotMinDb,   //FFT dB level corresponding to output value == 0
					topPanelStartFreq, //-sampleRate/2, //Low frequency
					topPanelEndFreq, //sampleRate/2, //High frequency
					topPanelFftMap );
			} else {
				signalSpectrum->mapFFTZoomedToScreen(
					topPanelPlotArea.height(),
					topPanelPlotArea.width(),
					//These are same as testbench
					plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					plotMinDb,   //FFT dB level corresponding to output value == 0
					zoom,
					modeOffset,
					topPanelFftMap );
			}
			drawSpectrum(topPanelPlotArea, topPanelPlotOverlay, topPanelFftMap);

			//Draw bottom spectrum
			signalSpectrum->mapFFTToScreen(
				plotArea.height(),
				plotArea.width(),
				//These are same as testbench
				plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb,   //FFT dB level corresponding to output value == 0
				startFreq, //-sampleRate/2, //Low frequency
				endFreq, //sampleRate/2, //High frequency
				fftMap );
			drawSpectrum(plotArea, plotOverlay, fftMap);
			//We can only display offscreen pixmap in paint() event, so call it to update
			update();
			break;
		case SPECTRUM_WATERFALL:
			//Upper Spectrum
			if (!topPanelHighResolution) {
				signalSpectrum->mapFFTToScreen(
					topPanelPlotArea.height(),
					topPanelPlotArea.width(),
					//These are same as testbench
					plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					plotMinDb,   //FFT dB level corresponding to output value == 0
					topPanelStartFreq, //-sampleRate/2, //Low frequency
					topPanelEndFreq, //sampleRate/2, //High frequency
					topPanelFftMap );
			} else {
				signalSpectrum->mapFFTZoomedToScreen(
					topPanelPlotArea.height(),
					topPanelPlotArea.width(),
					//These are same as testbench
					plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					plotMinDb,   //FFT dB level corresponding to output value == 0
					zoom,
					modeOffset,
					topPanelFftMap );
			}
			drawSpectrum(topPanelPlotArea, topPanelPlotOverlay, topPanelFftMap);

			//Lower waterfall
			signalSpectrum->mapFFTToScreen(
				255, //Equates to spectrumColor array
				plotArea.width(),
				//These are same as testbench
				plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb, //FFT dB level corresponding to output value == 0
				startFreq, //Low frequency
				endFreq, //High frequency
				fftMap );

			drawWaterfall(plotArea, plotOverlay, fftMap);
			update();
			break;
		case WATERFALL_WATERFALL:
			//Top waterfall
			if (!topPanelHighResolution) {
				signalSpectrum->mapFFTToScreen(
					255, //Equates to spectrumColor array
					topPanelPlotArea.width(),
					//These are same as testbench
					plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
					plotMinDb, //FFT dB level corresponding to output value == 0
					topPanelStartFreq, //Low frequency
					topPanelEndFreq, //High frequency
					topPanelFftMap );
			} else {
				signalSpectrum->mapFFTZoomedToScreen(
					255,
					topPanelPlotArea.width(),
					//These are same as testbench
					plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					plotMinDb,   //FFT dB level corresponding to output value == 0
					zoom,
					modeOffset,
					topPanelFftMap );
			}
			drawWaterfall(topPanelPlotArea, topPanelPlotOverlay, topPanelFftMap);

			//Lower waterfall
			signalSpectrum->mapFFTToScreen(
				255, //Equates to spectrumColor array
				plotArea.width(),
				//These are same as testbench
				plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb, //FFT dB level corresponding to output value == 0
				startFreq, //Low frequency
				endFreq, //High frequency
				fftMap );
			drawWaterfall(plotArea,plotOverlay,fftMap);
			update();
			break;
		case WATERFALL: {
			//Instead of plot area coordinates we convert to screen color array
			//Width is unchanged, but height is # colors we have for each db
			signalSpectrum->mapFFTToScreen(
				255, //Equates to spectrumColor array
				plotArea.width(),
				//These are same as testbench
				plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				plotMinDb, //FFT dB level corresponding to output value == 0
				startFreq, //Low frequency
				endFreq, //High frequency
				fftMap );
			drawWaterfall(plotArea,plotOverlay,fftMap);
			update();
			break;
		}
		default:
			break;
	}
}

//Update top pane with new mode
void SpectrumWidget::updateTopPanel(DISPLAYMODE _newMode, bool updateSlider)
{
	//setValue will automatically signal value changed and call connected SLOT unless we block signals
	switch(_newMode) {
		case NODISPLAY:
			//Hide top pane and update sizes
			ui.topLabelFrame->setVisible(false);
			ui.topPlotFrame->setVisible(false);
			ui.topFrameSplitter->setVisible(false);
			zoom = 1;
			ui.zoomSlider->setValue(0);
			ui.hiResButton->setChecked(false);
			topPanelHighResolution = false;
			ui.hiResButton->setVisible(false);

			resizePixmaps(false);
			if (updateSlider)
				ui.zoomSlider->setValue(0);
			update();
			break;
		case SPECTRUM:
			//Show top pane and update sizes
			ui.topLabelFrame->setVisible(true);
			ui.topPlotFrame->setVisible(true);
			ui.topFrameSplitter->setVisible(true);

			ui.hiResButton->setChecked(false);
			topPanelHighResolution = false;
			ui.hiResButton->setVisible(true);

			resizePixmaps(false);
			if (updateSlider)
				ui.zoomSlider->setValue((ui.zoomSlider->maximum() - ui.zoomSlider->minimum()) / 4);
			update();
			break;
		case WATERFALL:
			ui.topLabelFrame->setVisible(true);
			ui.topPlotFrame->setVisible(true);
			ui.topFrameSplitter->setVisible(true);

			ui.hiResButton->setChecked(false);
			topPanelHighResolution = false;
			ui.hiResButton->setVisible(true);

			resizePixmaps(false);
			if (updateSlider)
				ui.zoomSlider->setValue((ui.zoomSlider->maximum() - ui.zoomSlider->minimum()) / 4);
			update();
			break;
		default:
			qDebug()<<"Invalid mode for top spectrum pane";
			break;
	}
}

//Returns a formatted string for use in scale
//If precision >=0, then override what we would normally display -1 to use default precision
QString SpectrumWidget::frequencyLabel(double f, qint16 precision)
{
	QString label;
	//Change precision and suffix based on frequency
	//Keep to 4 significant digts if possible
	// 9.999g 999.1m 99.99m 1.70k
	if (f < 0)
		label = "---";
	else if (f <= 1700000) //AM band in kHz
		label = QString::number(f/1000.0,'f',precision >= 0 ? precision : 2)+"k";
	else if (f <= 99999999)
		label = QString::number(f/1000000.0,'f',precision >= 0 ? precision : 2)+"m";
	else if (f < 1000000000)
		label = QString::number(f/1000000.0,'f',precision >= 0 ? precision : 1)+"m";
	else
		label = QString::number(f/1000000000.0,'f',precision >= 0 ? precision : 3)+"g";

	return label;
}

void SpectrumWidget::drawScale(QPainter *labelPainter, double centerFreq, bool drawTopFrame)
{
    if (!isRunning)
        return;

	int numXDivs;

    //Draw the frequency scale
    float pixPerHdiv;
    int x,y;
    QRect rect;
	int plotLabelHeight;

    //Show actual hi/lo frequency range
    QString horizLabels[maxHDivs];
    //zoom is fractional
    //100,000 sps * 1.00 / 10 = 10,000 hz per division
    //100,000 sps * 0.10 / 10 = 1000 hz per division
    //Bug with 16bit at 2msps rate
	quint32 hzPerhDiv;
	if (drawTopFrame) {
		numXDivs = (topPanelPlotLabel.width() / 100) * 2; //x2 to make sure always even number
		plotLabelHeight = topPanelPlotLabel.height();
		pixPerHdiv = (float)topPanelPlotLabel.width() / (float)numXDivs;
		if (!topPanelHighResolution) {
			hzPerhDiv = sampleRate * zoom / numXDivs;
		} else {
			//Smaller range
			hzPerhDiv = signalSpectrum->getHiResSampleRate() * zoom / numXDivs;
		}
		//qDebug()<<"Top "<<hzPerhDiv;
	} else {
		numXDivs = (plotLabel.width() / 100) * 2; //x2 to make sure always even number
		plotLabelHeight = plotLabel.height();
		pixPerHdiv = (float)plotLabel.width() / (float)numXDivs;
		hzPerhDiv = sampleRate / numXDivs;
		//qDebug()<<"Bottom "<<hzPerhDiv;
	}


    //horizDivs must be even number so middle is 0
    //So for 10 divs we start out with [0] at low and [9] at high
#if 1
	quint64 leftFreq = centerFreq - ((numXDivs / 2) * hzPerhDiv);
	quint64 rightFreq = centerFreq + ((numXDivs / 2) * hzPerhDiv);
	quint64 labelFreq;

	//Truncate far left and right labels with no decimal places
	horizLabels[0] = frequencyLabel(leftFreq,1);
	horizLabels[numXDivs] = frequencyLabel(rightFreq,1);
	for (int i = 1; i<numXDivs; i++) {
		labelFreq = leftFreq + (i * hzPerhDiv);
		horizLabels[i] = frequencyLabel(labelFreq);
	}
#else
	quint64 leftFreq,rightFreq;
	int center = numXDivs / 2;
	quint32 tick; //+/- freq from center
	horizLabels[center] = frequencyLabel(centerFreq);
	//All divs including far left and far right
	for (int i=0; i <= center; i++) {
        tick = i * hzPerhDiv;
		leftFreq = centerFreq - tick;
		horizLabels[center - i] = frequencyLabel(leftFreq);

		rightFreq = centerFreq + tick;
		horizLabels[center + i] = frequencyLabel(rightFreq);
    }
#endif

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

void SpectrumWidget::drawSpectrumOverlay(bool drawTopPanel)
{
	QPainter *painter = new QPainter();
	QPainter *labelPainter = new QPainter();

	int overlayWidth;
	int overlayHeight;
	QRect plotFr;
	quint16 dbLabelInterval;

	int numXDivs; //Must be even number to split right around center
	int numYDivs;

	if (!drawTopPanel) {
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
		if (topPanelPlotOverlay.isNull())
			return;

		topPanelPlotOverlay.fill(Qt::black); //Clear every time because we are update cursor and other info here also
		topPanelPlotLabel.fill(Qt::black);
		overlayWidth = topPanelPlotOverlay.width();
		overlayHeight = topPanelPlotOverlay.height();
		plotFr = ui.topPlotFrame->geometry(); //relative to parent
		painter->begin(&topPanelPlotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&topPanelPlotLabel);
	}

	QFont overlayFont("Arial");
	overlayFont.setPointSize(10);
	overlayFont.setWeight(QFont::Normal);
	painter->setFont(overlayFont);

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

	drawCursor(painter, plotFr, drawTopPanel, Qt::white);
	drawCursor(labelPainter, plotFr, drawTopPanel, Qt::white); //Continues into label area

	if (!drawTopPanel)
		drawScale(labelPainter, loFreq, false);
	else
		drawScale(labelPainter, loFreq + fMixer, true);

	painter->end();  //Because we use pointer, automatic with instance
	labelPainter->end();
	delete painter;
	delete labelPainter;
}

void SpectrumWidget::drawWaterfallOverlay(bool drawTopPanel)
{
	QPainter *painter = new QPainter();
	QPainter *labelPainter = new QPainter();
	QRect plotFr;

	if (!drawTopPanel) {
		plotFr = ui.plotFrame->geometry(); //relative to parent
		plotLabel.fill(Qt::black);
		painter->begin(&plotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&plotLabel);
	} else {
		plotFr = ui.topPlotFrame->geometry(); //relative to parent
		topPanelPlotLabel.fill(Qt::black);
		painter->begin(&topPanelPlotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&topPanelPlotLabel);
	}

	QFont overlayFont("Arial");
	overlayFont.setPointSize(10);
	overlayFont.setWeight(QFont::Normal);
	labelPainter->setFont(overlayFont);

#if 0
	//Draw spectrum colors centered at bottom of label pane
	for (int i=0; i<dbRange; i++) {
		labelPainter.setPen(spectrumColors[i]);
		labelPainter.drawPoint(overlayWidth/2 - 80 + i, plotLabelHeight);
		labelPainter.drawPoint(overlayWidth/2 - 80 + i, plotLabelHeight);
	}
#endif
	drawCursor(labelPainter,plotFr, false, Qt::white);

	if (!drawTopPanel)
		drawScale(labelPainter, loFreq, false);
	else
		drawScale(labelPainter, loFreq + fMixer, true);

	painter->end();  //Because we use pointer, automatic with instance
	labelPainter->end();
	delete painter;
	delete labelPainter;
}

void SpectrumWidget::drawOverlay()
{
    if (!isRunning)
        return;

    //!!! 1/11/14 This never seems to get called with isRunning == true

	//Update span label
	qint32 range = sampleRate;
	qint32 zoomRange = signalSpectrum->getHiResSampleRate();
	if (!topPanelHighResolution) {
		ui.zoomLabel->setText(QString().sprintf(" %.0f kHz",range/1000 * zoom));
	} else {
		//HiRes
		ui.zoomLabel->setText(QString().sprintf(" %.0f kHz",zoomRange/1000 * zoom));
	}

	switch(spectrumMode) {
		case SPECTRUM:
			drawSpectrumOverlay(false);
			break;
		case SPECTRUM_SPECTRUM:
			drawSpectrumOverlay(true);
			drawSpectrumOverlay(false);
			break;
		case SPECTRUM_WATERFALL:
			drawSpectrumOverlay(true);
			drawWaterfallOverlay(false);
			break;
		case WATERFALL_WATERFALL:
			drawWaterfallOverlay(true);
			drawWaterfallOverlay(false);
			break;
		case WATERFALL:
			drawWaterfallOverlay(false);
			break;
		case NODISPLAY:
			break;
	}

#if 0
    //If we're not running, display overlay as plotArea
    if (!isRunning) {
        plotArea = plotOverlay.copy(0,0,overlayWidth,overlayHeight);
        update();
    }
#endif
}
