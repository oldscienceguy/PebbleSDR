//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include <QPalette>
#include <QMenu>
#include <QKeyEvent>
#include <QMessageBox>
#include <QLCDNumber>
#include "receiverwidget.h"
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
    agcModes << "FAST" << "MED" << "SLOW" << "LONG" << "OFF";
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

    ui.filterBox->addItems(Demod::demodInfo[dmAM].filters); //Default
    ui.filterBox->setCurrentIndex(Demod::demodInfo[dmAM].defaultFilter);

    ui.dataSelectionBox->addItem("No Data",NO_DATA);
    ui.dataSelectionBox->addItem("Band Data",BAND_DATA);
    foreach (QString name,receiver->getPluginNames())
        ui.dataSelectionBox->addItem(name,PLUGIN_DATA);

    connect(ui.dataSelectionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(dataSelectionChanged(int)));
    SetDataMode((NO_DATA));

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

    ui.squelchSlider->setMinimum(global->minDb);
    ui.squelchSlider->setMaximum(global->maxDb);
    ui.squelchSlider->setValue(global->minDb);
    connect(ui.squelchSlider,SIGNAL(valueChanged(int)),this,SLOT(squelchSliderChanged(int)));

    currentBandIndex = -1;

	connect(ui.loButton,SIGNAL(toggled(bool)),this,SLOT(setLoMode(bool)));
    ui.powerButton->setCheckable(true); //Make it a toggle button
    ui.recButton->setCheckable(true);
    //ui.sdrOptions->setCheckable(true);
    connect(ui.powerButton,SIGNAL(toggled(bool)),this,SLOT(powerToggled(bool)));
    connect(ui.recButton,SIGNAL(toggled(bool)),receiver,SLOT(RecToggled(bool)));
    connect(ui.sdrOptions,SIGNAL(pressed()),receiver,SLOT(SdrOptionsPressed()));

    connect(ui.anfButton,SIGNAL(toggled(bool)),this,SLOT(anfButtonToggled(bool)));
	connect(ui.nbButton,SIGNAL(toggled(bool)),this,SLOT(nbButtonToggled(bool)));
	connect(ui.nb2Button,SIGNAL(toggled(bool)),this,SLOT(nb2ButtonToggled(bool)));
	connect(ui.agcBox,SIGNAL(currentIndexChanged(int)),this,SLOT(agcBoxChanged(int)));
	connect(ui.muteButton,SIGNAL(toggled(bool)),this,SLOT(muteButtonToggled(bool)));
	connect(ui.modeBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(modeSelectionChanged(QString)));
	connect(ui.filterBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(filterSelectionChanged(QString)));
	connect(ui.gainSlider,SIGNAL(valueChanged(int)),this,SLOT(gainSliderChanged(int)));
	connect(ui.agcSlider,SIGNAL(valueChanged(int)),this,SLOT(agcSliderChanged(int)));
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

    ui.recButton->setEnabled(false);

    //Clock
    //Todo: Match bold black look of nixie
    QPalette palette;
    palette.setColor(QPalette::WindowText,Qt::black);
    ui.clockWidget->setPalette(palette);
    ui.clockWidget->setSegmentStyle(QLCDNumber::Flat);

    connect(ui.utcClockButton,SIGNAL(clicked()),this,SLOT(utcClockButtonClicked()));
    connect(ui.localClockButton,SIGNAL(clicked()),this,SLOT(localClockButtonClicked()));
    utcClockButtonClicked();

    //Does clockTimer need to be a member?
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, SIGNAL(timeout()), this, SLOT(showTime()));
    clockTimer->start(1000);
    showTime();

    QComboBox *sdrSelector = ui.sdrSelector;
    sdrSelector->addItem("SR Ensemble",SDR::SR_ENSEMBLE);
    sdrSelector->addItem("SR Ensemble 2M",SDR::SR_ENSEMBLE_2M);
    sdrSelector->addItem("SR Ensemble 4M",SDR::SR_ENSEMBLE_4M);
    sdrSelector->addItem("SR Ensemble 6M",SDR::SR_ENSEMBLE_6M);
    sdrSelector->addItem("SR V9-ABPF",SDR::SR_V9);
    sdrSelector->addItem("SR LITE II",SDR::SR_LITE);
    sdrSelector->addItem("FiFi",SDR::FiFi);
    sdrSelector->addItem("Elektor SDR",SDR::ELEKTOR);
    sdrSelector->addItem("RFSpace SDR-IQ",SDR::SDR_IQ_USB);
    sdrSelector->addItem("RFSpace SDR-IP",SDR::SDR_IP_TCP);
    sdrSelector->addItem("HPSDR USB",SDR::HPSDR_USB);
    //sdrSelector->addItem("HPSDR TCP",SDR::HPSDR_TCP);
    sdrSelector->addItem("FUNcube Pro",SDR::FUNCUBE);
    sdrSelector->addItem("FUNcube Pro+",SDR::FUNCUBE_PLUS);
    sdrSelector->addItem("File",SDR::FILE);
    sdrSelector->addItem("RTL2832 Family",SDR::DVB_T);

    int cur = sdrSelector->findData(global->settings->sdrDevice);
    sdrSelector->setCurrentIndex(cur);
    connect(sdrSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(ReceiverChanged(int)));

    //Widget must have a parent
    directInputWidget = new QWidget(this->window(), Qt::Popup);
    directInputUi = new Ui::DirectInput;
    directInputUi->setupUi(directInputWidget);
    //Direct input is modal to avoid any confusion with other UI
    //directInputWidget->setWindowModality(Qt::WindowModal);
    //directInputUi->directEntry->setInputMask("0000000"); //All digits, none required
    connect(directInputUi->directEntry,SIGNAL(returnPressed()),this,SLOT(directEntryAccepted()));
    connect(directInputUi->enterButton,SIGNAL(clicked()),this,SLOT(directEntryAccepted()));
    connect(directInputUi->cancelButton,SIGNAL(clicked()),this,SLOT(directEntryCanceled()));

    ui.dataFrame->setVisible(false);

    connect(ui.spectrumWidget,SIGNAL(mixerLimitsChanged(int,int)),this,SLOT(setMixerLimits(int,int)));

}

