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


    ui.displayBox->addItem("Spectrum");
    ui.displayBox->addItem("Waterfall");
    ui.displayBox->addItem("I/Q");
    ui.displayBox->addItem("Phase");
    ui.displayBox->addItem("Off");
    //Testing, may not leave these in live product
    ui.displayBox->addItem("Post Mixer");
    ui.displayBox->addItem("Post BandPass");
    ui.displayBox->setCurrentIndex(-1);

    connect(ui.displayBox,SIGNAL(currentIndexChanged(int)),this,SLOT(displayChanged(int)));

    //Sets db offset
    ui.dbOffsetBox->addItem("Offset -40",-40);
    ui.dbOffsetBox->addItem("Offset -30",-30);
    ui.dbOffsetBox->addItem("Offset -20",-20);
    ui.dbOffsetBox->addItem("Offset -10",-10);
    ui.dbOffsetBox->addItem("Offset 0",0);
    ui.dbOffsetBox->addItem("Offset +10",+10);
    ui.dbOffsetBox->addItem("Offset +20",+20);
    ui.dbOffsetBox->addItem("Offset +30",+30);
    ui.dbOffsetBox->setCurrentIndex(4);
    connect(ui.dbOffsetBox,SIGNAL(currentIndexChanged(int)),this,SLOT(dbOffsetChanged(int)));
    spectrumOffset=0;

    ui.dbGainBox->addItem("Gain: 0.50x",0.50);
    ui.dbGainBox->addItem("Gain: 0.75x",0.75);
    ui.dbGainBox->addItem("Gain: 1.00x",1.00); //Default
    ui.dbGainBox->addItem("Gain: 1.25x",1.25);
    ui.dbGainBox->addItem("Gain: 1.50x",1.50);
    ui.dbGainBox->addItem("Gain: 2.00x",2.00);
    ui.dbGainBox->addItem("Gain: 2.50x",2.50);
    ui.dbGainBox->addItem("Gain: 3.00x",3.00);
    ui.dbGainBox->setCurrentIndex(2);
    connect(ui.dbGainBox,SIGNAL(currentIndexChanged(int)),this,SLOT(dbGainChanged(int)));
    spectrumGain = 1;

    //For 48k sample rate, 100% is +/- 24k
    ui.zoomBox->addItem("1.00x",1.00);
    ui.zoomBox->addItem("1.25x",0.80);
    ui.zoomBox->addItem("1.50x",0.66);
    ui.zoomBox->addItem("2.00x",0.50);
    ui.zoomBox->addItem("2.50x",0.40);
    ui.zoomBox->addItem("5.00x",0.20);
    ui.zoomBox->addItem("10.00x",0.10);
    ui.zoomBox->setCurrentIndex(0);
    connect(ui.zoomBox,SIGNAL(currentIndexChanged(int)),this,SLOT(zoomChanged(int)));
    zoom = 1;

	spectrumMode=SignalSpectrum::SPECTRUM;

	plotArea = NULL;
	//message = NULL;
	signalSpectrum = NULL;
	averageSpectrum = NULL;
	lastSpectrum = NULL;

	fMixer = 0;

    dbRange = abs(global->maxDb - global->minDb);

    //Spectrum power is converted to 8bit int for display
    //This array maps the colors for each power level
    //Technique from CuteSDR to get a smooth color palette across the spectrum
#if 0
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

    //Turns on mouse events without requiring button press
    //This does not work if plotFrame is set in code or if both plotFrame and it's parent widgetFrame are set in code
    //But if I set in the designer poperty editor (all in hierarchy have to be set), we get events
    //Maybe a Qt bug related to where they get set
    //ui.widgetFrame->setMouseTracking(true);
    //ui.plotFrame->setMouseTracking(true);

    //Set focus policy so we get key strokes
    setFocusPolicy(Qt::StrongFocus); //Focus can be set by click or tab

	//Start paint thread
	st = new SpectrumThread(this);
	connect(st,SIGNAL(repaint()),this,SLOT(updateSpectrum()));
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
    //Global is not initialized in constructor
    ui.displayBox->setFont(global->settings->medFont);
    ui.dbOffsetBox->setFont(global->settings->medFont);
    ui.dbGainBox->setFont(global->settings->medFont);
    ui.cursorLabel->setFont(global->settings->medFont);
    ui.zoomBox->setFont(global->settings->medFont);

	if (r) {
        ui.displayBox->setCurrentIndex(global->settings->lastDisplayMode); //Initial display mode

		st->start();
		isRunning = true;
	}
	else {
        ui.displayBox->setCurrentIndex(-1);

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
    event->ignore(); //We don't handle
}

