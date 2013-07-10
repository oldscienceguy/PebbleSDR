//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QPalette>
#include <QMenu>
#include <QKeyEvent>
#include <QMessageBox>
#include <QLCDNumber>
#include "ReceiverWidget.h"
#include "receiver.h"
#include "global.h"

ReceiverWidget::ReceiverWidget(QWidget *parent):QWidget(parent)
{
	//Delegates to QtDesigner managed ui for actual components
	ui.setupUi(this);
}
//This does the actual setup and connections to receiver class
void ReceiverWidget::SetReceiver(Receiver *r)
{
	receiver = r;
    presets = NULL;

	powerOn = false;
	frequency = 0;

    QFont smFont = receiver->GetSettings()->smFont;
    QFont medFont = receiver->GetSettings()->medFont;
    QFont lgFont = receiver->GetSettings()->lgFont;

	QStringList modes;
    modes << "AM"<<"SAM"<<"FMN"<<"FM-Mono"<<"FM-Stereo"<<"DSB"<<"LSB"<<"USB"<<"CWL"<<"CWU"<<"DIGL"<<"DIGU"<<"NONE";
	ui.modeBox->addItems(modes);

	QStringList agcModes;
	agcModes << "OFF" << "SLOW" << "MED" << "FAST" << "LONG";
	ui.agcBox->addItems(agcModes);

#if 0
	QMenu *settingsMenu = new QMenu();
	//Shortcut, does connect(...triggered(bool) ...)
	settingsMenu->addAction("General",receiver,SLOT(ShowSettings(bool)));
	settingsMenu->addAction("SDR Device",receiver,SLOT(ShowSdrSettings(bool)));
	settingsMenu->addAction("IQ Balance",receiver,SLOT(ShowIQBalance(bool)));
    settingsMenu->setFont(medFont);
	ui.settingsButton->setMenu(settingsMenu);
#endif
    connect(ui.settingsButton,SIGNAL(clicked(bool)),receiver,SLOT(ShowSettings(bool)));

	//ui.spectrumWidget->setStyleSheet("border: 1px solid white");
	//Todo: Make lcd glow when on, set up/down button colors, etc
	//ui.nixie1->setStyleSheet("background: rgba(240, 255, 255, 75%)");

	//Set up filter option lists
	//Broadcast 15Khz 20 hz -15 khz
	//Speech 3Khz 300hz to 3khz
	//RTTY 250-1000hz
	//CW 200-500hz
	//PSK31 100hz

	//Defaults from IC7000 & PowerSDR
	//Order these wide->narrow
	//Note: These are expressed in bandwidth, ie 16k am = -8000 to +8000 filter
	amFilterOptions << "16000" << "12000" << "6000" << "3000";
	lsbFilterOptions << "3000" << "2400" << "1800"; //lsb, usb
	diglFilterOptions << "2400" << "1200" << "500" << "250" ;
	cwFilterOptions << "1800" << "1200" << "500" << "250" ; //overlap with lsb at wide end
	fmFilterOptions << "10000" << "7000" ;
    //wfmFilterOptions << "80000"<<"40000";
	ui.filterBox->addItems(amFilterOptions); //Default

    ui.dataSelectionBox->addItem("Off",NO_DATA);
    ui.dataSelectionBox->addItem("Band",BAND_DATA);
    ui.dataSelectionBox->addItem("CW",CW_DATA);
    ui.dataSelectionBox->addItem("RTTY",RTTY_DATA);
    ui.dataSelectionBox->setFont(medFont);
    connect(ui.dataSelectionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(dataSelectionChanged(int)));
    SetDataMode((BAND_DATA));

	loMode = true;
	//Set intial gain slider position
	ui.gainSlider->setValue(50);
	tunerStep=1000;

	modeOffset = 0;

    //Band setup.  Presets must have already been read in
    ui.bandType->clear();
    ui.bandType->addItem("All",Band::ALL);
    ui.bandType->addItem("Ham",Band::HAM);
    ui.bandType->addItem("Broadcast",Band::SW);
    ui.bandType->addItem("Scanner",Band::SCANNER);
    ui.bandType->addItem("Other",Band::OTHER);
    connect(ui.bandType,SIGNAL(currentIndexChanged(int)),this,SLOT(bandTypeChanged(int)));
    connect(ui.bandCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(bandChanged(int)));
    connect(ui.stationCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(stationChanged(int)));

    currentBandIndex = -1;

	connect(ui.loButton,SIGNAL(toggled(bool)),this,SLOT(setLoMode(bool)));
	connect(ui.powerButton,SIGNAL(toggled(bool)),this,SLOT(powerToggled(bool)));
    connect(ui.recButton,SIGNAL(toggled(bool)),receiver,SLOT(RecToggled(bool)));
    connect(ui.anfButton,SIGNAL(toggled(bool)),this,SLOT(anfButtonToggled(bool)));
	connect(ui.nbButton,SIGNAL(toggled(bool)),this,SLOT(nbButtonToggled(bool)));
	connect(ui.nb2Button,SIGNAL(toggled(bool)),this,SLOT(nb2ButtonToggled(bool)));
	connect(ui.agcBox,SIGNAL(currentIndexChanged(int)),this,SLOT(agcBoxChanged(int)));
	connect(ui.lpfButton,SIGNAL(toggled(bool)),this,SLOT(lpfButtonToggled(bool)));
	connect(ui.muteButton,SIGNAL(toggled(bool)),this,SLOT(muteButtonToggled(bool)));
	connect(ui.modeBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(modeSelectionChanged(QString)));
	connect(ui.filterBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(filterSelectionChanged(QString)));
	connect(ui.gainSlider,SIGNAL(valueChanged(int)),this,SLOT(gainSliderChanged(int)));
	connect(ui.agcSlider,SIGNAL(valueChanged(int)),this,SLOT(agcSliderChanged(int)));
	connect(ui.squelchSlider,SIGNAL(valueChanged(int)),this,SLOT(squelchSliderChanged(int)));
	connect(ui.spectrumWidget,SIGNAL(mixerChanged(int)),this,SLOT(mixerChanged(int)));
    connect(ui.spectrumWidget,SIGNAL(mixerChanged(int,bool)),this,SLOT(mixerChanged(int,bool)));
    connect(ui.addMemoryButton,SIGNAL(clicked()),this,SLOT(addMemoryButtonClicked()));
    connect(ui.findStationButton,SIGNAL(clicked()),this,SLOT(findStationButtonClicked()));

	connect(ui.nixie1Up,SIGNAL(clicked()),this,SLOT(nixie1UpClicked()));
	connect(ui.nixie1Down,SIGNAL(clicked()),this,SLOT(nixie1DownClicked()));
	connect(ui.nixie10Up,SIGNAL(clicked()),this,SLOT(nixie10UpClicked()));
	connect(ui.nixie10Down,SIGNAL(clicked()),this,SLOT(nixie10DownClicked()));
	connect(ui.nixie100Up,SIGNAL(clicked()),this,SLOT(nixie100UpClicked()));
	connect(ui.nixie100Down,SIGNAL(clicked()),this,SLOT(nixie100DownClicked()));
	connect(ui.nixie1kUp,SIGNAL(clicked()),this,SLOT(nixie1kUpClicked()));
	connect(ui.nixie1kDown,SIGNAL(clicked()),this,SLOT(nixie1kDownClicked()));
	connect(ui.nixie10kUp,SIGNAL(clicked()),this,SLOT(nixie10kUpClicked()));
	connect(ui.nixie10kDown,SIGNAL(clicked()),this,SLOT(nixie10kDownClicked()));
	connect(ui.nixie100kUp,SIGNAL(clicked()),this,SLOT(nixie100kUpClicked()));
	connect(ui.nixie100kDown,SIGNAL(clicked()),this,SLOT(nixie100kDownClicked()));
	connect(ui.nixie1mUp,SIGNAL(clicked()),this,SLOT(nixie1mUpClicked()));
	connect(ui.nixie1mDown,SIGNAL(clicked()),this,SLOT(nixie1mDownClicked()));
	connect(ui.nixie10mUp,SIGNAL(clicked()),this,SLOT(nixie10mUpClicked()));
	connect(ui.nixie10mDown,SIGNAL(clicked()),this,SLOT(nixie10mDownClicked()));
	connect(ui.nixie100mUp,SIGNAL(clicked()),this,SLOT(nixie100mUpClicked()));
	connect(ui.nixie100mDown,SIGNAL(clicked()),this,SLOT(nixie100mDownClicked()));
    connect(ui.nixie1gUp,SIGNAL(clicked()),this,SLOT(nixie1gUpClicked()));
    connect(ui.nixie1gDown,SIGNAL(clicked()),this,SLOT(nixie1gDownClicked()));

	//Send widget component events to this class eventFilter
	ui.nixie1->installEventFilter(this); //So we can grab clicks
	ui.nixie10->installEventFilter(this);
	ui.nixie100->installEventFilter(this);
	ui.nixie1k->installEventFilter(this);
	ui.nixie10k->installEventFilter(this);
	ui.nixie100k->installEventFilter(this);
	ui.nixie1m->installEventFilter(this);
	ui.nixie10m->installEventFilter(this);
	ui.nixie100m->installEventFilter(this);
    ui.nixie1g->installEventFilter(this);
    ui.directEntry->installEventFilter(this); //So we can grab Enter key

    ui.directEntry->setInputMask("0000000.000"); //All digits, none required

    ui.agcBox->setFont(medFont);
    ui.agcSlider->setFont(smFont);
    ui.anfButton->setFont(smFont);
    ui.gainSlider->setFont(medFont);
    ui.directEntry->setFont(medFont);
    ui.label->setFont(medFont);
    ui.label_2->setFont(smFont);
    ui.label_3->setFont(medFont);
    ui.loButton->setFont(smFont);
    ui.lpfButton->setFont(smFont);
    ui.mixButton->setFont(smFont);
    ui.modeBox->setFont(medFont);
    ui.muteButton->setFont(medFont);
    ui.nb2Button->setFont(smFont);
    ui.nbButton->setFont(smFont);
    ui.powerButton->setFont(medFont);
    ui.recButton->setFont(medFont);
    ui.recButton->setEnabled(false);

    ui.settingsButton->setFont(medFont);
    ui.squelchSlider->setFont(smFont);
    ui.filterBox->setFont(medFont);
    ui.stationCombo->setFont(medFont);
    ui.bandCombo->setFont(medFont);
    ui.bandType->setFont(medFont);
    ui.addMemoryButton->setFont(smFont);
    ui.findStationButton->setFont(smFont);

    //Clock
    //Todo: Match bold black look of nixie
    QPalette palette;
    palette.setColor(QPalette::WindowText,Qt::black);
    ui.clockWidget->setPalette(palette);
    ui.clockWidget->setSegmentStyle(QLCDNumber::Flat);
    //Same as power on background
    ui.clockWidget->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0.471, y1:0, x2:0.483, y2:0.982955, stop:0 rgba(255, 243, 72, 255), stop:0.778409 rgba(255, 247, 221, 255))");

    ui.utcClockButton->setFont(smFont);
    ui.localClockButton->setFont(smFont);

    connect(ui.utcClockButton,SIGNAL(clicked()),this,SLOT(utcClockButtonClicked()));
    connect(ui.localClockButton,SIGNAL(clicked()),this,SLOT(localClockButtonClicked()));
    utcClockButtonClicked();

    //Does clockTimer need to be a member?
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, SIGNAL(timeout()), this, SLOT(showTime()));
    clockTimer->start(1000);
    showTime();

}

