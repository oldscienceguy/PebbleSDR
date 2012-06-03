// wfmdemod.cpp: implementation of the CWFmDemod class.
//
//  This class takes I/Q baseband data and performs
// Wideband FM demodulation
//
// History:
//	2011-09-17  Initial creation MSW
//	2011-09-17  Initial release
//////////////////////////////////////////////////////////////////////

//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2011 Moe Wheatley. All rights reserved.
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
#include "wfmdemod.h"
//#include "gui/testbench.h"
//#include "dsp/datatypes.h"
//#include "interface/perform.h"
#include <QDebug>

//#define FMDEMOD_GAIN 8000.0
//RL Pebble needs much smaller values, else overloads
#define FMDEMOD_GAIN .001

#define PILOTPLL_RANGE 20.0	//maximum deviation limit of PLL
#define PILOTPLL_BW 10.0	//natural frequency ~loop bandwidth
#define PILOTPLL_ZETA .707	//PLL Loop damping factor
#define PILOTPLL_FREQ 19000.0	//Centerfreq
#define LOCK_TIMECONST .5		//Lock filter time in seconds
#define LOCK_MAG_THRESHOLD 0.05	//Lock error magnitude threshold

#define PHASE_ADJ_M -7.267e-6	//fudge factor slope to compensate for PLL delay
#define PHASE_ADJ_B 3.677		//fudge factor intercept to compensate for PLL delay

//bunch of RDS constants
#define USE_FEC 1	//set to zero to disable FEC correction

#define RDS_FREQUENCY 57000.0
#define RDS_BITRATE (RDS_FREQUENCY/48.0) //1187.5 bps bitrate
#define RDSPLL_RANGE 12.0	//maximum deviation limit of PLL
#define RDSPLL_BW 1.0	//natural frequency ~loop bandwidth
#define RDSPLL_ZETA .707	//PLL Loop damping factor

//RDS decoder states
#define STATE_BITSYNC 0		//looking for initial bit position in Block 1
#define STATE_BLOCKSYNC 1	//looking for initial correct block order
#define STATE_GROUPDECODE 2	//decode groups after achieving bit  and block sync
#define STATE_GROUPRESYNC 3	//waiting for beginning of new group after getting a block error

#define BLOCK_ERROR_LIMIT 5		//number of bad blocks before trying to resync at the bit level

#define HILB_LENGTH 61
const double HILBLP_H[HILB_LENGTH] =
{	//LowPass filter prototype that is shifted and "hilbertized" to get 90 deg phase shift
	//and convert to baseband complex domain.
	//kaiser-Bessel alpha 1.4  cutoff 30Khz at sample rate of 250KHz
	-0.000389631665953405,0.000115430826670992,0.000945331102222503,0.001582460677684605,
	 0.001370803713784687,-0.000000000000000002,-0.002077413537668161,-0.003656132107176520,
	-0.003372610825000167,-0.000649815020884706, 0.003583263233560064, 0.006997162933343487,
	 0.006990985399916562, 0.002383133886438500,-0.005324501734543406,-0.012092135317628615,
	-0.013212201698221963,-0.006168904735839018, 0.007082277142635906, 0.020017841466263672,
	 0.024271835962039127, 0.014255112728911837,-0.008597071392140753,-0.034478282954624850,
	-0.048147195828726633,-0.035409729589347565, 0.009623663461671806, 0.080084441681677138,
	 0.157278883310078170, 0.217148915611638180, 0.239688166538436750, 0.217148915611638180,
	 0.157278883310078170, 0.080084441681677138, 0.009623663461671806,-0.035409729589347565,
	-0.048147195828726633,-0.034478282954624850,-0.008597071392140753, 0.014255112728911837,
	 0.024271835962039127, 0.020017841466263672, 0.007082277142635906,-0.006168904735839018,
	-0.013212201698221963,-0.012092135317628615,-0.005324501734543406, 0.002383133886438500,
	 0.006990985399916562, 0.006997162933343487, 0.003583263233560064,-0.000649815020884706,
	-0.003372610825000167,-0.003656132107176520,-0.002077413537668161,-0.000000000000000002,
	 0.001370803713784687, 0.001582460677684605, 0.000945331102222503, 0.000115430826670992,
	-0.000389631665953405
};


/////////////////////////////////////////////////////////////////////////////////
//	Construct/destruct WFM demod object
/////////////////////////////////////////////////////////////////////////////////
CWFmDemod::CWFmDemod(TYPEREAL samplerate) : m_SampleRate(samplerate)
{
	m_pDecBy2A = NULL;
	m_pDecBy2B = NULL;
	m_pDecBy2C = NULL;
	m_PilotPhaseAdjust = 0.0;
	SetSampleRate(samplerate, true);
	m_InBitStream = 0;
	m_CurrentBitPosition = 0;
	m_CurrentBlock = BLOCK_A;
	m_DecodeState = STATE_BITSYNC;
	m_BGroupOffset = 0;
	m_PilotLocked = false;
	m_LastPilotLocked = !m_PilotLocked;
	m_BlockErrors = 0;
}

