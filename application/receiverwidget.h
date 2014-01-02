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
	void SetReceiver (Receiver *r); //Our 'model'
	void SetFrequency(double f);
	double GetFrequency();
	void SetMode(DEMODMODE m); 
    void SetDataMode(int _dataMode);
    void SetDisplayedGain(int g, int min = 10, int max = 100);
    void SetDisplayedAgcThreshold(int g);
    void SetDisplayedSquelch(int s);
	DEMODMODE GetMode();
	//Set tuner limits
	void SetLimits(double highF, double lowF, int highM, int lowM);
	void SetMessage(QStringList s);

    void DisplayBand(double freq);

public slots:
		void setLoMode(bool b);
		void powerToggled(bool b);
        void directEntryAccepted();
        void directEntryCanceled();
        void showDataFrame(bool b);
        void setMixerLimits(int highM, int lowM);


signals:

protected:		
	bool eventFilter(QObject *o, QEvent *e);

private:
	Receiver *receiver;
    QWidget *directInputWidget;
    Ui::DirectInput *directInputUi;

    void DisplayNixieNumber(double d);
    double GetNixieNumber();

	Ui::ReceiverWidget ui;
	//Converts value to separate digits and updates display
	int tunerStep;
	int gain;
	int squelch;
	bool loMode;
	double frequency; //Current frequency
	double loFrequency;
	int mixer; //Current mixer value
	//Frequency limits, tuner won't go beyond these
	double highFrequency;
	double lowFrequency;
	int highMixer;
	int lowMixer;
	bool powerOn;
	DEMODMODE mode;
    int modeOffset; //make CW +- tone instead of actual freq
    Presets *presets;

    PluginInfo dataSelection;

    //Band currently selected, used to detect when band changes as a result of freq change
    int currentBandIndex;

    bool showUtcTime;

	private slots:
        void ReceiverChanged(int i);

        void stationChanged(int s);
        void bandTypeChanged(int s);
        void bandChanged(int s);
        void dataSelectionChanged(int s);
		void agcSliderChanged(int a);
		void gainSliderChanged(int g);
		void squelchSliderChanged(int s);
		void modeSelectionChanged(QString item);
		void filterSelectionChanged(QString item);
		void mixerChanged(int m);
        void mixerChanged(int m, bool b);
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

        void showTime();
        void utcClockButtonClicked();
        void localClockButtonClicked();
};