ReceiverWidget::~ReceiverWidget(void)
{
	
}

void ReceiverWidget::mixerChanged(int m)
{
	SetFrequency(loFrequency + m);
}

void ReceiverWidget::mixerChanged(int m, bool b)
{
    setLoMode(b);
    SetFrequency(loFrequency + m);
}

//Filters all nixie events
bool ReceiverWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o->inherits("QLCDNumber")) {
        QLCDNumber *num = (QLCDNumber *) o;

        if (e->type() == QEvent::Enter) {
            num->setFocus();
            //Change something to indicate focus
            return true;
        } else if (e->type() == QEvent::Leave) {
            num->clearFocus();
            return true;
        } else if (e->type() == QEvent::KeyPress) {
            //Direct number entry on nixie
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            int key =  keyEvent->key();
            if (key >= '0' && key <= '9') {
                num->display(key - '0');
                double d = GetNixieNumber();
                SetFrequency(d);
                //Set freq on each digit entry?
                //Or let user keep typing in direct entry fashion, moving focus as necessary?
                return true;
            } else if (key == Qt::Key_Up) {
                int v = qBound(0.0, num->value() + 1, 9.0);
                num->display(v);
                double d = GetNixieNumber();
                SetFrequency(d);
                return true;
            } else if (key == Qt::Key_Down) {
                int v = qBound(0.0, num->value() - 1, 9.0);
                num->display(v);
                double d = GetNixieNumber();
                SetFrequency(d);
                return true;
            }
        } else if (e->type() == QEvent::MouseButtonRelease) {
            //Clicking on a digit sets tuning step and resets lower digits to zero
            if (o == ui.nixie1) {
                tunerStep=1;
            }
            else if (o == ui.nixie10) {
                tunerStep=10;
            }
            else if (o == ui.nixie100) {
                tunerStep=100;
            }
            else if (o == ui.nixie1k) {
                tunerStep=1000;
            }
            else if (o == ui.nixie10k) {
                tunerStep=10000;
            }
            else if (o == ui.nixie100k) {
                tunerStep=100000;
            }
            else if (o == ui.nixie1m) {
                tunerStep=1000000;
            }
            else if (o == ui.nixie10m) {
                tunerStep=10000000;
            }
            else if (o == ui.nixie100m) {
                tunerStep=100000000;
            }
            else if (o == ui.nixie1g) {
                tunerStep=1000000000;
            }

            frequency = (int)(frequency/tunerStep) * tunerStep;
            SetFrequency(frequency);

            return true;
        } else if (e->type() == QEvent::Wheel) {
            //Same logic as in spectrum widget
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(e);

            QPoint angleDelta = wheelEvent->angleDelta();

            if (angleDelta.ry() == 0) {
                if (angleDelta.rx() > 0) {
                    //Scroll Right
                    frequency += 100;
                    SetFrequency(frequency);
                } else if (angleDelta.rx() < 0) {
                    //Scroll Left
                    frequency -= 100;
                    SetFrequency(frequency);

                }
            } else if (angleDelta.rx() == 0) {
                if (angleDelta.ry() > 0) {
                    //Scroll Down
                    frequency -= 1000;
                    SetFrequency(frequency);

                } else if (angleDelta.ry() < 0) {
                    //Scroll Up
                    frequency += 1000;
                    SetFrequency(frequency);
                }
            }
            return true;
        }
    }
	//Handle enter from direct entry freq box
	if (o == ui.directEntry) {
		if (e->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
			//qDebug("Ate key press %d", keyEvent->key());
			if (keyEvent->key() == Qt::Key_Enter ||
				keyEvent->key() == Qt::Key_Return) {
				double freq = QString(ui.directEntry->text()).toDouble();
                if (freq > 0) {
                    //Freq is in kHx
                    freq *= 1000.0;
                    //Direct is always LO mode
                    setLoMode(true);
					SetFrequency(freq);
                    return true;
                }
			}
		}
	}

	return false; //Allow ojects to see event
    //or
    //return QObject::eventFilter(o,e);
}