CWFmDemod::~CWFmDemod()
{	//destroy resources
	if(m_pDecBy2A)
		delete m_pDecBy2A;
	if(m_pDecBy2B)
		delete m_pDecBy2B;
	if(m_pDecBy2C)
		delete m_pDecBy2C;
}

/////////////////////////////////////////////////////////////////////////////////
//	Sets demodulator parameters based on input sample rate
// returns the audio sample rate that is produced
// Input sample rate should be in the range 200 to 400Ksps
// The output rate will be between 50KHz and 100KHz
/////////////////////////////////////////////////////////////////////////////////
TYPEREAL CWFmDemod::SetSampleRate(TYPEREAL samplerate, bool USver)
{
	//delete any resources that may still exist
	if(m_pDecBy2A)
		delete m_pDecBy2A;
	if(m_pDecBy2B)
		delete m_pDecBy2B;
	if(m_pDecBy2C)
		delete m_pDecBy2C;
	m_pDecBy2A = NULL;
	m_pDecBy2B = NULL;
	m_pDecBy2C = NULL;

	m_OutRate = m_SampleRate = samplerate;
	//Determine post demod decimation rate based on input sample rate range
	// try to get down to close to 50khz
    //RL: Target 48k
    //if(m_SampleRate>400000)//need dec by 8
    if(m_SampleRate>=384000)//need dec by 8
    {
		m_pDecBy2C = new CDecimateBy2(HB47TAP_LENGTH, HB47TAP_H);
		m_OutRate /= 2.0;
	}
    //if(m_SampleRate>200000)//need dec by 4
    if(m_SampleRate>=192000)//need dec by 4
    {
		m_pDecBy2B = new CDecimateBy2(HB47TAP_LENGTH, HB47TAP_H);
		m_OutRate /= 2.0;
	}
    //if(m_SampleRate>100000)//need dec by 2
    if(m_SampleRate>=96000)//need dec by 2
    {
		m_pDecBy2A = new CDecimateBy2(HB47TAP_LENGTH, HB47TAP_H);
		m_OutRate /= 2.0;
	}
//qDebug()<<"WFW Rates = "<<m_SampleRate <<m_OutRate;

	//set Stereo Pilot phase adjustment values based on sample rate
	// compensation function is a straight line approximation with
	// form y = Mx + B
	m_PilotPhaseAdjust = PHASE_ADJ_M*m_SampleRate + PHASE_ADJ_B;
//qDebug()<<"PhaseAdj = "<< m_PilotPhaseAdjust;

	m_MonoLPFilter.InitLP(75000, 1.0, m_SampleRate);

	//create filters to create baseband complex data from real fmdemoulator output
	m_HilbertFilter.InitConstFir(HILB_LENGTH, HILBLP_H, m_SampleRate);
	m_HilbertFilter.GenerateHBFilter(42000);	//shift +/-30KHz LP filter by 42KHz to make 12 to 72KHz bandpass

	//Create narrow BP filter around 19KHz pilot tone with Q=500
	m_PilotBPFilter.InitBP(PILOTPLL_FREQ, 500, m_SampleRate);
	InitPilotPll(m_SampleRate);

	//create LP filter to roll off audio
	m_LPFilter.InitLPFilter(0, 1.0,60.0, 15000.0,1.4*15000.0, m_OutRate);

	//create 19KHz pilot notch filter with Q=5
	m_NotchFilter.InitBR(PILOTPLL_FREQ, 5, m_OutRate);
	//create deemphasis filter with 75uSec or 50uSec LP corner
	if(USver)
		InitDeemphasis(75E-6, m_OutRate);
	else
		InitDeemphasis(50E-6, m_OutRate);

	m_RdsOutputRate = m_RdsDownConvert.SetDataRate(m_SampleRate, 8000.0);
	m_RdsDownConvert.SetFrequency(-RDS_FREQUENCY);	//set up to shift 57KHz RDS down to baseband and decimate
//qDebug()<<"RDS Rate = "<< m_RdsOutputRate;

	InitRds(m_RdsOutputRate);

	m_PilotLocked = false;
	m_LastPilotLocked = !m_PilotLocked;
	return m_OutRate;
}

