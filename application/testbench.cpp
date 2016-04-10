//////////////////////////////////////////////////////////////////////
// testbench.cpp: implementation of the CTestBench class.
//
//  This class creates a test bench dialog that generates complex
// signals and displays complex data and spectrum for testing and debug
//
// History:
//	2010-12-18  Initial creation MSW
//	2011-03-27  Initial release
//	2011-08-07  Added WFM test generator
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
//==========================================================================================

//UI and functionality adapted and extended for Pebble by Richard Landsman
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "testbench.h"
#include "ui_testbench.h"
#include <QDebug>
#include "nco.h"

#define USE_FILE 0
//#define FILE_NAME "SSB-7210000Hz_001.wav"
#define FILE_NAME "fmstereo250Khz.wav"
#define USE_SVFILE 1
#define USE_PERSEUSFILE 0

//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////
#define TB_VERT_DIVS 14 //18	//specify grid screen divisions
#define TB_TIMEVERT_DIVS 10	//specify time display grid screen divisions

#define FFT_AVE 2

#define TRIG_OFF 0
#define TRIG_PNORM 1
#define TRIG_PSINGLE 2
#define TRIG_NNORM 3
#define TRIG_NSINGLE 4

#define TRIGSTATE_WAIT 0
#define TRIGSTATE_CAPTURE 1
#define TRIGSTATE_DISPLAY 2
#define TRIGSTATE_WAITDISPLAY 3

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
TestBench::TestBench(QWidget *parent) :
    QDialog(parent),
	ui(new Ui::TestBench)
{
	m_active = false;
	m_2DPixmap = QPixmap(0,0);
	m_overlayPixmap = QPixmap(0,0);
	m_size = QSize(0,0);
	m_rect = QRect(0,0,100,100);

	m_maxDb = DB::maxDb;
	m_minDb = DB::minDb;
	m_dBStepSize = 10;
	m_freqUnits = 1;
	m_centerFreq = 0;
	m_genSampleRate = 1;
	m_displaySampleRate = 1;
	m_span = m_displaySampleRate/2;
	m_fftBufPos = 0;

	m_genOn = false;
	m_peakOn = false;
	m_newDataIsCpx = false;
	m_currentDataIsCpx = true;
	m_timeDisplay = false;
	m_displayRate = 10;
	m_horzSpan = 100;
	m_vertRange = 65000;
	m_trigLevel = 100;
	m_trigBufPos = 0;
	m_trigCounter = 0;
	m_trigState = TRIGSTATE_WAIT;

	m_pulseWidth = .01;
	m_pulsePeriod = .5;
	m_pulseTimer = 0.0;

    //m_pWFmMod = NULL;

	connect(this, SIGNAL(resetSignal()), this,  SLOT( reset() ) );
	connect(this, SIGNAL(newFftData()), this,  SLOT( drawFftPlot() ) );
	connect(this, SIGNAL(newTimeData()), this,  SLOT( drawTimePlot() ) );
	connect( this, SIGNAL( sendTxt(QString)), this, SLOT( gotTxt(QString) ) );

	m_fft.FFTParams( 2048, 0.0, m_genSampleRate, 2048, WindowFunction::BLACKMANHARRIS);

	ui->setupUi(this);
    setWindowTitle("Pebble Test Bench");
	ui->textEdit->clear();
	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimer()));

#if USE_FILE	//test file reading kludge
	QDir::setCurrent("d:/");
	m_File.setFileName(FILE_NAME);
	if(m_File.open(QIODevice::ReadOnly))
	{
		qDebug()<<"file Opend OK";
		if(USE_SVFILE)
			m_File.seek(0x7e);		//SV
		else if(USE_PERSEUSFILE)
			m_File.seek(0x7A);		//perseus
	}
	else
		qDebug()<<"file Failed to Open";
#endif

    //m_pWFmMod = new CWFmMod();

	connect(ui->horizontalSliderTest,SIGNAL(valueChanged(int)),this,SLOT(onTestSlider1(int)));
    testBenchValue = 0.0;

	m_debugOn = true;
	ui->debugOutputGroup->setChecked(m_debugOn);
	connect(ui->debugOutputGroup,SIGNAL(toggled(bool)),this,SLOT(onDebugBox(bool)));

	connect(ui->debugClearButton,SIGNAL(clicked(bool)),this,SLOT(onDebugClear(bool)));

	m_noiseOn = false;
	ui->noiseGeneratorGroup->setChecked(m_noiseOn);
	connect(ui->noiseGeneratorGroup,SIGNAL(toggled(bool)),this,SLOT(onNoiseBox(bool)));

}

TestBench::~TestBench()
{
	if(m_file.isOpen())
		m_file.close();
//	if(m_pWFmMod)
//		delete m_pWFmMod;
    delete ui;
}

//////////////////////////////////////////////////////////////////////
//  Handle window close and show events
//////////////////////////////////////////////////////////////////////
void TestBench::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);
	m_active = false;
	m_pTimer->stop();		//stop timer
}

void TestBench::showEvent(QShowEvent *event)
{
	Q_UNUSED(event);
	m_active = true;
    m_pTimer->start(500);		//start up timer
}

void TestBench::onTestSlider1(int val)
{
    testBenchValue = (double)val/100.0;
}

void TestBench::onDebugBox(bool b)
{
	m_debugOn = b;
}

void TestBench::onDebugClear(bool b)
{
    ui->textEdit->clear();
}

void TestBench::onNoiseBox(bool b)
{
	m_noiseOn = b;
}

