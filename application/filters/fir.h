//////////////////////////////////////////////////////////////////////
// fir.h: interface for the CFir class.
//
//  This class implements a FIR  filter using a double flat coefficient
//array to eliminate testing for buffer wrap around.
//
//Also a decimate by 3 half band filter class CDecimateBy2 is implemented
//
// History:
//	2011-01-29  Initial creation MSW
//	2011-03-27  Initial release
//	2011-08-05  Added decimate by 2 class
//	2011-08-07  Modified FIR filter initialization
//////////////////////////////////////////////////////////////////////
#ifndef FIR_H
#define FIR_H

//Adapt to Pebble types
#include "cpx.h"

#include "filtercoef.h"

#define MAX_NUMCOEF 75
#include <QMutex>

////////////
//class for FIR Filters
////////////
class CFir
{
public:
    CFir();

	void InitConstFir( int NumTaps, const double* pCoef, TYPEREAL Fsamprate);
	void InitConstFir( int NumTaps, const double* pICoef, const double* pQCoef, TYPEREAL Fsamprate);
	int InitLPFilter(int NumTaps, TYPEREAL Scale, TYPEREAL Astop, TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Fsamprate);
	int InitHPFilter(int NumTaps, TYPEREAL Scale, TYPEREAL Astop, TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Fsamprate);
	void GenerateHBFilter( TYPEREAL FreqOffset);
	void ProcessFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);
	void ProcessFilter(int InLength, TYPEREAL* InBuf, TYPECPX* OutBuf);
	void ProcessFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf);

private:
	TYPEREAL Izero(TYPEREAL x);
	TYPEREAL m_SampleRate;
	int m_NumTaps;
	int m_State;
	TYPEREAL m_Coef[MAX_NUMCOEF*2];
	TYPEREAL m_ICoef[MAX_NUMCOEF*2];
	TYPEREAL m_QCoef[MAX_NUMCOEF*2];
	TYPEREAL m_rZBuf[MAX_NUMCOEF];
	TYPECPX m_cZBuf[MAX_NUMCOEF];
	QMutex m_Mutex;		//for keeping threads from stomping on each other
};

////////////
//class for the Half Band decimate by 2 FIR filters
////////////
class CDecimateBy2
{
public:
	CDecimateBy2(int len, const TYPEREAL* pCoef);
    ~CDecimateBy2();
    int DecBy2(int InLength, TYPEREAL* pInData, TYPEREAL* pOutData);
	int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
	TYPEREAL* m_pHBFirRBuf;
	TYPECPX* m_pHBFirCBuf;
	int m_FirLength;
	const TYPEREAL* m_pCoef;
};

#endif // FIR_H
