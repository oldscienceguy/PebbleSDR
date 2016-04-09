//////////////////////////////////////////////////////////////////////
// testbench.h: interface of the CTestBench class.
//
//  This class creates a test bench dialog that generates complex
// signals and displays complex data and spectrum for testing and debug
//
// History:
//	2010-12-18  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//=============================================================================
#ifndef TESTBENCH_H
#define TESTBENCH_H

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include <QDialog>
#include <QtGui>
#include <QFrame>
#include <QImage>

#include "fftcute.h"
#include "demod/demod_wfm.h"


//////////////////////////////////////////////////////////////////////
//  global defines
//////////////////////////////////////////////////////////////////////

#define TEST_FFTSIZE 2048

#define TB_HORZ_DIVS 10
#define TB_MAX_SCREENSIZE 2048


typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
		unsigned char b2;
		unsigned char b3;
	}bytes;
	int all;
}tBtoL2;


namespace Ui {
	class TestBench;
}

class TestBench : public QDialog
{
    Q_OBJECT

public:
	explicit TestBench(QWidget *parent = 0);
	~TestBench();
	void init();	//called to initialize controls after setting all class variables

	void createGeneratorSamples(int length, TYPECPX* pBuf, double samplerate);
	void createGeneratorSamples(int length, TYPEREAL* pBuf, double samplerate);

	void sendDebugTxt(QString Str){ if(m_active) emit sendTxt(Str);}

	//Exposed Dialog Class variables for persistant saving/restoring by parent
	bool m_timeDisplay;
	bool m_genOn;
	bool m_peakOn;
	bool m_newDataIsCpx;
	bool m_currentDataIsCpx;
	bool m_useFmGen;
	int m_profile;
	int m_trigIndex;
	int m_displayRate;
	int m_horzSpan;
	int m_vertRange;
	int m_trigLevel;
	double m_pulseWidth;
	double m_pulsePeriod;
	double m_signalPower;
	double m_noisePower;
	double m_sweepStartFrequency;
	double m_sweepStopFrequency;
	double m_sweepRate;

    //Added by RAL
    //These variables are controlled by testbench UI and can be used anywhere we need a temporary control
    //Example would be an adjustable gain somewhere we don't have a UI to change.
    double testBenchValue;
	bool m_debugOn;
	bool m_noiseOn;

	void resetProfiles(); //Delete all profiles

	void mixNoiseSamples(int length, CPX *pBuf, double samplerate);

public slots:
    bool AddProfile(QString profileName, int profileNumber); //false if profilenumber already exists
	void removeProfile(quint16 profileNumber);

    // overloaded data display routines
	void displayData(int n, TYPECPX* pBuf, double samplerate, int profile);
	void displayData(int n, TYPEREAL* pBuf, double samplerate, int profile);
	void displayData(int n, TYPEMONO16* pBuf, double samplerate, int profile);
	void displayData(int n, TYPESTEREO16* pBuf, double samplerate, int profile);

	void reset();		//called by GUI Reset button
	void drawFftPlot();	//called to draw new fft data onto screen plot
	void drawTimePlot();	//called to draw new Time data onto screen plot
	void gotTxt(QString);
	void onTimer();

	void onGenOn(bool On);
	void onFmGen(bool On);
	void onTimeDisplay(bool timemode);
	void onEnablePeak(bool enablepeak);
	void onSweepStart(int start);
	void onSweepStop(int stop);
	void onSweepRate(int rate);
	void onDisplayRate(int rate);
	void onVertRange(int range);
	void onHorzSpan(int span);
	void onTrigLevel(int level);
	void onTriggerMode(int trigindex);
	void onProfile(int profindex);
	void onPulseWidth(int pwidth);
	void onPulsePeriod(int pperiod);
	void onSignalPwr(int pwr);
	void onNoisePwr(int pwr);
	void onTestSlider1(int val);
	void onDebugBox(bool b);
	void onDebugClear(bool b);
	void onNoiseBox(bool b);

signals:
	void resetSignal();		//internal signals from worker thread called functions
	void newFftData();
	void newTimeData();
	void sendTxt(QString);

protected:
		//re-implemented widget event handlers
	void resizeEvent(QResizeEvent* event);
	void paintEvent(QPaintEvent *event);
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);

private:
	Ui::TestBench *ui;
	void drawFreqOverlay();
	void drawTimeOverlay();
	void makeFrequencyStrs();
	void chkForTrigger(qint32 sample);
	quint64 m_rdtsctime();
	QPixmap m_2DPixmap;
	QPixmap m_overlayPixmap;
	QSize m_size;
	QRect m_rect;
	QTimer *m_pTimer;
	bool m_active;
	qint32 m_span;
	qint32 m_maxDb;
	qint32 m_minDb;
	qint32 m_dBStepSize;
	qint32 m_freqUnits;
	qint64 m_centerFreq;
	double m_genSampleRate;
	double m_displaySampleRate;
	QString m_str;
	QString m_hDivText[TB_HORZ_DIVS+1];
	TYPECPX m_fftInBuf[TEST_FFTSIZE];
	qint32 m_fftPkBuf[TB_MAX_SCREENSIZE];
	qint32 m_timeBuf1[TB_MAX_SCREENSIZE];
	qint32 m_timeBuf2[TB_MAX_SCREENSIZE];
	qint32 m_timeScrnBuf1[TB_MAX_SCREENSIZE];
	qint32 m_timeScrnBuf2[TB_MAX_SCREENSIZE];
	qint32 m_previousSample;

	int m_fftBufPos;
	qint32 m_displaySkipValue;
	qint32 m_displaySkipCounter;
	int m_timeScrnPos;
	int m_timeInPos;
	int m_trigBufPos;
	int m_trigState;
	int m_trigCounter;
	int m_postScrnCaptureLength;
	double m_timeScrnPixel;

	double m_sweepFrequency;
	double m_sweepFreqNorm;
	double m_sweepAcc;
	double m_sweepRateInc;
	double m_signalAmplitude;
	double m_noiseAmplitude;
	double m_pulseTimer;

	CFft m_fft;
	double m_spectrumBuf[TEST_FFTSIZE];

	QFile m_file;
    //CWFmMod* m_pWFmMod;

	//Display is updated from producer thread and needs to be protected if we make changes via TB UI
	QMutex m_displayMutex;
};

#endif // TESTBENCH_H