ReceiverWidget::~ReceiverWidget(void)
{
	
}

void ReceiverWidget::showDataFrame(bool b)
{
    ui.dataFrame->setVisible(b);
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

//User hit enter, esc
void ReceiverWidget::directEntryAccepted()
{
    double freq = QString(directInputUi->directEntry->text()).toDouble();
    if (freq > 0) {
        //Freq is in kHx
        freq *= 1000.0;
        //Direct is always LO mode
        setLoMode(true);
        SetFrequency(freq);
    }

    directInputWidget->close();
}

void ReceiverWidget::directEntryCanceled()
{
    directInputWidget->close();
}

//Filters all nixie events
bool ReceiverWidget::eventFilter(QObject *o, QEvent *e)
{
    static int scrollCounter = 0; //Used to slow down and smooth freq changes from scroll wheel
    static const int smoothing = 10;

    if (!powerOn)
        return false;

    //If the event (any type) is in a nixie we may need to know which one
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
                QPoint pt = this->window()->pos();
                directInputWidget->move(pt.x()+50,pt.y()+35);
                directInputWidget->show();
                directInputUi->directEntry->setFocus();
                directInputUi->directEntry->setText(QString::number(key-'0'));
                //Modal direct entry window is now active until user completes edit with Enter
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
            frequency = (int)(frequency/tunerStep) * tunerStep;
            SetFrequency(frequency);

            return true;
        } else if (e->type() == QEvent::Wheel) {
            //Same logic as in spectrum widget
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(e);

            QPoint angleDelta = wheelEvent->angleDelta();

            if (angleDelta.ry() == 0) {
                //Left-Right scrolling Same as up-down for now
                if ((angleDelta.rx() > 0) && (++scrollCounter > smoothing)) {
                    //Scroll Right
                    scrollCounter = 0;
                    frequency += tunerStep;
                    SetFrequency(frequency);
                } else if ((angleDelta.rx() < 0) && (++scrollCounter > smoothing)) {
                    //Scroll Left
                    scrollCounter = 0;
                    frequency -= tunerStep;
                    SetFrequency(frequency);

                }
            } else if (angleDelta.rx() == 0) {
                //Up-down scrolling
                if ((angleDelta.ry() > 0) && (++scrollCounter > smoothing)) {
                    //Scroll Down
                    scrollCounter = 0;
                    frequency -= tunerStep;
                    SetFrequency(frequency);
                } else if ((angleDelta.ry() < 0) && (++scrollCounter > smoothing)) {
                    //Scroll Up
                    scrollCounter = 0;
                    frequency += tunerStep;
                    SetFrequency(frequency);
                }
            }
            return true;
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

void ReceiverWidget::setMixerLimits(int highM, int lowM)
{
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
        loFrequency = receiver->SetFrequency(f, frequency );
        frequency = loFrequency;
		mixer = 0;
        //Mixer is actually set including modeOffset so we hear tone, but display shows actual freq
        receiver->SetMixer(mixer + modeOffset);

	} else {
		//Mixer is delta between what's displayed and last LO Frequency
		mixer = f - loFrequency;
		//Check limits
        //mixer = mixer < lowMixer ? lowMixer : (mixer > highMixer ? highMixer : mixer);
        if (mixer < lowMixer || mixer > highMixer) {
            setLoMode(true); //Switch to LO mode, resets mixer etc
            //Set LO to new freq and contine
            loFrequency = receiver->SetFrequency(f, loFrequency );
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

	if (on) {
        powerOn = true;
        if (!receiver->Power(true)) {
			ui.powerButton->setChecked(false); //Turn power button back off
			return; //Error setting up receiver
		}

        //Set squelch to default

        //Don't allow SDR changes when receiver is on
        ui.sdrSelector->setEnabled(false);

        //Make sure record button is init properly
        ui.recButton->setEnabled(true);
        ui.recButton->setChecked(false);

        //Presets are only loaded when receiver is on
        presets = receiver->GetPresets();

		//Give SMeter access to signal strength SignalProcessing block
		ui.sMeterWidget->setSignalStrength(receiver->GetSignalStrength());
        ui.sMeterWidget->SetSignalSpectrum(receiver->GetSignalSpectrum());
        ui.spectrumWidget->SetSignalSpectrum(receiver->GetSignalSpectrum());

        ui.spectrumWidget->plotSelectionChanged((SignalSpectrum::DISPLAYMODE)global->sdr->lastDisplayMode);
        ui.bandType->setCurrentIndex(Band::HAM);

		ui.spectrumWidget->Run(true);
        ui.sMeterWidget->start();
		setLoMode(true);

	} else {
		//Turning power off, shut down receiver widget display BEFORE telling receiver to clean up
		//Objects
        //Make sure we reset data frame
        SetDataMode(NO_DATA);
        powerOn = false;

        //Don't allow SDR changes when receiver is on
        ui.sdrSelector->setEnabled(true);

        //Make sure pwr and rec buttons are off
        ui.powerButton->setChecked(false);
        ui.recButton->setChecked(false);
        ui.recButton->setEnabled(false);

        presets = NULL;

		ui.spectrumWidget->Run(false);
        ui.sMeterWidget->stop();
		//We have to make sure that widgets are stopped before cleaning up supporting objects
		receiver->Power(false);

	}
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
        receiver->SetMixer(mixer + modeOffset);
		SetFrequency(loFrequency);
	} else {
		//Mixer Mode
        ui.mixButton->setChecked(true);
	}
}
void ReceiverWidget::anfButtonToggled(bool b)
{
    if (!powerOn)
        return;

	receiver->SetAnfEnabled(b);
}
void ReceiverWidget::nbButtonToggled(bool b)
{
    if (!powerOn)
        return;
	receiver->SetNbEnabled(b);
}
void ReceiverWidget::nb2ButtonToggled(bool b)
{
    if (!powerOn)
        return;
	receiver->SetNb2Enabled(b);
}
void ReceiverWidget::agcBoxChanged(int item)
{
    if (!powerOn)
        return;
	receiver->SetAgcMode((AGC::AGCMODE)item);
}
void ReceiverWidget::muteButtonToggled(bool b)
{
    if (!powerOn)
        return;
    receiver->SetMute(b);
}

void ReceiverWidget::addMemoryButtonClicked()
{
    if (!powerOn)
        return;

    if (presets == NULL)
        return;

    if (presets->AddMemory(frequency,"????","Added from Pebble"))
        QMessageBox::information(NULL,"Add Memory","Frequency added to memories.csv. Edit file and toggle power to re-load");
    else
        QMessageBox::information(NULL,"Add Memory","Add Memory Failed!");
}

void ReceiverWidget::findStationButtonClicked()
{
    if (!powerOn)
        return;

    quint16 range = 25;
    //Search all stations in current band looking for match or close (+/- N  khz)
    //Display in station box, message box, or band info area?
    QList<Station> stationList = presets->FindStation(frequency,range); // +/- khz
    QString str;
    if (!stationList.empty()) {
        Station s;
        for (int i=0; i<stationList.count(); i++) {
            s = stationList[i];
            str += QString("%1 %2 %3\n").arg(QString::number(s.freq/1000.0,'f',3), s.station, s.remarks);
        }
        receiver->getDemod()->OutputBandData("", "", QString().sprintf("Stations within %d kHz",range), str);
    }


}

/*
8/1/13 Review these settings which were done in Pebble toddler years
CuteSDR defaults are always centered on 0
AM: -5k +5k
SAM -5k +5k
FM: -5k +5k
WFM: -100k +100k
USB: 0 5k
LSB -5k 0
CWU: -1k 1k
CWL: -1k 1k
*/
void ReceiverWidget::filterSelectionChanged(QString f)
{
	int filter = f.toInt();
	int lo=0,hi=0;
	switch (mode)
	{
	case dmLSB:
		lo=-filter;
        hi = 0;
		break;
	case dmUSB:
        lo=0;
		hi=filter;
		break;
	case dmAM:
	case dmSAM:
	case dmDSB:
		lo=-filter/2;
		hi=filter/2;
		break;
    /*
     * This gets very confusing, so for the record ...
     * Let F = freq of actual CW carrier.  This is what we want to see on freq and spectrum Display
     * But we actuall want to hear a tone of 'modeOffset' or 800hz
     * If mode is CWU we expect to tune from low to high, so we subtact 800hz to set device/mixer frequency
     * So lets say F=10k and Pebble shows 10k.  Device will be set to 9200 and we hear 800hz tone
     * Tuning higher decreases tone, tuning lower increases
     *
     * If mode is CWL, tuneing higher increases tone, tuning lower decreases
     * This is all taken care of when we set the mixer frequency by offsetting modemOffset
     *
     * So far so good, but now we have to make sure our filters always keep the freq including offset in band
     * To test this we should be able to go from 1200 to 250 hz filter without losing 1k audio tone.
     * We want to put our 800hz tone in the middle of the filter
     * So we take the filter width and add 1/2 it to the modeOffset and 1/2 to the filter itself
     *
    */
    case dmCWU:
        //modeOffset = -1000 (so we can use it directly in other places without comparing cwu and cwl)
        //We want 500hz filter to run from 750 to 1250 so modeOffset is right in the middle
        lo = -modeOffset - (filter/2); // --1000 - 250 = 750
        hi = -modeOffset + (filter/2); // --1000 + 250 = 1250
		break;
	case dmCWL:
        //modeOffset = +1000 (default)
        //We want 500hz filter to run from -1250 to -750 so modeOffset is right in the middle
        lo = -modeOffset - (filter/2); // - +1000 - 250 = -1250
        hi = -modeOffset + (filter/2); // +1000 + 250 = -750
        break;
    //Same as CW but with no offset tone, drop if not needed
	case dmDIGU:
        lo=0;
		hi=filter;
		break;
	case dmDIGL:
		lo=-filter;
        hi= 0;
		break;
    case dmFMM:
    case dmFMS:
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
    if (!powerOn)
        return;

    //Clear any previous data selection
    switch(dataSelection) {
        case PLUGIN_DATA:

            //Reset decoder
            receiver->SetDigitalModem(NULL,NULL);
            //Delete all children
            foreach (QObject *obj, ui.dataFrame->children()) {
                //Normally we get a grid layout object, uiFrame, dataFrame
                delete obj;
            }
            break;
        case BAND_DATA:
            receiver->getDemod()->SetupDataUi(NULL);
            //Delete all children
            foreach (QObject *obj, ui.dataFrame->children()) {
                //Normally we get a grid layout object, uiFrame, dataFrame
                delete obj;
            }
            break;
        default:
            break;
    }

    //enums are stored as user data with each menu item
    dataSelection = (DATA_SELECTION)ui.dataSelectionBox->itemData(s).toInt();

    switch (dataSelection) {
        case NO_DATA:
            ui.dataFrame->setVisible(false);
            break;
        case BAND_DATA:
            receiver->getDemod()->SetupDataUi(ui.dataFrame);
            ui.dataFrame->setVisible(true);
            break;
        case PLUGIN_DATA:
            receiver->SetDigitalModem(ui.dataSelectionBox->currentText(), ui.dataFrame);
            ui.dataFrame->setVisible(true);
            break;
        default:
            //Todo, delete any previous active dataUI
            ui.dataFrame->setVisible(false);
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
        //Subtract modeOffset from actual freq so we hear upper tone
        modeOffset = -global->settings->modeOffset; //Same as CW decoder
		break;
	case dmCWL:
        //Add modeOffset to actual tuned freq so we hear lower tone
        modeOffset = global->settings->modeOffset;
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
    ui.filterBox->addItems(Demod::demodInfo[mode].filters);
    ui.filterBox->setCurrentIndex(Demod::demodInfo[mode].defaultFilter);

	ui.spectrumWidget->SetMode(mode);
	receiver->SetMode(mode);
	ui.filterBox->blockSignals(false);
	this->filterSelectionChanged(ui.filterBox->currentText());
}
void ReceiverWidget::SetDisplayedAgcThreshold(int g)
{
	ui.agcSlider->setValue(g);
}
//Allows receiver to set gain and range
void ReceiverWidget::SetDisplayedGain(int g, int min, int max)
{
	ui.gainSlider->setMinimum(min);
	ui.gainSlider->setMaximum(max);
	//Set gain slider and let signals do the rest
	ui.gainSlider->setValue(g);
}
void ReceiverWidget::SetDisplayedSquelch(int s)
{
	ui.squelchSlider->setValue(s);
	squelchSliderChanged(s);
}
void ReceiverWidget::agcSliderChanged(int g)
{
    if (!powerOn)
        return;
    receiver->SetAgcThreshold(g);
}

void ReceiverWidget::gainSliderChanged(int g) 
{
    if (!powerOn)
        return;
	gain=g; 
	receiver->SetGain(g);
}
void ReceiverWidget::squelchSliderChanged(int s)
{
    if (!powerOn)
        return;
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

void ReceiverWidget::ReceiverChanged(int i)
{
    //Power is off when this is called
    int cur = ui.sdrSelector->currentIndex();
    global->settings->sdrDevice = (SDR::SDRDEVICE)ui.sdrSelector->itemData(cur).toInt();
    //Close the sdr option window if open
    receiver->CloseSdrOptions();
}

//Updates all the nixies to display a number
void ReceiverWidget::DisplayNixieNumber(double n)
{
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
