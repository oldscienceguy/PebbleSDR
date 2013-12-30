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
#include "ui_spectrumwidget.h"
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
    void SetMode(DEMODMODE m, int _modeOffset);
	//Text is displayed when spectrum is 'off' for now
	void SetMessage(QStringList s);
	void SetSignalSpectrum(SignalSpectrum *s);

public slots:
		void plotSelectionChanged(SignalSpectrum::DISPLAYMODE m);

signals:
		//User clicked in spectrum
		void mixerChanged(int m);
        void mixerChanged(int m, bool changeLO);
        void mixerLimitsChanged(int high, int low);

		private slots:
            void displayChanged(int item);
            void maxDbChanged(int t);
            void zoomChanged(int item);
            void newFftData();
private:
	SignalSpectrum *signalSpectrum; //Source of spectrum data
    double *averageSpectrum;
    double *lastSpectrum;
    //Holds values mapped to screen using utility in fft
    qint32 fftMap[2048]; //!!Fix to dynamic max screen width?

	int upDownIncrement;
	int leftRightIncrement;

    double zoom; //Percentage of total spectrum to display

	Ui::SpectrumWidgetClass ui;
    void DrawCursor(QPainter *painter, QRect plotFr, bool isZoomed, QColor color);
    void DrawOverlay(bool isZoomed);
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
    double spectrumOffset; //!!Remove
    qint16 plotMaxDb; //Set by UI
    qint16 plotMinDb;

	SignalSpectrum::DISPLAYMODE spectrumMode;
	int sampleRate;
	DEMODMODE demodMode;

    bool useZoomSpectrum; //True if zoom is within high-res buffer range
	bool isRunning;
	//QString *message;
	QStringList message;

    QPixmap plotArea;
    QPixmap plotOverlay;
    QPixmap plotLabel;
    QPixmap zoomPlotArea;
    QPixmap zoomPlotOverlay;
    QPixmap zoomPlotLabel;

    QPoint LineBuf[2048]; //!!Todo: This should be configured to max #pixels in plot area for full screen

    int dbRange;

    QColor *spectrumColors;

    double GetMouseFreq();
    int GetMouseDb();

    //Grid display, will change depending on plotArea size
    static const int maxHDivs = 50;
    int horizDivs; //Must be even number to split right around center
    int vertDivs;

    int modeOffset;

    void resizeFrames();
    void DrawScale(QPainter *labelPainter, double centerFreq, bool isZoomed);
};

#endif // SPECTRUMWIDGET_H
