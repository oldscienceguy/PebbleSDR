//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QPalette>
#include <QMenu>
#include <QKeyEvent>
#include "ReceiverWidget.h"
#include "receiver.h"

ReceiverWidget::ReceiverWidget(QWidget *parent):QWidget(parent)
{
	//Delegates to QtDesigner managed ui for actual components
	ui.setupUi(this);
}
//This does the actual setup and connections to receiver class
void ReceiverWidget::SetReceiver(Receiver *r)
{
	receiver = r;

	powerOn = false;
	frequency = 0;

    QFont smFont = receiver->GetSettings()->smFont;
    QFont medFont = receiver->GetSettings()->medFont;
    QFont lgFont = receiver->GetSettings()->lgFont;

	QStringList modes;
	modes << "AM"<<"SAM"<<"FMN"<<"FMW"<<"DSB"<<"LSB"<<"USB"<<"CWL"<<"CWU"<<"DIGL"<<"DIGU"<<"NONE";
	ui.modeBox->addItems(modes);

	QStringList agcModes;
	agcModes << "OFF" << "SLOW" << "MED" << "FAST" << "LONG";
	ui.agcBox->addItems(agcModes);

	QMenu *settingsMenu = new QMenu();
	//Shortcut, does connect(...triggered(bool) ...)
	settingsMenu->addAction("General",receiver,SLOT(ShowSettings(bool)));
	settingsMenu->addAction("SDR Device",receiver,SLOT(ShowSdrSettings(bool)));
	settingsMenu->addAction("IQ Balance",receiver,SLOT(ShowIQBalance(bool)));
    settingsMenu->setFont(medFont);
	ui.settingsButton->setMenu(settingsMenu);

	//Pulled from SpoectrumWidget to allow us to display other data in same area
	//Station Information
	//Presets
	// etc
	ui.displayBox->addItem("Spectrum"); 
	ui.displayBox->addItem("Waterfall");
	ui.displayBox->addItem("I/Q");
	ui.displayBox->addItem("Phase");
	ui.displayBox->addItem("Off");
	//Testing, may not leave these in live product
	ui.displayBox->addItem("Post Mixer");
	ui.displayBox->addItem("Post BandPass");

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
	wfmFilterOptions << "80000"<<"40000";
	ui.filterBox->addItems(amFilterOptions); //Default

    ui.dataSelectionBox->addItem("Off",NO_DATA);
    ui.dataSelectionBox->addItem("Band",BAND_DATA);
    ui.dataSelectionBox->addItem("CW",CW_DATA);
    ui.dataSelectionBox->addItem("RTTY",RTTY_DATA);
    ui.dataSelectionBox->setFont(medFont);
    connect(ui.dataSelectionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(dataSelectionChanged(int)));


	loMode = true;
	//Set intial gain slider position
	ui.gainSlider->setValue(50);
	tunerStep=1000;

	modeOffset = 0;

	connect(ui.loButton,SIGNAL(toggled(bool)),this,SLOT(setLoMode(bool)));
	connect(ui.powerButton,SIGNAL(toggled(bool)),this,SLOT(powerToggled(bool)));
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
	connect(ui.presetsButton,SIGNAL(clicked(bool)),this,SLOT(presetsClicked(bool)));
	connect(ui.displayBox,SIGNAL(currentIndexChanged(int)),this,SLOT(displayChanged(int)));

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

	ui.directEntry->setInputMask("000000000"); //All digits, none required

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
    ui.presetsButton->setFont(medFont);
    ui.settingsButton->setFont(medFont);
    ui.squelchSlider->setFont(smFont);
    ui.displayBox->setFont(medFont);
    ui.filterBox->setFont(medFont);
}

ReceiverWidget::~ReceiverWidget(void)
{
	
}

void ReceiverWidget::displayChanged(int item)
{
	switch (item)
	{
	case 0:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::SPECTRUM);
		break;
	case 1:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::WATERFALL);
		break;
	case 2:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::IQ);
		break;
	case 3:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::PHASE);
		break;
	case 4:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::NODISPLAY);
		break;
	case 5:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::POSTMIXER);
		break;
	case 6:
		ui.spectrumWidget->plotSelectionChanged(SignalSpectrum::POSTBANDPASS);
		break;
    }
}

