#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalprocessing.h"
#include "demod.h"
#include <QMutex>
#include "settings.h"
#include "goertzel.h"
#include "DSP/fftw.h"

class SignalSpectrum :
	public SignalProcessing
{
    Q_OBJECT

public:
	//Moved from spectrumWidget to avoid .h circular ref.  We need to know mode so we can skip non-visible displays
    enum DISPLAYMODE {SPECTRUM = 0,WATERFALL,IQ,PHASE,NODISPLAY};

	SignalSpectrum(int sr, int ns, Settings *set);
	~SignalSpectrum(void);
	void SetDisplayMode(DISPLAYMODE m);
	//Pass in soundcard buffer under/overflow counts for display
	void Unprocessed(CPX * in, double inUnder, double inOver,double outUnder, double outOver);
    void MakeSpectrum(CPX *in, double *out, int size); //Use if we just have CPX samples
    void MakeSpectrum(FFTfftw *fft, double *out); //Used if we already have FFT

    bool MapFFTToScreen(qint32 maxHeight, qint32 maxWidth,
                                    double maxdB, double mindB,
                                    qint32 startFreq, qint32 stopFreq,
                                    qint32* outBuf );

    int BinCount() {return fftSize;}
    double *Unprocessed() {return unprocessed;}
	CPX *RawIQ() {return rawIQ;}

	double inBufferUnderflowCount;
	double inBufferOverflowCount;
	double outBufferUnderflowCount;
	double outBufferOverflowCount;

	Settings *settings;

    //New technique from CuteSdr to ignore spectrum data between updates
    void SetUpdatesPerSec(int updatespersec);
    bool displayUpdateComplete; //Wrap in access func after testing
    int displayUpdateOverrun; //temp counter

public slots:

signals:
    void newFftData(); //New spectrum data to display


private:
	QMutex mutex;
	DISPLAYMODE displayMode; //Don't FFT spectrum if that display is not active
    int fftSize;

	//Spectrum data at different steps in receive chaing
	CPX *rawIQ;
    double *unprocessed;
    double * window;

    CPX *tmp_cpx;
	CPX *window_cpx;
    FFTfftw *fft;

    float dbOffset; //Used to calibrate power to db calculations

    int updatesPerSec; //Refresh rate per second
    int skipFfts; //How many samples should we skip to sync with rate
    int skipFftsCounter; //Keep count of samples we've skipped

};
