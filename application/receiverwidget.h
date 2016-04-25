#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "ui_receiverwidget.h"
#include "ui_directinput.h"
//#include "ui/ui_data-morse.h"
#include "spectrumwidget.h"
#include "smeterwidget.h"
#include "demod.h"
#include "plugins.h"
//#include "receiver.h" //Compiler error, maybe gets confused with ui_ReceiverWidget
class Receiver; //include receiver.h in cpp file
class Presets;

/*
Took me a long time to figure out how to create a custom widget in Qt Designer and use it
1. Create new custom widget
2. Edit object name to ReceiverWidget.
3. Generated file ui_receiverwidget.h will now define new class ui::ReceiverWidget
4. But this class doesn't derive from QWidget and can't be modified with additional code
5. So we create a regular c++ class called ReceiverWidget derived from QWidget
6. It wraps the ui_receiverwidget and allows us to to use the Promote feature in QtDesigner

Update: Now I know I can just create a new QtGui element in VS and it does all this automagically
*/
class ReceiverWidget:public QWidget
{
	//This needs to included in any class that uses signals or slots
	Q_OBJECT

public:

	ReceiverWidget(QWidget *parent =0);
	~ReceiverWidget(void);
	void setReceiver (Receiver *r); //Our 'model'
	void setFrequency(double f);
	double getFrequency();
	void setMode(DeviceInterface::DEMODMODE m);
	void setDataMode(int _dataMode);
	DeviceInterface::DEMODMODE getMode();
	void setMessage(QStringList s);

	void displayBand(double freq);

public slots:
		void setLoMode(bool b);
		void powerToggled(bool b);
        void directEntryAccepted();
        void directEntryCanceled();
        void showDataFrame(bool b);
        void setMixerLimits(int highM, int lowM);
		void newSignalStrength(double peakDb, double avgDb, double snrDb, double floorDb, double extValue);


signals:
		void demodChanged(	DeviceInterface::DEMODMODE _demodMode);
		void audioGainChanged(int _audioGain);
		void agcThresholdChanged(int _threshold);
		void squelchChanged(int _squelch);
		void widgetMixerChanged(int _mixer);

protected:		
	bool eventFilter(QObject *o, QEvent *e);

private:
	static const quint16 MASTER_CLOCK_INTERVAL = 100; //ms

	DeviceInterface *m_sdr; //global->sdr is always updated whenever the user changes device selection
	Receiver *m_receiver;
	QWidget *m_directInputWidget;
	Ui::DirectInput *m_directInputUi;

	//Master clock to update time, refresh status etc
	QTimer *m_masterClock;
	quint64 m_masterClockTicks;

	//Set tuner limits
	void setLimits(double highF, double lowF, int highM, int lowM);

	void displayNixieNumber(double d);
	double getNixieNumber();

	Ui::ReceiverWidget ui;
	//Converts value to separate digits and updates display
	int m_tunerStep;
	int m_gain;
	int m_squelch;
	bool m_loMode;
	double m_frequency; //Current frequency
	double m_loFrequency;
	int m_mixer; //Current mixer value
	//Frequency limits, tuner won't go beyond these
	double m_highFrequency;
	double m_lowFrequency;
	int m_highMixer;
	int m_lowMixer;
	bool m_powerOn;
	DeviceInterface::DEMODMODE m_mode;
	int m_modeOffset; //make CW +- tone instead of actual freq
	Presets *m_presets;

	PluginInfo m_dataSelection;

    //Band currently selected, used to detect when band changes as a result of freq change
	int m_currentBandIndex;

	bool m_showUtcTime;
	bool m_slaveMode;

	void updateClock();
	void updateHealth();
	void updateSlaveInfo();
	void powerStyle(bool on);

private slots:
		void receiverChanged(int i);

        void stationChanged(int s);
        void bandTypeChanged(int s);
        void bandChanged(int s);
        void dataSelectionChanged(int s);
		void agcSliderChanged(int a);
		void gainSliderChanged(int g);
		void squelchSliderChanged(int s);
		void modeSelectionChanged(QString item);
		void filterSelectionChanged(QString item);
		void mixerChanged(qint32 m);
		void mixerChanged(qint32 m, bool b);
        void anfButtonToggled(bool b);
		void nbButtonToggled(bool b);
		void nb2ButtonToggled(bool b);
		void agcBoxChanged(int item);
		void muteButtonToggled(bool b);
        void addMemoryButtonClicked();
        void findStationButtonClicked();

		void nixie1UpClicked();
		void nixie1DownClicked();
		void nixie10UpClicked();
		void nixie10DownClicked();
		void nixie100UpClicked();
		void nixie100DownClicked();
		void nixie1kUpClicked();
		void nixie1kDownClicked();
		void nixie10kUpClicked();
		void nixie10kDownClicked();
		void nixie100kUpClicked();
		void nixie100kDownClicked();
		void nixie1mUpClicked();
		void nixie1mDownClicked();
		void nixie10mUpClicked();
		void nixie10mDownClicked();
		void nixie100mUpClicked();
		void nixie100mDownClicked();
        void nixie1gUpClicked();
        void nixie1gDownClicked();

		void masterClockTicked();
        void utcClockButtonClicked();
        void localClockButtonClicked();
};