void ReceiverWidget::SetLimits(double highF,double lowF,int highM,int lowM)
{
	highFrequency = highF;
	lowFrequency = lowF;
	highMixer = highM;
	lowMixer = lowM;
}
//Set frequency by changing LO or Mixer, depending on radio button chosen
//Todo: disable digits that are below step value?
void ReceiverWidget::SetFrequency(double f)
{
	//if low and high are 0 or -1, ignore for now
	if (lowFrequency > 0 && f < lowFrequency) {
        //qApp->beep(); //Doesn't work on mac
        DisplayNixieNumber(frequency); //Restore display
		return;
	}
	if (highFrequency > 0 && f > highFrequency){
        //qApp->beep();
        DisplayNixieNumber(frequency);
		return;
	}

	if (loMode)
	{
		//Ask the receiver if requested freq is within limits and step size
		//Set actual frequency to next higher or lower step
		//Receiver is set with modeOffset, but we keep display at what was actually set
        loFrequency = receiver->SetFrequency(f + modeOffset, frequency ) - modeOffset;
        frequency = loFrequency;
		mixer = 0;
		receiver->SetMixer(mixer);

	} else {
		//Mixer is delta between what's displayed and last LO Frequency
		mixer = f - loFrequency;
		//Check limits
        //mixer = mixer < lowMixer ? lowMixer : (mixer > highMixer ? highMixer : mixer);
        if (mixer < lowMixer || mixer > highMixer) {
            setLoMode(true); //Switch to LO mode, resets mixer etc
            //Set LO to new freq and contine
            loFrequency = receiver->SetFrequency(f + modeOffset, loFrequency );
            frequency = loFrequency;
            //Back to Mixer mode
            setLoMode(false);
        } else {
            //LoFreq not changing, just displayed freq
            frequency = loFrequency + mixer;
            receiver->SetMixer(mixer + modeOffset);
        }
	}
	//frequency is what's displayed, ie combination of loFrequency and mixer (if any)
    DisplayNixieNumber(frequency);
    ui.spectrumWidget->SetMixer(mixer,loFrequency); //Spectrum tracks mixer
    //Update band info
    DisplayBand(frequency);


}
double ReceiverWidget::GetFrequency()
{
	return frequency;
}
void ReceiverWidget::SetMessage(QStringList s)
{
	ui.spectrumWidget->SetMessage(s);
}
void ReceiverWidget::SetMode(DEMODMODE m)
{
	QString text = Demod::ModeToString(m);
	int i = ui.modeBox->findText(text);
	if (i >= 0) {
		ui.modeBox->setCurrentIndex(i);
		mode = m;
		modeSelectionChanged(text);
	}
}
DEMODMODE ReceiverWidget::GetMode()
{
	return mode;
}
//Slots