/////////////////////////////////////////////////////////////////////////////////
//					Process WFM demod MONO version
// Simple demod without stereo or RDS decoding
//
//		InLength == number of complex input samples in complex array pInData
//		pInData == pointer to callers complex input array (users input data is overwriten!!)
//		pOutData == pointer to callers real(mono audio) output array
//	returns number of samples placed in callers output array
/////////////////////////////////////////////////////////////////////////////////
int CWFmDemod::ProcessDataMono(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
    m_MonoLPFilter.ProcessFilter(InLength,pInData, pInData);
//g_pTestBench->DisplayData(InLength, pInData, m_SampleRate,PROFILE_2);
	for(int i=0; i<InLength; i++)
	{
		m_D0 = pInData[i];
        pOutData[i].re = pOutData[i].im = FMDEMOD_GAIN*atan2( (m_D1.re*m_D0.im - m_D0.re*m_D1.im), (m_D1.re*m_D0.re + m_D1.im*m_D0.im));
		m_D1 = m_D0;
	}
	//decimate down close to final audio rate by dividing by 2's
	if(m_pDecBy2A)
		InLength = m_pDecBy2A->DecBy2(InLength, pOutData, pOutData);
	if(m_pDecBy2B)
		InLength = m_pDecBy2B->DecBy2(InLength, pOutData, pOutData);
	if(m_pDecBy2C)
		InLength = m_pDecBy2C->DecBy2(InLength, pOutData, pOutData);

//g_pTestBench->DisplayData(InLength, m_RawFm, m_SampleRate,PROFILE_2);

    m_LPFilter.ProcessFilter( InLength, pOutData, pOutData);	//rolloff audio above 15KHz
    ProcessDeemphasisFilter(InLength, pOutData, pOutData);		//50 or 75uSec de-emphasis one pole filter
    m_NotchFilter.ProcessFilter( InLength, pOutData, pOutData);	//notch out 19KHz pilot
	m_PilotLocked = false;
	return InLength;
}


