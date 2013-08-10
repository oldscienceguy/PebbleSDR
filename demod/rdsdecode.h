//////////////////////////////////////////////////////////////////////
// rdsdecode.h: decodes RDS messages into text.
//
// History:
//	2011-08-26  Initial creation MSW
//	2011-08-26  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef RDSDECODE_H
#define RDSDECODE_H

#include "global.h"
#include <QtGlobal>
#include "rbdsconstants.h"


#define MAX_TEXT 128


class CRdsDecode
{
public:
    CRdsDecode();
	void DecodeRdsGroup(tRDS_GROUPS* pGrp);
	int GetRdsString( char* Str);
	int GetRdsCallString( char* Str);
	void DecodeReset(int USFm);

private:
	void Decode32RadioText();
	void Decode64RadioText();
	void DecodePTYText();
	void DecodePSText();
	void DecodePIcode();

	tRDS_GROUPS m_Group;
	int m_USFm;
	int m_GrpType;
	int m_PrgType;
	int m_LastPrgType;
	int m_LastABBit;
	int m_LastPICode;
	int m_MaxTextPos;
	char m_PTYText[MAX_TEXT];
	char m_RDSText[MAX_TEXT];
	char m_RTextOut[MAX_TEXT];
	char m_RText[MAX_TEXT];
	char m_PSTextOut[MAX_TEXT];
	char m_PSText[MAX_TEXT];
	char m_RBDSCallSign[MAX_TEXT];
};
#endif // RDSDECODE_H