//Note: Pushbutton text color is changed with a style sheet in deigner
//	QPushButton:on {color:red;}
//
void ReceiverWidget::powerToggled(bool on) 
{
    powerOn = on;

	if (on) {
		if (!receiver->Power(true)) {
			ui.powerButton->setChecked(false); //Turn power button back off
			return; //Error setting up receiver
		}

        //Make sure record button is init properly
        ui.recButton->setEnabled(true);
        ui.recButton->setChecked(false);

        //Presets are only loaded when receiver is on
        presets = receiver->GetPresets();

		//Give SMeter access to signal strength SignalProcessing block
		ui.sMeterWidget->setSignalStrength(receiver->GetSignalStrength());
		ui.spectrumWidget->SetSignalSpectrum(receiver->GetSignalSpectrum());

		ui.spectrumWidget->plotSelectionChanged((SignalSpectrum::DISPLAYMODE)receiver->GetSettings()->lastDisplayMode);
        ui.bandType->setCurrentIndex(Band::HAM);

		ui.spectrumWidget->Run(true);
		ui.sMeterWidget->Run(true);
		setLoMode(true);

	} else {
		//Turning power off, shut down receiver widget display BEFORE telling receiver to clean up
		//Objects

        ui.recButton->setChecked(false);
        ui.recButton->setEnabled(false);

        presets = NULL;

		ui.spectrumWidget->Run(false);
		ui.sMeterWidget->Run(false);
		//We have to make sure that widgets are stopped before cleaning up supporting objects
		receiver->Power(false);

	}
	//Common to on or off
    //See pebble.qss for on/off styles
    ui.directEntry->setProperty("powerOn",powerOn);
    ui.directEntry->setStyleSheet("");

    ui.nixie1->setProperty("powerOn",powerOn);
    ui.nixie1->setStyleSheet(""); //Required to force widget to update it's style sheet based on changed property
    ui.nixie10->setProperty("powerOn",powerOn);
    ui.nixie10->setStyleSheet("");
    ui.nixie100->setProperty("powerOn",powerOn);
    ui.nixie100->setStyleSheet("");
    ui.nixie1k->setProperty("powerOn",powerOn);
    ui.nixie1k->setStyleSheet("");
    ui.nixie10k->setProperty("powerOn",powerOn);
    ui.nixie10k->setStyleSheet("");
    ui.nixie100k->setProperty("powerOn",powerOn);
    ui.nixie100k->setStyleSheet("");
    ui.nixie1m->setProperty("powerOn",powerOn);
    ui.nixie1m->setStyleSheet("");
    ui.nixie10m->setProperty("powerOn",powerOn);
    ui.nixie10m->setStyleSheet("");
    ui.nixie100m->setProperty("powerOn",powerOn);
    ui.nixie100m->setStyleSheet("");
    ui.nixie1g->setProperty("powerOn",powerOn);
    ui.nixie1g->setStyleSheet("");

}

