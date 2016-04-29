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
#include <QWindow>

ReceiverWidget::ReceiverWidget(QWidget *parent):QWidget(parent)
{
	//Delegates to QtDesigner managed ui for actual components
	ui.setupUi(this);
}
//This does the actual setup and connections to receiver class
void ReceiverWidget::setReceiver(Receiver *r)
{
	m_receiver = r;
    m_presets = NULL;

	m_powerOn = false;
	m_frequency = 0;

	m_slaveMode = false;

	QFont smFont = m_receiver->getSettings()->m_smFont;
	QFont medFont = m_receiver->getSettings()->m_medFont;
	QFont lgFont = m_receiver->getSettings()->m_lgFont;

	QStringList modes;
    modes << "AM"<<"SAM"<<"FMN"<<"FM-Mono"<<"FM-Stereo"<<"DSB"<<"LSB"<<"USB"<<"CWL"<<"CWU"<<"DIGL"<<"DIGU"<<"NONE";
	ui.modeBox->addItems(modes);

	ui.agcBox->addItem("Off",AGC::OFF);
	ui.agcBox->addItem("Fast",AGC::FAST);
	ui.agcBox->addItem("Med",AGC::MED);
	ui.agcBox->addItem("Slow",AGC::SLOW);
	ui.agcBox->addItem("Long",AGC::LONG);

#if 0
	QMenu *settingsMenu = new QMenu();
	//Shortcut, does connect(...triggered(bool) ...)
	settingsMenu->addAction("General",receiver,SLOT(ShowSettings(bool)));
	settingsMenu->addAction("SDR Device",receiver,SLOT(ShowSdrSettings(bool)));
	settingsMenu->addAction("IQ Balance",receiver,SLOT(ShowIQBalance(bool)));
    settingsMenu->setFont(medFont);
	ui.settingsButton->setMenu(settingsMenu);
#endif

	ui.filterBox->addItems(Demod::demodInfo[DeviceInterface::dmAM].filters); //Default
	ui.filterBox->setCurrentIndex(Demod::demodInfo[DeviceInterface::dmAM].defaultFilter);

    QVariant v;
    foreach (PluginInfo p, m_receiver->getModemPluginInfo()) {
        v.setValue(p);
        ui.dataSelectionBox->addItem(p.name, v);
    }

    connect(ui.dataSelectionBox,SIGNAL(currentIndexChanged(int)),this,SLOT(dataSelectionChanged(int)));
    setDataMode(0);

	m_loMode = true;
	ui.gainSlider->setValue(0);
	m_tunerStep=1000;

	m_modeOffset = 0;

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

	//10 ticks per db
	m_squelchDb = DB::minDb;
	ui.squelchSlider->setMinimum(m_squelchDb * m_squelchDbRes); //-120db
	//Squelch values above some limit don't make sense, try 30 and see how it works
	ui.squelchSlider->setMaximum(-300); //-30db
	ui.squelchSlider->setSingleStep(1);
	ui.squelchSlider->setValue(m_squelchDb * m_squelchDbRes);
    connect(ui.squelchSlider,SIGNAL(valueChanged(int)),this,SLOT(squelchSliderChanged(int)));

    m_currentBandIndex = -1;

	connect(ui.loButton,SIGNAL(toggled(bool)),this,SLOT(setLoMode(bool)));
    ui.powerButton->setCheckable(true); //Make it a toggle button
    ui.recButton->setCheckable(true);
    //ui.sdrOptions->setCheckable(true);
    connect(ui.powerButton,SIGNAL(toggled(bool)),this,SLOT(powerToggled(bool)));
	connect(ui.recButton,SIGNAL(toggled(bool)),m_receiver,SLOT(recToggled(bool)));
	connect(ui.sdrOptions,SIGNAL(pressed()),m_receiver,SLOT(sdrOptionsPressed()));

    connect(ui.anfButton,SIGNAL(toggled(bool)),this,SLOT(anfButtonToggled(bool)));
	connect(ui.nbButton,SIGNAL(toggled(bool)),this,SLOT(nbButtonToggled(bool)));
	connect(ui.nb2Button,SIGNAL(toggled(bool)),this,SLOT(nb2ButtonToggled(bool)));
	connect(ui.agcBox,SIGNAL(currentIndexChanged(int)),this,SLOT(agcBoxChanged(int)));
	connect(ui.muteButton,SIGNAL(toggled(bool)),this,SLOT(muteButtonToggled(bool)));
	connect(ui.modeBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(modeSelectionChanged(QString)));
	connect(ui.filterBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(filterSelectionChanged(QString)));
	connect(ui.gainSlider,SIGNAL(valueChanged(int)),this,SLOT(gainSliderChanged(int)));
	connect(ui.agcSlider,SIGNAL(valueChanged(int)),this,SLOT(agcSliderChanged(int)));
	connect(ui.spectrumWidget,SIGNAL(spectrumMixerChanged(qint32)),this,SLOT(mixerChanged(qint32)));
	connect(ui.spectrumWidget,SIGNAL(spectrumMixerChanged(qint32,bool)),this,SLOT(mixerChanged(qint32,bool)));
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
	qApp->installEventFilter(this);

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
	m_masterClock = new QTimer(this);
	connect(m_masterClock, SIGNAL(timeout()), this, SLOT(masterClockTicked()));
	m_masterClockTicks = 0;
	m_masterClock->start(MASTER_CLOCK_INTERVAL);
	updateClock();

    QComboBox *sdrSelector = ui.sdrSelector;
    //Add device plugins
    int cur;
    foreach (PluginInfo p,m_receiver->getDevicePluginInfo()) {
        v.setValue(p);
        sdrSelector->addItem(p.name,v);
		if (p.fileName == global->settings->m_sdrDeviceFilename &&
			p.type == PluginInfo::DEVICE_PLUGIN) {
                cur = sdrSelector->count()-1;
				m_sdr = p.deviceInterface;
				m_sdr->Command(DeviceInterface::CmdReadSettings,0);
				global->sdr = m_sdr;
        }
    }

    sdrSelector->setCurrentIndex(cur);
    connect(sdrSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(receiverChanged(int)));

    //Widget must have a parent
    m_directInputWidget = new QWidget(this->window(), Qt::Popup);
    m_directInputUi = new Ui::DirectInput;
    m_directInputUi->setupUi(m_directInputWidget);
    //Direct input is modal to avoid any confusion with other UI
    //directInputWidget->setWindowModality(Qt::WindowModal);
    //directInputUi->directEntry->setInputMask("0000000"); //All digits, none required
    connect(m_directInputUi->directEntry,SIGNAL(returnPressed()),this,SLOT(directEntryAccepted()));
    connect(m_directInputUi->enterButton,SIGNAL(clicked()),this,SLOT(directEntryAccepted()));
    connect(m_directInputUi->cancelButton,SIGNAL(clicked()),this,SLOT(directEntryCanceled()));

    ui.dataFrame->setVisible(false);

    connect(ui.spectrumWidget,SIGNAL(mixerLimitsChanged(int,int)),this,SLOT(setMixerLimits(int,int)));

	powerStyle(m_powerOn);

}

