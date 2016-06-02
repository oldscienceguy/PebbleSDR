#ifndef MORSEGEN_H
#define MORSEGEN_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "../morseCode.h"
#include "cpx.h"

class MorseGen
{
public:
	MorseGen(double sampleRate, double frequency, double dbAmplitude, quint32 wpm, quint32 msRise);

	//Sets the text that will be generated
	void setTextOut(QString textOut);

	//Returns next sample to be output
	bool hasOutputSamples();
	CPX nextOutputSample();

	//Generates the IQ data for element and puts in out.
	//Returns #samples added to out
	quint32 genText(CPX* out, quint8 text);
	quint32 genDot(CPX* out);
	quint32 genDash(CPX* out);
	quint32 genElement(CPX * out);
	quint32 genChar(CPX *out);
	quint32 genWord(CPX *out);
	//For testing, dump whole table
	quint32 genToken(CPX *out, quint16 token);
	quint32 genTable(CPX* out, quint32 index);

private:
	double m_sampleRate;
	double m_frequency;
	double m_dbAmplitude; //In db
	double m_amplitude;
	quint32 m_wpm;
	quint32 m_msRiseFall;
	MorseCode::MorseSymbolTypes m_lastSymbol;

	//Output buffers for morse elements
	quint32 m_numSamplesRise;
	quint32 m_numSamplesFall;

	quint32 m_numSamplesDot; //Full power, not including rise/fall
	quint32 m_numSamplesDotBuf;
	CPX* m_dotSampleBuf;

	quint32 m_numSamplesDash; //Full power, not including rise/fall
	quint32 m_numSamplesDashBuf;
	CPX* m_dashSampleBuf;

	quint32 m_numSamplesElementBuf;
	CPX* m_elementSampleBuf;

	quint32 m_numSamplesCharBuf;
	CPX *m_charSampleBuf;

	quint32 m_numSamplesWordBuf;
	CPX* m_wordSampleBuf;

	MorseCode m_morseCode;

	QString m_textOut;
	qint32 m_textOutIndex;
	quint32 m_outSampleBufLen;
	quint32 m_numSamplesOutBuf;
	CPX* m_outSampleBuf;
	quint32 m_outSampleBufIndex;
};

#endif // MORSEGEN_H