void ReceiverWidget::SetDataMode(int _dataMode)
{
    ui.dataSelectionBox->setCurrentIndex(_dataMode);
}
//Tuning display can drive LO or Mixer depending on selection
void ReceiverWidget::setLoMode(bool b)
{
	loMode = b;
    if (loMode)
	{
        //LO Mode
        ui.loButton->setChecked(true); //Make sure button is toggled if called from presets
        mixer=0;
		receiver->SetMixer(mixer);
		SetFrequency(loFrequency);
	} else {
		//Mixer Mode
        ui.mixButton->setChecked(true);
	}
}
void ReceiverWidget::anfButtonToggled(bool b)
{
	receiver->SetAnfEnabled(b);
}
void ReceiverWidget::nbButtonToggled(bool b)
{
	receiver->SetNbEnabled(b);
}
void ReceiverWidget::nb2ButtonToggled(bool b)
{
	receiver->SetNb2Enabled(b);
}
void ReceiverWidget::agcBoxChanged(int item)
{
	receiver->SetAgcMode((AGC::AGCMODE)item);
}
void ReceiverWidget::lpfButtonToggled(bool b)
{
	receiver->SetLpfEnabled(b);
}
void ReceiverWidget::muteButtonToggled(bool b)
{
    receiver->SetMute(b);
}

void ReceiverWidget::addMemoryButtonClicked()
{
    if (presets == NULL)
        return;

    if (presets->AddMemory(frequency,"????","Added from Pebble"))
        QMessageBox::information(NULL,"Add Memory","Frequency added to memories.csv. Edit file and toggle power to re-load");
    else
        QMessageBox::information(NULL,"Add Memory","Add Memory Failed!");
}