//////////////////////////////////////////////////////////////////////
// Called by parent to Initialize testbench controls after persistent
// variables are initialized
//////////////////////////////////////////////////////////////////////
void TestBench::init()
{
	int tmp = m_trigIndex;	//save since adding items changes m_TrigIndex
	ui->comboBoxTrig->clear();
	ui->comboBoxTrig->addItem("Free Run");
	ui->comboBoxTrig->addItem("Pos Edge");
	ui->comboBoxTrig->addItem("Pos Single");
	ui->comboBoxTrig->addItem("Neg Edge");
	ui->comboBoxTrig->addItem("Neg Single");
	m_trigIndex = tmp;
	ui->comboBoxTrig->setCurrentIndex(m_trigIndex);

    ui->comboBoxProfile->blockSignals(true);
    ui->comboBoxProfile->clear();
    ui->comboBoxProfile->addItem("Off",0);
    ui->comboBoxProfile->blockSignals(false);
    ui->comboBoxProfile->setCurrentIndex(0);
	m_profile = 0;

	ui->spinBoxStart->setValue(m_sweepStartFrequency/1000.0);
	ui->spinBoxStop->setValue(m_sweepStopFrequency/1000.0);
	ui->spinBoxSweep->setValue(m_sweepRate);


	ui->timeDomainGroup->setChecked(m_timeDisplay);
	connect(ui->timeDomainGroup, SIGNAL(toggled(bool)),this,SLOT(onTimeDisplay(bool)));


	ui->spinBoxHorzSpan->setValue(m_horzSpan);

	ui->spinBoxThresh->setValue(m_trigLevel);
	ui->spinBoxVertRange->setValue(m_vertRange);
	ui->spinBoxRate->setValue(m_displayRate);

	ui->signalGeneratorGroup->setChecked(m_genOn);
	connect(ui->signalGeneratorGroup, SIGNAL(toggled(bool)),this,SLOT(onGenOn(bool)));

	ui->noiseGeneratorGroup->setChecked(m_noiseOn);

	ui->checkBoxFm->setChecked(m_useFmGen);
	ui->checkBoxPeak->setChecked(m_peakOn);
	ui->spinBoxAmp->setValue((int)m_signalPower);
	ui->spinBoxNoise->setValue((int)m_noisePower);
	ui->spinBoxPulseWidth->setValue((int)(1000.0*m_pulseWidth));
	ui->spinBoxPulsePeriod->setValue((int)(1000.0*m_pulsePeriod));

	reset();
}

//Profile # starts with 1, 0 means no profiles
bool TestBench::addProfile(QString profileName, int profileNumber)
{
    //See if profileNumber already exists in menu and remove
	removeProfile(profileNumber);

    ui->comboBoxProfile->blockSignals(true);
    ui->comboBoxProfile->addItem(profileName,profileNumber);
    ui->comboBoxProfile->blockSignals(false);
    return true;
}

void TestBench::removeProfile(quint16 profileNumber)
{
    int item = ui->comboBoxProfile->findData(profileNumber);
    if (item > 0) {
        ui->comboBoxProfile->blockSignals(true);
        ui->comboBoxProfile->removeItem(item);
        ui->comboBoxProfile->blockSignals(false);
    }
}

void TestBench::resetProfiles()
{
    int profileNumber;
    for (int i=1; i<=ui->comboBoxProfile->count(); i++) {
        profileNumber = ui->comboBoxProfile->itemData(i).toInt();
		removeProfile(profileNumber);
    }
}

//////////////////////////////////////////////////////////////////////
// Bunch of Control Slot functions called when a control value changes
//////////////////////////////////////////////////////////////////////
void TestBench::onSweepStart(int start)
{
	m_sweepStartFrequency = (double)start*1000.0;
	m_sweepFrequency = m_sweepStartFrequency;
	m_sweepAcc = 0.0;
//	if(m_UseFmGen)
//		m_pWFmMod->SetSweep(m_SweepFreqNorm,m_SweepFrequency,m_SweepStopFrequency,m_SweepRateInc);
}

void TestBench::onSweepStop(int stop)
{
	m_sweepStopFrequency = (double)stop*1000.0;
	m_sweepFrequency = m_sweepStartFrequency;
	m_sweepAcc = 0.0;
//	if(m_UseFmGen)
//		m_pWFmMod->SetSweep(m_SweepFreqNorm,m_SweepFrequency,m_SweepStopFrequency,m_SweepRateInc);
}

void TestBench::onSweepRate(int rate)
{
	m_sweepRate = (double)rate; // Hz/sec
	m_sweepAcc = 0.0;
	m_sweepRateInc = m_sweepRate/m_genSampleRate;
//	if(m_UseFmGen)
//		m_pWFmMod->SetSweep(m_SweepFreqNorm,m_SweepFrequency,m_SweepStopFrequency,m_SweepRateInc);

}


void TestBench::onDisplayRate(int rate)
{
	m_displayRate = rate;
	if(m_timeDisplay)
	{
		double capturesize = ( (double)m_horzSpan*m_displaySampleRate/1000.0);
		m_displaySkipValue = m_displaySampleRate/(capturesize*m_displayRate);
	}
	else
	{
		m_displaySkipValue = m_displaySampleRate/(TEST_FFTSIZE*m_displayRate);
	}
}

void TestBench::onVertRange(int range )
{
	m_vertRange = range;
	ui->spinBoxThresh->setMaximum(m_vertRange/2);
	ui->spinBoxThresh->setMinimum(-m_vertRange/2);
	if(m_timeDisplay)
		drawTimeOverlay();
}

void TestBench::onHorzSpan(int span)
{
	m_horzSpan = span;
	if(m_timeDisplay)
	{
		drawTimeOverlay();
		double capturesize = ( (double)m_horzSpan*m_displaySampleRate/1000.0);
		m_displaySkipValue = m_displaySampleRate/(capturesize*m_displayRate);
		m_timeScrnPixel = .001* (double)((double)m_horzSpan/(double)m_rect.width());	//time per pixel on screen in seconds
	}
}

