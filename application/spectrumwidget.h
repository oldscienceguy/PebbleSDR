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
	enum DisplayMode {SPECTRUM = 0, WATERFALL, SPECTRUM_SPECTRUM, SPECTRUM_WATERFALL,
		WATERFALL_WATERFALL, NODISPLAY};


	SpectrumWidget(QWidget *parent = 0);
	~SpectrumWidget();

	void setMixer(int m, double f);
	void setFilter(int lo, int hi);
	void run(bool r);
	void setMode(DeviceInterface::DemodMode m, int _modeOffset);
	//Text is displayed when spectrum is 'off' for now
	void setMessage(QStringList s);
	void setSignalSpectrum(SignalSpectrum *s);

public slots:
		void plotSelectionChanged(DisplayMode _mode);
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
	void paintFreqCursor(QPainter *painter, QRect plotFr, bool isZoomed, QColor color);
	void drawOverlay();
	double getMouseFreq();
	int getMouseDb();
	void resizePixmaps(bool scale = true);
	void drawScale(QPainter *labelPainter, double centerFreq, bool drawTopFrame);
	void paintSpectrum(bool paintTopPanel, QPainter *painter);
	void drawSpectrum(QPixmap &_pixMap, QPixmap &_pixOverlayMap, qint32 *_fftMap);
	void drawSpectrumOverlay(bool drawTopPanel);

	void paintWaterfall(bool paintTopPanel, QPainter *painter);
	void drawWaterfall(QPixmap &_pixMap, QPixmap &_pixOverlayMap, qint32 *_fftMap);
	void drawWaterfallOverlay(bool drawTopPanel);

	QString frequencyLabel(double f, qint16 precision = -1);
	double calcZoom(int item);
	void paintMouseCursor(bool paintTopPanel, QPainter *painter, QColor color, bool paintDb, bool paintFreq);

	void updateTopPanel(DisplayMode _newMode, bool updateSlider);
	QRect mapFrameToWidget(QFrame *frame);

	void recalcScale();

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

	SignalSpectrum *m_signalSpectrum; //Source of spectrum data
	double *m_averageSpectrum;
	double *m_lastSpectrum;
    //Holds values mapped to screen using utility in fft
	qint32 *m_fftMap;
	qint32 *m_topPanelFftMap;

	int m_upDownIncrement;
	int m_leftRightIncrement;

	double m_zoom; //Percentage of total spectrum to display

	Ui::SpectrumWidgetClass ui;

	//Show mixer freq
	qint32 m_fMixer;
	double m_loFreq;
	double m_mouseFreq; //Freq under mouse pointer

	int m_loFilter; //Used to display bandpass
	int m_hiFilter;

	qint16 m_plotMaxDb; //Set by UI
	qint16 m_plotMinDb;
	const quint16 m_minMaxDbDelta = 30; //Delta between min and max can never be less than this

	DisplayMode m_spectrumMode;
	int m_sampleRate;
	DeviceInterface::DemodMode m_demodMode;

	bool m_isRunning;
	//QString *message;
	QStringList m_message;

	QPixmap m_plotArea;
	QPixmap m_plotOverlay;
	QPixmap m_plotLabel;
	QPixmap m_topPanelPlotArea;
	QPixmap m_topPanelPlotOverlay;
	QPixmap m_topPanelPlotLabel;

	QPoint *m_lineBuf;

	int m_dbRange;

	QColor *m_spectrumColors;

    //Grid display, will change depending on plotArea size
	static const int m_maxHDivs = 50;

	int m_modeOffset;

	bool m_topPanelHighResolution; //True if top panel is in hi-res mode

	bool m_autoScaleMax;
	bool m_autoScaleMin;
	bool m_scaleNeedsRecalc;

	//Limits on what user can set ui controls
	const int m_maxDbScaleLimit = -50;
	const int m_minDbScaleLimit = -70;

};

#endif // SPECTRUMWIDGET_H