void ReceiverWidget::findStationButtonClicked()
{
    //Search all stations in current band looking for match or close (+/- N  khz)
    //Display in station box, message box, or band info area?
    Station * s = presets->FindStation(frequency,2); // +/- 2khz
    QString str;
    if (s != NULL) {
        str = QString("%1 %2 %3").arg(QString::number(s->freq/1000.0,'f',3), s->station, s->remarks);
        QMessageBox::information(NULL,"Find Station",str);
    }


}

void ReceiverWidget::filterSelectionChanged(QString f)
{
	int filter = f.toInt();
	int lo=0,hi=0;
	switch (mode)
	{
	case dmLSB:
		lo=-filter;
		hi = -50;
		break;
	case dmUSB:
		lo=50;
		hi=filter;
		break;
	case dmAM:
	case dmSAM:
	case dmDSB:
		lo=-filter/2;
		hi=filter/2;
		break;
	case dmCWU:
		lo=50;
		hi=filter;
		break;
	case dmCWL:
		lo=-filter;
		hi= -50;
		break;
	case dmDIGU:
		lo=200;
		hi=filter;
		break;
	case dmDIGL:
		lo=-filter;
		hi= -200;
		break;
    case dmFMMono:
    case dmFMStereo:
	case dmFMN:
		lo=-filter/2;
		hi= filter/2;
		break;
	case dmNONE:
		break;
	}

	ui.spectrumWidget->SetFilter(lo,hi); //So we can display filter around cursor

	receiver->SetFilter(lo,hi);
}

void ReceiverWidget::dataSelectionChanged(int s)
{
    //enums are stored as user data with each menu item
    dataSelection = (DATA_SELECTION)ui.dataSelectionBox->itemData(s).toInt();
    receiver->SetDataSelection(dataSelection);
    if (dataSelection == NO_DATA)
        ui.dataEdit->clear();
}

//Change to DataPower so we can use as real time tuner
void ReceiverWidget::DataBit(bool onOff)
{
    if (onOff)
        ui.dataBar->setValue(100);
    else
        ui.dataBar->setValue(0);
}

void ReceiverWidget::OutputData(const char *d)
{
    switch (dataSelection) {
    case NO_DATA:
        //Display version information, help, etc
        break;
    case BAND_DATA:
        //FM RDS initially
        //No scroll, static display
        ui.dataEdit->clear(); //Look for faster way
        ui.dataEdit->setText(d);
        break;
    case CW_DATA:
        ui.dataEdit->insertPlainText(d); //At cursor
        ui.dataEdit->moveCursor(QTextCursor::End);
        break;
    case RTTY_DATA:
        break;
    }


}

void ReceiverWidget::modeSelectionChanged(QString m) 
{
	mode = Demod::StringToMode(m);
		//Adj to mode, ie we don't want to be exactly on cw, we want to be +/- 200hz
	switch (mode)
	{
	//Todo: Work on this, still not accurately reflecting click
	case dmCWU:
		modeOffset = -700;
		break;
	case dmCWL:
		modeOffset = 700;
		break;
	default:
		modeOffset = 0;
		break;
	}
	//Reset frequency to update any change in modeOffset
	if (frequency !=0)
		//This may get called during power on
		SetFrequency(frequency);

	//Set filter options for this mode
	ui.filterBox->blockSignals(true); //No signals while we're changing box
	ui.filterBox->clear();
	if (mode == dmAM || mode == dmSAM || mode==dmDSB || mode == dmNONE)
	{
		ui.filterBox->addItems(amFilterOptions);
	}
	else if (mode == dmUSB || mode == dmLSB )
	{
		ui.filterBox->addItems(lsbFilterOptions);
	}
	else if (mode == dmCWL || mode == dmCWU)
	{
		ui.filterBox->addItems(cwFilterOptions);
	}
	else if (mode == dmDIGU || mode == dmDIGL)
	{
		ui.filterBox->addItems(diglFilterOptions);
	}
	else if (mode == dmFMN)
	{
		ui.filterBox->addItems(fmFilterOptions);	
	}
    else if (mode == dmFMMono)
	{
		ui.filterBox->addItems(this->wfmFilterOptions);
	}
    else if (mode == dmFMStereo)
    {
        ui.filterBox->addItems(this->wfmFilterOptions);
    }

	ui.spectrumWidget->SetMode(mode);
	receiver->SetMode(mode);
	ui.filterBox->blockSignals(false);
	this->filterSelectionChanged(ui.filterBox->currentText());
}
void ReceiverWidget::SetAgcGainTop(int g)
{
	ui.agcSlider->setValue(g);
}
//Allows receiver to set gain and range
void ReceiverWidget::SetGain(int g, int min, int max)
{
	ui.gainSlider->setMinimum(min);
	ui.gainSlider->setMaximum(max);
	//Set gain slider and let signals do the rest
	ui.gainSlider->setValue(g);
}
void ReceiverWidget::SetSquelch(int s)
{
	ui.squelchSlider->setValue(s);
	squelchSliderChanged(s);
}
void ReceiverWidget::agcSliderChanged(int g)
{
	receiver->SetAgcGainTop(g);
}