/////////////////////////////////////////////////////////////////////////////////
//						Process WFM demod STEREO version
// Process complex I/Q  baseband data input by:
// Perform wideband FM demod into a REAL data stream.
// Perform REAL to complex filtering to make easier to shift and process signals
//		within the demodulated FM signal.
// IIR Filter sround the 19KHz Pilot then Phase Lock a PLL to it.
// If locked, perform stereo demuxing using the PLL signal and a delay line to
//		match the raw REAL data stream.
// Shift the 57KHz RDS signal to baseband and decimate its sample rate down.
// PLL the DSB RDS signal and recover the RDS DSB signal.
// Run the RDS signal through a matched filter to recover the  biphase data.
// Use a IIR resonator to recover the bit clock and sample the RDS data.
// Call the RDS decoder routine with each new bit to recover the RDS data groups.
//
//		InLength == number of complex input samples in complex array pInData
//		pInData == pointer to callers complex input array
//		pOutData == pointer to callers complex(stereo audio) output array
//	returns number of samples placed in callers output array
/////////////////////////////////////////////////////////////////////////////////
int CWFmDemod::ProcessDataStereo(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPEREAL LminusR;
//StartPerformance();
	for(int i=0; i<InLength; i++)
	{
		m_D0 = pInData[i];
		m_RawFm[i] = FMDEMOD_GAIN*atan2( (m_D1.re*m_D0.im - m_D0.re*m_D1.im), (m_D1.re*m_D0.re + m_D1.im*m_D0.im));//~266 nSec/sample
		m_D1 = m_D0;
	}
//StopPerformance(InLength);

//g_pTestBench->DisplayData(InLength, m_RawFm, m_SampleRate,PROFILE_2);
	//create complex data from demodulator real data
	m_HilbertFilter.ProcessFilter(InLength, m_RawFm, m_CpxRawFm);	//~173 nSec/sample
//g_pTestBench->DisplayData(InLength, m_CpxRawFm, m_SampleRate,PROFILE_3);

	m_PilotBPFilter.ProcessFilter(InLength, m_CpxRawFm, pInData);//~173 nSec/sample, use input buffer for complex output storage
	if(ProcessPilotPll(InLength, pInData) )
	{	//if pilot tone present, do stereo demuxing
		for(int i=0; i<InLength; i++)
		{
			TYPEREAL in = m_RawFm[i];
			//Left minus Right signal is created by multiplying by 38KHz recovered pilot
			// scale by 2 since DSB amplitude is half of the Right plus Left signal
			LminusR = 2.0 * in * sin( m_PilotPhase[i]*2.0);
			pOutData[i].re = in + LminusR;		//extract left and right signals
			pOutData[i].im = in - LminusR;
		}
		m_PilotLocked = true;
	}
	else
	{	//no pilot so is mono. Just copy real fm demod into both right and left channels
		for(int i=0; i<InLength; i++)
		{
			pOutData[i].re = m_RawFm[i];
			pOutData[i].im = m_RawFm[i];
		}
		m_PilotLocked = false;
	}

	//translate 57KHz RDS signal to baseband and decimate RDS complex signal
	int length = m_RdsDownConvert.ProcessData(InLength, m_CpxRawFm, m_RdsRaw);

	//filter baseband RDS signal
//g_pTestBench->DisplayData(length, m_RdsRaw, m_RdsOutputRate, PROFILE_3);
	m_RdsBPFilter.ProcessFilter(length, m_RdsRaw, m_RdsRaw);
//g_pTestBench->DisplayData(length, m_RdsRaw, m_RdsOutputRate, PROFILE_3);

	//PLL to remove any rotation since may not be phase locked to 19KHz Pilot or may not even have pilot
	ProcessRdsPll(length, m_RdsRaw, m_RdsMag);
//g_pTestBench->DisplayData(length, m_RdsMag, m_RdsOutputRate, PROFILE_3);

	//run matched filter correlator to extract the bi-phase data bits
	m_RdsMatchedFilter.ProcessFilter(length,m_RdsMag,m_RdsData);
//g_pTestBench->DisplayData(length, m_RdsData, m_RdsOutputRate, PROFILE_6);
	//create bit sync signal in m_RdsMag[] by squaring data
	for(int i=0; i<length; i++)
		m_RdsMag[i] = m_RdsData[i]* m_RdsData[i];	//has high energy at the bit clock rate and 2x bit rate

//g_pTestBench->DisplayData(length, m_RdsMag, m_RdsOutputRate, PROFILE_6);
	//run Hi-Q resonator filter that create a sin wave that will lock to BitRate clock and not 2X rate
	m_RdsBitSyncFilter.ProcessFilter(length, m_RdsMag, m_RdsMag);
//g_pTestBench->DisplayData(length, m_RdsMag, m_RdsOutputRate, PROFILE_6);
	//now loop through samples to determine where bit position is and extract binary digital data
	for(int i=0; i<length; i++)
	{
		TYPEREAL Data = m_RdsData[i];
		TYPEREAL SyncVal = m_RdsMag[i];
		//the best bit sync position is at the positive peak of the sync sine wave
		TYPEREAL Slope = SyncVal - m_RdsLastSync;	//current slope
		m_RdsLastSync = SyncVal;
		//see if at the top of the sine wave
		if( (Slope<0.0) && (m_RdsLastSyncSlope*Slope)<0.0 )
		{	//are at sample time so read previous bit time since we are one sample behind in sync position
			int bit;
			if(m_RdsLastData>=0)
			{
				bit = 1;
m_RdsRaw[i].re = m_RdsLastData;
			}
			else
			{
				bit = 0;
m_RdsRaw[i].re = m_RdsLastData;
			}
			//need to XOR with previous bit to get actual data bit value
			ProcessNewRdsBit(bit^m_RdsLastBit);		//go process new RDS Bit
			m_RdsLastBit = bit;
		}
		else
		{
m_RdsRaw[i].re = 0;
		}

		m_RdsLastData = Data;		//keep last bit since is differential data
		m_RdsLastSyncSlope = Slope;
m_RdsRaw[i].im = Data;
	}
//g_pTestBench->DisplayData(length, m_RdsRaw, m_RdsOutputRate, PROFILE_3);	//display rds data and sample point

//g_pTestBench->DisplayData(length, m_RdsData, m_RdsOutputRate, PROFILE_3);
    //qDebug("%s %d",m_RdsRaw, m_RdsData);


	//decimate by 2's down close to final audio rate
	if(m_pDecBy2A)
		InLength = m_pDecBy2A->DecBy2(InLength, pOutData, pOutData);
	if(m_pDecBy2B)
		InLength = m_pDecBy2B->DecBy2(InLength, pOutData, pOutData);
	if(m_pDecBy2C)
		InLength = m_pDecBy2C->DecBy2(InLength, pOutData, pOutData);

	m_LPFilter.ProcessFilter( InLength, pOutData, pOutData);	//rolloff audio above 15KHz
	ProcessDeemphasisFilter(InLength, pOutData, pOutData);		//50 or 75uSec de-emphasis one pole filter
	m_NotchFilter.ProcessFilter( InLength, pOutData, pOutData);	//notch out 19KHz pilot
	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	Iniitalize variables for FM Pilot PLL
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::InitPilotPll( TYPEREAL SampleRate )
{
	m_PilotNcoPhase = 0.0;
	m_PilotNcoFreq = -PILOTPLL_FREQ;	//freq offset to bring to baseband

	TYPEREAL norm = K_2PI/SampleRate;	//to normalize Hz to radians

	//initialize the PLL
	m_PilotNcoLLimit = (m_PilotNcoFreq-PILOTPLL_RANGE) * norm;		//clamp FM PLL NCO
	m_PilotNcoHLimit = (m_PilotNcoFreq+PILOTPLL_RANGE) * norm;
	m_PilotPllAlpha = 2.0*PILOTPLL_ZETA*PILOTPLL_BW * norm;
	m_PilotPllBeta = (m_PilotPllAlpha * m_PilotPllAlpha)/(4.0*PILOTPLL_ZETA*PILOTPLL_ZETA);
	m_PhaseErrorMagAve = 0.0;
	m_PhaseErrorMagAlpha = (1.0-exp(-1.0/(m_SampleRate*LOCK_TIMECONST)) );
}

/////////////////////////////////////////////////////////////////////////////////
//	Process IQ wide FM data to lock Pilot PLL
//returns TRUE if Locked.  Fills m_PilotPhase[] with locked 19KHz NCO phase data
/////////////////////////////////////////////////////////////////////////////////
bool CWFmDemod::ProcessPilotPll( int InLength, TYPECPX* pInData )
{
double Sin;
double Cos;
TYPECPX tmp;
//StartPerformance();
//m_PilotPhaseAdjust = g_TestValue;
	for(int i=0; i<InLength; i++)	//175 nSec
	{
#if 1
		asm volatile ("fsincos" : "=%&t" (Cos), "=%&u" (Sin) : "0" (m_PilotNcoPhase));	//126nS
#else
		Sin = sin(m_PilotNcoPhase);		//178ns for sin/cos calc
		Cos = cos(m_PilotNcoPhase);
#endif
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = -arctan2(tmp.im, tmp.re);

		//create new NCO frequency term
		m_PilotNcoFreq += (m_PilotPllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't get out of lock range
		if(m_PilotNcoFreq > m_PilotNcoHLimit)
			m_PilotNcoFreq = m_PilotNcoHLimit;
		else if(m_PilotNcoFreq < m_PilotNcoLLimit)
			m_PilotNcoFreq = m_PilotNcoLLimit;
		//update NCO phase with new value
		m_PilotNcoPhase += (m_PilotNcoFreq + m_PilotPllAlpha * phzerror);
		m_PilotPhase[i] = m_PilotNcoPhase + m_PilotPhaseAdjust;	//phase fudge for exact phase delay
		//create long average of error magnitude for lock detection
		m_PhaseErrorMagAve = (1.0-m_PhaseErrorMagAlpha)*m_PhaseErrorMagAve + m_PhaseErrorMagAlpha*phzerror*phzerror;
	}
	m_PilotNcoPhase = fmod(m_PilotNcoPhase, K_2PI);	//keep radian counter bounded
//StopPerformance(InLength);
	if(m_PhaseErrorMagAve < LOCK_MAG_THRESHOLD)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
// Get present Stereo lock status and put in pPilotLock.
// Returns true if lock status has changed since last call.
/////////////////////////////////////////////////////////////////////////////////
int CWFmDemod::GetStereoLock(int* pPilotLock)
{
	if(pPilotLock)
		*pPilotLock = m_PilotLocked;
	if(m_PilotLocked != m_LastPilotLocked)
	{
		m_LastPilotLocked = m_PilotLocked;
		return true;
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////////////
//	Iniitalize IIR variables for De-emphasis IIR filter.
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::InitDeemphasis( TYPEREAL Time, TYPEREAL SampleRate)	//create De-emphasis LP filter
{
	m_DeemphasisAlpha = (1.0-exp(-1.0/(SampleRate*Time)) );
	m_DeemphasisAveRe = 0.0;
	m_DeemphasisAveIm = 0.0;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//REAL version
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::ProcessDeemphasisFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf)
{
	for(int i=0; i<InLength; i++)
	{
		m_DeemphasisAveRe = (1.0-m_DeemphasisAlpha)*m_DeemphasisAveRe + m_DeemphasisAlpha*InBuf[i];
		OutBuf[i] = m_DeemphasisAveRe*2.0;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//complex (stereo) version
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::ProcessDeemphasisFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf)
{
	for(int i=0; i<InLength; i++)
	{
		m_DeemphasisAveRe = (1.0-m_DeemphasisAlpha)*m_DeemphasisAveRe + m_DeemphasisAlpha*InBuf[i].re;
		m_DeemphasisAveIm = (1.0-m_DeemphasisAlpha)*m_DeemphasisAveIm + m_DeemphasisAlpha*InBuf[i].im;
		OutBuf[i].re = m_DeemphasisAveRe*2.0;
		OutBuf[i].im = m_DeemphasisAveIm*2.0;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//	Initialize variables for RDS PLL and matched filter
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::InitRds( TYPEREAL SampleRate )
{
	m_RdsNcoPhase = 0.0;
	m_RdsNcoFreq = 0.0;	//freq offset to bring to baseband

	//Create complex LP filter of RDS signal with 2400Hz passband
	m_RdsBPFilter.InitLPFilter(0, 1.0,40.0, 2400.0,1.3*2400.0, m_RdsOutputRate);

	TYPEREAL norm = K_2PI/SampleRate;	//to normalize Hz to radians
	//initialize the PLL that is used to de-rotate the rds DSB signal
	m_RdsNcoLLimit = (m_RdsNcoFreq-RDSPLL_RANGE) * norm;		//clamp RDS PLL NCO
	m_RdsNcoHLimit = (m_RdsNcoFreq+RDSPLL_RANGE) * norm;
	m_RdsPllAlpha = 2.0*RDSPLL_ZETA*RDSPLL_BW * norm;
	m_RdsPllBeta = (m_RdsPllAlpha * m_RdsPllAlpha)/(4.0*RDSPLL_ZETA*RDSPLL_ZETA);
	//create matched filter to extract bi-phase bits
	// This is basically the time domain shape of a single bi-phase bit
	// as defined for RDS and is close to a single cycle sine wave in shape
	m_MatchCoefLength = SampleRate / RDS_BITRATE;
	for(int i= 0; i<=m_MatchCoefLength; i++)
	{
		TYPEREAL t = (TYPEREAL)i/(SampleRate);
		TYPEREAL x = t*RDS_BITRATE;
		TYPEREAL x64 = 64.0*x;
		m_RdsMatchCoef[i+m_MatchCoefLength] = .75*cos(2.0*K_2PI*x)*( (1.0/(1.0/x-x64)) -
								(1.0/(9.0/x-x64)) );
		m_RdsMatchCoef[m_MatchCoefLength-i] = -.75*cos(2.0*K_2PI*x)*( (1.0/(1.0/x-x64)) -
								(1.0/(9.0/x-x64)) );
	}
	m_MatchCoefLength *= 2;
	//load the matched filter coef into FIR filter
	m_RdsMatchedFilter.InitConstFir(m_MatchCoefLength, m_RdsMatchCoef, SampleRate);
	//create Hi-Q resonator at the bit rate to recover bit sync position Q==500
	m_RdsBitSyncFilter.InitBP(RDS_BITRATE, 500, SampleRate);
	//initialize a bunch of variables pertaining to the rds decoder
	m_RdsLastSync = 0.0;
	m_RdsLastSyncSlope = 0.0;
	m_RdsQHead = 0;
	m_RdsQTail = 0;
	m_RdsLastBit = 0;
	m_CurrentBitPosition = 0;
	m_CurrentBlock = BLOCK_A;
	m_DecodeState = STATE_BITSYNC;
	m_BGroupOffset = 0;
	m_LastRdsGroup.BlockA = 0;
	m_LastRdsGroup.BlockB = 0;
	m_LastRdsGroup.BlockC = 0;
	m_LastRdsGroup.BlockD = 0;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process I/Q RDS baseband stream to lock PLL
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::ProcessRdsPll( int InLength, TYPECPX* pInData, TYPEREAL* pOutData )
{
double Sin;
double Cos;
TYPECPX tmp;
	for(int i=0; i<InLength; i++)
	{
#if 1
		asm volatile ("fsincos" : "=%&t" (Cos), "=%&u" (Sin) : "0" (m_RdsNcoPhase));	//126nS
#else
		Sin = sin(m_RdsNcoPhase);		//178ns for sin/cos calc
		Cos = cos(m_RdsNcoPhase);
#endif
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = -arctan2(tmp.im, tmp.re);
		//create new NCO frequency term
		m_RdsNcoFreq += (m_RdsPllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't get out of lock range
		if(m_RdsNcoFreq > m_RdsNcoHLimit)
			m_RdsNcoFreq = m_RdsNcoHLimit;
		else if(m_RdsNcoFreq < m_RdsNcoLLimit)
			m_RdsNcoFreq = m_RdsNcoLLimit;
		//update NCO phase with new value
		m_RdsNcoPhase += (m_RdsNcoFreq + m_RdsPllAlpha * phzerror);
		pOutData[i] = tmp.im;
	}
	m_RdsNcoPhase = fmod(m_RdsNcoPhase, K_2PI);	//keep radian counter bounded
//g_pTestBench->DisplayData(InLength, m_RdsData, m_RdsOutputRate, PROFILE_6);
}



/////////////////////////////////////////////////////////////////////////////////
//	Process one new bit from RDS data stream.
//	Manages state machine to find block data bit position, runs chksum and FEC on
// each block, recovers good groups of 4 data blocks and places in data queue
// for further upper level GUI processing depending on the application
/////////////////////////////////////////////////////////////////////////////////
void CWFmDemod::ProcessNewRdsBit(int bit)
{
	m_InBitStream =	(m_InBitStream<<1) | bit;	//shift in new bit
	switch(m_DecodeState)
	{
		case STATE_BITSYNC:		//looking at each bit position till we find a "good" block A
			if( 0 == CheckBlock(OFFSET_SYNDROME_BLOCK_A, false) )
			{	//got initial good chkword on Block A not using FEC
				m_CurrentBitPosition = 0;
				m_BGroupOffset = 0;
				m_BlockData[BLOCK_A] = m_InBitStream>>NUMBITS_CRC;
				m_CurrentBlock = BLOCK_B;
				m_DecodeState = STATE_BLOCKSYNC;	//next state is looking for blocks B,C, and D in sequence
			}
			break;
		case STATE_BLOCKSYNC:	//Looking for 4 blocks in correct sequence to have good probability bit position is good
			m_CurrentBitPosition++;
			if(m_CurrentBitPosition >= NUMBITS_BLOCK)
			{
				m_CurrentBitPosition = 0;
				if( CheckBlock(BLK_OFFSET_TBL[m_CurrentBlock+m_BGroupOffset], false ) )
				{	//bad chkword so go look for bit sync again
					m_DecodeState = STATE_BITSYNC;
				}
				else
				{	//good chkword so save data and setup for next block
					m_BlockData[m_CurrentBlock] = m_InBitStream>>NUMBITS_CRC;	//save msg data
					//see if is group A or Group B
					if( (BLOCK_B == m_CurrentBlock) && (m_BlockData[m_CurrentBlock] & GROUPB_BIT) )
						m_BGroupOffset = 4;
					else
						m_BGroupOffset = 0;
					if(m_CurrentBlock >= BLOCK_D)
					{	//good chkword on all 4 blocks in correct sequence so are sure of bit position
						//Place all group data into data queue
						m_RdsGroupQueue[m_RdsQHead].BlockA = m_BlockData[BLOCK_A];
						m_RdsGroupQueue[m_RdsQHead].BlockB = m_BlockData[BLOCK_B];
						m_RdsGroupQueue[m_RdsQHead].BlockC = m_BlockData[BLOCK_C];
						m_RdsGroupQueue[m_RdsQHead++].BlockD = m_BlockData[BLOCK_D];
						if(m_RdsQHead >= RDS_Q_SIZE )
							m_RdsQHead = 0;
						m_CurrentBlock = BLOCK_A;
						m_BlockErrors = 0;
						m_DecodeState = STATE_GROUPDECODE;
//qDebug()<<"RDS Blk Sync";
					}
					else
						m_CurrentBlock++;
				}
			}
			break;
		case STATE_GROUPDECODE:		//here after getting a good sequence of blocks
			m_CurrentBitPosition++;
			if(m_CurrentBitPosition>=NUMBITS_BLOCK)
			{
				m_CurrentBitPosition = 0;
				if( CheckBlock(BLK_OFFSET_TBL[m_CurrentBlock+m_BGroupOffset], USE_FEC ) )
				{
					m_BlockErrors++;
					if( m_BlockErrors > BLOCK_ERROR_LIMIT  )
					{
						m_RdsQHead = m_RdsQTail = 0;	//clear data queue
						m_RdsGroupQueue[m_RdsQHead].BlockA = 0;	//stuff all zeros in que to indicate
						m_RdsGroupQueue[m_RdsQHead].BlockB = 0;	//loss of signal
						m_RdsGroupQueue[m_RdsQHead].BlockC = 0;
						m_RdsGroupQueue[m_RdsQHead++].BlockD = 0;
						m_DecodeState = STATE_BITSYNC;
					}
					else
					{
						m_CurrentBlock++;
						if(m_CurrentBlock>BLOCK_D)
							m_CurrentBlock = BLOCK_A;
						if( BLOCK_A != m_CurrentBlock )	//skip remaining blocks of this group if error
							m_DecodeState = STATE_GROUPRESYNC;
					}
				}
				else
				{	//good block so save and get ready for next one
					m_BlockData[m_CurrentBlock] = m_InBitStream>>NUMBITS_CRC;	//save msg data
					//see if is group A or Group B
					if( (BLOCK_B == m_CurrentBlock) && (m_BlockData[m_CurrentBlock] & GROUPB_BIT) )
						m_BGroupOffset = 4;
					else
						m_BGroupOffset = 0;
					m_CurrentBlock++;
					if(m_CurrentBlock>BLOCK_D)
					{
						//Place all group data into data queue
						m_RdsGroupQueue[m_RdsQHead].BlockA = m_BlockData[BLOCK_A];
						m_RdsGroupQueue[m_RdsQHead].BlockB = m_BlockData[BLOCK_B];
						m_RdsGroupQueue[m_RdsQHead].BlockC = m_BlockData[BLOCK_C];
						m_RdsGroupQueue[m_RdsQHead++].BlockD = m_BlockData[BLOCK_D];
						if(m_RdsQHead >= RDS_Q_SIZE )
							m_RdsQHead = 0;
						m_CurrentBlock = BLOCK_A;
						m_BlockErrors = 0;
//qDebug("Grp %X %X %X %X",m_BlockData[BLOCK_A],m_BlockData[BLOCK_B],m_BlockData[BLOCK_C],m_BlockData[BLOCK_D]);
						//here with complete good group
					}
				}
			}
			break;
		case STATE_GROUPRESYNC:		//ignor blocks until start of next group
			m_CurrentBitPosition++;
			if(m_CurrentBitPosition>=NUMBITS_BLOCK)
			{
				m_CurrentBitPosition = 0;
				m_CurrentBlock++;
				if(m_CurrentBlock>BLOCK_D)
				{
					m_CurrentBlock = BLOCK_A;
					m_DecodeState = STATE_GROUPDECODE;
//qDebug()<<"Grp Resync";
				}
			}
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//	Check block 'm_InBitStream' with 'BlockOffset' for errors.
// if UseFec is false then no FEC is done else correct up to 5 bits.
// Returns zero if no remaining errors if FEC is specified.
/////////////////////////////////////////////////////////////////////////////////
quint32 CWFmDemod::CheckBlock(quint32 SyndromeOffset, int UseFec)
{
	//First calculate syndrome for current 26 m_InBitStream bits
	quint32 testblock = (0x3FFFFFF & m_InBitStream);	//isolate bottom 26 bits
	//copy top 10 bits of block into 10 syndrome bits since first 10 rows
	//of the check matrix is just an identity matrix(diagonal one's)
	quint32 syndrome = testblock>>16;
	for(int i=0; i<NUMBITS_MSG; i++)
	{	//do the 16 remaining bits of the check matrix multiply
		if(testblock&0x8000)
			syndrome ^= PARCKH[i];
		testblock <<= 1;
	}
	syndrome ^= SyndromeOffset;		//add depending on desired block

	if(syndrome && UseFec)	//if errors and can use FEC
	{
		quint32 correctedbits = 0;
		quint32 correctmask = (1<<(NUMBITS_BLOCK-1));	//start pointing to msg msb
		//Run Meggitt FEC algorithm to correct up to 5 consecutive burst errors
		for(int i=0; i<NUMBITS_MSG; i++)
		{
			if(syndrome & 0x200)	//chk msbit of syndrome for error state
			{	//is possible bit error at current position
				if(0 == (syndrome & 0x1F) ) //bottom 5 bits == 0 tell it is correctable
				{	// Correct i-th bit
					m_InBitStream ^= correctmask;
					correctedbits++;
					syndrome <<= 1;		//shift syndrome to next msb
				}
				else
				{
					syndrome <<= 1;	//shift syndrome to next msb
					syndrome ^= CRC_POLY;	//recalculate new syndrome if bottom 5 bits not zero
				}							//and syndrome msb bit was a one
			}
			else
			{	//no error at this bit position so just shift to next position
				syndrome <<= 1;	//shift syndrome to next msb
			}
			correctmask >>= 1;	//advance correctable bit position
		}
		syndrome &= 0x3FF;	//isolate syndrome bits if non-zero then still an error
		if(correctedbits && !syndrome)
		{
//			qDebug()<<"corrected bits "<<correctedbits;
		}
	}
	return syndrome;
}

/////////////////////////////////////////////////////////////////////////////////
// Get next group data from RDS data queue.
// Returns zero if queue is empty or null pointer passed or data has not changed
/////////////////////////////////////////////////////////////////////////////////
int CWFmDemod::GetNextRdsGroupData(tRDS_GROUPS* pGroupData)
{
	if( (m_RdsQHead == m_RdsQTail) || (NULL == pGroupData) )
	{
		return 0;
	}
	pGroupData->BlockA = m_RdsGroupQueue[m_RdsQTail].BlockA;
	pGroupData->BlockB = m_RdsGroupQueue[m_RdsQTail].BlockB;
	pGroupData->BlockC = m_RdsGroupQueue[m_RdsQTail].BlockC;
	pGroupData->BlockD = m_RdsGroupQueue[m_RdsQTail++].BlockD;
	if(m_RdsQTail >= RDS_Q_SIZE )
		m_RdsQTail = 0;
	if( (m_LastRdsGroup.BlockA != pGroupData->BlockA) ||
		(m_LastRdsGroup.BlockB != pGroupData->BlockB) ||
		(m_LastRdsGroup.BlockC != pGroupData->BlockC) ||
		(m_LastRdsGroup.BlockD != pGroupData->BlockD) )
	{
		m_LastRdsGroup = *pGroupData;
		return true;
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////////////
// Less acurate but somewhat faster atan2() function
// |error| < 0.005
// Useful for plls but not for main FM demod if best audio quality desired.
/////////////////////////////////////////////////////////////////////////////////
inline TYPEREAL CWFmDemod::arctan2(TYPEREAL y, TYPEREAL x)
{
TYPEREAL angle;
	if( x == 0.0 )
	{	//avoid divide by zero and just return angle
		if( y > 0.0 ) return K_PI2;
		if( y == 0.0 ) return 0.0;
		return -K_PI2;
	}
	TYPEREAL z = y/x;
	if( fabs( z ) < 1.0 )
	{
		angle = z/(1.0 + 0.2854*z*z);
		if( x < 0.0 )
		{
			if( y < 0.0 )
				return angle - K_PI;
			return angle + K_PI;
		}
	}
	else
	{
		angle = K_PI2 - z/(z*z + 0.2854);
		if( y < 0.0 )
			return angle - K_PI;
	}
	return angle;
}
