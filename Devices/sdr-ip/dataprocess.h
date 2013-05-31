//////////////////////////////////////////////////////////////////////
// dataprocess.h: implementation for the CdataProcess class.
//
// This class implements a worker thread that takes raw UDP data packets
// as they are received, converts to Complex data and puts in a FIFO.
// The worker thread then is signaled and removes data from the FIFO and
// calls the DSP processing chain.
//
// History:
//	2013-02-05  Initial creation MSW
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

#ifndef DATAPROCESS_H
#define DATAPROCESS_H
#include "threadwrapper.h"
#include "dsp/datatypes.h"

class CDataProcess : public CThreadWrapper
{
	Q_OBJECT
public:
	explicit CDataProcess(QObject* pParent=NULL);
	~CDataProcess();
	void ThreadInit();	//overrided function is called by new thread when started
	void PutInQ(char* pBuf, qint64 Length);

signals:
	void GotNewData();

public slots:

private slots:
	void ProcNewData();

private:
	QObject* m_pParent;
	int m_InHead;
	int m_InTail;
	int m_PacketSize;
	quint16 m_LastSeqNum;
	TYPECPX** m_pInQueue;

};

#endif // DATAPROCESS_H