void ReceiverWidget::gainSliderChanged(int g) 
{

	gain=g; 
	receiver->SetGain(g);
}
void ReceiverWidget::squelchSliderChanged(int s)
{
	squelch = s;
	receiver->SetSquelch(s);
}
void ReceiverWidget::nixie1UpClicked() {SetFrequency(frequency+1);}
void ReceiverWidget::nixie1DownClicked(){SetFrequency(frequency-1);}
void ReceiverWidget::nixie10UpClicked(){SetFrequency(frequency+10);}
void ReceiverWidget::nixie10DownClicked(){SetFrequency(frequency-10);}
void ReceiverWidget::nixie100UpClicked(){SetFrequency(frequency+100);}
void ReceiverWidget::nixie100DownClicked(){SetFrequency(frequency-100);}
void ReceiverWidget::nixie1kUpClicked(){SetFrequency(frequency+1000);}
void ReceiverWidget::nixie1kDownClicked(){SetFrequency(frequency-1000);}
void ReceiverWidget::nixie10kUpClicked(){SetFrequency(frequency+10000);}
void ReceiverWidget::nixie10kDownClicked(){SetFrequency(frequency-10000);}
void ReceiverWidget::nixie100kUpClicked(){SetFrequency(frequency+100000);}
void ReceiverWidget::nixie100kDownClicked(){SetFrequency(frequency-100000);}
void ReceiverWidget::nixie1mUpClicked(){SetFrequency(frequency+1000000);}
void ReceiverWidget::nixie1mDownClicked(){SetFrequency(frequency-1000000);}
void ReceiverWidget::nixie10mUpClicked(){SetFrequency(frequency+10000000);}
void ReceiverWidget::nixie10mDownClicked(){SetFrequency(frequency-10000000);}
void ReceiverWidget::nixie100mUpClicked(){SetFrequency(frequency+100000000);}
void ReceiverWidget::nixie100mDownClicked(){SetFrequency(frequency-100000000);}
void ReceiverWidget::nixie1gUpClicked(){SetFrequency(frequency+1000000000);}
void ReceiverWidget::nixie1gDownClicked(){SetFrequency(frequency-1000000000);}

void ReceiverWidget::utcClockButtonClicked()
{
   showUtcTime = true;
   ui.utcClockButton->setFlat(false);
   ui.localClockButton->setFlat(true);
   showTime();
}

void ReceiverWidget::localClockButtonClicked()
{
    showUtcTime = false;
    ui.utcClockButton->setFlat(true);
    ui.localClockButton->setFlat(false);
    showTime();
}

void ReceiverWidget::showTime()
{
    //Add Day of the week
    QDateTime time;
    if (showUtcTime)
        time = QDateTime::currentDateTimeUtc();
    else
        time = QDateTime::currentDateTime();
    QString text = time.toString("hh:mm");
    //text = "23:59"; //For testing
    if ((time.time().second() % 2) == 0)
        text[2] = ' ';
    ui.clockWidget->display(text);

}

//Returns the number displayed by all the nixies
double ReceiverWidget::GetNixieNumber()
{
    double v = 0;

    v += ui.nixie1g->value() *   1000000000;
    v += ui.nixie100m->value() * 100000000;
    v += ui.nixie10m->value() *  10000000;
    v += ui.nixie1m->value() *   1000000;
    v += ui.nixie100k->value() * 100000;
    v += ui.nixie10k->value() *  10000;
    v += ui.nixie1k->value() *   1000;
    v += ui.nixie100->value() *  100;
    v += ui.nixie10->value() *   10;
    v += ui.nixie1->value() *    1;

    return v;
}

//Updates all the nixies to display a number
void ReceiverWidget::DisplayNixieNumber(double n)
{
    //Direct entry disply in khz
    ui.directEntry->setText(QString::number(n / 1000.0,'f',3));

	qint32 d = n; //Saves us having to cast to int below to get mod
    ui.nixie1g->display( (d / 1000000000)%10);
    ui.nixie100m->display( (d / 100000000)%10);
	ui.nixie10m->display( (d / 10000000)%10);
	ui.nixie1m->display( (d / 1000000)%10);
	ui.nixie100k->display( (d / 100000)%10);
	ui.nixie10k->display( (d / 10000)%10);
	ui.nixie1k->display( (d / 1000)%10);
	ui.nixie100->display( (d / 100)%10);
	ui.nixie10->display( (d /10) % 10);
    ui.nixie1->display( (d % 10));
}

