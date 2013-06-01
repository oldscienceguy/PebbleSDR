//////////////////////////////////////////////////////////////////////
// netio.h: interface for the CNetio class.
//
// History:
//	2012-12-12  Initial creation MSW
//	2013-02-05  Modified for CuteSDR
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
#ifndef NETIO_H
#define NETIO_H

#include "threadwrapper.h"
#include "ascpmsg.h"
#include <QTcpServer>
#include <QUdpSocket>
#include <QHostAddress>

/////////////////////////////////////////////////////
// ************     C U d p     *********************
/////////////////////////////////////////////////////
class CUdp : public CThreadWrapper
{
	Q_OBJECT
public:
	CUdp(QObject *parent = 0);
	~CUdp();
	void ThreadInit();	//overrided function is called by new thread when started
	void SendUdpData(char* pBuf, qint32 Length);

signals:

private slots:
	void StartUdpSlot(quint32 ServerAdr, quint32 ClientAdr, quint16 ServerPort);
	void StopUdpSlot();
	void GotUdpData();

private:
	QObject* m_pParent;
	QUdpSocket* m_pUdpSocket;
	QHostAddress m_ServerIPAdr;
	quint16 m_ServerPort;
};

/////////////////////////////////////////////////////
// ************   C N e t i o   *********************
/////////////////////////////////////////////////////
class CNetio : public QObject
{
	Q_OBJECT
public:
	explicit CNetio();
	~CNetio();

	enum eStatus {
		NOT_CONNECTED,
		CONNECTING,
		CONNECTED,
		RUNNING,
		ADOVR,
		ERR
	};

    bool ConnectToServer(QHostAddress IPAdr, quint16 Port, bool wait);
	void SendStatus(eStatus status);
	virtual void ParseAscpMsg( CAscpRxMsg* pMsg){Q_UNUSED(pMsg)}
	virtual void ProcessUdpData(char* pBuf, qint64 Length){Q_UNUSED(pBuf);Q_UNUSED(Length)}
	void SendAscpMsg(CAscpTxMsg* pMsg);
	void SendUdpData(char* pBuf, qint32 Length);

	eStatus m_Status;

signals:
	void StartUdp(quint32 ServerAdr, quint32 ClientAdr, quint16 ServerPort);
	void StopUdp();
	void NewStatus(int status);	//emitted when sdr status changes

public slots:
    void DisconnectFromServer();

private slots:
	void ReadTcpData();
	void TcpStateChanged(QAbstractSocket::SocketState State);
private:
	void AssembleAscpMsg(quint8* Buf, int Len);

	CAscpRxMsg m_RxAscpMsg;
	int m_RxMsgLength;
	int m_RxMsgIndex;
	int m_MsgState;

	QHostAddress m_ServerIPAdr;
	quint16 m_ServerPort;
	QHostAddress m_ClientIPAdr;
	quint16 m_ClientPort;
	CUdp* m_pUdpIo;
	QTcpSocket* m_pTcpClient;
};

#endif // NETIO_H
