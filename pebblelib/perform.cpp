/*
 * Modified by RAL to class so multiple timers can be used in same program
 */
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"


///////////////////////////////////////////////////////
// Perform.cpp : implementation file
//
//  NOTE:: these functions depend on the 64 bit performance timer
// found in Pentium processors. This timer may not be present
//  in some of the non-Intel versions.
// Also it may not work with multi core CPU's across threads.
//
// History:
//	2010-11-10  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-26  Added define to remove assembly from compile
////////////////////////////////////////////////////////////////////////


#define USE_PERFORMANCE	//uncomment to use

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
#include "perform.h"
#include <QDebug>

//#define CPUCLOCKRATEGHZ 3		//manually set to CPU core clock speed
//#define CPUCLOCKRATEGHZ 1.7   //Macbook Air mid 2011
#define CPUCLOCKRATEGHZ 2.5		//MacbookPro 13" Retina early 2013

/////////////////////////////////////////////////////////////////////////////

Perform::Perform()
{

}

////////////////  Time measuring routine using Pentium timer
/// This return number of CPU ticks, not time.  So we convert to uSec using CountFreq
quint64 Perform::QueryPerformanceCounter()
{
quint64 val=0;
#ifdef USE_PERFORMANCE
quint32 eax, edx;
	__asm__ __volatile__("cpuid": : : "ax", "bx", "cx", "dx");
	__asm__ __volatile__("rdtsc":"=a"(eax), "=d"(edx));
	val = edx;
	val = val << 32;
	val += eax;
#endif
	return val;
}

// call to initialize the prformance timer
void Perform::InitPerformance()
{
	DeltaTimeMax = 0;
	DeltaTimeAve = 0;
	DeltaSamples = 0;
    DeltaTimeMin = 0x7FFFFFF;
    //Time is reported in uSec, so we want CPU clock as a quint64 so we can normalize clock ticks to time
	CountFreq = CPUCLOCKRATEGHZ;
}

// Starts the performance timer
void Perform::StartPerformance(QString _desc)
{
    if(DeltaSamples == 0)
        InitPerformance();  //Self initialize on first use

    desc = _desc;
	StartTime = QueryPerformanceCounter();
}

// Stop performance timer and calculate timing values
//Report and clear if DeltaSamples >=n
void Perform::StopPerformance(quint64 n)
{
	StopTime = QueryPerformanceCounter();
	DeltaTime = (StopTime-StartTime);
	DeltaTimeAve += DeltaTime;
	DeltaSamples++;
	if( DeltaTime>DeltaTimeMax )
	{
		DeltaTimeMax = DeltaTime;
	}
	if( DeltaTime<DeltaTimeMin )
	{
		DeltaTimeMin = DeltaTime;
	}
    if (DeltaSamples >= n) {
        ReadPerformance();
        InitPerformance();
    }
}

// Call this to measure time between succesive calls to SamplePerformance()
void Perform::SamplePerformance()
{
	if(	DeltaTimeMax == 0 )
	{
		StartTime = QueryPerformanceCounter();
		DeltaTimeMax = 1;
	}
	else
	{
		StopTime = QueryPerformanceCounter();
		DeltaTime = StopTime-StartTime;
		DeltaTimeAve += DeltaTime;
		if( DeltaTime>DeltaTimeMax )
		{
			DeltaTimeMax = DeltaTime;
		}
		if( DeltaTime<DeltaTimeMin )
		{
			DeltaTimeMin = DeltaTime;
		}
		StartTime = QueryPerformanceCounter();
		DeltaSamples++;
	}
}

// output various timing statistics to Message Box
// DeltaTimeMax == maximum time between start()-stop() or sample()-Sample()
// DeltaTimeMin == minimum time between start()-stop() or sample()-Sample()
// DeltaTimeAve == average time between start()-stop() or sample()-Sample()
// DeltaSamples == number of time samples captured
void Perform::ReadPerformance()
{
	if(DeltaSamples != 0 )
	{
        //Convert everything to actual time based on CPU freq
		DeltaTime = (DeltaTime)/CountFreq;
		DeltaTimeMin = (DeltaTimeMin)/CountFreq;
		DeltaTimeMax = (DeltaTimeMax)/CountFreq;
		DeltaTimeAve = (DeltaTimeAve)/CountFreq;

        float fAve= (float)DeltaTimeAve/( (float)DeltaSamples);
        qDebug()<<desc;

        //Force to usec for easier comparison
#if 0
		if(fAve>1e6)
		{
            qDebug()<<"Max mSec = "<<DeltaTimeMax/1000000  <<"Min mSec = "<<DeltaTimeMin/1000000;
			qDebug()<<"Ave mSec= "<<fAve/1000000.0<<" #Samples = "<< DeltaSamples;
		}
		else if(fAve>1e3)
#endif
		{
            qDebug()<<"Max uSec = "<<DeltaTimeMax/1000  <<"Min uSec = "<<DeltaTimeMin/1000;
			qDebug()<<"Ave uSec= "<<fAve/1000.0<<" #Samples = "<< DeltaSamples;
		}
#if 0
		else
		{
            qDebug()<<"Max nSec = "<<DeltaTimeMax  <<"Min nSec = "<<DeltaTimeMin;
			qDebug()<<"Ave nSec= "<<fAve<<" #Samples = "<< DeltaSamples;
		}
#endif
	}
}

int Perform::GetDeltaPerformance()
{
quint64 delta = ((StopTime-StartTime))/CountFreq;
	return (int)delta;
}