ReceiverWidget::~ReceiverWidget(void)
{
	
}

void ReceiverWidget::showDataFrame(bool b)
{
    ui.dataFrame->setVisible(b);
}

//Mixer changed by another widget, like spectrum
void ReceiverWidget::mixerChanged(qint32 m)
{
	if (m_loFrequency + m > 0)
		setFrequency(m_loFrequency + m);
}

void ReceiverWidget::mixerChanged(qint32 m, bool b)
{
	if (m_loFrequency + m > 0) {
		setLoMode(b);
		setFrequency(m_loFrequency + m);
	}
}

//User hit enter, esc
void ReceiverWidget::directEntryAccepted()
{
    double freq = QString(m_directInputUi->directEntry->text()).toDouble();
    if (freq > 0) {
        //Freq is in kHx
        freq *= 1000.0;
        //Direct is always LO mode
        setLoMode(true);
        setFrequency(freq);
    }

    m_directInputWidget->close();
}

void ReceiverWidget::directEntryCanceled()
{
    m_directInputWidget->close();
}

//Filters all application events, installed in constructor
bool ReceiverWidget::eventFilter(QObject *o, QEvent *e)
{
    static int scrollCounter = 0; //Used to slow down and smooth freq changes from scroll wheel
    static const int smoothing = 10;

	//First check for application level shortcuts
	if (e->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
		//int key =  keyEvent->key();
		double freqChange = 0;

		switch (keyEvent->key()) {
			case Qt::Key_P:
				ui.powerButton->toggle();
				return true;
			case Qt::Key_M:
				ui.muteButton->toggle();
				return true;
			case Qt::Key_L:
				ui.loButton->toggled(!m_loMode);
				return true;
			case Qt::Key_J:
				freqChange = -1;
				break;
			case Qt::Key_H:
				freqChange = -10;
				break;
			case Qt::Key_G:
				freqChange = -100;
				break;
			case Qt::Key_F:
				freqChange = -1000;
				break;
			case Qt::Key_D:
				freqChange = -10000;
				break;
			case Qt::Key_S:
				freqChange = -100000;
				break;
			case Qt::Key_A:
				freqChange = -1000000;
				break;
			case Qt::Key_U:
				freqChange = 1;
				break;
			case Qt::Key_Y:
				freqChange = 10;
				break;
			case Qt::Key_T:
				freqChange = 100;
				break;
			case Qt::Key_R:
				freqChange = 1000;
				break;
			case Qt::Key_E:
				freqChange =  10000;
				break;
			case Qt::Key_W:
				freqChange = 100000;
				break;
			case Qt::Key_Q:
				freqChange = 1000000;
				break;
		}

		if (freqChange != 0) {
			setFrequency(m_frequency + freqChange);
			keyEvent->accept();
			return true;
		} else {
			//keyEvent->ignore(); //We didn't handle it, pass it on
		}
	}

    //If the event (any type) is in a nixie we may need to know which one
    if (o == ui.nixie1) {
        m_tunerStep=1;
    }
    else if (o == ui.nixie10) {
        m_tunerStep=10;
    }
    else if (o == ui.nixie100) {
        m_tunerStep=100;
    }
    else if (o == ui.nixie1k) {
        m_tunerStep=1000;
    }
    else if (o == ui.nixie10k) {
        m_tunerStep=10000;
    }
    else if (o == ui.nixie100k) {
        m_tunerStep=100000;
    }
    else if (o == ui.nixie1m) {
        m_tunerStep=1000000;
    }
    else if (o == ui.nixie10m) {
        m_tunerStep=10000000;
    }
    else if (o == ui.nixie100m) {
        m_tunerStep=100000000;
    }
    else if (o == ui.nixie1g) {
        m_tunerStep=1000000000;
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
                m_directInputWidget->move(pt.x()+50,pt.y()+35);
                m_directInputWidget->show();
                m_directInputUi->directEntry->setFocus();
                m_directInputUi->directEntry->setText(QString::number(key-'0'));
                //Modal direct entry window is now active until user completes edit with Enter
                return true;
            } else if (key == Qt::Key_Up) {
                int v = qBound(0.0, num->value() + 1, 9.0);
                num->display(v);
                double d = getNixieNumber();
                setFrequency(d);
                return true;
            } else if (key == Qt::Key_Down) {
                int v = qBound(0.0, num->value() - 1, 9.0);
                num->display(v);
                double d = getNixieNumber();
                setFrequency(d);
                return true;
            }
        } else if (e->type() == QEvent::MouseButtonRelease) {
            //Clicking on a digit sets tuning step and resets lower digits to zero
			m_frequency = (quint64)(m_frequency/m_tunerStep) * m_tunerStep;
            setFrequency(m_frequency);

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
                    m_frequency += m_tunerStep;
                    setFrequency(m_frequency);
                } else if ((angleDelta.rx() < 0) && (++scrollCounter > smoothing)) {
                    //Scroll Left
                    scrollCounter = 0;
                    m_frequency -= m_tunerStep;
                    setFrequency(m_frequency);

                }
            } else if (angleDelta.rx() == 0) {
                //Up-down scrolling
                if ((angleDelta.ry() > 0) && (++scrollCounter > smoothing)) {
                    //Scroll Down
                    scrollCounter = 0;
                    m_frequency -= m_tunerStep;
                    setFrequency(m_frequency);
                } else if ((angleDelta.ry() < 0) && (++scrollCounter > smoothing)) {
                    //Scroll Up
                    scrollCounter = 0;
                    m_frequency += m_tunerStep;
                    setFrequency(m_frequency);
                }
            }
            return true;
        }
    }

	//return false; //Allow ojects to see event
    //or
	return QObject::eventFilter(o,e);
}