void TestBench::onTimeDisplay(bool timemode)
{
	m_timeDisplay = timemode;
	reset();
}

void TestBench::onTriggerMode(int trigindex)
{
	m_trigIndex = trigindex;
	reset();
}

void TestBench::onTrigLevel(int level)
{
	m_trigLevel = level;
	if(m_timeDisplay)
		drawTimeOverlay();
}

void TestBench::onProfile(int profindex)
{
    //Get the item data
    int profile = ui->comboBoxProfile->itemData(profindex).toInt();
	m_profile = profile;

}

void TestBench::onGenOn(bool On)
{
	m_genOn = On;
}

void TestBench::onFmGen(bool On)
{
	m_useFmGen = On;
}


void TestBench::onPulseWidth(int pwidth)
{
	m_pulseWidth = (double)pwidth * .001;
}

void TestBench::onPulsePeriod(int pperiod)
{
	m_pulsePeriod = (double)pperiod * .001;
}

void TestBench::onSignalPwr(int pwr)
{
	m_signalPower = pwr;
	//m_SignalAmplitude = m_Fft.ampMax*pow(10.0, m_SignalPower/20.0);
	m_signalAmplitude = m_fft.ampMax * DB::dBToAmplitude(m_signalPower);
}

void TestBench::onNoisePwr(int pwr)
{
	//UI selection changed for noise power.  UI returns -160 to 0 in db
	m_noisePower = pwr;
	//m_NoiseAmplitude = m_Fft.ampMax*pow(10.0, m_NoisePower/20.0);
	m_noiseAmplitude = m_fft.ampMax * DB::dBToAmplitude(m_noisePower);
}

void TestBench::onEnablePeak(bool enablepeak)
{
	for( int i=0; i<TB_MAX_SCREENSIZE; i++)
	{
		m_fftPkBuf[i] = m_rect.height();	//set pk buffer to minimum screen posistion
		m_timeBuf1[i] = 0;
		m_timeBuf2[i] = 0;
	}
	m_peakOn = enablepeak;
}



//////////////////////////////////////////////////////////////////////
// Call to Create 'length' complex sweep/pulse/noise generator samples
//and place in users pBuf.
//Called by thread so no GUI calls!
//////////////////////////////////////////////////////////////////////
void TestBench::createGeneratorSamples(int length, TYPECPX* pBuf, double samplerate)
{
int i;
	if(!m_active || !m_genOn)
		return;
	if(m_genSampleRate != samplerate)
	{	//reset things if sample rate changes on the fly
		m_genSampleRate = samplerate;
//		if(m_UseFmGen)
//		{
//			m_pWFmMod->SetSampleRate(m_GenSampleRate);
//			m_pWFmMod->SetSweep(m_SweepFreqNorm,m_SweepFrequency,m_SweepStopFrequency,m_SweepRateInc);
//		}
		emit resetSignal();
	}

//	if(m_UseFmGen)
//		m_pWFmMod->GenerateData(length, m_SignalAmplitude, pBuf);

    bool mix = ui->genMixBox->checkState();

	for(i=0; i<length; i++)
	{
		double amp = m_signalAmplitude;
		if(m_pulseWidth > 0.0)
		{	//if pulse width is >0 create pulse modulation
			m_pulseTimer += (1.0/m_genSampleRate);
			if(m_pulseTimer > m_pulsePeriod)
				m_pulseTimer = 0.0;
			if(m_pulseTimer > m_pulseWidth)
				amp = 0.0;
		}

#if 0		//way to skip over passband for filter alias testing
if( (m_SweepFrequency>-31250) && (m_SweepFrequency<31250) )
{
	m_SweepFrequency = 31250;
	amp = 0.0;
	m_Fft.ResetFFT();
	m_DisplaySkipCounter = -2;
}
#endif

		if(!m_useFmGen)
        {

            //create complex sin/cos signal
            if (mix) {
                //Add signal to incoming
				pBuf[i].re += amp*cos(m_sweepAcc);
				pBuf[i].im += amp*sin(m_sweepAcc);
            } else {
                //Replace incoming signal with generator
				pBuf[i].re = amp*cos(m_sweepAcc);
				pBuf[i].im = amp*sin(m_sweepAcc);
            }
            //inc phase accummulator with normalized freqeuency step
			m_sweepAcc += ( m_sweepFrequency*m_sweepFreqNorm );
			m_sweepFrequency += m_sweepRateInc;	//inc sweep frequency
			if(m_sweepFrequency >= m_sweepStopFrequency) {	//reached end of sweep?
                //3 choices: stop, start over, return in opposite direction
				m_sweepRateInc = 0.0;						//stop sweep when end is reached
                //m_SweepFrequency = m_SweepStartFrequency;	//restart sweep when end is reached
            }
        }

	}
	m_sweepAcc = (double)fmod((double)m_sweepAcc, TWOPI);	//keep radian counter bounded
}

void TestBench::mixNoiseSamples(int length, TYPECPX* pBuf, double samplerate)
{
	if(!m_active || !m_noiseOn)
        return;

#if 0
	if (noiseOn) {
		NCO::genNoise(pBuf, length, m_NoiseAmplitude, true);
		//DB::analyzeCPX(pBuf,length,"Noise Gen");
	}
#else

    double u1;
    double u2;
    double rad;
    double r;

    //Different types of noise generators
    for(int i=0; i<length; i++) {

        //////////////////  Gaussian Noise generator
        // Generate two uniform random numbers between -1 and +1
        // that are inside the unit circle
		if(m_noiseOn && m_noisePower > -160.0)
        {	//create and add noise samples to signal
            do {
                u1 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
                u2 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
                r = u1*u1 + u2*u2;
            } while(r >= 1.0 || r == 0.0);
			//0<r<1
			rad = sqrt(-2.0*log(r)/r);
            //add noise samples to generator output
			pBuf[i].re += (m_noiseAmplitude*u1*rad) * m_fft.ampMax;
			pBuf[i].im += (m_noiseAmplitude*u2*rad) * m_fft.ampMax;
        }
    }
#endif

}