void ReceiverWidget::mixerChanged(int m)
{
	SetFrequency(loFrequency + m);
}
//Filters all nixie events
bool ReceiverWidget::eventFilter(QObject *o, QEvent *e)
{
	//Handle enter from direct entry freq box
	if (o == ui.directEntry) {
		if (e->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
			//qDebug("Ate key press %d", keyEvent->key());
			if (keyEvent->key() == Qt::Key_Enter ||
				keyEvent->key() == Qt::Key_Return) {
				double freq = QString(ui.directEntry->text()).toDouble();
				if (freq > 0)
					SetFrequency(freq);
			}
			//return true; //Eat event
		}
		return false; //Don't consume event
	}
	//Clicking on a digit sets tuning step and resets lower digits to zero
	if (e->type() == QEvent::MouseButtonRelease)
	{
		if (o == ui.nixie1) {
			tunerStep=1;
		}
		else if (o == ui.nixie10) {
			tunerStep=10;
			frequency = (int)(frequency/10)*10;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie100) {
			tunerStep=100;
			frequency = (int)(frequency/100)*100;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie1k) {
			tunerStep=1000;
			frequency = (int)(frequency/1000)*1000;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie10k) {
			tunerStep=10000;
			frequency = (int)(frequency/10000)*10000;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie100k) {
			tunerStep=100000;
			frequency = (int)(frequency/100000)*100000;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie1m) {
			tunerStep=1000000;
			frequency = (int)(frequency/1000000)*1000000;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie10m) {
			tunerStep=10000000;
			frequency = (int)(frequency/10000000)*10000000;
			SetFrequency(frequency);
		}
		else if (o == ui.nixie100m) {
			tunerStep=100000000;
			frequency = (int)(frequency/100000000)*100000000;
			SetFrequency(frequency);
		}
        else if (o == ui.nixie1g) {
            tunerStep=1000000000;
            frequency = (int)(frequency/1000000000)*1000000000;
            SetFrequency(frequency);
        }

	}
	
	return false; //Allow ojects to see event
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
		//Beep or do something
		return;
	}
	if (highFrequency > 0 && f > highFrequency){
		return;
	}
	double newFreq;

	if (loMode)
	{
		//Ask the receiver if requested freq is within limits and step size
		//Set actual frequency to next higher or lower step
		//Receiver is set with modeOffset, but we keep display at what was actually set
		newFreq = receiver->SetFrequency(f + modeOffset, frequency ) - modeOffset;
		loFrequency=newFreq;
		mixer = 0;
		receiver->SetMixer(mixer);
		ui.spectrumWidget->SetMixer(mixer,loFrequency); //Spectrum tracks mixer

	} else {
		//Mixer is delta between what's displayed and last LO Frequency
		mixer = f - loFrequency;
		//Check limits
		mixer = mixer < lowMixer ? lowMixer : (mixer > highMixer ? highMixer : mixer);
		newFreq=loFrequency+mixer;
		receiver->SetMixer(mixer + modeOffset);
		ui.spectrumWidget->SetMixer(mixer,loFrequency); //Spectrum tracks mixer
	}
	//frequency is what's displayed, ie combination of loFrequency and mixer (if any)
	frequency=newFreq;
	DisplayNumber(frequency);
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
	QString pwrStyle = "";
	if (on) {
		if (!receiver->Power(true)) {
			ui.powerButton->setChecked(false); //Turn power button back off
			return; //Error setting up receiver
		}

		//Create style in Qt Designer style editor, then paste here
		pwrStyle = "background-color: qlineargradient(spread:pad, x1:0.471, y1:0, x2:0.483, y2:0.982955, stop:0 rgba(255, 243, 72, 255), stop:0.778409 rgba(255, 247, 221, 255))";
		//Give SMeter access to signal strength SignalProcessing block
		ui.sMeterWidget->setSignalStrength(receiver->GetSignalStrength());
		ui.spectrumWidget->SetSignalSpectrum(receiver->GetSignalSpectrum());

		ui.displayBox->setCurrentIndex(receiver->GetSettings()->lastDisplayMode); //Initial display mode
		ui.spectrumWidget->plotSelectionChanged((SignalSpectrum::DISPLAYMODE)receiver->GetSettings()->lastDisplayMode);

		ui.spectrumWidget->Run(true);
		ui.sMeterWidget->Run(true);
		setLoMode(true);

	} else {
		//Turning power off, shut down receiver widget display BEFORE telling receiver to clean up
		//Objects
		ui.spectrumWidget->Run(false);
		ui.sMeterWidget->Run(false);
		//We have to make sure that widgets are stopped before cleaning up supporting objects
		receiver->Power(false);

	}
	//Common to on or off

	ui.directEntry->setStyleSheet(pwrStyle);

	ui.nixie1->setStyleSheet(pwrStyle);
	ui.nixie1->setEnabled(on);
	ui.nixie10->setStyleSheet(pwrStyle);
	ui.nixie10->setEnabled(on);
	ui.nixie100->setStyleSheet(pwrStyle);
	ui.nixie100->setEnabled(on);
	ui.nixie1k->setStyleSheet(pwrStyle);
	ui.nixie1k->setEnabled(on);
	ui.nixie10k->setStyleSheet(pwrStyle);
	ui.nixie10k->setEnabled(on);
	ui.nixie100k->setStyleSheet(pwrStyle);
	ui.nixie100k->setEnabled(on);
	ui.nixie1m->setStyleSheet(pwrStyle);
	ui.nixie1m->setEnabled(on);
	ui.nixie10m->setStyleSheet(pwrStyle);
	ui.nixie10m->setEnabled(on);
    ui.nixie100m->setStyleSheet(pwrStyle);
	ui.nixie100m->setEnabled(on);
    ui.nixie1g->setStyleSheet(pwrStyle);
    ui.nixie1g->setEnabled(on);

	powerOn = on;
}
void ReceiverWidget::SetDisplayMode(int dm)
{
	ui.displayBox->setCurrentIndex(dm); 
	ui.spectrumWidget->plotSelectionChanged((SignalSpectrum::DISPLAYMODE)dm);
}
int ReceiverWidget::GetDisplayMode()
{
	return ui.displayBox->currentIndex();
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
	case dmFMW:
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

void ReceiverWidget::OutputData(char *d)
{
    switch (dataSelection) {
    case NO_DATA:
        break;
    case BAND_DATA:
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
	else if (mode == dmFMW)
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

void ReceiverWidget::DisplayNumber(double n)
{
	ui.directEntry->setText(QString::number(n,'f',0));

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


void ReceiverWidget::presetsClicked(bool b)
{
	receiver->ShowPresets();
}