void ReceiverWidget::setLimits(double highF,double lowF,int highM,int lowM)
{
	m_highFrequency = highF;
	m_lowFrequency = lowF;
	m_highMixer = highM;
	m_lowMixer = lowM;
}

void ReceiverWidget::setMixerLimits(int highM, int lowM)
{
    m_highMixer = highM;
	m_lowMixer = lowM;
}

//Receiver connection when new data is available
void ReceiverWidget::newSignalStrength(double peakDb, double avgDb, double snrDb, double floorDb, double extValue)
{
	ui.sMeterWidget->newSignalStrength(peakDb, avgDb, snrDb, floorDb, extValue);
}

//Set frequency by changing LO or Mixer, depending on radio button chosen
//Todo: disable digits that are below step value?
void ReceiverWidget::setFrequency(double f)
{
	if (!m_powerOn)
		return;

	//if low and high are 0 or -1, ignore for now
	if (m_lowFrequency > 0 && f < m_lowFrequency) {
		//qDebug()<<"Low Limit "<<lowFrequency<<" "<<f;
		global->beep.play();
		displayNixieNumber(m_frequency); //Restore display
		return;
	}
	if (m_highFrequency > 0 && f > m_highFrequency){
		//qDebug()<<"High Limit "<<highFrequency<<" "<<f;
		global->beep.play();
        displayNixieNumber(m_frequency);
		return;
	}

	//If slave mode, just display frequency and return
	if (m_slaveMode) {
		m_frequency = f;
		displayNixieNumber(m_frequency);
		return;
	}

	if (m_loMode)
	{
		//Ask the receiver if requested freq is within limits and step size
		//Set actual frequency to next higher or lower step
		m_loFrequency = m_receiver->setSDRFrequency(f, m_frequency );
        m_frequency = m_loFrequency;
		m_mixer = 0;
        //Mixer is actually set including modeOffset so we hear tone, but display shows actual freq
		emit widgetMixerChanged(m_mixer + m_modeOffset);

	} else {
		//Mixer is delta between what's displayed and last LO Frequency
		qint32 oldMixer = m_mixer;
		m_mixer = f - m_loFrequency;
		//Check limits
        //mixer = mixer < lowMixer ? lowMixer : (mixer > highMixer ? highMixer : mixer);
		if (m_mixer < m_lowMixer || m_mixer > m_highMixer) {
			//Testing auto- tune.
			//When frequency is outside of mixer range, reset LO by difference between last freq and new freq
			//Result should be smoothly scrolling spectrum (depending on freq diff)
			qint32 mixerDelta = m_mixer - oldMixer;
			//Temporarily switch to LO mode
			m_loMode = true;
            //Set LO to new freq and contine
			m_loFrequency = m_receiver->setSDRFrequency(m_loFrequency + mixerDelta, m_loFrequency );
			m_mixer = f - m_loFrequency;
			m_frequency = m_loFrequency + m_mixer;  //Should be the same as f
            //Back to Mixer mode
			m_loMode = false;
			emit widgetMixerChanged(m_mixer + m_modeOffset);
        } else {
            //LoFreq not changing, just displayed freq
            m_frequency = m_loFrequency + m_mixer;
			emit widgetMixerChanged(m_mixer + m_modeOffset);
        }
	}
	//frequency is what's displayed, ie combination of loFrequency and mixer (if any)
    displayNixieNumber(m_frequency);
	ui.spectrumWidget->setMixer(m_mixer,m_loFrequency); //Spectrum tracks mixer
    //Update band info
    displayBand(m_frequency);


}
double ReceiverWidget::getFrequency()
{
	return m_frequency;
}
void ReceiverWidget::setMessage(QStringList s)
{
	ui.spectrumWidget->setMessage(s);
}
void ReceiverWidget::setMode(DeviceInterface::DEMODMODE m)
{
	QString text = Demod::ModeToString(m);
	int i = ui.modeBox->findText(text);
	if (i >= 0) {
		ui.modeBox->setCurrentIndex(i);
		m_mode = m;
		modeSelectionChanged(text);
	}
}
DeviceInterface::DEMODMODE ReceiverWidget::getMode()
{
	return m_mode;
}

