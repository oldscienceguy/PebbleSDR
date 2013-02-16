///////////////////////////////////////////////////////
// Perform.h : Global interface for the performance functions
// History:
//	2010-11-10  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
//
#if !defined(_INCLUDE_PERFORMXXX_H_)
#define _INCLUDE_PERFORMXXX_H_
#include "math.h"
#include <QtGlobal>
/////////////////////////////////////////////////////////////////////////////

class Perform
{
public:
    Perform();
    quint64 QueryPerformanceCounter();
    void InitPerformance();
    void StartPerformance();
    void StopPerformance(int n);
    void ReadPerformance();
    void SamplePerformance();
    int GetDeltaPerformance();

private:
    quint64 StartTime;
    quint64 StopTime;
    quint64 DeltaTime;
    quint64 CountFreq;
    quint64 DeltaTimeMax;
    quint64 DeltaTimeMin;
    quint64 DeltaTimeAve;
    quint64 DeltaSamples;
    quint64 Length;
};

#endif //#if !defined(_INCLUDE_PERFORMXXX_H_)
