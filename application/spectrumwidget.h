#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include <QWidget>
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
	//Waterfall_Spectrum is non-standard mode and is deliberately skipped
	enum DISPLAYMODE {SPECTRUM = 0, WATERFALL, SPECTRUM_SPECTRUM, SPECTRUM_WATERFALL,
		WATERFALL_WATERFALL, NODISPLAY};


	SpectrumWidget(QWidget *parent = 0);
	~SpectrumWidget();

	void SetMixer(int m, double f);
	void SetFilter(int lo, int hi);
	void Run(bool r);
    void SetMode(DeviceInterface::DEMODMODE m, int _modeOffset);
	//Text is displayed when spectrum is 'off' for now
	void SetMessage(QStringList s);
	void SetSignalSpectrum(SignalSpectrum *s);

public slots:
		void plotSelectionChanged(DISPLAYMODE _mode);
		void updatesPerSecChanged(int item);
		void splitterMoved(int x, int y);
		void hiResClicked(bool _b);

signals:
		//User clicked in spectrum
		void spectrumMixerChanged(qint32 m);
		void spectrumMixerChanged(qint32 m, bool changeLO);
        void mixerLimitsChanged(int high, int low);

private slots:
	void displayChanged(int item);
	void maxDbChanged(int t);
	void minDbChanged(int t);
	void zoomChanged(int item);
	void newFftData();

private:
	SignalSpectrum *signalSpectrum; //Source of spectrum data
    double *averageSpectrum;
    double *lastSpectrum;
    //Holds values mapped to screen using utility in fft
	qint32 *fftMap;
	qint32 *topPanelFftMap;

	int upDownIncrement;
	int leftRightIncrement;

    double zoom; //Percentage of total spectrum to display

	Ui::SpectrumWidgetClass ui;
	void paintFreqCursor(QPainter *painter, QRect plotFr, bool isZoomed, QColor color);
	void drawOverlay();

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
	qint32 fMixer;
	double loFreq;
    double mouseFreq; //Freq under mouse pointer

	int loFilter; //Used to display bandpass
	int hiFilter;

    qint16 plotMaxDb; //Set by UI
    qint16 plotMinDb;
	const quint16 minMaxDbDelta = 30; //Delta between min and max can never be less than this

	DISPLAYMODE spectrumMode;
	int sampleRate;
	DeviceInterface::DEMODMODE demodMode;

	bool isRunning;
	//QString *message;
	QStringList message;

    QPixmap plotArea;
    QPixmap plotOverlay;
    QPixmap plotLabel;
	QPixmap topPanelPlotArea;
	QPixmap topPanelPlotOverlay;
	QPixmap topPanelPlotLabel;

	QPoint *lineBuf;

    int dbRange;

    QColor *spectrumColors;

	double getMouseFreq();
	int getMouseDb();

    //Grid display, will change depending on plotArea size
    static const int maxHDivs = 50;

    int modeOffset;

	void resizePixmaps(bool scale = true);
	void drawScale(QPainter *labelPainter, double centerFreq, bool drawTopFrame);

	void updateTopPanel(DISPLAYMODE _newMode, bool updateSlider);
	QRect mapFrameToWidget(QFrame *frame);

	bool topPanelHighResolution; //True if top panel is in hi-res mode
	void paintSpectrum(bool paintTopPanel, QPainter *painter);
	void drawSpectrum(QPixmap &_pixMap, QPixmap &_pixOverlayMap, qint32 *_fftMap);
	void drawSpectrumOverlay(bool drawTopPanel);

	void paintWaterfall(bool paintTopPanel, QPainter *painter);
	void drawWaterfall(QPixmap &_pixMap, QPixmap &_pixOverlayMap, qint32 *_fftMap);
	void drawWaterfallOverlay(bool drawTopPanel);

	QString frequencyLabel(double f, qint16 precision = -1);
	double calcZoom(int item);
	void paintMouseCursor(bool paintTopPanel, QPainter *painter, QColor color, bool paintDb, bool paintFreq);
};

#endif // SPECTRUMWIDGET_H