//Applies style sheet to controls that toggle on/off
void ReceiverWidget::powerStyle(bool on) {
	//Changes the stylesheet for QLCDNumber in tunerFrame, see pebble.qss to change
	if (on) {
		ui.nixieFrame->setProperty("power","on");
		ui.clockFrame->setProperty("power","on");
	} else {
		ui.nixieFrame->setProperty("power","off");
		ui.clockFrame->setProperty("power","off");
	}

	//Re-apply the updated style sheets
	ui.nixieFrame->style()->polish(ui.nixie1);
	ui.nixieFrame->style()->polish(ui.nixie10);
	ui.nixieFrame->style()->polish(ui.nixie100);
	ui.nixieFrame->style()->polish(ui.nixie1k);
	ui.nixieFrame->style()->polish(ui.nixie10k);
	ui.nixieFrame->style()->polish(ui.nixie100k);
	ui.nixieFrame->style()->polish(ui.nixie1m);
	ui.nixieFrame->style()->polish(ui.nixie10m);
	ui.nixieFrame->style()->polish(ui.nixie100m);
	ui.nixieFrame->style()->polish(ui.nixie1g);
	ui.nixieFrame->update();

	ui.clockFrame->style()->polish(ui.clockWidget);
	ui.clockFrame->update();

}

//Slots

