//////////////////////////////////////////////////////////////////////
// ascpmsg.h: interface/implementation for the CAscpMsg class.
//
// Helper Class to aid in creating and decoding ASCP format msgs
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
#ifndef ASCPMSG_H
#define ASCPMSG_H
#include <QtGlobal>

/*---------------------------------------------------------------------------*/
/*---------------------> Message Header Defines <----------------------------*/
/*---------------------------------------------------------------------------*/
#define LENGTH_MASK 0x1FFF			/* mask for message length               */
#define TYPE_MASK 0xE0				/* mask for upper byte of header         */

#define TYPE_HOST_SET_CITEM (0<<5)
#define TYPE_HOST_REQ_CITEM (1<<5)
#define TYPE_HOST_REQ_CITEM_RANGE (2<<5)
#define TYPE_HOST_DATA_ITEM0 (4<<5)
#define TYPE_HOST_DATA_ITEM1 (5<<5)
#define TYPE_HOST_DATA_ITEM2 (6<<5)
#define TYPE_HOST_DATA_ITEM3 (7<<5)

#define TYPE_TARG_RESP_CITEM (0<<5)
#define TYPE_TARG_UNSOLICITED_CITEM (1<<5)
#define TYPE_TARG_RESP_CITEM_RANGE (2<<5)
#define TYPE_TARG_DATA_ITEM0 (4<<5)
#define TYPE_TARG_DATA_ITEM1 (5<<5)
#define TYPE_TARG_DATA_ITEM2 (6<<5)
#define TYPE_TARG_DATA_ITEM3 (7<<5)

#define TYPE_DATA_ITEM_ACK (3<<5)
 #define DATA_ITEM_ACK_LENGTH (3)

/*  2 byte NAK response to any unimplemented messages */
#define TARG_RESP_NAK (0x0002)

#define MAX_ASCPMSG_LENGTH (8192+2)

class CAscpRxMsg
{
public:
	CAscpRxMsg(){Length=0;}
	// For reading msgs
	// Put raw received bytes in Buf8[]
	// Call InitRxMsg before reading msg
	// Call Get type,CItem, Length as needed
	// Call Getparmx() in sequence to read msg parameters in msg order.
	void InitRxMsg(){Length = 4;}
	quint8 GetType(){return Buf8[1]&TYPE_MASK;}
	quint16 GetLength(){return Buf16[0]&LENGTH_MASK;}
	quint16 GetCItem(){return Buf16[1];}
	quint8 GetParm8(){return Buf8[Length++];}
	quint16 GetParm16(){tmp16 = Buf8[Length++]; tmp16 |= (quint16)(Buf8[Length++])<<8;
						return tmp16;	}
	quint32 GetParm32(){tmp32 = Buf8[Length++]; tmp32 |= (quint32)(Buf8[Length++])<<8;
						tmp32 |= (quint32)(Buf8[Length++])<<16;
						tmp32 |= (quint32)(Buf8[Length++])<<24;
						return tmp32;	}
	union
	{
		quint8 Buf8[MAX_ASCPMSG_LENGTH];
		quint16 Buf16[MAX_ASCPMSG_LENGTH/2];
	};
private:
	quint16 Length;
	quint16 tmp16;
	quint32 tmp32;
};

class CAscpTxMsg
{
public:
	CAscpTxMsg(){Length=0;}
	//  For creating msgs to Tx
	// Call InitTxMsg with msg type to start creation
	// Add CItem and parameters, msg length is automatically created in hdr
	void InitTxMsg(quint8 type){Buf16[0] = type<<8; Buf16[0]+=2;}
	void AddCItem(quint16 item){Buf16[1] = item; Buf16[0]+=2;}
	void AddParm8(quint8 parm8){Buf8[ Buf16[0]&LENGTH_MASK ] = parm8; Buf16[0]++;}
	void AddParm16(quint16 parm16){Buf8[ (Buf16[0]&LENGTH_MASK) ] = parm16&0xFF; Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm16>>8)&0xFF;Buf16[0]++;}
	void AddParm32(quint32 parm32){Buf8[ (Buf16[0]&LENGTH_MASK) ] = parm32&0xFF; Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>8)&0xFF;Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>16)&0xFF;Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>24)&0xFF;Buf16[0]++;}
	void SetTxMsgLength(quint16 len){Buf16[0]+=len;}
	quint16 GetLength(){return Buf16[0]&LENGTH_MASK;}
	union
	{
		quint8 Buf8[MAX_ASCPMSG_LENGTH];
		quint16 Buf16[MAX_ASCPMSG_LENGTH/2];
	};
private:
	quint16 Length;
	quint16 tmp16;
	quint32 tmp32;
};

#endif // ASCPMSG_H
