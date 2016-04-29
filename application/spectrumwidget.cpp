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
	int index;

	ui.setupUi(this);

	ui.topLabelFrame->setVisible(false);
	ui.topPlotFrame->setVisible(false);
	ui.topFrameSplitter->setVisible(false);

	ui.displayBox->addItem("Spectrum",SPECTRUM);
	ui.displayBox->addItem("Waterfall",WATERFALL);
	ui.displayBox->addItem("Spect/Spect",SPECTRUM_SPECTRUM);
	ui.displayBox->addItem("Spect/Wfall",SPECTRUM_WATERFALL);
	ui.displayBox->addItem("Wfall/Wfall",WATERFALL_WATERFALL);
	//ui.displayBox->addItem("I/Q",IQ);
	//ui.displayBox->addItem("Phase",PHASE);
	ui.displayBox->addItem("No Display",NODISPLAY);
    ui.displayBox->setCurrentIndex(-1);

    connect(ui.displayBox,SIGNAL(currentIndexChanged(int)),this,SLOT(displayChanged(int)));

    //Starting plot range
	//CuteSDR defaults to -50
	m_plotMaxDb = global->settings->fullScaleDb;
	m_autoScaleMax = global->settings->autoScaleMax;
	ui.maxDbBox->addItem("Auto",10);
	ui.maxDbBox->addItem("  0db",0);
	ui.maxDbBox->addItem("-10db",-10);
	ui.maxDbBox->addItem("-20db",-20);
	ui.maxDbBox->addItem("-30db",-30);
	ui.maxDbBox->addItem("-40db",-40);
	ui.maxDbBox->addItem("-50db",-50);
	if (m_autoScaleMax)
		index = 0;
	else
		index = ui.maxDbBox->findData(m_plotMaxDb);
	ui.maxDbBox->setCurrentIndex(index);
	connect(ui.maxDbBox,SIGNAL(currentIndexChanged(int)),this,SLOT(maxDbChanged(int)));

	m_plotMinDb = global->settings->baseScaleDb;
	m_autoScaleMin = global->settings->autoScaleMin;
	ui.minDbBox->addItem("Auto",10);
	ui.minDbBox->addItem("-120db",-120);
	ui.minDbBox->addItem("-110db",-110);
	ui.minDbBox->addItem("-100db",-100);
	ui.minDbBox->addItem("- 90db",-90);
	ui.minDbBox->addItem("- 80db",-80);
	ui.minDbBox->addItem("- 70db",-70);
	if (m_autoScaleMin)
		index = 0;
	else
		index = ui.minDbBox->findData(m_plotMinDb);
	ui.minDbBox->setCurrentIndex(index);
	connect(ui.minDbBox,SIGNAL(currentIndexChanged(int)),this,SLOT(minDbChanged(int)));

	ui.updatesPerSec->addItem("5x",5);
	ui.updatesPerSec->addItem("10x",10);
	ui.updatesPerSec->addItem("15x",15);
	ui.updatesPerSec->addItem("20x",20);
	ui.updatesPerSec->addItem("25x",25);
	ui.updatesPerSec->addItem("30x",30);
	ui.updatesPerSec->addItem("Frz",0);
	index = ui.updatesPerSec->findData(global->settings->updatesPerSecond);
	ui.updatesPerSec->setCurrentIndex(index);
	connect(ui.updatesPerSec,SIGNAL(currentIndexChanged(int)),this,SLOT(updatesPerSecChanged(int)));

	m_spectrumMode=SPECTRUM;

	//message = NULL;
	m_signalSpectrum = NULL;
	m_averageSpectrum = NULL;
	m_lastSpectrum = NULL;

	m_fMixer = 0;

	m_dbRange = std::abs(DB::maxDb - DB::minDb);

    //Spectrum power is converted to 8bit int for display
    //This array maps the colors for each power level
    //Technique from CuteSDR to get a smooth color palette across the spectrum
#if 1
	m_spectrumColors = new QColor[255];

    for( int i=0; i<256; i++)
    {
        if( (i<43) )
			m_spectrumColors[i].setRgb( 0,0, 255*(i)/43);
        if( (i>=43) && (i<87) )
			m_spectrumColors[i].setRgb( 0, 255*(i-43)/43, 255 );
        if( (i>=87) && (i<120) )
			m_spectrumColors[i].setRgb( 0,255, 255-(255*(i-87)/32));
        if( (i>=120) && (i<154) )
			m_spectrumColors[i].setRgb( (255*(i-120)/33), 255, 0);
        if( (i>=154) && (i<217) )
			m_spectrumColors[i].setRgb( 255, 255 - (255*(i-154)/62), 0);
        if( (i>=217)  )
			m_spectrumColors[i].setRgb( 255, 0, 128*(i-217)/38);
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
	m_zoom = 1;

	m_modeOffset = 0;

	//Size arrays to be able to handle full scree, whatever the resolution
	quint32 screenWidth = global->primaryScreen->availableSize().width();
	m_fftMap = new qint32[screenWidth];
	m_topPanelFftMap = new qint32[screenWidth];
	m_lineBuf = new QPoint[screenWidth];

	//We need to resize objects as frames change sizes
	connect(ui.splitter,SIGNAL(splitterMoved(int,int)),this,SLOT(splitterMoved(int,int)));

	connect(ui.hiResButton,SIGNAL(clicked(bool)),this,SLOT(hiResClicked(bool)));
	m_topPanelHighResolution = false;
	m_isRunning = false;
}