//////////////////////////////////////////////////////////////////////
// Call to Create 'length' real sweep/pulse/noise generator samples
//and place in users pBuf.
//Called by thread so no GUI calls!
//////////////////////////////////////////////////////////////////////
void TestBench::createGeneratorSamples(int length, TYPEREAL* pBuf, double samplerate)
{
int i;
double rad;
double r;
double u1;
double u2;
	if(!m_active || !m_genOn)
		return;
	if(m_genSampleRate != samplerate)
	{	//reset things if sample rate changes on the fly
		m_genSampleRate = samplerate;
		emit resetSignal();
	}
	for(i=0; i<length; i++)
	{
		double amp = m_signalAmplitude;
		if(m_pulseWidth > 0.0)
		{	//if pulse width is >0 create pulse modulation
			m_pulseTimer += (1.0/m_genSampleRate);
			if(m_pulseTimer > m_pulsePeriod)
				m_pulseTimer = 0.0;
			if(m_pulseTimer > m_pulseWidth)
				amp = 0.0;
		}

#if 0		//way to skip over passband for filter alias testing
if( (m_SweepFrequency>-31250) && (m_SweepFrequency<31250) )
{
	m_SweepFrequency = 31250;
	amp = 0.0;
	m_Fft.ResetFFT();
	m_DisplaySkipCounter = -2;
}
#endif

		//create cos signal
		pBuf[i] = 3.0*amp*cos(m_sweepAcc);
		//inc phase accummulator with normalized freqeuency step


		m_sweepAcc += ( m_sweepFrequency*m_sweepFreqNorm );
		m_sweepFrequency += m_sweepRateInc;	//inc sweep frequency
		if(m_sweepFrequency >= m_sweepStopFrequency)	//reached end of sweep?
			m_sweepRateInc = 0.0;						//stop sweep when end is reached
//			m_SweepFrequency = m_SweepStartFrequency;	//restart sweep when end is reached

		//////////////////  Gaussian Noise generator
		// Generate two uniform random numbers between -1 and +1
		// that are inside the unit circle
		if(m_noisePower > -160.0)
		{	//create and add noise samples to signal
			do {
				u1 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
				u2 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
				r = u1*u1 + u2*u2;
			} while(r >= 1.0 || r == 0.0);
			rad = sqrt(-2.0*log(r)/r);
			//add noise samples to generator output
			pBuf[i] += (m_noiseAmplitude*u1*rad);
		}
	}
	m_sweepAcc = (double)fmod((double)m_sweepAcc, TWOPI);	//keep radian counter bounded
}

