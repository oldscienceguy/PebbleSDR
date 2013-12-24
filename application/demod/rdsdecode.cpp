/////////////////////////////////////////////////////////////////////
// rdsdecode.cpp: implementation of the CRdsDecode class.
//
//	This class implements a an RDS data decoder
//
// History:
//	2011-09-18  Initial creation MSW
//	2011-09-18  Initial release
/////////////////////////////////////////////////////////////////////

//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2011 Moe Wheatley. All rights reserved.
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
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rdsdecode.h"
#include <QDebug>

CRdsDecode::CRdsDecode()
{
	DecodeReset(true);
}

///////////////////////////////////////////////////////////////////////////////
//Call to reset the decoder and clear out the current text
///////////////////////////////////////////////////////////////////////////////
void CRdsDecode::DecodeReset(int USFm)
{
int i;
	m_USFm = USFm;
	for(i=0; i<64; i++)
	{
		m_RText[i] = ' ';
		m_RTextOut[i] = ' ';
		m_PSText[i] = ' ';
		m_PSTextOut[i] = ' ';
		m_RBDSCallSign[i] = ' ';
	}
	m_RText[0] = 0;
	m_RTextOut[0] = 0;
	m_PSText[0] = 0;
	m_PSTextOut[0] = 0;
	m_RBDSCallSign[0] = 0;
	m_PTYText[0] = 0;
	m_LastABBit = -1;
	m_LastPrgType = -1;
	m_LastPICode = -1;
	m_MaxTextPos = -1;
//qDebug()<<"Reset RDS";
}