//Returns +/- freq from LO based on where mouse cursor is
//Called from paint
double SpectrumWidget::GetMouseFreq()
{
    QPoint mp = QCursor::pos(); //Current mouse position in global coordinates
    mp = mapFromGlobal(mp); //Convert to widget relative
    QRect pf = this->ui.plotFrame->geometry();
    if (!pf.contains(mp))
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
        return global->minDb;

    //Find db at cursor
    int db = (pf.height() - mp.y()); //Num pixels
    db = dbRange/pf.height() * db; //Scale to get db
    //Convert back to minDb relative
    db += global->minDb - spectrumOffset;
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
	}
    event->accept();
}
void SpectrumWidget::mousePressEvent ( QMouseEvent * event ) 
{
	if (!isRunning || signalSpectrum == NULL)
		return;
    QRect pf = this->ui.plotFrame->geometry();
    if (!pf.contains(event->pos()))
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

void SpectrumWidget::plotSelectionChanged(SignalSpectrum::DISPLAYMODE mode)
{
	if (signalSpectrum != NULL) {
		signalSpectrum->SetDisplayMode(mode);
	}
    global->settings->lastDisplayMode = mode;

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
// Diplays frequency cursor and filter range
void SpectrumWidget::paintCursor(QColor color)
{
    QPainter painter(this);
    QRect plotFr = ui.plotFrame->geometry(); //relative to parent
    int plotWidth = plotFr.width();
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

	QPen pen;
	pen.setStyle(Qt::SolidLine);
	pen.setWidth(1);
	pen.setColor(color);
	painter.setPen(pen);
    painter.drawLine(x1,0,x1, plotFr.height()); //Extend line into label frame

    //Show bandpass filter range in labelFrame
    QRect fr = ui.labelFrame->geometry(); //relative to parent
	int xLo = x1 + loFilter/hzPerPixel;
	int xHi = x1 + hiFilter/hzPerPixel;
    int y2 = fr.y() + 3; //Top of label frame
    pen.setWidth(1);
	pen.setColor(Qt::black);
	//pen.setBrush(Qt::Dense5Pattern);
	painter.setPen(pen); //If we don't set pen again, changes have no effect
	painter.drawLine(xLo,y2,xHi,y2);
    painter.drawLine(x1,fr.y(),x1, fr.y()+5); //small vert cursor in label frame

    QString label;
    mouseFreq = GetMouseFreq() + loFreq;
    int mouseDb = GetMouseDb();
    if (mouseFreq > 0)
        label.sprintf("%.3f kHz @ %ddB",mouseFreq / 1000.0,mouseDb);
    else
        label = "";

    QRect freqFr = ui.cursorLabel->geometry();
    QRect ctrlFr = ui.controlFrame->geometry();
    painter.translate(ctrlFr.x(),ctrlFr.y()); //Relative to labelFrame
    //painter.translate(freqFr.x(),freqFr.y());
    painter.setFont(global->settings->medFont);
    painter.drawText(freqFr.bottomLeft(),label);
}

void SpectrumWidget::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);
    QRect plotFr = ui.plotFrame->geometry(); //relative to parent
    int plotHeight = plotFr.height(); //Plot area height
    int plotWidth = plotFr.width();

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

    //Map spectrum[] to display
    int binCountRaw = signalSpectrum->BinCount(); //Total number of spectrum data available to us
    int binCount = binCountRaw * zoom; //Number of spectrum data we'll actually display
    int binMid = binCountRaw / 2;
    int binStart = binMid - (binMid * zoom);
    int binEnd = binMid + (binMid * zoom);

    //binStart + plotWidth*binCount must be less than binCountRaw or we overflow spectrum[]
    //scale samples to # pixels we have available
    double binStep = (double) binCount / plotWidth;

#if 0
	painter.setFont(QFont("Arial", 7,QFont::Normal));
	painter.setPen(Qt::red);
	if (signalSpectrum->inBufferOverflowCount > 0)
        painter.drawText(plotWidth-20,10,"over");
	if (signalSpectrum->inBufferUnderflowCount > 0)
		painter.drawText(0,10,"under");
	painter.setPen(Qt::black);
#endif

	//Draw scale
    //labelRect X,Y will be relative to 0.0
    QRect labelRect = ui.labelFrame->geometry();
    //This makes logical 0,0 map to left,upper of labelFrame
    painter.resetTransform();
    painter.translate(labelRect.x(),labelRect.y());

    painter.setPen(Qt::black);
    painter.setFont(global->settings->smFont);

    //Show actual hi/lo frequency range
    double loRange = (loFreq - sampleRate/2 * zoom)/1000;
	double midRange = loFreq/1000;
    double hiRange = (loFreq + sampleRate/2 * zoom)/1000;
	QString loText = QString::number(loRange,'f',0);
	QString midText = QString::number(midRange,'f',0);
	QString hiText = QString::number(hiRange,'f',0);
    painter.drawText(0,labelRect.height()-2, loText+"k");
    painter.drawText(labelRect.width()/2 - 15,labelRect.height()-2, midText+"k");
    painter.drawText(labelRect.width() - 35,labelRect.height()-2, hiText+"k");

	//Tics
    //Draw center tick, then on either side
    int labelMid = labelRect.width()/2;
    int numTicks = labelMid / 10; //1 tick every N pixel regardless of spectrum width
    int inc = labelMid / numTicks;

    painter.drawLine(labelMid,5,labelMid,2);
    for (int i=0; i<=numTicks; i++){
        painter.drawLine(labelMid - i*inc,3, labelMid - i*inc, 2);
        painter.drawLine(labelMid + i*inc,3, labelMid + i*inc, 2);
	}

    //Draw spectrum colors centered at bottom of label pane
    if (spectrumMode == SignalSpectrum::WATERFALL) {
        for (int i=0; i<dbRange; i++) {
            painter.setPen(spectrumColors[i]);
            painter.drawPoint(labelMid - 80 + i,labelRect.height()-2);
            painter.drawPoint(labelMid - 80 + i,labelRect.height()-1);
    }
    }
    painter.resetTransform();  //If we don't call this, translate gets confused
    painter.translate(plotFr.x(),plotFr.y());

	//In case we try to paint beyond plotFrame, set this AFTER area outside plotFrame has been painted
    //painter.setClipRegion(fr); //This prevents us from painting into the labelFrame with new ui, so skip


	//Calculate y scaling
    /*
     * We keep db between dbMin and dbMax at all times (0 and dbRange when zero based
     * dbZeroAdj is the amount we add to any db to make sure it's zero to dbRange
     */
    double dbZeroAdj = abs(global->minDb); //Adj to get to zero base
    //qDebug("dbZero %f dbRange %f",dbZeroAdj, dbRange);

    //Scale dbRange to number of pixels we have
    float y_scale = (float)(plotHeight) / dbRange;

    double db = 0;
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

    if (spectrum != NULL && smoothing) {
		if (averageSpectrum == NULL) {
            averageSpectrum = new float[binCountRaw];
            lastSpectrum = new float[binCountRaw];
            memcpy(averageSpectrum, spectrum, binCountRaw * sizeof(float));
		} else {
            for (int i = 0; i < binCountRaw; i++)
				averageSpectrum[i] = (lastSpectrum[i] + spectrum[i]) / 2;
		}
		//Save this spectrum for next iteration
        memcpy(lastSpectrum, spectrum, binCountRaw * sizeof(float));
		spectrum = averageSpectrum;
	}

    //plotArea has to be created after layout has done it's work, not in constructor
    //Todo: Find a post layout event and more
    if (plotArea == NULL) {
        plotArea = new QPixmap(plotFr.width(),plotFr.height());
        plotArea->fill(Qt::black);
    } else if (plotArea->width() != plotFr.width() || plotArea->height() != plotFr.height()) {
        //Window resized
        delete plotArea;
        plotArea = new QPixmap(plotFr.width(),plotFr.height());
        plotArea->fill(Qt::black);
    }
    QPainter plotPainter(plotArea);

    //Color code each bin and draw line
    QColor plotColor;

	if (spectrumMode == SignalSpectrum::SPECTRUM)
	{
        plotArea->fill(Qt::lightGray); //Erase to background color each time
        //BUG: Won't handle case where we have more pixels than samples
        int lastX = 0;
        int lastY = 0;
        for (int i=0; i<plotFr.width(); i++)
		{
			plotX = i;

			//Taking average results in spectrum not being displayed correctly due to 'chunks'
			//Just use every Nth sample and throw away the rest
            //We should only be getting values from -130 to +10 in spectrum array
            //but occasionally we get a garbage value which I think is caused by spectrum thread running while spectrum array is being update
            //Rather than lock, we'll just bound the results and ignore
            db = spectrum[binStart + qRound(binStep * i)]; //Spectrum is already bound to minDb and maxDb

            //Make relative to zero.
            db = qBound(global->minDb, db + spectrumOffset, global->maxDb);
            db = qBound(0.0, db + dbZeroAdj, (double)dbRange);  //ensure zero base
            db = qBound(0.0, db * spectrumGain, (double)dbRange);
            //Experiment to use same colors as waterfall
            //Too much color change to be useful, but maybe a single color change for strong signals?
            //painter.setPen(spectrumColors[(int)db]);

            //Test
            //if (db < 0 || db > dbRange)
            //    qDebug("Spectrum db value out of range. dbRange:%f, rawDb:%f db:%f",dbRange,rawDb,db);

			//scale to fit our smaller Y axis
			db *= y_scale;

			//Vertical line from base of widget to height in db
            db = plotHeight - db; //Make rel to bottom of frame
            if (db > plotHeight)
                db = plotHeight;  //Min

            plotPainter.setPen(Qt::darkGray);
            plotPainter.drawLine(plotX,plotHeight,plotX,db);
            plotPainter.setPen(Qt::blue);
            if (i>0)
                plotPainter.drawLine(lastX,lastY,plotX,db);
            lastX = plotX;
            lastY = db;
        }
        painter.drawPixmap(0,0,plotFr.width(),plotFr.height(),*plotArea);

        //Paint cursor last because painter changes - hack
        paintCursor(Qt::black);


	} else if (spectrumMode == SignalSpectrum::WATERFALL ||
			   spectrumMode == SignalSpectrum::POSTMIXER  ||
			   spectrumMode == SignalSpectrum::POSTBANDPASS){
		//Waterfall
        //Scroll fr rect 1 pixel to create new line
        //plotArea->scroll(0,1,0,0, fr.width(), fr.height()); //This has to be done before attaching painter
        plotArea->scroll(0,1,plotFr); //This has to be done before attaching painter

		for (int i=0; i<plotArea->width(); i++)
		{
			plotX = i;
			plotY = 0;

            db = spectrum[binStart + qRound(binStep * i)];

            //Make relative to zero.
            db = qBound(global->minDb, db + spectrumOffset, global->maxDb);
            db = qBound(0.0, db + dbZeroAdj, (double)dbRange);  //ensure zero base
            db = qBound(0.0, db * spectrumGain, (double)dbRange);

            plotColor = spectrumColors[(int)db];

            plotPainter.setPen(plotColor);
            plotPainter.drawPoint(plotX,plotY);

		}
        painter.drawPixmap(0,0,plotFr.width(),plotFr.height(),*plotArea);

		//Draw cursor on top of image
        paintCursor(Qt::white);
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
        int gain = (spectrumGain * 5000);

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
    case 5:
        plotSelectionChanged(SignalSpectrum::POSTMIXER);
        break;
    case 6:
        plotSelectionChanged(SignalSpectrum::POSTBANDPASS);
        break;
    }
}

void SpectrumWidget::dbOffsetChanged(int s)
{
    spectrumOffset = ui.dbOffsetBox->itemData(s).toInt();
    repaint();
}

void SpectrumWidget::dbGainChanged(int s)
{
    spectrumGain = ui.dbGainBox->itemData(s).toDouble();
    repaint();
}

void SpectrumWidget::zoomChanged(int item)
{
    zoom = ui.zoomBox->itemData(item).toDouble();
    repaint();
}
