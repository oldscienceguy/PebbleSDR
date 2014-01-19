///////////////////////////////////////////////////////
// Perform.h : Global interface for the performance functions
// History:
//	2010-11-10  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
//
#if !defined(_INCLUDE_PERFORMXXX_H_)
#define _INCLUDE_PERFORMXXX_H_
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

#include "math.h"
#include <QtGlobal>
#include <QString>
/////////////////////////////////////////////////////////////////////////////

class Perform
{
public:
    Perform();
    quint64 QueryPerformanceCounter();
    void InitPerformance();
    void StartPerformance(QString desc="");
    void StopPerformance(quint64 n);
    void ReadPerformance();
    void SamplePerformance();
    int GetDeltaPerformance();

private:
    quint64 StartTime;
    quint64 StopTime;
    quint64 DeltaTime;
    //quint64 CountFreq; //Original from Moe, but doesn't work with fractional CPU rates
    double CountFreq;
    quint64 DeltaTimeMax;
    quint64 DeltaTimeMin;
    quint64 DeltaTimeAve;
    quint64 DeltaSamples;
    QString desc;
};

#endif //#if !defined(_INCLUDE_PERFORMXXX_H_)