void ReceiverWidget::bandTypeChanged(int s)
{
    if (!powerOn || presets==NULL)
        return; //bands aren't loaded until power on

    //enums are stored as user data with each menu item
    Band::BANDTYPE type = (Band::BANDTYPE)ui.bandType->itemData(s).toInt();
    //Populate bandCombo with type
    //Turn off signals
    ui.bandCombo->blockSignals(true);
    ui.bandCombo->clear();
    Band *bands = presets->GetBands();
    if (bands == NULL)
        return;

    int numBands = presets->GetNumBands();
    double freq;
    QString buf;
    for (int i=0; i<numBands; i++) {
        if (bands[i].bType == type  || type==Band::ALL) {
            freq = bands[i].tune;
            //If no specific tune freq for band select, use low
            if (freq == 0)
                freq = bands[i].low;
            //Is freq within lo/hi range of SDR?  If not ignore
            if (freq >= lowFrequency && freq < highFrequency) {
                //What a ridiculous way to get to char* from QString for sprintf
                buf.sprintf("%s : %.4f to %.4f",bands[i].name.toUtf8().constData(),bands[i].low/1000000,bands[i].high/1000000);
                ui.bandCombo->addItem(buf,i);
            }
        }
    }
    //Clear selection so we don't show something that doesn't match frequency
    ui.bandCombo->setCurrentIndex(-1);
    //ui.bandCombo->setEditText("Choose a band");
    ui.bandCombo->blockSignals(false);
    ui.stationCombo->blockSignals(true);
    ui.stationCombo->clear();
    ui.stationCombo->blockSignals(false);
}

void ReceiverWidget::bandChanged(int s)
{
    if (!powerOn || s==-1 || presets==NULL) //-1 means we're just clearing selection
        return;

    Band *bands = presets->GetBands();
    if (bands == NULL)
        return;

    int bandIndex = ui.bandCombo->itemData(s).toInt();
    double freq = bands[bandIndex].tune;
    //If no specific tune freq for band select, use low
    if (freq == 0)
        freq = bands[bandIndex].low;
    DEMODMODE mode = bands[bandIndex].mode;

    //Make sure we're in LO mode
    setLoMode(true);

    SetFrequency(freq);
    SetMode(mode);

}

void ReceiverWidget::stationChanged(int s)
{
    if (!powerOn || presets==NULL)
        return;
    Station *stations = presets->GetStations();
    int stationIndex = ui.stationCombo->itemData(s).toInt();
    double freq = stations[stationIndex].freq;
    SetFrequency(freq);
    //SetMode(mode);
}

void ReceiverWidget::DisplayBand(double freq)
{
    if (!powerOn || presets==NULL)
        return;

    Band *band = presets->FindBand(freq);
    if (band != NULL) {
        //Set bandType to match band, this will trigger bandChanged to load bands for type
        ui.bandType->setCurrentIndex(band->bType);

        //Now matching band
        int index = ui.bandCombo->findData(band->bandIndex);
        ui.bandCombo->blockSignals(true); //Just select band to display, don't reset freq to band
        ui.bandCombo->setCurrentIndex(index);
        ui.bandCombo->blockSignals(false);

        //If band has changed, update station list to match band
        if (currentBandIndex != band->bandIndex) {
            //List of stations per band should be setup when we read eibi.csv
            ui.stationCombo->blockSignals(true); //Don't trigger stationChanged() every addItem
            ui.stationCombo->clear();
            int stationIndex;
            Station *stations = presets->GetStations();
            Station station;
            QString str;
            QString lastStation;
            for (int i=0; i<band->numStations; i++) {
                stationIndex = band->stations[i];
                station = stations[stationIndex];
                //Ignore duplicate stations that just have start/stop differences
                if (station.station != lastStation) {
                    str = QString("%1 %2KHz").arg(station.station).arg(station.freq/1000);
                    ui.stationCombo->addItem(str,stationIndex);
                    lastStation = station.station;
                }
            }
            ui.stationCombo->setCurrentIndex(-1);
            ui.stationCombo->blockSignals(false);
        }

        //Done, update currentBandIndex so we can see if it changed next time
        currentBandIndex = band->bandIndex;

    } else {
        ui.bandCombo->setCurrentIndex(-1); //no band selected
        ui.stationCombo->clear(); //No stations
        currentBandIndex = -1;
    }
}
