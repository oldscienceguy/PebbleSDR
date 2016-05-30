#include "morsegen.h"
#include "db.h"

//Todo: add variability to amplitude to simulate fading
//Todo: add variability to wpm to simulate real fists
//Todo: add noise option
//Todo: add timed tuning option before output
//Todo: add random non-morse option
MorseGen::MorseGen(double sampleRate, double frequency, double dbAmplitude, quint32 wpm)
{
	m_sampleRate = sampleRate;
	m_frequency = frequency;
	m_dbAmplitude = dbAmplitude;
	m_amplitude = DB::dBToAmplitude(dbAmplitude);
	m_wpm = wpm;
	m_msRiseFall = 10; //10ms attack/decay, 5ms ARRL recomendation, 20ms max

	//Calculate size of output elements
	quint32 msTcw = MorseCode::wpmToTcwMs(wpm);
	//How many samples do we need for each tcw
	quint32 samplesPerTcw = msTcw / (1000.0/sampleRate);
	m_numSamplesDot = samplesPerTcw * MorseCode::c_tcwDot;
	m_dotSamples = CPX::memalign(m_numSamplesDot);

	m_numSamplesDash = samplesPerTcw * MorseCode::c_tcwDash;
	m_dashSamples = CPX::memalign(m_numSamplesDash);

	m_numSamplesElement = samplesPerTcw * MorseCode::c_tcwElementSpace;
	m_elementSamples = CPX::memalign(m_numSamplesElement);

	m_numSamplesChar = samplesPerTcw * MorseCode::c_tcwCharSpace;
	m_charSamples = CPX::memalign(m_numSamplesChar);

	m_numSamplesWord = samplesPerTcw * MorseCode::c_tcwWordSpace;
	m_wordSamples = CPX::memalign(m_numSamplesWord);

	//Build IQ data for each element
	double phase = TWOPI * m_frequency / m_sampleRate;
	double acc = 0;
	quint32 numSamplesRise = m_msRiseFall / (1000/m_sampleRate);
	quint32 numSamplesFall = m_msRiseFall / (1000/m_sampleRate);
	quint32 numSamplesSteady = m_numSamplesDot - numSamplesRise - numSamplesFall;
	//Dot samples
	//Ramp amplitude during rise
	double ampInc = m_amplitude / numSamplesRise;
	double amplitude = ampInc;
	for (quint32 i=0; i<numSamplesRise; i++) {
		m_dotSamples[i].re = cos(acc) * amplitude;
		m_dotSamples[i].im = sin(acc) * amplitude;
		amplitude += ampInc;
		acc += phase;
	}
	//Steady signal
	for (quint32 i=numSamplesRise; i< numSamplesSteady; i++) {
		m_dotSamples[i].re = cos(acc) * m_amplitude;
		m_dotSamples[i].im = sin(acc) * m_amplitude;
		acc += phase;
	}
	//Decline amplitude during fall
	amplitude = m_amplitude - ampInc;
	for (quint32 i= numSamplesSteady; i< m_numSamplesDot; i++) {
		m_dotSamples[i].re = cos(acc) * amplitude;
		m_dotSamples[i].im = sin(acc) * amplitude;
		amplitude -= ampInc;
		acc += phase;
	}

	numSamplesSteady = m_numSamplesDash - numSamplesRise - numSamplesFall;
	acc = 0;
	ampInc = m_amplitude / numSamplesFall;
	amplitude = ampInc;
	for (quint32 i=0; i<numSamplesRise; i++) {
		m_dashSamples[i].re = cos(acc) * amplitude;
		m_dashSamples[i].im = sin(acc) * amplitude;
		amplitude += ampInc;
		acc += phase;
	}
	//Steady signal
	for (quint32 i=numSamplesRise; i< numSamplesSteady; i++) {
		m_dashSamples[i].re = cos(acc) * m_amplitude;
		m_dashSamples[i].im = sin(acc) * m_amplitude;
		acc += phase;
	}
	//Decline amplitude during fall
	amplitude = m_amplitude - ampInc;
	for (quint32 i= numSamplesSteady; i< m_numSamplesDot; i++) {
		m_dashSamples[i].re = cos(acc) * amplitude;
		m_dashSamples[i].im = sin(acc) * amplitude;
		amplitude -= ampInc;
		acc += phase;
	}

	for (quint32 i=0; i<m_numSamplesElement; i++) {
		m_elementSamples[i].re = 0;
		m_elementSamples[i].im = 0;
	}
	for (quint32 i=0; i<m_numSamplesChar; i++) {
		m_charSamples[i].re = 0;
		m_charSamples[i].im = 0;
	}
	for (quint32 i=0; i<m_numSamplesWord; i++) {
		m_wordSamples[i].re = 0;
		m_wordSamples[i].im = 0;
	}
}

quint32 MorseGen::genText(CPX *out, quint8 text)
{

}

quint32 MorseGen::genDot(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesDot; i++) {
		out[i].re = m_dotSamples[i].re;
		out[i].im = m_dotSamples[i].im;
	}
	return m_numSamplesDot;
}

quint32 MorseGen::genDash(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesDash; i++) {
		out[i].re = m_dashSamples[i].re;
		out[i].im = m_dashSamples[i].im;
	}
	return m_numSamplesDash;
}

quint32 MorseGen::genElement(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesElement; i++) {
		out[i].re = m_elementSamples[i].re;
		out[i].im = m_elementSamples[i].im;
	}
	return m_numSamplesElement;
}

quint32 MorseGen::genChar(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesChar; i++) {
		out[i].re = m_charSamples[i].re;
		out[i].im = m_charSamples[i].im;
	}
	return m_numSamplesChar;
}

quint32 MorseGen::genWord(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesWord; i++) {
		out[i].re = m_wordSamples[i].re;
		out[i].im = m_wordSamples[i].im;
	}
	return m_numSamplesWord;

}
