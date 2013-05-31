//////////////////////////////////////////////////////////////////////
// ad6620.h: interface for the Cad6620 class.
// Needed only for sdr-14 and sdr-iq radios
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
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
#ifndef AD6620_H
#define AD6620_H

#include "interface/netiobase.h"


#define MAX_COEF 256
#define MAX_MSGS 512

class Cad6620
{
public:
	Cad6620();
	virtual ~Cad6620();
	enum eSAMPLERATE
	{
		BWKHZ_5,
		BWKHZ_10,
		BWKHZ_25,
		BWKHZ_50,
		BWKHZ_100,
		BWKHZ_150,
		BWKHZ_190,
		BWKHZ_250,
		BWKHZ_500,
		BWKHZ_1000,
		BWKHZ_1500,
		BWKHZ_2000,
		BWKHZ_4000
	};

	void CreateLoad6620Msgs(int filter);		//called to fill up msg buffer with all msgs needed to load 'filter'
	bool GetNext6620Msg(CAscpTxMsg &pAscpMsg);	//call to get formatted msg to send to radio


private:
	void PutInMsgBuffer(int adr, quint32 data);
	struct mq
	{
		quint16 adr;
		quint32 data;
		quint8 datah;
	}m_MsgBuffer[MAX_MSGS];
	int m_BufIndx;
	int m_BufLength;
	bool m_NCOphzDither;
	bool m_NCOampDither;
	int	m_UseableBW;
	int	m_CIC2Rate;
	int	m_CIC2Scale;
	int	m_CIC5Rate;
	int	m_CIC5Scale;
	int	m_RCFRate;
	int	m_RCFScale;
	int m_RCFTaps;
	int m_FIRcoef[MAX_COEF];
	int m_TotalDecimationRate;
};

#endif // AD6620_H
