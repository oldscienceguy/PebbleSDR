#ifndef MORSEGEN_H
#define MORSEGEN_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "../morseCode.h"
#include "nco.h"

class MorseGen
{
public:
	MorseGen(double sampleRate, double frequency, double dbAmplitude, quint32 wpm);

	//Generates the IQ data for element and puts in out.
	//Returns #samples added to out
	quint32 genText(CPX* out, quint8 text);
	quint32 genDot(CPX* out);
	quint32 genDash(CPX* out);
	quint32 genElement(CPX * out);
	quint32 genChar(CPX *out);
	quint32 genWord(CPX *out);

private:
	double m_sampleRate;
	double m_frequency;
	double m_dbAmplitude; //In db
	double m_amplitude;
	quint32 m_wpm;

	//Output buffers for morse elements
	quint32 m_numSamplesDot;
	CPX* m_dotSamples;

	quint32 m_numSamplesDash;
	CPX* m_dashSamples;

	quint32 m_numSamplesElement;
	CPX* m_elementSamples;

	quint32 m_numSamplesChar;
	CPX *m_charSamples;

	quint32 m_numSamplesWord;
	CPX* m_wordSamples;
};

#endif // MORSEGEN_H
