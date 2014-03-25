#ifndef ALAWCOMPRESSION_H
#define ALAWCOMPRESSION_H

#include <QtCore>
#if 0
	A-Law and Mu-Law 2:1 audio compression
	Tables and code from http://www.threejacks.com/?q=node/176

#endif

class ALawCompression
{
public:
	ALawCompression();

	quint8 LinearToMuLaw(qint16 sample);
	qint16 MuLawToLinear(quint8 compressed) {return muLawDecompressTable[compressed];}

	quint8 LinearToALaw(qint16 sample);
	qint16 ALawToLinear(quint8 compressed) {return aLawDecompressTable[compressed];}

private:
	const int cBias = 0x84;
	const int cClip = 32635;

	static quint8 aLawCompressTable[128];
	static qint16 aLawDecompressTable[256];

	static quint8 muLawCompressTable[256];
	static qint16 muLawDecompressTable[256];

	void GenerateAlawToLinearTable();
	};

#endif // ALAWCOMPRESSION_H