SpectrumWidget::~SpectrumWidget()
{
	if (m_lastSpectrum != NULL) free (m_lastSpectrum);
	if (m_averageSpectrum != NULL) free (m_averageSpectrum);
	delete[] m_fftMap;
	delete[] m_topPanelFftMap;
	delete[] m_lineBuf;
}
void SpectrumWidget::run(bool r)
{
    //Global is not initialized in constructor
    ui.displayBox->setFont(global->settings->medFont);
    ui.maxDbBox->setFont(global->settings->medFont);

    QRect plotFr = ui.plotFrame->geometry(); //relative to parent

    //plotArea has to be created after layout has done it's work, not in constructor
	m_plotArea = QPixmap(plotFr.width(),plotFr.height());
	m_plotArea.fill(Qt::black);
	m_plotOverlay = QPixmap(plotFr.width(),plotFr.height());
	m_plotOverlay.fill(Qt::black);
    QRect plotLabelFr = ui.labelFrame->geometry();
	m_plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
	m_plotLabel.fill(Qt::black);

	QRect topPlotFr = ui.topPlotFrame->geometry();
	m_topPanelPlotArea = QPixmap(topPlotFr.width(),topPlotFr.height());
	m_topPanelPlotArea.fill(Qt::black);
	m_topPanelPlotOverlay = QPixmap(topPlotFr.width(), topPlotFr.height());
	QRect zoomPlotLabelFr = ui.topLabelFrame->geometry();
	m_topPanelPlotLabel = QPixmap(zoomPlotLabelFr.width(),zoomPlotLabelFr.height());
	m_topPanelPlotLabel.fill(Qt::black);

	if (r) {
		m_spectrumMode = (DisplayMode)global->sdr->Get(DeviceInterface::LastSpectrumMode).toInt();
		//Triggers connection slot to set current mode
		ui.displayBox->setCurrentIndex(ui.displayBox->findData(m_spectrumMode)); //Initial display mode
		ui.zoomLabel->setText(QString().sprintf("S: %.0f kHz",m_sampleRate/1000.0));
		m_isRunning = true;
		m_scaleNeedsRecalc = true;
	}
	else {
		ui.displayBox->blockSignals(true);
		ui.displayBox->setCurrentIndex(-1);
		ui.displayBox->blockSignals(false);
		ui.zoomLabel->setText(QString().sprintf(""));
		m_isRunning =false;
		m_signalSpectrum = NULL;
		m_plotArea.fill(Qt::black); //Start with a  clean plot every time
		m_plotOverlay.fill(Qt::black); //Start with a  clean plot every time
        update();
	}
}
void SpectrumWidget::setMessage(QStringList s)
{
	m_message = s;
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
	if (!m_plotArea.isNull() && scale) {
		//Warnings if we scale a null pixmap
		m_plotArea = m_plotArea.scaled(plotFr.width(),plotFr.height());
	} else {
		m_plotArea = QPixmap(plotFr.width(),plotFr.height());
		m_plotArea.fill(Qt::black);
	}

	if (!m_plotOverlay.isNull() && scale) {
		m_plotOverlay = m_plotOverlay.scaled(plotFr.width(),plotFr.height());
	} else {
		m_plotOverlay = QPixmap(plotFr.width(),plotFr.height());
		m_plotOverlay.fill(Qt::black);
	}

	m_plotLabel = QPixmap(plotLabelFr.width(),plotLabelFr.height());
	m_plotLabel.fill(Qt::black);

	if (!m_topPanelPlotArea.isNull() && scale) {
		m_topPanelPlotArea = m_topPanelPlotArea.scaled(topPlotFr.width(),topPlotFr.height());
	} else {
		m_topPanelPlotArea = QPixmap(topPlotFr.width(),topPlotFr.height());
		m_topPanelPlotArea.fill(Qt::black);
	}

	if (!m_topPanelPlotOverlay.isNull() && scale) {
		m_topPanelPlotOverlay = m_topPanelPlotOverlay.scaled(topPlotFr.width(), topPlotFr.height());
	} else {
		m_topPanelPlotOverlay = QPixmap(topPlotFr.width(), topPlotFr.height());
		m_topPanelPlotOverlay.fill(Qt::black);
	}

	m_topPanelPlotLabel = QPixmap(topPlotLabelFr.width(),topPlotLabelFr.height());
	m_topPanelPlotLabel.fill(Qt::black);
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
		float hzPerPixel = m_sampleRate / plotFr.width();
        //Convert to +/- relative to center
		qint32 m = mp.x() - plotFr.center().x();
        //And conver to freq
        m *= hzPerPixel;
        return m;

	} else if (topPlotFr.contains(mp)) {
        //Find freq at cursor
        float hzPerPixel;
		if (!m_topPanelHighResolution)
			hzPerPixel= m_sampleRate / topPlotFr.width() * m_zoom;
        else
			hzPerPixel= m_signalSpectrum->getHiResSampleRate() / topPlotFr.width() * m_zoom;

        //Convert to +/- relative to center
		qint32 m = mp.x() - topPlotFr.center().x();
        //And conver to freq
        m *= hzPerPixel;
        //and factor in mixer
		m += m_fMixer;
        return m;

    } else {
		//Not in our area, return current frequency
		return m_loFreq * m_fMixer;
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
		double scale = (double)(m_plotMaxDb - m_plotMinDb)/plotFr.height();
		db *= scale; //Scale to get db
		//Convert back to minDb relative
		db += m_plotMinDb;
		return db;
	} else if (zoomPlotFr.contains(mp)) {
		mp -= zoomPlotFr.topLeft();

		int db = (zoomPlotFr.height() - mp.y()); //Num pixels
		double scale = (double)(m_plotMaxDb - m_plotMinDb)/zoomPlotFr.height();
		db *= scale; //Scale to get db
		//Convert back to minDb relative
		db += m_plotMinDb;
		return db;

	} else {
		return m_plotMinDb;
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

	if (m_spectrumMode == NODISPLAY)
		return;

	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	plotFr.adjust(-1,0,+1,0);

	//QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect topPlotFr = mapFrameToWidget(ui.topPlotFrame);
	topPlotFr.adjust(-1,0,+1,0);

	if (!plotFr.contains(event->pos()) && !topPlotFr.contains(event->pos()))
		return;

    QPoint angleDelta = event->angleDelta();
	qint32 freq = m_fMixer;

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
	qint32 m = m_fMixer;
	switch (key)
	{
	case Qt::Key_Up: //Bigger step
		m += m_upDownIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Right:
		m += m_leftRightIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Down:
		m -= m_upDownIncrement;
		emit spectrumMixerChanged(m);
		event->accept();
		return;
	case Qt::Key_Left:
		m -= m_leftRightIncrement;
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
	if (!m_isRunning || m_signalSpectrum == NULL)
		return;
	if (m_spectrumMode == NODISPLAY)
		return;

    Qt::MouseButton button = event->button();
    // Mac: Qt::ControlModifier == Command Key
    // Mac: Qt::AltModifer == Option(Alt) Key
    // Mac: Qt::MetaModifier == Control Key

    Qt::KeyboardModifiers modifiers = event->modifiers(); //Keyboard modifiers

	double deltaFreq = getMouseFreq();
	if (deltaFreq < -m_sampleRate / 2 || deltaFreq > m_sampleRate/2) {
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
void SpectrumWidget::setMode(DeviceInterface::DEMODMODE m, int _modeOffset)
{
	m_demodMode = m;
	m_modeOffset = _modeOffset;
}
void SpectrumWidget::setMixer(int m, double f)
{
	if (m_loFreq != f) {
		m_loFreq = f;
		//Recalc auto scale whenever LO changes, but wait for next fft to have new data
		m_scaleNeedsRecalc = true;
	}
	m_fMixer = m;
	drawOverlay();

}
//Track bandpass so we can display with cursor
void SpectrumWidget::setFilter(int lo, int hi)
{
	m_loFilter = lo;
	m_hiFilter = hi;
	drawOverlay();
}

//NOTE: This gets called when UI selection changes, as well as when we need to change modes from code
void SpectrumWidget::plotSelectionChanged(DisplayMode _mode)
{
	DisplayMode oldMode = m_spectrumMode; //Save
	m_spectrumMode = _mode;
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

	switch (m_spectrumMode) {
		case NODISPLAY:
			//Changing from spectrum or WF to Off
			//Hide everything except the plot selector so we can switch back on
			//Turn zoom off
			m_zoom = 1;
			m_topPanelHighResolution = false;

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
	if (m_signalSpectrum == NULL)
		return;

	m_signalSpectrum->setUpdatesPerSec(ui.updatesPerSec->currentData().toInt());
}

void SpectrumWidget::splitterMoved(int x, int y)
{
	resizePixmaps(true); //Scale so display isnt' blanked
	drawOverlay();
}

void SpectrumWidget::hiResClicked(bool _b)
{
	m_signalSpectrum->setHiRes(_b); //Tell spectrum to start collecting hires data
	m_topPanelHighResolution = _b;
	ui.zoomSlider->blockSignals(true);
	if (_b) {
		ui.zoomSlider->setValue(10);
		m_zoom = calcZoom(10); //Set new low zoom rate
	} else {
		ui.zoomSlider->setValue(0);
		m_zoom = calcZoom(0); //Set new low zoom rate
	}
	ui.zoomSlider->blockSignals(false);
	drawOverlay();
	update(); //Redraw scale
}

//Update auto-scale
void SpectrumWidget::recalcScale()
{
	if (!m_autoScaleMax && !m_autoScaleMin)
		return;

	//Similar to SignalStrength, but works across entire spectrum, not just bandpass
	double *spectrum = m_signalSpectrum->getUnprocessed();
	double pwr = 0;
	double totalPwr = 0;
	double peakPwr = 0;
	double avgPwr;
	int binCount = m_signalSpectrum->binCount();
	for (int i=0; i<binCount; i++) {
		pwr = DB::dBToPower(spectrum[i]);
		totalPwr += pwr;
		if (pwr > peakPwr)
			peakPwr = pwr;
	}
	avgPwr = totalPwr / binCount;
	//If auto-scale is on, update
	//Only change in increments of 5 and only if changed
	//Don't change above/below ui limits
	int newAvgDb = DB::powerTodB(avgPwr) - 10;
	newAvgDb = (newAvgDb / 5) * 5;
	newAvgDb = qBound(-120, newAvgDb, m_minDbScaleLimit);
	if (m_autoScaleMin) {
		//ui.minDbBox->setValue(newAvgDb);
		m_plotMinDb = newAvgDb;
	}

	int newPeakDb = DB::powerTodB(peakPwr) + 10;
	newPeakDb = (newPeakDb / 5) * 5;
	newPeakDb = qBound(m_maxDbScaleLimit, newPeakDb, 0);
	if (m_autoScaleMax) {
		//ui.maxDbBox->setValue(newPeakDb);
		m_plotMaxDb = newPeakDb;
	}

	drawOverlay();
	update();

}

void SpectrumWidget::setSignalSpectrum(SignalSpectrum *s)
{
	m_signalSpectrum = s;
	if (s!=NULL) {
		connect(m_signalSpectrum,SIGNAL(newFftData()),this,SLOT(newFftData()));

		m_sampleRate = s->getSampleRate();
		m_upDownIncrement = global->settings->upDownIncrement;
		m_leftRightIncrement = global->settings->leftRightIncrement;
	}
}
// Diplays frequency cursor and filter range
//Now called from DrawOverlay
void SpectrumWidget::paintFreqCursor(QPainter *painter, QRect plotFr, bool isZoomed, QColor color)
{
	if (!m_isRunning)
        return;

    int plotWidth = plotFr.width();
    int plotHeight = plotFr.height();

	int hzPerPixel = m_sampleRate / plotWidth;
    if (isZoomed) {
		if (!m_topPanelHighResolution)
			hzPerPixel = m_sampleRate * m_zoom / plotWidth;
        else
			hzPerPixel = m_signalSpectrum->getHiResSampleRate() * m_zoom / plotWidth;
    }

    //Show mixer cursor, fMixer varies from -f..0..+f relative to LO
    //Map to coordinates
	qint32 x1=0;
	if (m_spectrumMode != NODISPLAY) {
        if (isZoomed) {
            x1 = 0.5 * plotWidth; //Zoom is always centered
        } else {
			x1 = m_fMixer / hzPerPixel;  //Convert fMixer to pixels
            x1 = x1 + plotFr.center().x(); //Make relative to center
        }
	}
#if 0
	else {
        x1 = 0.5 * plotWidth; //Cursor doesn't move in other modes, keep centered
    }
#endif


	int y = plotFr.y();
    //Show filter range
    //This doesn't work for CW because we have to take into account offset
    int xLo, xHi;
	int filterWidth = m_hiFilter - m_loFilter; //Could be pos or neg
	if (m_demodMode == DeviceInterface::dmCWU) {
        //loFilter = 500 hiFilter = 1500 modeOffset = 1000
        //Filter should be displayed 0 to 1000 or -1000 to 0 relative to cursor
        xLo = x1; //At the cursor
        xHi = x1 + filterWidth / hzPerPixel;
	} else if (m_demodMode == DeviceInterface::dmCWL) {
        xLo = x1 - filterWidth / hzPerPixel;
        xHi = x1;
    } else {
		xLo = x1 + m_loFilter/hzPerPixel;
		xHi = x1 + m_hiFilter/hzPerPixel;
    }

    //Shade filter area
    painter->setBrush(Qt::SolidPattern);
    painter->setOpacity(0.50);
	painter->fillRect(xLo, y, x1 - xLo - 1, y + plotHeight, Qt::gray);
	painter->fillRect(x1+2, y, xHi - x1 - 2, y + plotHeight, Qt::gray);

    //Show filter boundaries
    painter->setPen(QPen(Qt::cyan, 1,Qt::SolidLine));
	painter->drawLine(xLo, y, xLo, y + plotFr.height()); //Extend line into label frame
	painter->drawLine(xHi, y, xHi, y + plotFr.height()); //Extend line into label frame

    //Main cursor, draw last so it's on top
    painter->setOpacity(1.0);
    painter->setPen(QPen(color, 1,Qt::SolidLine));
	painter->drawLine(x1, y, x1, y + plotFr.height()); //Extend line into label frame

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

	//We don't want to paint the left and right pixmap borders, just the spectrum data
	//So we expand plotFr so the left border is off screen (-1) same with right border (+1)
	if (paintTopPanel) {
		topPanelFr.adjust(-1,0,+1,0);
		painter->drawPixmap(topPanelFr, m_topPanelPlotArea); //Includes plotOverlay which was copied to plotArea
		painter->drawPixmap(topPanelLabelFr,m_topPanelPlotLabel);
		paintFreqCursor(painter, topPanelFr, true, Qt::white);
		paintFreqCursor(painter, topPanelLabelFr, true, Qt::white);
	} else {
		plotFr.adjust(-1,0,+1,0);
		painter->drawPixmap(plotFr, m_plotArea); //Includes plotOverlay which was copied to plotArea
		painter->drawPixmap(plotLabelFr,m_plotLabel);
		paintFreqCursor(painter, plotFr, false, Qt::white);
		paintFreqCursor(painter, plotLabelFr, false, Qt::white);
	}

	paintMouseCursor(paintTopPanel, painter, Qt::white, true, true);

}

void SpectrumWidget::paintEvent(QPaintEvent *e)
{

    QPainter painter(this);

	if (!m_isRunning)
	{
		if (true)
		{
            painter.setFont(global->settings->medFont);
			for (int i=0; i<m_message.count(); i++)
			{
				painter.drawText(20, 15 + (i*12) , m_message[i]);
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

	if (m_spectrumMode == NODISPLAY || m_signalSpectrum == NULL)
        return;

	switch (m_spectrumMode) {
		case SPECTRUM:
			paintSpectrum(false,&painter); //Main frame
			break;

		case SPECTRUM_SPECTRUM:
			paintSpectrum(true,&painter); //Top frame
			paintSpectrum(false,&painter); //Main frame
			break;

		case SPECTRUM_WATERFALL:
			paintSpectrum(true,&painter); //Top frame
			paintWaterfall(false, &painter);
			break;

		case WATERFALL:
			paintWaterfall(false, &painter);
			break;

		case WATERFALL_WATERFALL:
			paintWaterfall(true, &painter);
			paintWaterfall(false, &painter);
			break;

		default:
			break;
	}

    //Tell spectrum generator it ok to give us another one
	m_signalSpectrum->m_displayUpdateComplete = true;
}

void SpectrumWidget::displayChanged(int s)
{
    //Get mode from itemData
	DisplayMode displayMode = (DisplayMode)ui.displayBox->itemData(s).toInt();
	//Save in device ini
	global->sdr->Set(DeviceInterface::LastSpectrumMode,displayMode);
    plotSelectionChanged(displayMode);
}

void SpectrumWidget::maxDbChanged(int s)
{
	int db = ui.maxDbBox->currentData().toInt();
	if (db > 0) {
		m_autoScaleMax = true;
		m_scaleNeedsRecalc = true;
	} else {
		m_autoScaleMax = false;
		m_plotMaxDb = db;
	}

	global->settings->autoScaleMax = m_autoScaleMax;
	global->settings->fullScaleDb = m_plotMaxDb;
	global->settings->WriteSettings(); //save
	drawOverlay();
	update();
}

void SpectrumWidget::minDbChanged(int t)
{
	int db = ui.minDbBox->currentData().toInt();
	if (db > 0) {
		m_autoScaleMin = true;
		m_scaleNeedsRecalc = true;
	} else {
		m_autoScaleMin = false;
		m_plotMinDb = db;
	}

	global->settings->autoScaleMin = m_autoScaleMin;
	global->settings->baseScaleDb = m_plotMinDb;
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
	if (!m_topPanelHighResolution)
		//Highest zoom factor is with item==400, .0625
		return 1.0 / pow(2.0,item/100.0);
	else
		//Since SR is lower (48k or 64k typically) max zoom at 400 is only .15749
		return 1.0 / pow(2.0,item/150.0);
}

void SpectrumWidget::zoomChanged(int item)
{
	m_zoom = calcZoom(item);

	if (m_spectrumMode == SPECTRUM  && item > 0)
		//Top panel is off, Turn on as if user clicked on selection
		ui.displayBox->setCurrentIndex(ui.displayBox->findData(SPECTRUM_SPECTRUM));
	else if (m_spectrumMode == WATERFALL && item > 0)
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
	m_lineBuf[numPoints].setX(0);
	m_lineBuf[numPoints].setY(_fftMap[0]);
	numPoints++;
	//Plot data
	for (int i=0; i< plotWidth; i++)
	{
		m_lineBuf[numPoints].setX(i+1); //Adjust for hidden border
		m_lineBuf[numPoints].setY(_fftMap[i]);
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
	m_lineBuf[numPoints].setX(plotWidth);
	m_lineBuf[numPoints].setY(lastFftMap);
	numPoints++;
	//Bottom Right
	m_lineBuf[numPoints].setX(plotWidth);
	m_lineBuf[numPoints].setY(plotHeight);

	numPoints++;
	//Bottom Left
	m_lineBuf[numPoints].setX(0);
	m_lineBuf[numPoints].setY(plotHeight);

	numPoints++;
	//Top fftMap[0]
	m_lineBuf[numPoints].setX(0);
	m_lineBuf[numPoints].setY(_fftMap[0]);

#if 0
	//Just draw the spectrum line, no fill
	plotPainter.drawPolyline(LineBuf, numPoints);
#else
	//Draw the filled polygon.  Note this may take slightly more CPUs
	QBrush tmpBrush = QBrush(gradient);
	plotPainter.setBrush(tmpBrush);
	plotPainter.drawPolygon(m_lineBuf,numPoints);

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
			plotColor = m_spectrumColors[255 - _fftMap[i]];
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
	if (!m_isRunning)
        return;

	//Safety check to make sure pixmaps are always sized correctly
	if (m_topPanelPlotArea.size() != ui.topPlotFrame->size() ||
			m_plotArea.size() != ui.plotFrame->size()) {
		//qDebug()<<"Resize pixmap";
		resizePixmaps(false); //Don't scale, we may have switched modes
		drawOverlay();
	}

	//Only auto-scale when we need to.  Lo changed, user request, etc.
	//Otherwise too jittery
	if (m_scaleNeedsRecalc) {
		recalcScale();
		m_scaleNeedsRecalc = false;
	}

	double startFreq =  - (m_sampleRate/2); //Relative to 0
	double endFreq = m_sampleRate/2;
	//Top panel is always centered on fMixer
    //Start by assuming we have full +/- samplerate and make fMixer zero centered
	double topPanelStartFreq =  startFreq * m_zoom;
	double topPanelEndFreq = endFreq * m_zoom ;
	topPanelStartFreq += m_fMixer;
	topPanelEndFreq += m_fMixer;

	switch (m_spectrumMode) {
		case NODISPLAY:
			//Do nothing
			break;

		case SPECTRUM:
			//Convert to plot area coordinates
			//This gets passed straight through to FFT MapFFTToScreen
			m_signalSpectrum->mapFFTToScreen(
				m_plotArea.height(),
				m_plotArea.width(),
				//These are same as testbench
				m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
				m_plotMinDb,   //FFT dB level corresponding to output value == 0
				startFreq, //-sampleRate/2, //Low frequency
				endFreq, //sampleRate/2, //High frequency
				m_fftMap );
			drawSpectrum(m_plotArea, m_plotOverlay, m_fftMap);
			//We can only display offscreen pixmap in paint() event, so call it to update
			update();
			break;
		case SPECTRUM_SPECTRUM:
			//Draw top
			if (!m_topPanelHighResolution) {
				//Convert to plot area coordinates
				//This gets passed straight through to FFT MapFFTToScreen
				m_signalSpectrum->mapFFTToScreen(
					m_topPanelPlotArea.height(),
					m_topPanelPlotArea.width(),
					//These are same as testbench
					m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					m_plotMinDb,   //FFT dB level corresponding to output value == 0
					topPanelStartFreq, //-sampleRate/2, //Low frequency
					topPanelEndFreq, //sampleRate/2, //High frequency
					m_topPanelFftMap );
			} else {
				m_signalSpectrum->mapFFTZoomedToScreen(
					m_topPanelPlotArea.height(),
					m_topPanelPlotArea.width(),
					//These are same as testbench
					m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					m_plotMinDb,   //FFT dB level corresponding to output value == 0
					m_zoom,
					m_modeOffset,
					m_topPanelFftMap );
			}
			drawSpectrum(m_topPanelPlotArea, m_topPanelPlotOverlay, m_topPanelFftMap);

			//Draw bottom spectrum
			m_signalSpectrum->mapFFTToScreen(
				m_plotArea.height(),
				m_plotArea.width(),
				//These are same as testbench
				m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
				m_plotMinDb,   //FFT dB level corresponding to output value == 0
				startFreq, //-sampleRate/2, //Low frequency
				endFreq, //sampleRate/2, //High frequency
				m_fftMap );
			drawSpectrum(m_plotArea, m_plotOverlay, m_fftMap);
			//We can only display offscreen pixmap in paint() event, so call it to update
			update();
			break;
		case SPECTRUM_WATERFALL:
			//Upper Spectrum
			if (!m_topPanelHighResolution) {
				m_signalSpectrum->mapFFTToScreen(
					m_topPanelPlotArea.height(),
					m_topPanelPlotArea.width(),
					//These are same as testbench
					m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					m_plotMinDb,   //FFT dB level corresponding to output value == 0
					topPanelStartFreq, //-sampleRate/2, //Low frequency
					topPanelEndFreq, //sampleRate/2, //High frequency
					m_topPanelFftMap );
			} else {
				m_signalSpectrum->mapFFTZoomedToScreen(
					m_topPanelPlotArea.height(),
					m_topPanelPlotArea.width(),
					//These are same as testbench
					m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					m_plotMinDb,   //FFT dB level corresponding to output value == 0
					m_zoom,
					m_modeOffset,
					m_topPanelFftMap );
			}
			drawSpectrum(m_topPanelPlotArea, m_topPanelPlotOverlay, m_topPanelFftMap);

			//Lower waterfall
			m_signalSpectrum->mapFFTToScreen(
				255, //Equates to spectrumColor array
				m_plotArea.width(),
				//These are same as testbench
				m_plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				m_plotMinDb, //FFT dB level corresponding to output value == 0
				startFreq, //Low frequency
				endFreq, //High frequency
				m_fftMap );

			drawWaterfall(m_plotArea, m_plotOverlay, m_fftMap);
			update();
			break;
		case WATERFALL_WATERFALL:
			//Top waterfall
			if (!m_topPanelHighResolution) {
				m_signalSpectrum->mapFFTToScreen(
					255, //Equates to spectrumColor array
					m_topPanelPlotArea.width(),
					//These are same as testbench
					m_plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
					m_plotMinDb, //FFT dB level corresponding to output value == 0
					topPanelStartFreq, //Low frequency
					topPanelEndFreq, //High frequency
					m_topPanelFftMap );
			} else {
				m_signalSpectrum->mapFFTZoomedToScreen(
					255,
					m_topPanelPlotArea.width(),
					//These are same as testbench
					m_plotMaxDb,      //FFT dB level  corresponding to output value == MaxHeight
					m_plotMinDb,   //FFT dB level corresponding to output value == 0
					m_zoom,
					m_modeOffset,
					m_topPanelFftMap );
			}
			drawWaterfall(m_topPanelPlotArea, m_topPanelPlotOverlay, m_topPanelFftMap);

			//Lower waterfall
			m_signalSpectrum->mapFFTToScreen(
				255, //Equates to spectrumColor array
				m_plotArea.width(),
				//These are same as testbench
				m_plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				m_plotMinDb, //FFT dB level corresponding to output value == 0
				startFreq, //Low frequency
				endFreq, //High frequency
				m_fftMap );
			drawWaterfall(m_plotArea,m_plotOverlay,m_fftMap);
			update();
			break;
		case WATERFALL: {
			//Instead of plot area coordinates we convert to screen color array
			//Width is unchanged, but height is # colors we have for each db
			m_signalSpectrum->mapFFTToScreen(
				255, //Equates to spectrumColor array
				m_plotArea.width(),
				//These are same as testbench
				m_plotMaxDb, //FFT dB level  corresponding to output value == MaxHeight
				m_plotMinDb, //FFT dB level corresponding to output value == 0
				startFreq, //Low frequency
				endFreq, //High frequency
				m_fftMap );
			drawWaterfall(m_plotArea,m_plotOverlay,m_fftMap);
			update();
			break;
		}
		default:
			break;
	}

}

//Update top pane with new mode
void SpectrumWidget::updateTopPanel(DisplayMode _newMode, bool updateSlider)
{
	//setValue will automatically signal value changed and call connected SLOT unless we block signals
	switch(_newMode) {
		case NODISPLAY:
			//Hide top pane and update sizes
			ui.topLabelFrame->setVisible(false);
			ui.topPlotFrame->setVisible(false);
			ui.topFrameSplitter->setVisible(false);
			m_zoom = 1;
			ui.zoomSlider->setValue(0);
			ui.hiResButton->setChecked(false);
			m_topPanelHighResolution = false;
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
			m_topPanelHighResolution = false;
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
			m_topPanelHighResolution = false;
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
	if (!m_isRunning)
        return;

	int numXDivs;

    //Draw the frequency scale
    float pixPerHdiv;
    int x,y;
    QRect rect;
	int plotLabelHeight;

    //Show actual hi/lo frequency range
	QString horizLabels[m_maxHDivs];
    //zoom is fractional
    //100,000 sps * 1.00 / 10 = 10,000 hz per division
    //100,000 sps * 0.10 / 10 = 1000 hz per division
    //Bug with 16bit at 2msps rate
	quint32 hzPerhDiv;
	if (drawTopFrame) {
		numXDivs = (m_topPanelPlotLabel.width() / 100) * 2; //x2 to make sure always even number
		plotLabelHeight = m_topPanelPlotLabel.height();
		pixPerHdiv = (float)m_topPanelPlotLabel.width() / (float)numXDivs;
		if (!m_topPanelHighResolution) {
			hzPerhDiv = m_sampleRate * m_zoom / numXDivs;
		} else {
			//Smaller range
			hzPerhDiv = m_signalSpectrum->getHiResSampleRate() * m_zoom / numXDivs;
		}
		//qDebug()<<"Top "<<hzPerhDiv;
	} else {
		numXDivs = (m_plotLabel.width() / 100) * 2; //x2 to make sure always even number
		plotLabelHeight = m_plotLabel.height();
		pixPerHdiv = (float)m_plotLabel.width() / (float)numXDivs;
		hzPerhDiv = m_sampleRate / numXDivs;
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
		if (m_plotOverlay.isNull())
			return; //Not created yet

		m_plotOverlay.fill(Qt::black); //Clear every time because we are update cursor and other info here also
		m_plotLabel.fill(Qt::black);
		overlayWidth = m_plotOverlay.width();
		overlayHeight = m_plotOverlay.height();
		plotFr = ui.plotFrame->geometry(); //relative to parent
		painter->begin(&m_plotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&m_plotLabel);

	} else {
		if (m_topPanelPlotOverlay.isNull())
			return;

		m_topPanelPlotOverlay.fill(Qt::black); //Clear every time because we are update cursor and other info here also
		m_topPanelPlotLabel.fill(Qt::black);
		overlayWidth = m_topPanelPlotOverlay.width();
		overlayHeight = m_topPanelPlotOverlay.height();
		plotFr = ui.topPlotFrame->geometry(); //relative to parent
		painter->begin(&m_topPanelPlotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&m_topPanelPlotLabel);
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
	numYDivs = (m_plotMaxDb - m_plotMinDb) / dbLabelInterval; //Range / db per div

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
		painter->drawText(0, y-2, "-"+QString::number(abs(m_plotMaxDb) + (i * dbLabelInterval)));
	}

	if (!drawTopPanel)
		drawScale(labelPainter, m_loFreq, false);
	else
		drawScale(labelPainter, m_loFreq + m_fMixer, true);

	painter->end();  //Because we use pointer, automatic with instance
	labelPainter->end();
	delete painter;
	delete labelPainter;
}

void SpectrumWidget::paintMouseCursor(bool paintTopPanel, QPainter *painter, QColor color,
		bool paintDb, bool paintFreq)
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
	m_mouseFreq = getMouseFreq() + m_loFreq;
	int mouseDb = getMouseDb();
	if (m_mouseFreq > 0)
		freqLabel.sprintf("%.3f kHz",m_mouseFreq / 1000.0);
	else
		freqLabel = "";
	dbLabel.sprintf("%d db",mouseDb);

	painter->setFont(global->settings->medFont);
	//How many pixels do we need to display label with specified font
	QFontMetrics metrics(global->settings->medFont);
	QRect rect = metrics.boundingRect(freqLabel);
	if (cursorPos.x() + rect.width() > plotFr.width())
		cursorPos.setX(cursorPos.x() - rect.width()); //left of cursor

	//Draw cursor label on top of pixmap, but not control frame
	if ((paintTopPanel && topPanelFr.contains(cursorPos)) || (!paintTopPanel && plotFr.contains(cursorPos))) {
		painter->setPen(color);
		if (paintFreq) {
			painter->drawText(cursorPos,freqLabel);
		}
		if (paintDb) {
			//Paint db under freq
			cursorPos.setY(cursorPos.y() + metrics.height());
			painter->drawText(cursorPos,dbLabel);
		}
	}

	//Draw overload indicator
	if (!paintTopPanel && m_signalSpectrum->getOverload()) {
		cursorPos.setX(plotFr.width() - 50);
		cursorPos.setY(10);
		painter->setPen(Qt::red);
		painter->drawText(cursorPos,"Overload");
	}


}

//topPanelPlotArea, topPanelPlotLabel, plotArea, plotLabel are all pixmaps that are updated whenever
//we get new FFT data.  The paint routine doensn't care whether its a waterfall or spectrum,
//but we may want to have special labels etc for different displays
void SpectrumWidget::paintWaterfall(bool paintTopPanel, QPainter *painter)
{
	QRect plotFr = mapFrameToWidget(ui.plotFrame);
	QRect plotLabelFr = mapFrameToWidget(ui.labelFrame);
	QRect topPanelFr = mapFrameToWidget(ui.topPlotFrame);
	QRect topPanelLabelFr = mapFrameToWidget(ui.topLabelFrame);

	if (paintTopPanel) {
		painter->drawPixmap(topPanelFr, m_topPanelPlotArea);
		painter->drawPixmap(topPanelLabelFr,m_topPanelPlotLabel);
		//Cursor is drawn on top of pixmap
		paintFreqCursor(painter, topPanelFr, true, Qt::white);
		paintFreqCursor(painter, topPanelLabelFr, true, Qt::white);
		paintMouseCursor(true, painter, Qt::white, false,true);
	} else {
		painter->drawPixmap(plotFr, m_plotArea);
		painter->drawPixmap(plotLabelFr,m_plotLabel);
		//Cursor is drawn on top of pixmap
		paintFreqCursor(painter, plotFr, false, Qt::white);
		paintFreqCursor(painter, plotLabelFr, false, Qt::white);
		paintMouseCursor(false, painter, Qt::white, false,true);
	}
}

void SpectrumWidget::drawWaterfallOverlay(bool drawTopPanel)
{
	QPainter *painter = new QPainter();
	QPainter *labelPainter = new QPainter();
	QRect plotFr;

	if (!drawTopPanel) {
		plotFr = ui.plotFrame->geometry(); //relative to parent
		m_plotLabel.fill(Qt::black);
		painter->begin(&m_plotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&m_plotLabel);
	} else {
		plotFr = ui.topPlotFrame->geometry(); //relative to parent
		m_topPanelPlotLabel.fill(Qt::black);
		painter->begin(&m_topPanelPlotOverlay);  //Because we use pointer, automatic with instance
		labelPainter->begin(&m_topPanelPlotLabel);
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

	if (!drawTopPanel)
		drawScale(labelPainter, m_loFreq, false);
	else
		drawScale(labelPainter, m_loFreq + m_fMixer, true);

	painter->end();  //Because we use pointer, automatic with instance
	labelPainter->end();
	delete painter;
	delete labelPainter;
}

void SpectrumWidget::drawOverlay()
{
	if (!m_isRunning)
        return;

    //!!! 1/11/14 This never seems to get called with isRunning == true

	//Update span label
	qint32 range = m_sampleRate;
	qint32 zoomRange = m_signalSpectrum->getHiResSampleRate();
	if (!m_topPanelHighResolution) {
		ui.zoomLabel->setText(QString().sprintf(" %.0f kHz",range/1000 * m_zoom));
	} else {
		//HiRes
		ui.zoomLabel->setText(QString().sprintf(" %.0f kHz",zoomRange/1000 * m_zoom));
	}

	switch(m_spectrumMode) {
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
