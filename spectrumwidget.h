#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include <QWidget>
#include <QThread>
#include <QImage>
#include <QHoverEvent>

#include "cpx.h"
#include "ui/ui_spectrumwidget.h"
#include "demod.h"
#include "signalspectrum.h"

class SpectrumThread;

class SpectrumWidget : public QWidget
{
	Q_OBJECT

public:
	SpectrumWidget(QWidget *parent = 0);
	~SpectrumWidget();

	void SetMixer(int m, double f);
	void SetFilter(int lo, int hi);
	void Run(bool r);
	void SetMode(DEMODMODE m);
	//Text is displayed when spectrum is 'off' for now
	void SetMessage(QStringList s);
	void SetSignalSpectrum(SignalSpectrum *s);

public slots:
		void plotSelectionChanged(SignalSpectrum::DISPLAYMODE m);

signals:
		//User clicked in spectrum
		void mixerChanged(int m);
        void mixerChanged(int m, bool changeLO);

		private slots:
            void displayChanged(int item);
            void dbOffsetChanged(int i);
            void dbGainChanged(int t);
            void zoomChanged(int item);
            void newFftData();
private:
	SignalSpectrum *signalSpectrum; //Source of spectrum data
    double *averageSpectrum;
    double *lastSpectrum;

	int upDownIncrement;
	int leftRightIncrement;

    double zoom; //Percentage of total spectrum to display

	Ui::SpectrumWidgetClass ui;
    void DrawCursor(QPainter &painter,QColor color);
    void DrawOverlay();
    void DrawSpectrum();
    void DrawWaterfall();

	//Event overrides
	void paintEvent(QPaintEvent *event);
	void mousePressEvent ( QMouseEvent * event );
	void keyPressEvent (QKeyEvent *event); //Change mixer with arrows
	void enterEvent ( QEvent *event );
	void leaveEvent (QEvent *event);
    void resizeEvent(QResizeEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent( QWheelEvent * event );
    void hoverEnter(QHoverEvent *event);
    void hoverLeave(QHoverEvent *event);

	//Show mixer freq
	int fMixer;
	double loFreq;
    double mouseFreq; //Freq under mouse pointer

	int loFilter; //Used to display bandpass
	int hiFilter;
    double spectrumOffset;
    double spectrumGain;
	SignalSpectrum::DISPLAYMODE spectrumMode;
	int sampleRate;
	DEMODMODE demodMode;

	bool isRunning;
	//QString *message;
	QStringList message;

    QPixmap plotArea;
    QPixmap plotOverlay;
    QPixmap plotLabel;

    int dbRange;

    QColor *spectrumColors;

    double GetMouseFreq();
    double GetXYFreq(int x, int y);
    int GetMouseDb();

    //Grid display, will change depending on plotArea size
    int horizDivs;
    int vertDivs;


};

#endif // SPECTRUMWIDGET_H