////////////////////////////////////////////////////////////////////////
//Call with user char buffer to get filled with latest RDS text string.
//Returns true if a change in text string
////////////////////////////////////////////////////////////////////////
int CRdsDecode::GetRdsString( char* Str)
{
	m_RDSText[0] = 0;
	strcat( m_RDSText, m_PTYText );
	strcat( m_RDSText, m_RTextOut );
	if( strcmp(m_RDSText,Str) )
	{
		strcpy(Str,m_RDSText);
//qDebug()<<m_RDSText;
		return 1;
	}
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////
//Call with user char buffer to get filled with latest RDS callsign string.
//Returns true if a change in text string
////////////////////////////////////////////////////////////////////////
int CRdsDecode::GetRdsCallString( char* Str)
{
	if( strcmp(m_RBDSCallSign, Str) )
	{
		strcpy(Str,m_RBDSCallSign);
		return 1;
	}
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////
//Decode pGrp RDS Data into various text strings
////////////////////////////////////////////////////////////////////////
void CRdsDecode::DecodeRdsGroup(tRDS_GROUPS* pGrp)
{
	if( m_Group.BlockA && (m_Group.BlockA != m_LastPICode) )
	{	//station changed so reset
		DecodeReset(m_USFm);
		DecodePIcode();
		m_LastPICode = m_Group.BlockA;
	}
	DecodePTYText();
	m_Group = *pGrp;
	m_GrpType = (m_Group.BlockB>>11) & 0x1F;
	DecodePIcode();
	switch(m_GrpType)
	{
		case GRPTYPE_0A:	//Program Service Text
		case GRPTYPE_0B:
			DecodePSText();
			break;
		case GRPTYPE_1A:	//Extended Country Codes
			break;
		case GRPTYPE_2A:	//64 character Radio text
			Decode64RadioText();
			break;
		case GRPTYPE_2B:	//32 character Radio text
			Decode32RadioText();
			break;
		default:
//qDebug("unimplemented GrpType = %X\n\r", m_GrpType);
			break;
	}
}


////////////////////////////////////////////////////////////////////////
// Generate Program type text string from PTY code
////////////////////////////////////////////////////////////////////////
void CRdsDecode::DecodePTYText()
{
	m_PrgType = (m_Group.BlockB>>5) & 0x1F;
	if(m_PrgType != m_LastPrgType)
	{
		m_LastPrgType = m_PrgType;
		if(m_USFm)
			strcpy(m_PTYText, PTYPETABLERBDS[m_PrgType] );
		else
			strcpy(m_PTYText, PTYPETABLERDS[m_PrgType] );
	}
}

////////////////////////////////////////////////////////////////////////
//Generate 32 character Radio Text message from rds data
////////////////////////////////////////////////////////////////////////
void CRdsDecode::Decode32RadioText()
{
char ch;
int adr = (m_Group.BlockB&0x0F)<<1;
	if( (m_Group.BlockB & A_B_BIT) != m_LastABBit)
	{
		m_LastABBit = (m_Group.BlockB & A_B_BIT);
		m_RText[0] = 0;
		m_MaxTextPos = -1;
//qDebug("Erase\n\r");
	}
	ch = m_Group.BlockD>>8 & 0xFF;
	if(ch < ' ') ch = ' ';
	m_RText[adr+0] = ch;
	ch = m_Group.BlockD & 0xFF;
	if(ch < ' ') ch = ' ';
	m_RText[adr+1] = ch;
	if(0==adr)
	{	//is starting at beginning of new line
		if(m_MaxTextPos>0)
		{
			m_RText[m_MaxTextPos+1] = 0;
			strcpy(m_RTextOut,m_RText);
		}
		m_MaxTextPos = 0;
	}
	else
	{
		if(m_MaxTextPos >= 0)
		{
			if(adr>m_MaxTextPos)
				m_MaxTextPos = adr+1;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//Generate 64 character Radio Text message from rds data
////////////////////////////////////////////////////////////////////////
void CRdsDecode::Decode64RadioText()
{
char ch;
int adr = (m_Group.BlockB&0x0F)<<2;
	if( (m_Group.BlockB & A_B_BIT) != m_LastABBit)
	{
		m_LastABBit = (m_Group.BlockB & A_B_BIT);
		m_RText[0] = 0;
		m_MaxTextPos = -1;
//qDebug("Erase\n\r");
	}
	ch = m_Group.BlockC>>8 & 0xFF;
	if(ch < ' ') ch = ' ';
	m_RText[adr+0] = ch;
	ch = m_Group.BlockC & 0xFF;
	if(ch < ' ') ch = ' ';
	m_RText[adr+1] = ch;
	ch = m_Group.BlockD>>8 & 0xFF;
	if(ch < ' ') ch = ' ';
	m_RText[adr+2] = ch;
	ch = m_Group.BlockD & 0xFF;
	if(ch < ' ') ch = ' ';
	m_RText[adr+3] = ch;
	if(0==adr)
	{	//is starting at beginning of new line
		if( (m_MaxTextPos>0) )
		{
			m_RText[m_MaxTextPos+1] = 0;
			strcpy(m_RTextOut,m_RText);
//qDebug("%s:   %u\n\r",m_RText,m_MaxTextPos);
		}
		m_MaxTextPos = 0;
	}
	else
	{
		if(m_MaxTextPos >= 0)
		{
			if(adr>m_MaxTextPos)
				m_MaxTextPos = adr+3;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//Generate Program Service name from rds data
////////////////////////////////////////////////////////////////////////
void CRdsDecode::DecodePSText()
{
int adr = (m_Group.BlockB&0x03)<<1;
//qDebug("adr = %u\n\r",adr);

	m_PSText[adr+0] = m_Group.BlockD>>8 & 0xFF;
	m_PSText[adr+1] = m_Group.BlockD & 0xFF;
	if(0==adr)
	{	//is starting at beginning of new string
		m_PSText[8]=0;
		strcpy(m_PSTextOut,m_PSText);
//qDebug("%s\n\r",m_PSTextOut);
	}
}


/////////////////////////////////////////////////////////////////////////////////
// Kludge to get US RBDS radio call signs from PI code.  The format was
// bastardized by a recent standards chenge so a lot of callsigns cannot be
// derived from the code.
/////////////////////////////////////////////////////////////////////////////////
void CRdsDecode::DecodePIcode()
{
quint16 PIcode = m_Group.BlockA;
	if(m_LastPICode == PIcode)
		return;
	if(m_USFm && ( (PIcode&0xF000) != 0x1000) )
	{	//if US version and doesn't have the new 1xxx hack added by the new RBDS standard
		if( (PIcode&0xF000) == 0xA000)
		{	//deal with exception codes
			if( (PIcode&0x0F00) == 0x0F00)
			{	//is AFyz so map to yz00
				PIcode = (PIcode<<8);
			}
			else
			{
				quint16 tmp = PIcode&0x00FF;	//save P3,P4
				PIcode = ( (PIcode<<4)&0xF000 ) | tmp;
			}
		}
		if( (PIcode>=0x9950) && (PIcode<=0x9EFF) )
		{	// is a 3 letter call so need to search lookup table
			m_RBDSCallSign[3] = 0;
			for(int i=0; i<NUMENTRIES; i++)
			{
				if(CALL3TABLE[i].pi == PIcode)
				{
					m_RBDSCallSign[0] = CALL3TABLE[i].csign[0];
					m_RBDSCallSign[1] = CALL3TABLE[i].csign[1];
					m_RBDSCallSign[2] = CALL3TABLE[i].csign[2];
					m_RBDSCallSign[3] = 0;
					break;
				}
			}
			return;
		}
		if(PIcode>=21672)
		{
			PIcode -= 21672;
			m_RBDSCallSign[0] = 'W';
		}
		else
		{
			PIcode -= 4096;
			m_RBDSCallSign[0] = 'K';
		}
		int x = PIcode/676;
		m_RBDSCallSign[1] = x + 'A';
		PIcode = PIcode - x*676;
		x = PIcode/26;
		m_RBDSCallSign[2] = x + 'A';
		PIcode = PIcode - x*26;
		m_RBDSCallSign[3] = PIcode + 'A';
		m_RBDSCallSign[4] = 0;

	}
	else	//for RDS version just make HEX string of PI value
	{
		sprintf(m_RBDSCallSign,"%4.4X",PIcode);
	}
}