//Note: Pushbutton text color is changed with a style sheet in deigner
//	QPushButton:on {color:red;}
//
//sdr is already initialized by SetReceiver or ReceiverChanged when we get here
void ReceiverWidget::powerToggled(bool on) 
{

	if (on) {
        m_powerOn = true;
        if (!m_receiver->togglePower(true)) {
			ui.powerButton->setChecked(false); //Turn power button back off
			return; //Error setting up receiver
		}

		powerStyle(m_powerOn);

		//Limit tuning range and mixer range
		int sampleRate = m_sdr->Get(DeviceInterface::SampleRate).toInt();
		setLimits(m_sdr->Get(DeviceInterface::HighFrequency).toDouble(),
					m_sdr->Get(DeviceInterface::LowFrequency).toDouble(),
					sampleRate/2,-sampleRate/2);

		//Set intial gain slider position and range
		//QTAudio uses this as 0 - 1 volume setting
		ui.gainSlider->setMinimum(0);
		ui.gainSlider->setMaximum(100);
		ui.gainSlider->setValue(30);
		emit audioGainChanged(30);

		//Setup Squelch
		m_squelchDb = DB::minDb;
		ui.squelchSlider->setValue(m_squelchDb * m_squelchDbRes);
		emit squelchChanged(m_squelchDb);

		//Set default AGC mode
		//agcBoxChanged(AGC::FAST);
		int agcIndex = ui.agcBox->findData(AGC::OFF);
		ui.agcBox->blockSignals(true);
		ui.agcBox->setCurrentIndex(agcIndex);
		ui.agcBox->blockSignals(false);
		agcBoxChanged(agcIndex);

        //Set squelch to default

        //Don't allow SDR changes when receiver is on
        ui.sdrSelector->setEnabled(false);

        //Make sure record button is init properly
        ui.recButton->setEnabled(true);
        ui.recButton->setChecked(false);

        //Presets are only loaded when receiver is on
        m_presets = m_receiver->getPresets();

		ui.spectrumWidget->setSignalSpectrum(m_receiver->getSignalSpectrum());

		ui.spectrumWidget->plotSelectionChanged((SpectrumWidget::DisplayMode)m_sdr->Get(DeviceInterface::LastSpectrumMode).toInt());
        ui.bandType->setCurrentIndex(Band::HAM);

		ui.spectrumWidget->run(true);
        ui.sMeterWidget->start();

		//Set startup frequency last
		DeviceInterface::STARTUP_TYPE startupType = (DeviceInterface::STARTUP_TYPE)m_sdr->Get(DeviceInterface::StartupType).toInt();
		if (startupType == DeviceInterface::DEFAULTFREQ) {
			m_frequency=m_sdr->Get(DeviceInterface::StartupFrequency).toDouble();
			setFrequency(m_frequency);
			//This triggers indirect frequency set, so make sure we set widget first
			setMode((DeviceInterface::DEMODMODE)m_sdr->Get(DeviceInterface::StartupDemodMode).toInt());
		}
		else if (startupType == DeviceInterface::SETFREQ) {
			m_frequency = m_sdr->Get(DeviceInterface::UserFrequency).toDouble();
			setFrequency(m_frequency);
			setMode((DeviceInterface::DEMODMODE)m_sdr->Get(DeviceInterface::LastDemodMode).toInt());
		}
		else if (startupType == DeviceInterface::LASTFREQ) {
			m_frequency = m_sdr->Get(DeviceInterface::LastFrequency).toDouble();
			setFrequency(m_frequency);
			setMode((DeviceInterface::DEMODMODE)m_sdr->Get(DeviceInterface::LastDemodMode).toInt());
		}
		else {
			m_frequency = 10000000;
			setFrequency(m_frequency);
			setMode(DeviceInterface::dmAM);
		}
		//Also sets loFreq
		setLoMode(true);


	} else {
		//Turning power off, shut down receiver widget display BEFORE telling receiver to clean up
		//Objects
        //Make sure we reset data frame
        setDataMode(0);
        m_powerOn = false;

		powerStyle(m_powerOn);

        //Don't allow SDR changes when receiver is on
        ui.sdrSelector->setEnabled(true);

        //Make sure pwr and rec buttons are off
        ui.powerButton->setChecked(false);
        ui.recButton->setChecked(false);
        ui.recButton->setEnabled(false);

        m_presets = NULL;

		ui.spectrumWidget->run(false);
        ui.sMeterWidget->stop();
		//We have to make sure that widgets are stopped before cleaning up supporting objects
		m_receiver->togglePower(false);

	}
}

void ReceiverWidget::setDataMode(int _dataMode)
{
    ui.dataSelectionBox->setCurrentIndex(_dataMode);
}
//Tuning display can drive LO or Mixer depending on selection
void ReceiverWidget::setLoMode(bool b)
{
	m_loMode = b;
    if (m_loMode)
	{
        //LO Mode
        ui.loButton->setChecked(true); //Make sure button is toggled if called from presets
        m_mixer=0;
		emit widgetMixerChanged(m_mixer + m_modeOffset);
		setFrequency(m_loFrequency);
	} else {
		//Mixer Mode
        ui.mixButton->setChecked(true);
	}
}
void ReceiverWidget::anfButtonToggled(bool b)
{
    if (!m_powerOn)
        return;

	emit anfChanged(b);
}
void ReceiverWidget::nbButtonToggled(bool b)
{
    if (!m_powerOn)
        return;
	emit nb1Changed(b);
}
void ReceiverWidget::nb2ButtonToggled(bool b)
{
    if (!m_powerOn)
        return;
	emit nb2Changed(b);
}
void ReceiverWidget::agcBoxChanged(int item)
{
    if (!m_powerOn)
        return;
	AGC::AGCMODE agcMode = (AGC::AGCMODE)ui.agcBox->currentData().toInt();
	emit agcModeChanged(agcMode);
	ui.agcSlider->setValue(30);
}
void ReceiverWidget::muteButtonToggled(bool b)
{
    if (!m_powerOn)
        return;
	emit muteChanged(b);
}

void ReceiverWidget::addMemoryButtonClicked()
{
    if (!m_powerOn)
        return;

    if (m_presets == NULL)
        return;

    if (m_presets->AddMemory(m_frequency,"????","Added from Pebble"))
        QMessageBox::information(NULL,"Add Memory","Frequency added to memories.csv. Edit file and toggle power to re-load");
    else
        QMessageBox::information(NULL,"Add Memory","Add Memory Failed!");
}

