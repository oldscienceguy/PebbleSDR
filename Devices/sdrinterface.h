//////////////////////////////////////////////////////////////////////
// sdrinterface.h: interface for the CSdrInterface class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-16  Added Frequency range logic for optional down converter modules
//	2011-08-07  Added WFM Support
/////////////////////////////////////////////////////////////////////
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
#ifndef SDRINTERFACE_H
#define SDRINTERFACE_H

#include "interface/netiobase.h"
#include "dsp/fft.h"
#include "interface/ad6620.h"
#include "dsp/demodulator.h"
#include "dsp/noiseproc.h"
#include "interface/soundout.h"
#include "interface/protocoldefs.h"
#include "dataprocess.h"


/////////////////////////////////////////////////////////////////////
// Derived class from NetIOBase for all the custom SDR msg processing
/////////////////////////////////////////////////////////////////////
class CSdrInterface : public CNetio
{
	Q_OBJECT
public:
	CSdrInterface();
	~CSdrInterface();


	enum eRadioType {
		SDR14,
		SDRIQ,
		SDRIP,
		NETSDR,
		CLOUDSDR
	};
	//NCO spur management commands for ManageNCOSpurOffsets(...)
	enum eNCOSPURCMD {
		NCOSPUR_CMD_SET,
		NCOSPUR_CMD_STARTCAL,
		NCOSPUR_CMD_READ
	};

	void StopIO();	//stops IO threads

	// virtual functioncalled by TCP thread with new msg from radio to parse
	void ParseAscpMsg(CAscpRxMsg *pMsg);
	//virtual function called by UDP thread with new raw data to process
	void ProcessUdpData(char* pBuf, qint64 Length){if(m_pdataProcess) m_pdataProcess->PutInQ(pBuf,Length);}
	//called by DataProcess thread with new I/Q data samples to process
	void ProcessIQData( TYPECPX* pIQData, int NumSamples);

	void StartSdr();
	void StopSdr();
	bool GetScreenIntegerFFTData(qint32 MaxHeight, qint32 MaxWidth,
									double MaxdB, double MindB,
									qint32 StartFreq, qint32 StopFreq,
									qint32* OutBuf );
	void ScreenUpdateDone(){m_ScreenUpateFinished = true;}
	void KeepAlive();
	void ManageNCOSpurOffsets( eNCOSPURCMD cmd, double* pNCONullValueI,  double* pNCONullValueQ);
	void SetRx2Parameters(double Rx2Gain, double Rx2Phase);

	//calls to these functions promt a signal in response
	void GetSdrInfo();
	void ReqStatus();
	quint64 SetRxFreq(quint64 freq);

	//bunch of public members containing sdr related information and data
	QString m_DeviceName;
	QString m_SerialNum;
	float m_BootRev;
	float m_AppRev;
	int m_MissedPackets;

	//GUI Public settings and access functions
	void SetRadioType(qint32 radiotype ){m_RadioType = (eRadioType)radiotype;}
	qint32 GetRadioType(){return (qint32)m_RadioType;}

	void SetSoundCardSelection(qint32 SoundInIndex, qint32 SoundOutIndex,bool StereoOut)
				{ m_pSoundCardOut->Stop(); m_SoundInIndex = SoundInIndex;
					m_SoundOutIndex = SoundOutIndex;  m_StereoOut = StereoOut;}

	int GetRateError(){return m_pSoundCardOut->GetRateError();}
	void SetVolume(qint32 vol){ m_pSoundCardOut->SetVolume(vol); }

	void SetChannelMode(qint32 channelmode);

	void SetSdrRfGain(qint32 gain);
	qint32 GetSdrRfGain(){return m_RfGain;}

	void SetSdrBandwidthIndex(qint32 bwindex);
	double GetSdrSampleRate(){return m_SampleRate;}
	quint32 GetSdrMaxBandwidth(){return m_SampleRate;}

	void SetFftSize(qint32 size);
	quint32 GetSdrFftSize(){return m_FftSize;}

	void SetFftAve(qint32 ave);
	quint32 GetSdrFftAve(){return m_FftAve;}

	qint32 GetMaxBWFromIndex(qint32 index);
	double GetSampleRateFromIndex(qint32 index);

	void SetMaxDisplayRate(int updatespersec){m_MaxDisplayRate = updatespersec;
							m_DisplaySkipValue = m_SampleRate/(m_FftSize*m_MaxDisplayRate);
							m_DisplaySkipCounter = 0; }

	void SetDemod(int Mode, tDemodInfo CurrentDemodInfo);
	void SetDemodFreq(qint64 Freq){m_Demodulator.SetDemodFreq((TYPEREAL)Freq);}

	void SetupNoiseProc(tNoiseProcdInfo* pNoiseProcSettings);

	double GetSMeterPeak(){return m_Demodulator.GetSMeterPeak() + m_GainCalibrationOffset - m_RfGain;}
	double GetSMeterAve(){return m_Demodulator.GetSMeterAve() + m_GainCalibrationOffset - m_RfGain;}

	void SetSpectrumInversion(bool Invert){m_InvertSpectrum = Invert;}
	void SetUSFmVersion(bool USFm){m_USFm = USFm;}
	bool GetUSFmVersion(){return m_USFm;}

	//access to WFM mode status/data
	int GetStereoLock(int* pPilotLock){ return m_Demodulator.GetStereoLock(pPilotLock);}
	int GetNextRdsGroupData(tRDS_GROUPS* pGroupData){return m_Demodulator.GetNextRdsGroupData(pGroupData);}

signals:
	void NewInfoData();			//emitted when sdr information is received after GetSdrInfo()
	void NewFftData();			//emitted when new FFT data is available
	void FreqChange(int freq);	//emitted if requested frequency has been clamped by radio

private:
	void SendAck(quint8 chan);
	void Start6620Download();
	void NcoSpurCalibrate(TYPECPX* pData, qint32 NumSamples);


	bool m_Running;
	bool m_ScreenUpateFinished;
	bool m_StereoOut;
	bool m_InvertSpectrum;
	bool m_USFm;
	qint32 m_BandwidthIndex;
	qint32 m_DisplaySkipCounter;
	qint32 m_DisplaySkipValue;
	qint32 m_FftSize;
	qint32 m_FftAve;
	qint32 m_FftBufPos;
	qint32 m_KeepAliveCounter;
	qint32 m_MaxBandwidth;
	qint32 m_MaxDisplayRate;
	qint32 m_RfGain;
	qint32 m_SoundInIndex;
	qint32 m_SoundOutIndex;
	qint32 m_ChannelMode;
	quint8 m_Channel;
	quint64 m_CurrentFrequency;
	quint64 m_BaseFrequencyRangeMin;
	quint64 m_BaseFrequencyRangeMax;
	quint64 m_OptionFrequencyRangeMin;
	quint64 m_OptionFrequencyRangeMax;
	double m_SampleRate;
	double m_GainCalibrationOffset;
	TYPECPX m_DataBuf[MAX_FFT_SIZE];

	Cad6620 m_AD6620;
	eRadioType m_RadioType;

	bool m_NcoSpurCalActive;	//NCO spur reduction variables
	qint32 m_NcoSpurCalCount;
	double m_NCOSpurOffsetI;
	double m_NCOSpurOffsetQ;

	CFft m_Fft;
	CDemodulator m_Demodulator;
	CNoiseProc m_NoiseProc;
	CSoundOut* m_pSoundCardOut;
	CDataProcess* m_pdataProcess;

CIir m_Iir;

};



#endif // SDRINTERFACE_H
