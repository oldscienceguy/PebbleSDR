//////////////////////////////////////////////////////////////////////
// dataprocess.cpp: implementation for the CdataProcess class.
//
// This class implements a worker thread that takes raw UDP data packets
// as they are received, converts to Complex data and puts in a FIFO.
// The worker thread then is signaled and removes data from the FIFO and
// calls the DSP processing chain.
//
// History:
//	2013-02-05  Initial creation MSW
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

/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include "dataprocess.h"
#include "sdrinterface.h"
#include <QDebug>

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
#define PKT_LENGTH_24 1444
#define PKT_LENGTH_16 1028

#define IN_QUEUE_SIZE 1000
#define QUEUE_BUF_LENGTH 256	//maximum number of possible complex samples in UDP packets

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   T Y P E D E F S <------------------------*/
/*---------------------------------------------------------------------------*/
typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
		unsigned char b2;
		unsigned char b3;
	}bytes;
	int all;
}tBtoL;

typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
	}bytes;
	signed short sall;
	unsigned short all;
}tBtoS;

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CDataProcess::CDataProcess(QObject *pParent) : m_pParent(pParent)
{
	m_pInQueue = new TYPECPX* [IN_QUEUE_SIZE];
	for(int i=0; i<IN_QUEUE_SIZE; i++)
	{	//now allocate memory for each row
		m_pInQueue[i] = new TYPECPX[QUEUE_BUF_LENGTH];	//enough for complex packet data length
	}
qDebug()<<"CDataProcess constructor";
}

CDataProcess::~CDataProcess()
{
qDebug()<<"CDataProcess destructor";
	disconnect();
	if(NULL != m_pInQueue)
	{
		for(int i=0; i<IN_QUEUE_SIZE; i++)	//first delete all the row memory
			delete m_pInQueue[i];
		delete [] m_pInQueue;				//delete column ptr memory
		m_pInQueue = NULL;
	}
}

////////////////////////////////////////////////////////////////////////
//  called by CDataProcess worker thread to initialize its world
////////////////////////////////////////////////////////////////////////
void CDataProcess::ThreadInit()
{
	m_pThread->setPriority(QThread::HighestPriority);
	m_InHead = 0;
	m_InTail = 0;
	m_LastSeqNum = 0;
	connect(this,SIGNAL( GotNewData()), this, SLOT(ProcNewData()) );
qDebug()<<"Data  Thread "<<this->thread()->currentThread();
}


////////////////////////////////////////////////////////////////////////
// Called from UDP thread to put new raw data into IQ sample queue
// Length is number of bytes in UDP packet
////////////////////////////////////////////////////////////////////////
void CDataProcess::PutInQ(char* pBuf, qint64 Length)
{
int i,j;
tBtoL data;
tBtoS seq;
TYPECPX cpxtmp;
	if(NULL == m_pInQueue)
		return;
	m_Mutex.lock();
	data.all = 0;
	//use packet length to determine whether 24 or 16 bit data format
	if(PKT_LENGTH_24 == Length)
	{	//24 bit I/Q data
		m_PacketSize = (PKT_LENGTH_24-4)/6;		//number of complex samples in packet
		seq.bytes.b0 = pBuf[2];
		seq.bytes.b1 = pBuf[3];
		if(0==seq.all)	//is first packet after started
			m_LastSeqNum = 0;
		if(seq.all != m_LastSeqNum)
		{
			( (CSdrInterface*)m_pParent)->m_MissedPackets += ((qint16)seq.all - (qint16)m_LastSeqNum);
			m_LastSeqNum = seq.all;
		}
		m_LastSeqNum++;
		if(0==m_LastSeqNum)
			m_LastSeqNum = 1;
		for( i=4,j=0; i<Length; i+=6,j++)
		{
			data.bytes.b1 = pBuf[i];		//combine 3 bytes into 32 bit signed int
			data.bytes.b2 = pBuf[i+1];
			data.bytes.b3 = pBuf[i+2];
			cpxtmp.re = (double)data.all/65536.0;
			data.bytes.b1 = pBuf[i+3];		//combine 3 bytes into 32 bit signed int
			data.bytes.b2 = pBuf[i+4];
			data.bytes.b3 = pBuf[i+5];
			cpxtmp.im = (double)data.all/65536.0;
			m_pInQueue[m_InHead][j] = cpxtmp;
		}
	}
	else if(PKT_LENGTH_16 == Length)
	{	//16 bit I/Q data
		m_PacketSize = (PKT_LENGTH_16-4)/4;	//number of complex samples in packet
		seq.bytes.b0 = pBuf[2];
		seq.bytes.b1 = pBuf[3];
		if(0==seq.all)	//is first packet after started
			m_LastSeqNum = 0;
		if(seq.all != m_LastSeqNum)
		{
			( (CSdrInterface*)m_pParent)->m_MissedPackets += ((qint16)seq.all - (qint16)m_LastSeqNum);
			m_LastSeqNum = seq.all;
		}
		m_LastSeqNum++;
		if(0==m_LastSeqNum)
			m_LastSeqNum = 1;
		for( i=4,j=0; i<Length; i+=4,j++)
		{	//use 'seq' as temp variable to combine bytes into short int
			seq.bytes.b0 = pBuf[i+0];
			seq.bytes.b1 = pBuf[i+1];
			cpxtmp.re = (double)seq.sall;
			seq.bytes.b0 = pBuf[i+2];
			seq.bytes.b1 = pBuf[i+3];
			cpxtmp.im = (double)seq.sall;
			m_pInQueue[m_InHead][j] = cpxtmp;
		}
	}
	if(++m_InHead >= IN_QUEUE_SIZE)
		m_InHead = 0;
	if(m_InHead == m_InTail)
		qDebug()<<"Q Ovrflow";
	m_Mutex.unlock();
	emit GotNewData();	//tell DataProcess worker thread there's data to process
}

////////////////////////////////////////////////////////////////////////
//  Called by CDataProcess worker thread to process data from I/Q queue
////////////////////////////////////////////////////////////////////////
void CDataProcess::ProcNewData()
{
	m_Mutex.lock();
	while(m_InHead != m_InTail )
	{
		//call function in parent that does all the DSP processing
		( (CSdrInterface*)m_pParent)->ProcessIQData(m_pInQueue[m_InTail],m_PacketSize);
		if(++m_InTail >= IN_QUEUE_SIZE)
			m_InTail = 0;
	}
	m_Mutex.unlock();
//qDebug()<<".";
}