void ReceiverWidget::findStationButtonClicked()
{
    if (!m_powerOn)
        return;

    quint16 range = 25;
    //Search all stations in current band looking for match or close (+/- N  khz)
    //Display in station box, message box, or band info area?
    QList<Station> stationList = m_presets->FindStation(m_frequency,range); // +/- khz
    QString str;
    if (!stationList.empty()) {
        Station s;
        for (int i=0; i<stationList.count(); i++) {
            s = stationList[i];
            str += QString("%1 %2 %3\n").arg(QString::number(s.freq/1000.0,'f',3), s.station, s.remarks);
        }
        m_receiver->getDemod()->OutputBandData("", "", QString().sprintf("Stations within %d kHz",range), str);
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
	switch (m_mode)
	{
	case DeviceInterface::dmLSB:
		lo=-filter;
        hi = 0;
		break;
	case DeviceInterface::dmUSB:
        lo=0;
		hi=filter;
		break;
	case DeviceInterface::dmAM:
	case DeviceInterface::dmSAM:
	case DeviceInterface::dmDSB:
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
	case DeviceInterface::dmCWU:
        //modeOffset = -1000 (so we can use it directly in other places without comparing cwu and cwl)
        //We want 500hz filter to run from 750 to 1250 so modeOffset is right in the middle
        lo = -m_modeOffset - (filter/2); // --1000 - 250 = 750
        hi = -m_modeOffset + (filter/2); // --1000 + 250 = 1250
		break;
	case DeviceInterface::dmCWL:
        //modeOffset = +1000 (default)
        //We want 500hz filter to run from -1250 to -750 so modeOffset is right in the middle
        lo = -m_modeOffset - (filter/2); // - +1000 - 250 = -1250
        hi = -m_modeOffset + (filter/2); // +1000 + 250 = -750
        break;
    //Same as CW but with no offset tone, drop if not needed
	case DeviceInterface::dmDIGU:
        lo=0;
		hi=filter;
		break;
	case DeviceInterface::dmDIGL:
		lo=-filter;
        hi= 0;
		break;
	case DeviceInterface::dmFMM:
	case DeviceInterface::dmFMS:
	case DeviceInterface::dmFMN:
		lo=-filter/2;
		hi= filter/2;
		break;
	case DeviceInterface::dmNONE:
		break;
	}

	if (lo == 0 && hi==0) {
		return; //Error
	}

	ui.spectrumWidget->setFilter(lo,hi); //So we can display filter around cursor

	emit filterChanged(lo,hi);
}

void ReceiverWidget::dataSelectionChanged(int s)
{
    if (!m_powerOn)
        return;

    //Clear any previous data selection
    if (m_dataSelection.fileName == "Band_Data") {
            m_receiver->getDemod()->SetupDataUi(NULL);
            //Delete all children
            foreach (QObject *obj, ui.dataFrame->children()) {
                //Normally we get a grid layout object, uiFrame, dataFrame
                delete obj;
            }
    } else if (m_dataSelection.fileName == "No_Data") {
    } else {
           //Reset decoder
            m_receiver->setDigitalModem(NULL,NULL);
            //Delete all children
            foreach (QObject *obj, ui.dataFrame->children()) {
                //Normally we get a grid layout object, uiFrame, dataFrame
                delete obj;
            }
    }

    //enums are stored as user data with each menu item
    m_dataSelection = ui.dataSelectionBox->itemData(s).value<PluginInfo>();

    QWidget *parent;

    if (m_dataSelection.fileName == "No_Data") {
            //Data frame is always open if we get here
            ui.dataFrame->setVisible(false);
            parent = ui.dataFrame;
            while (parent) {
                //Warning, adj size will use all layout hints, including h/v spacer sizing.  So overall size may change
                parent->adjustSize();
                parent = parent->parentWidget();
            }
            update();
    } else if (m_dataSelection.fileName == "Band_Data") {
            m_receiver->getDemod()->SetupDataUi(ui.dataFrame);
            ui.dataFrame->setVisible(true);
    } else {
            m_receiver->setDigitalModem(ui.dataSelectionBox->currentText(), ui.dataFrame);
            ui.dataFrame->setVisible(true);
    }
}


void ReceiverWidget::modeSelectionChanged(QString m) 
{
	if (m_slaveMode)
		return;

	m_mode = Demod::StringToMode(m);
		//Adj to mode, ie we don't want to be exactly on cw, we want to be +/- 200hz
	switch (m_mode)
	{
	//Todo: Work on this, still not accurately reflecting click
	case DeviceInterface::dmCWU:
        //Subtract modeOffset from actual freq so we hear upper tone
		m_modeOffset = -global->settings->m_modeOffset; //Same as CW decoder
		break;
	case DeviceInterface::dmCWL:
        //Add modeOffset to actual tuned freq so we hear lower tone
		m_modeOffset = global->settings->m_modeOffset;
		break;
	default:
		m_modeOffset = 0;
		break;
	}
	//Reset frequency to update any change in modeOffset
	if (m_frequency !=0)
		//This may get called during power on
		setFrequency(m_frequency);

	//Set filter options for this mode
	ui.filterBox->blockSignals(true); //No signals while we're changing box
	ui.filterBox->clear();
    ui.filterBox->addItems(Demod::demodInfo[m_mode].filters);
    ui.filterBox->setCurrentIndex(Demod::demodInfo[m_mode].defaultFilter);

	ui.spectrumWidget->setMode(m_mode, m_modeOffset);
	emit demodChanged(m_mode);
	ui.filterBox->blockSignals(false);
	this->filterSelectionChanged(ui.filterBox->currentText());
}

void ReceiverWidget::agcSliderChanged(int g)
{
	if (!m_powerOn)
		return;
	emit agcThresholdChanged(g);
}

void ReceiverWidget::gainSliderChanged(int g) 
{
    if (!m_powerOn)
        return;
	m_gain=g; 
	emit audioGainChanged(g);
}

//Squelch slider is in db
void ReceiverWidget::squelchSliderChanged(int s)
{
	if (!m_powerOn)
		return;
	m_squelchDb = (double)s/10;
	emit squelchChanged(m_squelchDb);
}

void ReceiverWidget::nixie1UpClicked() {setFrequency(m_frequency+1);}
void ReceiverWidget::nixie1DownClicked(){setFrequency(m_frequency-1);}
void ReceiverWidget::nixie10UpClicked(){setFrequency(m_frequency+10);}
void ReceiverWidget::nixie10DownClicked(){setFrequency(m_frequency-10);}
void ReceiverWidget::nixie100UpClicked(){setFrequency(m_frequency+100);}
void ReceiverWidget::nixie100DownClicked(){setFrequency(m_frequency-100);}
void ReceiverWidget::nixie1kUpClicked(){setFrequency(m_frequency+1000);}
void ReceiverWidget::nixie1kDownClicked(){setFrequency(m_frequency-1000);}
void ReceiverWidget::nixie10kUpClicked(){setFrequency(m_frequency+10000);}
void ReceiverWidget::nixie10kDownClicked(){setFrequency(m_frequency-10000);}
void ReceiverWidget::nixie100kUpClicked(){setFrequency(m_frequency+100000);}
void ReceiverWidget::nixie100kDownClicked(){setFrequency(m_frequency-100000);}
void ReceiverWidget::nixie1mUpClicked(){setFrequency(m_frequency+1000000);}
void ReceiverWidget::nixie1mDownClicked(){setFrequency(m_frequency-1000000);}
void ReceiverWidget::nixie10mUpClicked(){setFrequency(m_frequency+10000000);}
void ReceiverWidget::nixie10mDownClicked(){setFrequency(m_frequency-10000000);}
void ReceiverWidget::nixie100mUpClicked(){setFrequency(m_frequency+100000000);}
void ReceiverWidget::nixie100mDownClicked(){setFrequency(m_frequency-100000000);}
void ReceiverWidget::nixie1gUpClicked(){setFrequency(m_frequency+1000000000);}
void ReceiverWidget::nixie1gDownClicked(){setFrequency(m_frequency-1000000000);}

void ReceiverWidget::utcClockButtonClicked()
{
   m_showUtcTime = true;
   ui.utcClockButton->setFlat(false);
   ui.localClockButton->setFlat(true);
   updateClock();
}

void ReceiverWidget::localClockButtonClicked()
{
    m_showUtcTime = false;
    ui.utcClockButton->setFlat(true);
    ui.localClockButton->setFlat(false);
	updateClock();
}

//Called every MASTER_CLOCK_INTERVAL (100ms)
void ReceiverWidget::masterClockTicked()
{
	m_masterClockTicks++;

	//Process once per tick actions

	//Process once per second actions
	if (m_masterClockTicks % (1000 / MASTER_CLOCK_INTERVAL) == 0) {
		updateClock();
		updateHealth();
		updateSlaveInfo();
	}
}
//If device is in slave mode, then we get info from device and update display to track
void ReceiverWidget::updateSlaveInfo()
{
	if (!m_powerOn || m_sdr==NULL)
		return;

	if (!m_sdr->Get(DeviceInterface::DeviceSlave).toBool()) {
		m_slaveMode = false;
		return;
	}
	m_slaveMode = true;

	//Display last info fetched
	double f = m_sdr->Get(DeviceInterface::DeviceFrequency).toDouble();
	setFrequency(f);

	DeviceInterface::DEMODMODE dm = (DeviceInterface::DEMODMODE)m_sdr->Get(DeviceInterface::DeviceDemodMode).toInt();
	setMode(dm);

	m_receiver->setWindowTitle();

	//Ask the device to return new info
	m_sdr->Set(DeviceInterface::DeviceSlave,0);
}

void ReceiverWidget::updateHealth()
{
	if (m_powerOn && m_sdr != NULL) {
		quint16 freeBuf = m_sdr->Get(DeviceInterface::DeviceHealthValue).toInt();
		if (freeBuf >= 75)
			ui.sdrOptions->setStyleSheet("background:green");
		else if (freeBuf >= 25)
			ui.sdrOptions->setStyleSheet("background:orange");
		else
			ui.sdrOptions->setStyleSheet("background:red");
	} else {
		ui.sdrOptions->setStyleSheet("");
	}

}

void ReceiverWidget::updateClock()
{
	//Add Day of the week
	QDateTime time;
	if (m_showUtcTime)
		time = QDateTime::currentDateTimeUtc();
	else
		time = QDateTime::currentDateTime();
	QString text = time.toString("hh:mm");
	//text = "23:59"; //For testing
	if ((time.time().second() % 2) == 0)
		text[2] = ' '; //Flash ':' for seconds
	ui.clockWidget->display(text);

}

//Returns the number displayed by all the nixies
double ReceiverWidget::getNixieNumber()
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

void ReceiverWidget::receiverChanged(int i)
{
    //Power is off when this is called
    int cur = ui.sdrSelector->currentIndex();
    PluginInfo p = ui.sdrSelector->itemData(cur).value<PluginInfo>();
    //Replace
	global->settings->m_sdrDeviceFilename = p.fileName;
	global->settings->m_sdrDeviceNumber = p.deviceNumber;

	m_sdr = p.deviceInterface;
	global->sdr = m_sdr;
    //Close the sdr option window if open
    m_receiver->closeSdrOptions();
}

//Updates all the nixies to display a number
void ReceiverWidget::displayNixieNumber(double n)
{
	quint64 d = n;
	ui.nixie1g->display( fmod(d / 1000000000,10));
	ui.nixie100m->display( fmod(d / 100000000,10));
	ui.nixie10m->display( fmod(d / 10000000,10));
	ui.nixie1m->display( fmod(d / 1000000,10));
	ui.nixie100k->display( fmod(d / 100000,10));
	ui.nixie10k->display( fmod(d / 10000,10));
	ui.nixie1k->display( fmod(d / 1000,10));
	ui.nixie100->display( fmod(d / 100,10));
	ui.nixie10->display( fmod(d /10,10));
	ui.nixie1->display( fmod(d,10));
}

void ReceiverWidget::bandTypeChanged(int s)
{
    if (!m_powerOn || m_presets==NULL)
        return; //bands aren't loaded until power on

    //enums are stored as user data with each menu item
    Band::BANDTYPE type = (Band::BANDTYPE)ui.bandType->itemData(s).toInt();
    //Populate bandCombo with type
    //Turn off signals
    ui.bandCombo->blockSignals(true);
    ui.bandCombo->clear();
    Band *bands = m_presets->GetBands();
    if (bands == NULL)
        return;

    int numBands = m_presets->GetNumBands();
    double freq;
    QString buf;
    for (int i=0; i<numBands; i++) {
        if (bands[i].bType == type  || type==Band::ALL) {
            freq = bands[i].tune;
            //If no specific tune freq for band select, use low
            if (freq == 0)
                freq = bands[i].low;
            //Is freq within lo/hi range of SDR?  If not ignore
            if (freq >= m_lowFrequency && freq < m_highFrequency) {
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
    if (!m_powerOn || s==-1 || m_presets==NULL) //-1 means we're just clearing selection
        return;

    Band *bands = m_presets->GetBands();
    if (bands == NULL)
        return;

    int bandIndex = ui.bandCombo->itemData(s).toInt();
    double freq = bands[bandIndex].tune;
	//If no specific tune freq for band select, set to middle
    if (freq == 0)
		freq = bands[bandIndex].low + ((bands[bandIndex].high - bands[bandIndex].low) / 2.0);
	DeviceInterface::DEMODMODE mode = bands[bandIndex].mode;

    //Make sure we're in LO mode
    setLoMode(true);

    setFrequency(freq);
    setMode(mode);

}

void ReceiverWidget::stationChanged(int s)
{
    if (!m_powerOn || m_presets==NULL)
        return;
    Station *stations = m_presets->GetStations();
    int stationIndex = ui.stationCombo->itemData(s).toInt();
    double freq = stations[stationIndex].freq;
    setFrequency(freq);
    //SetMode(mode);
}

void ReceiverWidget::displayBand(double freq)
{
    if (!m_powerOn || m_presets==NULL)
        return;

    Band *band = m_presets->FindBand(freq);
    if (band != NULL) {
        //Set bandType to match band, this will trigger bandChanged to load bands for type
        ui.bandType->setCurrentIndex(band->bType);

        //Now matching band
        int index = ui.bandCombo->findData(band->bandIndex);
        ui.bandCombo->blockSignals(true); //Just select band to display, don't reset freq to band
        ui.bandCombo->setCurrentIndex(index);
        ui.bandCombo->blockSignals(false);

        //If band has changed, update station list to match band
        if (m_currentBandIndex != band->bandIndex) {
            //List of stations per band should be setup when we read eibi.csv
            ui.stationCombo->blockSignals(true); //Don't trigger stationChanged() every addItem
            ui.stationCombo->clear();
            int stationIndex;
            Station *stations = m_presets->GetStations();
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
        m_currentBandIndex = band->bandIndex;

    } else {
        ui.bandCombo->setCurrentIndex(-1); //no band selected
        ui.stationCombo->clear(); //No stations
        m_currentBandIndex = -1;
    }
}
