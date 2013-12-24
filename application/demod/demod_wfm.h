//////////////////////////////////////////////////////////////////////
// wfmdemod.h: interface for the Demod_WFM class.
//
// History:
//	2011-07-24  Initial creation MSW
//	2011-08-05  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef WFMDEMOD_H
#define WFMDEMOD_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#include "filters/fir.h"
#include "iir.h"
#include "downconvert.h"
#include "rbdsconstants.h"
#include "demod.h"

#define PHZBUF_SIZE 16384

#define RDS_Q_SIZE 100

class Demod_WFM : public Demod
{
    //Note:  If moc complier is not called, delete Makefile so it can be regenerated
    Q_OBJECT

public:
    Demod_WFM(int _inputRate, int _numSamples);
    virtual ~Demod_WFM();

    void Init(TYPEREAL samplerate, TYPEREAL _audioRate);
    TYPEREAL SetSampleRate(TYPEREAL samplerate, TYPEREAL _outRate, bool USver);
	//overloaded functions for mono and stereo
    int ProcessDataStereo(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
    int ProcessDataMono(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
	TYPEREAL GetDemodRate(){return m_OutRate;}

    int GetNextRdsGroupData(tRDS_GROUPS* pGroupData);
    int GetStereoLock(int* pPilotLock);
    void FMMono(CPX *in, CPX *out, int bufSize);
private:
    void FMDeemphasisFilter(int _bufSize, CPX *in, CPX *out);
    float fmDeemphasisAlpha;
    static const float usDeemphasisTime; //Use for US & Korea FM
    static const float intlDeemphasisTime;  //Use for international FM

    //CFir fmMonoLPFilter;
    CIir fmMonoLPFilter;
    CFir fmAudioLPFilter;
    CIir fmPilotNotchFilter;
    CIir fmPilotBPFilter;
    CFir hilbertFilter;

	void InitPll( TYPEREAL SampleRate );
	void ProcessPll( int InLength, TYPECPX* pInData, TYPEREAL* pOutData );
	void InitDeemphasis( TYPEREAL Time, TYPEREAL SampleRate);	//create De-emphasis LP filter
	void ProcessDeemphasisFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);
	void ProcessDeemphasisFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf);
	void InitPilotPll( TYPEREAL SampleRate );
	bool ProcessPilotPll( int InLength, TYPECPX* pInData );
	void InitRds( TYPEREAL SampleRate );
	void ProcessRdsPll( int InLength, TYPECPX* pInData, TYPEREAL* pOutData );
	inline TYPEREAL arctan2(TYPEREAL y, TYPEREAL x);

	void ProcessNewRdsBit(int bit);
	quint32 CheckBlock(quint32 BlockOffset, int UseFec);
	void CreateCallSign(quint16 PIcode);

	TYPEREAL m_SampleRate;
	TYPEREAL m_OutRate;
	TYPEREAL m_RawFm[PHZBUF_SIZE];
	TYPECPX m_CpxRawFm[PHZBUF_SIZE];

	TYPECPX m_D0;		//complex delay line variables
	TYPECPX m_D1;
	TYPECPX m_D2;
	TYPECPX m_D3;
	TYPECPX m_D4;
	TYPECPX m_D5;

	TYPEREAL m_DeemphasisAveRe;
	TYPEREAL m_DeemphasisAveIm;
	TYPEREAL m_DeemphasisAlpha;

	CIir m_MonoLPFilter;
	CFir m_LPFilter;
	CIir m_NotchFilter;
	CIir m_PilotBPFilter;
	CFir m_HilbertFilter;

	int m_PilotLocked;				//variables for Pilot PLL
	int m_LastPilotLocked;
	TYPEREAL m_PilotNcoPhase;
	TYPEREAL m_PilotNcoFreq;
	TYPEREAL m_PilotNcoAcc;
	TYPEREAL m_PilotNcoLLimit;
	TYPEREAL m_PilotNcoHLimit;
	TYPEREAL m_PilotPllAlpha;
	TYPEREAL m_PilotPllBeta;
	TYPEREAL m_PhaseErrorMagAve;
	TYPEREAL m_PhaseErrorMagAlpha;
	TYPEREAL m_PilotPhase[PHZBUF_SIZE];
	TYPEREAL m_PilotPhaseAdjust;

	TYPEREAL m_RdsNcoPhase;		//variables for RDS PLL
	TYPEREAL m_RdsNcoFreq;
	TYPEREAL m_RdsNcoAcc;
	TYPEREAL m_RdsNcoLLimit;
	TYPEREAL m_RdsNcoHLimit;
	TYPEREAL m_RdsPllAlpha;
	TYPEREAL m_RdsPllBeta;

	TYPECPX m_RdsRaw[PHZBUF_SIZE];	//variables for RDS processing
	TYPEREAL m_RdsMag[PHZBUF_SIZE];
	TYPEREAL m_RdsData[PHZBUF_SIZE];
	TYPEREAL m_RdsMatchCoef[PHZBUF_SIZE];
	TYPEREAL m_RdsLastSync;
	TYPEREAL m_RdsLastSyncSlope;
	TYPEREAL m_RdsLastData;
	int m_MatchCoefLength;
    CDownConvert m_RdsDownConvert;
	CFir m_RdsBPFilter;
	CFir m_RdsMatchedFilter;
	CIir m_RdsBitSyncFilter;
	TYPEREAL m_RdsOutputRate;
	int m_RdsLastBit;
	tRDS_GROUPS m_RdsGroupQueue[RDS_Q_SIZE];
	int m_RdsQHead;
	int m_RdsQTail;
	tRDS_GROUPS m_LastRdsGroup;
	quint32 m_InBitStream;	//input shift register for incoming raw data
	int m_CurrentBlock;
	int m_CurrentBitPosition;
	int m_DecodeState;
	int m_BGroupOffset;
	int m_BlockErrors;
	quint16 m_BlockData[4];


};

#endif // WFMDEMOD_H