//////////////////////////////////////////////////////////////////////
// Called to reset the generator and display peaks
//recalculates a bunch of variables based on settings
//////////////////////////////////////////////////////////////////////
void TestBench::reset()
{
	//Prevent display from updating while we're changing values
	m_displayMutex.lock();

	int i;
	//initialize sweep generator values
	m_sweepFrequency = m_sweepStartFrequency;
	m_sweepFreqNorm = TWOPI/m_genSampleRate;
	m_sweepAcc = 0.0;
	m_sweepRateInc = m_sweepRate/m_genSampleRate;
//	if(m_UseFmGen)
//		m_pWFmMod->SetSweep(m_SweepFreqNorm,m_SweepFrequency,m_SweepStopFrequency,m_SweepRateInc);
	//m_SignalAmplitude = m_Fft.ampMax*pow(10.0, m_SignalPower/20.0);
	m_signalAmplitude = m_fft.ampMax * DB::dBToAmplitude(m_signalPower);

	//m_NoiseAmplitude = m_Fft.ampMax*pow(10.0, m_NoisePower/20.0);
	m_noiseAmplitude = m_fft.ampMax * DB::dBToAmplitude(m_noisePower);

	//init FFT values
	m_fft.FFTParams(  TEST_FFTSIZE, 0.0, m_displaySampleRate, TEST_FFTSIZE, WindowFunction::BLACKMANHARRIS);
	m_fftBufPos = 0;
	m_span = m_displaySampleRate;
	m_span = ( m_span - (m_span+5)%10 + 5);

	//init time display values
	m_timeScrnPixel = .001* (double)((double)m_horzSpan/(double)m_rect.width());	//time per pixel on screen in seconds


	m_timeScrnPos = 0;
	m_timeInPos = 0;
	m_previousSample = 0;
	m_postScrnCaptureLength = (7*m_rect.width())/10;	//sets trigger position
	m_trigState = TRIGSTATE_WAIT;

	for(i=0; i<TEST_FFTSIZE; i++)
	{
		m_fftInBuf[i].re = 0.0;
		m_fftInBuf[i].im = 0.0;
	}
	for( i=0; i<TB_MAX_SCREENSIZE; i++)
	{
		m_fftPkBuf[i] = m_rect.height();	//set pk buffer to minimum screen posistion
		m_timeBuf1[i] = 0;
		m_timeBuf2[i] = 0;
	}
	if(m_timeDisplay)
	{
		drawTimeOverlay();
		double capturesize = ( (double)m_horzSpan*m_displaySampleRate/1000.0);
		m_displaySkipValue = m_displaySampleRate/(capturesize*m_displayRate);
	}
	else
	{
		drawFreqOverlay();
		m_displaySkipValue = m_displaySampleRate/(TEST_FFTSIZE*m_displayRate);
	}
	ui->textEdit->clear();
	m_fft.ResetFFT();
	m_displaySkipCounter = -2;
	m_pulseTimer = 0.0;
	m_displayMutex.unlock();
 }

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// COMPLEX Data version.
//////////////////////////////////////////////////////////////////////
void TestBench::displayData(int length, TYPECPX* pBuf, double samplerate, int profile)
{
	if(!m_active || (profile!=m_profile) )
		return;
	if(m_displaySampleRate != samplerate)
	{
		m_displaySampleRate = samplerate;
		emit resetSignal();
		return;
	}

	m_displayMutex.lock();

	m_newDataIsCpx = true;
	if(! m_timeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_fftInBuf[m_fftBufPos++] = (pBuf[i]);

			if(m_fftBufPos >= TEST_FFTSIZE )
			{
				m_fftBufPos = 0;
				if(++m_displaySkipCounter >= m_displaySkipValue )
				{
					m_displaySkipCounter = 0;
					m_fft.FFTSpectrum(m_fftInBuf,m_spectrumBuf, TEST_FFTSIZE);
					emit newFftData();
				}
			}
		}
	}
	else
	{	//if displaying time domain data
		for(int i=0; i<length; i++)
		{
			//calc to time variables, one in sample time units and
			//one in screen pixel units
			double intime = (double)m_timeInPos/samplerate;
			double scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
			m_timeInPos++;
			while(intime >= scrntime)
			{
                //Pebble -1 to +1 FP - TestBench -32767 to +32767
				chkForTrigger(pBuf[i].re * 32767);
				m_timeBuf1[m_timeScrnPos] = pBuf[i].re * 32767;
				m_timeBuf2[m_timeScrnPos++] = pBuf[i].im * 32767;
				scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
				if( m_timeScrnPos >= m_rect.width() )
				{
					m_timeScrnPos = 0;
					m_timeInPos = 0;
					break;
				}
			}
		}
	}
	m_displayMutex.unlock();
}

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// REAL Data version.
//////////////////////////////////////////////////////////////////////
void TestBench::displayData(int length, TYPEREAL* pBuf, double samplerate, int profile)
{
	if(!m_active || (profile!=m_profile) )
		return;
	if(m_displaySampleRate != samplerate)
	{
		m_displaySampleRate = samplerate;
		emit resetSignal();
		return;
	}
	m_newDataIsCpx = false;
	if(!m_timeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_fftInBuf[m_fftBufPos].re = pBuf[i];
			m_fftInBuf[m_fftBufPos++].im = 0.0;
			if(m_fftBufPos >= TEST_FFTSIZE )
			{
				m_fftBufPos = 0;
				if(++m_displaySkipCounter >= m_displaySkipValue )
				{
					m_displaySkipCounter = 0;
					m_fft.FFTSpectrum(m_fftInBuf, m_spectrumBuf, TEST_FFTSIZE);

					emit newFftData();
				}
			}
		}
	}
	else
	{
		for(int i=0; i<length; i++)
		{
			double intime = (double)m_timeInPos/samplerate;
			double scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
			m_timeInPos++;
			while(intime >= scrntime)
			{
				chkForTrigger( (int)pBuf[i] );
				m_timeBuf1[m_timeScrnPos] = (int)pBuf[i];
				m_timeBuf2[m_timeScrnPos++] = 0;
				scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
				if( m_timeScrnPos >= m_rect.width() )
				{
					m_timeScrnPos = 0;
					m_timeInPos = 0;
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// MONO 16 bit Data version.
//////////////////////////////////////////////////////////////////////
void TestBench::displayData(int length, TYPEMONO16* pBuf, double samplerate, int profile)
{
	if(!m_active || (profile!=m_profile) )
		return;
	if(m_displaySampleRate != samplerate)
	{
		m_displaySampleRate = samplerate;
		emit resetSignal();
		return;
	}
	m_newDataIsCpx = false;
	if(!m_timeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_fftInBuf[m_fftBufPos].re = (TYPEREAL)pBuf[i];
			m_fftInBuf[m_fftBufPos++].im = (TYPEREAL)pBuf[i];
			if(m_fftBufPos >= TEST_FFTSIZE )
			{
				m_fftBufPos = 0;
				if(++m_displaySkipCounter >= m_displaySkipValue )
				{
					m_displaySkipCounter = 0;
					m_fft.FFTSpectrum(m_fftInBuf, m_spectrumBuf, TEST_FFTSIZE);

					emit newFftData();
				}
			}
		}
	}
	else
	{
		for(int i=0; i<length; i++)
		{
			double intime = (double)m_timeInPos/samplerate;
			double scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
			m_timeInPos++;
			while(intime >= scrntime)
			{
				chkForTrigger( (int)pBuf[i<<1] );
				m_timeBuf1[m_timeScrnPos] = (int)pBuf[i];
				m_timeBuf2[m_timeScrnPos++] = 0;
				scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
				if( m_timeScrnPos >= m_rect.width() )
				{
					m_timeScrnPos = 0;
					m_timeInPos = 0;
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// STEREO 16 bit Data version.
//////////////////////////////////////////////////////////////////////
void TestBench::displayData(int length, TYPESTEREO16* pBuf, double samplerate, int profile)
{
	if(!m_active || (profile!=m_profile) )
		return;
	if(m_displaySampleRate != samplerate)
	{
		m_displaySampleRate = samplerate;
		emit resetSignal();
		return;
	}
	m_newDataIsCpx = true;
	if(! m_timeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_fftInBuf[m_fftBufPos].re = (TYPEREAL)pBuf[i].re;
			m_fftInBuf[m_fftBufPos++].im = (TYPEREAL)pBuf[i].im;
			if(m_fftBufPos >= TEST_FFTSIZE )
			{
				m_fftBufPos = 0;
				if(++m_displaySkipCounter >= m_displaySkipValue )
				{
					m_displaySkipCounter = 0;
					m_fft.FFTSpectrum(m_fftInBuf, m_spectrumBuf, TEST_FFTSIZE);

					emit newFftData();
				}
			}
		}
	}
	else
	{	//if displaying time domain data
		for(int i=0; i<length; i++)
		{
			double intime = (double)m_timeInPos/samplerate;
			double scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
			m_timeInPos++;
			while(intime >= scrntime)
			{
				chkForTrigger( (int)pBuf[i].re );
				m_timeBuf1[m_timeScrnPos] = pBuf[i].re;
				m_timeBuf2[m_timeScrnPos++] = pBuf[i].im;
				scrntime = (double)m_timeScrnPos*m_timeScrnPixel;
				if( m_timeScrnPos >= m_rect.width() )
				{
					m_timeScrnPos = 0;
					m_timeInPos = 0;
					break;
				}
			}

		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to check for O'Scope display trigger condition and manage trigger capture logic.
//////////////////////////////////////////////////////////////////////
void TestBench::chkForTrigger(qint32 sample)
{
	switch(m_trigIndex)
	{
		case TRIG_OFF:
			if( 0 == m_timeScrnPos )
			{
				if(( ++m_displaySkipCounter >= m_displaySkipValue ) &&
					  ( m_displaySkipCounter > 2) )
				{
					m_displaySkipCounter = 0;
					m_trigBufPos = 0;
					m_trigState = TRIGSTATE_DISPLAY;
				}
			}
			break;
		case TRIG_PNORM:
		case TRIG_PSINGLE:
				if(TRIGSTATE_WAIT == m_trigState)
				{
					//look for positive transition through trigger level
					if( (sample>=m_trigLevel) && (m_previousSample<m_trigLevel) )
					{
						m_trigBufPos = m_timeScrnPos;
						m_trigState = TRIGSTATE_CAPTURE;
						m_trigCounter = 0;
					}
				}
				else if(TRIGSTATE_CAPTURE == m_trigState)
				{
					if(++m_trigCounter >= m_postScrnCaptureLength )
					{
						m_trigState = TRIGSTATE_DISPLAY;
						m_trigCounter = 0;
					}
				}
			break;
		case TRIG_NNORM:
		case TRIG_NSINGLE:
			if(TRIGSTATE_WAIT == m_trigState)
			{
				//look for negative transition through trigger level
				if( (sample<=m_trigLevel) && (m_previousSample>m_trigLevel) )
				{
					m_trigBufPos = m_timeScrnPos;
					m_trigState = TRIGSTATE_CAPTURE;
					m_trigCounter = 0;
				}
			}
			else if(TRIGSTATE_CAPTURE == m_trigState)
			{
				if(++m_trigCounter >= m_postScrnCaptureLength )
				{
					m_trigState = TRIGSTATE_DISPLAY;
					m_trigCounter = 0;
				}
			}
			break;
		default:
			break;
	}
	if(TRIGSTATE_DISPLAY == m_trigState)
	{
		m_trigState = TRIGSTATE_WAITDISPLAY;
		//copy into screen display buffers
		int w = m_2DPixmap.width();
		int bufpos = m_trigBufPos + m_postScrnCaptureLength - w;
		if(bufpos<0)
			bufpos = w + bufpos;
		for(int i=0; i<w; i++)
		{	//copy into display buffers then signal for screen update
			m_timeScrnBuf1[i] = m_timeBuf1[bufpos];
			m_timeScrnBuf2[i] = m_timeBuf2[bufpos++];
			if(bufpos>=w)
				bufpos = 0;
		}
		emit newTimeData();
	}
	m_previousSample = sample;
}

//////////////////////////////////////////////////////////////////////
// Called to display text in debug text box.
//////////////////////////////////////////////////////////////////////
void TestBench::gotTxt(QString Str)
{
	if (m_debugOn)
        ui->textEdit->append(Str);
}

/////////////////////////////////////////////////////////////////////
// Status Timer event handler
/////////////////////////////////////////////////////////////////////
void TestBench::onTimer()
{
	//update current sweep frequency text
	ui->labelFreq->setText(QString().setNum((int)m_sweepFrequency));
}

//////////////////////////////////////////////////////////////////////
// Called when screen size changes so must recalculate bitmaps
//////////////////////////////////////////////////////////////////////
void TestBench::resizeEvent(QResizeEvent* )
{
	if(!ui->frameGraph->size().isValid())
		return;
	if( m_rect != ui->frameGraph->geometry())
	{	//if changed, resize pixmaps to new frame size
		m_rect = ui->frameGraph->geometry();
		m_overlayPixmap = QPixmap(m_rect.width(), m_rect.height());
		m_overlayPixmap.fill(Qt::black);
		m_2DPixmap = QPixmap(m_rect.width(), m_rect.height());
		m_2DPixmap.fill(Qt::black);
		reset();
	}
	if(m_timeDisplay)
		drawTimeOverlay();
	else
		drawFreqOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called by QT when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void TestBench::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawPixmap(m_rect, m_2DPixmap);
	return;
}

//////////////////////////////////////////////////////////////////////
// Called to update time domain data for displaying on the screen
//////////////////////////////////////////////////////////////////////
void TestBench::drawTimePlot()
{
int i;
int w;
int h;
QPoint LineBuf[TB_MAX_SCREENSIZE];
	if(m_2DPixmap.isNull())
		return;

	if(m_newDataIsCpx != m_currentDataIsCpx)
	{
		m_currentDataIsCpx = m_newDataIsCpx;
	}
	//get/draw the 2D spectrum
	w = m_2DPixmap.width();
	h = m_2DPixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_overlayPixmap.copy(0,0,w,h);
	QPainter painter2(&m_2DPixmap);

	//draw the time points
	int c = h/2;
	for(i=0; i<w; i++)
	{
		LineBuf[i].setX(i);
		LineBuf[i].setY(c - (2*c*m_timeScrnBuf1[i])/m_vertRange );
	}
	painter2.setPen( Qt::green );
	painter2.drawPolyline(LineBuf,w);

	if(m_currentDataIsCpx)
	{
		for(i=0; i<w; i++)
		{
			LineBuf[i].setX(i);
			LineBuf[i].setY(c - (2*c*m_timeScrnBuf2[i])/m_vertRange );
		}
		painter2.setPen( Qt::red );
		painter2.drawPolyline(LineBuf,w);
	}

	//trigger a new paintEvent
	update();
	if(	(TRIG_PSINGLE != m_trigIndex) &&
		(TRIG_NSINGLE != m_trigIndex) )
	{
		m_trigState = TRIGSTATE_WAIT;
	}
}

//////////////////////////////////////////////////////////////////////
// Called to update spectrum data for displaying on the screen
//////////////////////////////////////////////////////////////////////
void TestBench::drawFftPlot()
{
int i;
int w;
int h;
qint32 fftbuf[TB_MAX_SCREENSIZE];
QPoint LineBuf[TB_MAX_SCREENSIZE];
	if(m_2DPixmap.isNull())
		return;
	if(m_newDataIsCpx != m_currentDataIsCpx)
	{
		m_currentDataIsCpx = m_newDataIsCpx;
		drawFreqOverlay();
	}
	//get/draw the 2D spectrum
	w = m_2DPixmap.width();
	h = m_2DPixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_overlayPixmap.copy(0,0,w,h);
	QPainter painter2(&m_2DPixmap);
	//get new scaled fft data

	if(m_currentDataIsCpx)
	{
		m_fft.MapFFTToScreen( m_spectrumBuf, h, w,
							m_maxDb,
							m_minDb,
							-m_span/2,
							m_span/2,
							fftbuf );
	}
	else
	{
		m_fft.MapFFTToScreen( m_spectrumBuf, h, w,
							m_maxDb,
							m_minDb,
							0.0,
							m_span/2,
							fftbuf );

	}
	//draw the 2D spectrum
	for(i=0; i<w; i++)
	{
		LineBuf[i].setX(i);
        LineBuf[i].setY(fftbuf[i]);
		if(fftbuf[i] < m_fftPkBuf[i])
			m_fftPkBuf[i] = fftbuf[i];
	}
	painter2.setPen( Qt::green );
	painter2.drawPolyline(LineBuf,w);

	if(m_peakOn)
	{
		for(i=0; i<w; i++)
		{
			LineBuf[i].setX(i);
			LineBuf[i].setY(m_fftPkBuf[i]);
		}
		painter2.setPen( Qt::red );
		painter2.drawPolyline(LineBuf,w);
	}
	//trigger a new paintEvent
	update();
}

//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every Time data update.
//////////////////////////////////////////////////////////////////////
void TestBench::drawTimeOverlay()
{
	if(m_overlayPixmap.isNull())
		return;
int w = m_overlayPixmap.width();
int h = m_overlayPixmap.height();
int x,y;
float pixperdiv;
QRect rect;
	QPainter painter(&m_overlayPixmap);
	painter.initFrom(this);

	m_overlayPixmap.fill(Qt::black);

	//draw vertical grids
	pixperdiv = (float)w / (float)TB_HORZ_DIVS;
	y = h - h/TB_TIMEVERT_DIVS;
	painter.setPen(QPen(Qt::white, 0,Qt::DotLine));
	for( int i=1; i<TB_HORZ_DIVS; i++)
	{
		x = (int)( (float)i*pixperdiv );
		painter.drawLine(x, 0, x , y);
		painter.drawLine(x, h-5, x , h);
	}
	//create Font to use for scales
	QFont Font("Arial");
	Font.setPointSize(12);
	QFontMetrics metrics(Font);
	y = h/TB_TIMEVERT_DIVS;
	if(y<metrics.height())
		Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);

	//draw time values
	painter.setPen(Qt::cyan);
	y = h - (h/TB_TIMEVERT_DIVS);
	double xval = 0;
	for( int i=0; i<=TB_HORZ_DIVS; i++)
	{
		if(0==i)
		{	//left justify the leftmost text
			x = (int)( (float)i*pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_TIMEVERT_DIVS);
			painter.drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, QString::number(xval));
		}
		else if(TB_HORZ_DIVS == i)
		{	//right justify the rightmost text
			x = (int)( (float)i*pixperdiv - pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_TIMEVERT_DIVS);
			painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter,  QString::number(xval));
		}
		else
		{	//center justify the rest of the text
			x = (int)( (float)i*pixperdiv - pixperdiv/2);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_TIMEVERT_DIVS);
			painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter,  QString::number(xval));
		}
		xval += (double)m_horzSpan/(double)TB_HORZ_DIVS;
	}
	//draw horizontal grids
	pixperdiv = (float)h / (float)TB_TIMEVERT_DIVS;
	for( int i=1; i<TB_TIMEVERT_DIVS; i++)
	{
		y = (int)( (float)i*pixperdiv );
		if(i==TB_TIMEVERT_DIVS/2)
			painter.setPen(QPen(Qt::blue, 1,Qt::DotLine));
		else
			painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
		painter.drawLine(0, y, w, y);
	}

	painter.setPen(QPen(Qt::yellow, 1,Qt::DotLine));
	int c = h/2;
	y = c - (2*c*m_trigLevel)/m_vertRange;
	painter.drawLine(0, y, w, y);


	//draw amplitude values
	painter.setPen(Qt::darkYellow);
	Font.setWeight(QFont::Light);
	painter.setFont(Font);
	int yval = m_vertRange/2;
	for( int i=0; i<TB_TIMEVERT_DIVS-1; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawStaticText(5, y-1, QString::number(yval));
		yval -= (m_vertRange/10);
	}
	//copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_overlayPixmap.copy(0,0,w,h);
	//trigger a new paintEvent
	update();
}


//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
//////////////////////////////////////////////////////////////////////
void TestBench::drawFreqOverlay()
{
	if(m_overlayPixmap.isNull())
		return;
int w = m_overlayPixmap.width();
int h = m_overlayPixmap.height();
int x,y;
float pixperdiv;
QRect rect;
	QPainter painter(&m_overlayPixmap);
	painter.initFrom(this);

	m_overlayPixmap.fill(Qt::black);

	//draw vertical grids
	pixperdiv = (float)w / (float)TB_HORZ_DIVS;
	y = h - h/TB_VERT_DIVS;
	for( int i=1; i<TB_HORZ_DIVS; i++)
	{
		x = (int)( (float)i*pixperdiv );
		if(i==TB_HORZ_DIVS/2)
			painter.setPen(QPen(Qt::red, 0,Qt::DotLine));
		else
			painter.setPen(QPen(Qt::white, 0,Qt::DotLine));
		painter.drawLine(x, 0, x , y);
		painter.drawLine(x, h-5, x , h);
	}
	//create Font to use for scales
	QFont Font("Arial");
	Font.setPointSize(12);
	QFontMetrics metrics(Font);
	y = h/TB_VERT_DIVS;
	if(y<metrics.height())
		Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);

	//draw frequency values
	makeFrequencyStrs();
	painter.setPen(Qt::cyan);
	y = h - (h/TB_VERT_DIVS);
	for( int i=0; i<=TB_HORZ_DIVS; i++)
	{
		if(0==i)
		{	//left justify the leftmost text
			x = (int)( (float)i*pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_VERT_DIVS);
			painter.drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, m_hDivText[i]);
		}
		else if(TB_HORZ_DIVS == i)
		{	//right justify the rightmost text
			x = (int)( (float)i*pixperdiv - pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_VERT_DIVS);
			painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, m_hDivText[i]);
		}
		else
		{	//center justify the rest of the text
			x = (int)( (float)i*pixperdiv - pixperdiv/2);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_VERT_DIVS);
			painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, m_hDivText[i]);
		}
	}

	//draw horizontal grids
	pixperdiv = (float)h / (float)TB_VERT_DIVS;
	painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
	for( int i=1; i<TB_VERT_DIVS; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawLine(0, y, w, y);
	}

	//draw amplitude values
	painter.setPen(Qt::darkYellow);
    Font.setWeight(QFont::Light);
    painter.setFont(Font);
	int dB = m_maxDb;
	for( int i=0; i<TB_VERT_DIVS-1; i++)
	{
		y = (int)( (float)i*pixperdiv );
        //Was drawStaticText which was throwing Metric warnings.  DrawText is what we use everywhere else so...
        painter.drawText(5, y-1, QString::number(dB)+" dB");
		dB -= m_dBStepSize;
	}
	m_minDb = m_maxDb - (TB_VERT_DIVS)*m_dBStepSize;

	//copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_overlayPixmap.copy(0,0,w,h);
	//trigger a new paintEvent
	update();
}

//////////////////////////////////////////////////////////////////////
// Helper function Called to create all the frequency division text
//strings based on start frequency, span frequency, frequency units.
//Places in QString array m_HDivText
//Keeps all strings the same fractional length
//////////////////////////////////////////////////////////////////////
void TestBench::makeFrequencyStrs()
{
qint64 FreqPerDiv;
qint64 StartFreq;
float freq;
int i,j;
int numfractdigits = (int)log10((double)m_freqUnits);
	if(m_currentDataIsCpx)
	{
		FreqPerDiv = m_span/TB_HORZ_DIVS;
		StartFreq = m_centerFreq - m_span/2;
	}
	else
	{
		FreqPerDiv = m_span/(2*TB_HORZ_DIVS);
		StartFreq = 0;
	}
	if(1 == m_freqUnits)
	{	//if units is Hz then just output integer freq
		for(int i=0; i<=TB_HORZ_DIVS; i++)
		{
			freq = (float)StartFreq/(float)m_freqUnits;
			m_hDivText[i].setNum((int)freq);
			StartFreq += FreqPerDiv;
		}
		return;
	}
	//here if is fractional frequency values
	//so create max sized text based on frequency units
	for(int i=0; i<=TB_HORZ_DIVS; i++)
	{
		freq = (float)StartFreq/(float)m_freqUnits;
		m_hDivText[i].setNum(freq,'f', numfractdigits);
		StartFreq += FreqPerDiv;
	}
	//now find the division text with the longest non-zero digit
	//to the right of the decimal point.
	int max = 0;
	for(i=0; i<=TB_HORZ_DIVS; i++)
	{
		int dp = m_hDivText[i].indexOf('.');
		int l = m_hDivText[i].length()-1;
		for(j=l; j>dp; j--)
		{
			if(m_hDivText[i][j] != '0')
				break;
		}
		if( (j-dp) > max)
			max = j-dp;
	}
	//truncate all strings to maximum fractional length
	StartFreq = m_centerFreq - m_span/2;
	for( i=0; i<=TB_HORZ_DIVS; i++)
	{
		freq = (float)StartFreq/(float)m_freqUnits;
		m_hDivText[i].setNum(freq,'f', max);
		StartFreq += FreqPerDiv;
	}
}
