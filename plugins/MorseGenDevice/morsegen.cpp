#include "morsegen.h"
#include "db.h"

//Todo: add timed tuning option before output
//Todo: add random non-morse option
MorseGen::MorseGen(double sampleRate)
{
	m_sampleRate = sampleRate;
	m_dotSampleBuf = NULL;
	m_dashSampleBuf = NULL;
	m_elementSampleBuf = NULL;
	m_charSampleBuf = NULL;
	m_wordSampleBuf = NULL;
}

MorseGen::~MorseGen()
{
	if (m_dotSampleBuf != NULL)
		delete m_dotSampleBuf;
	if (m_dashSampleBuf != NULL)
		delete m_dashSampleBuf;
	if (m_elementSampleBuf != NULL)
		delete m_elementSampleBuf;
	if (m_charSampleBuf != NULL)
		delete m_charSampleBuf;
	if (m_wordSampleBuf != NULL)
		delete m_wordSampleBuf;
}

void MorseGen::setParams(double frequency, double dbAmplitude, quint32 wpm, quint32 msRise)
{
	m_frequency = frequency;
	m_dbAmplitude = dbAmplitude;
	m_amplitude = DB::dBToAmplitude(dbAmplitude);
	m_wpm = wpm;
	m_msRiseFall = msRise; //10ms attack/decay, 5ms ARRL recomendation, 20ms max

	//Calculate size of output elements
	quint32 msTcw = MorseCode::wpmToTcwMs(wpm);
	//How many samples do we need for each tcw
	quint32 samplesPerTcw = msTcw / (1000.0/m_sampleRate);

	//Samples for dot and dash have to include 50% of the rise and fall time
	// |--5ms--|--5ms--|------50ms------|--5ms--|--5ms--|
	// |    rise time  |    dot time    |     fall time |
	// |       |  Total Tcw time of 60ms        |       |
	// |               Total buffer size                |
	//So are longer than just based on Tcw
	m_numSamplesRise = m_msRiseFall / (1000/m_sampleRate);
	m_numSamplesFall = m_msRiseFall / (1000/m_sampleRate);
	//Adjustment to full Tcw samples to account for 50% rise and fall time
	quint32 tcwRiseFall = (m_numSamplesRise + m_numSamplesFall) / 2;
	m_numSamplesDot = samplesPerTcw * MorseCode::c_tcwDot - tcwRiseFall;
	//Full buffer size, including rise and fall
	m_numSamplesDotBuf = m_numSamplesRise + m_numSamplesFall + m_numSamplesDot;
	if (m_dotSampleBuf != NULL)
		delete m_dotSampleBuf;
	m_dotSampleBuf = memalign(m_numSamplesDotBuf);

	m_numSamplesDash = samplesPerTcw * MorseCode::c_tcwDash - tcwRiseFall;
	m_numSamplesDashBuf =  m_numSamplesRise + m_numSamplesFall + m_numSamplesDash;
	if (m_dashSampleBuf != NULL)
		delete m_dashSampleBuf;
	m_dashSampleBuf = memalign(m_numSamplesDashBuf);

	m_numSamplesElementBuf = samplesPerTcw * MorseCode::c_tcwElementSpace;
	if (m_elementSampleBuf != NULL)
		delete m_elementSampleBuf;
	m_elementSampleBuf = memalign(m_numSamplesElementBuf);

	m_numSamplesCharBuf = samplesPerTcw * MorseCode::c_tcwCharSpace;
	if (m_charSampleBuf != NULL)
		delete m_charSampleBuf;
	m_charSampleBuf = memalign(m_numSamplesCharBuf);

	//A word space already has a character space (3 Tcws)
	//So adjust accordingly
	m_numSamplesWordBuf = samplesPerTcw * MorseCode::c_tcwWordSpace - 3;
	if (m_wordSampleBuf != NULL)
		delete m_wordSampleBuf;
	m_wordSampleBuf = memalign(m_numSamplesWordBuf);

	//Build IQ data for each element
	double phase = TWOPI * m_frequency / m_sampleRate;
	double acc = 0;

	//Dot samples
	//Ramp amplitude during rise
	double ampInc = m_amplitude / m_numSamplesRise;
	double amplitude = ampInc;
	quint32 outIndex = 0;
	for (quint32 i=0; i<m_numSamplesRise; i++, outIndex++) {
		m_dotSampleBuf[outIndex].real(cos(acc) * amplitude);
		m_dotSampleBuf[outIndex].imag(sin(acc) * amplitude);
		amplitude += ampInc;
		acc += phase;
	}
	//Steady signal
	for (quint32 i=0; i< m_numSamplesDot; i++, outIndex++) {
		m_dotSampleBuf[outIndex].real(cos(acc) * m_amplitude);
		m_dotSampleBuf[outIndex].imag(sin(acc) * m_amplitude);
		acc += phase;
	}
	//Decline amplitude during fall
	amplitude = m_amplitude - ampInc;
	for (quint32 i=0; i< m_numSamplesFall; i++, outIndex++) {
		m_dotSampleBuf[outIndex].real(cos(acc) * amplitude);
		m_dotSampleBuf[outIndex].imag(sin(acc) * amplitude);
		amplitude -= ampInc;
		acc += phase;
	}

	acc = 0;
	ampInc = m_amplitude / m_numSamplesFall;
	amplitude = ampInc;
	outIndex = 0;
	for (quint32 i=0; i<m_numSamplesRise; i++, outIndex++) {
		m_dashSampleBuf[outIndex].real(cos(acc) * amplitude);
		m_dashSampleBuf[outIndex].imag(sin(acc) * amplitude);
		amplitude += ampInc;
		acc += phase;
	}
	//Steady signal
	for (quint32 i=0; i< m_numSamplesDash; i++, outIndex++) {
		m_dashSampleBuf[outIndex].real(cos(acc) * m_amplitude);
		m_dashSampleBuf[outIndex].imag(sin(acc) * m_amplitude);
		acc += phase;
	}
	//Decline amplitude during fall
	amplitude = m_amplitude - ampInc;
	for (quint32 i= 0; i< m_numSamplesFall; i++, outIndex++) {
		m_dashSampleBuf[outIndex].real(cos(acc) * amplitude);
		m_dashSampleBuf[outIndex].imag(sin(acc) * amplitude);
		amplitude -= ampInc;
		acc += phase;
	}

	for (quint32 i=0; i<m_numSamplesElementBuf; i++) {
		m_elementSampleBuf[i].real(0);
		m_elementSampleBuf[i].imag(0);
	}
	for (quint32 i=0; i<m_numSamplesCharBuf; i++) {
		m_charSampleBuf[i].real(0);
		m_charSampleBuf[i].imag(0);
	}
	for (quint32 i=0; i<m_numSamplesWordBuf; i++) {
		m_wordSampleBuf[i].real(0);
		m_wordSampleBuf[i].imag(0);
	}

	m_lastSymbol = MorseCode::WORD_SPACE;
}


void MorseGen::setTextOut(QString textOut)
{
	m_textOut = textOut;
	//Todo: Calc and limit
	m_outSampleBufLen = 128000; //Long enough for highest WPM at highest SR
	m_outSampleBuf = memalign(m_outSampleBufLen);
	m_outSampleBufIndex = 0;
	m_numSamplesOutBuf = 0;
	m_textOutIndex = 0;
}

bool MorseGen::hasOutputSamples()
{
	//If we have remaining characters or samples, return true
	if (m_textOutIndex < m_textOut.length() || m_numSamplesOutBuf > 0)
		return true;
	else
		return false;
}

CPX MorseGen::nextOutputSample()
{
	if (m_textOut.length() == 0)
		return CPX();

	//If no samples in CPX buf
	if (m_numSamplesOutBuf == 0) {
		//No more samples, fill buffer with next char
		QChar nextSymbol;
		//If no more text, start over
		if (m_textOutIndex >= m_textOut.length()) {
			//Restart
			m_textOutIndex = 0;
		}
		nextSymbol = m_textOut[m_textOutIndex++];
		m_numSamplesOutBuf = genText(m_outSampleBuf, nextSymbol.toLatin1());
		if (m_numSamplesOutBuf == 0) {
			//Error
			qDebug()<<"Symbol not found "<<nextSymbol;
			return CPX();
		}
		m_outSampleBufIndex = 0;
	}
	m_numSamplesOutBuf--;
	return m_outSampleBuf[m_outSampleBufIndex++];
}

quint32 MorseGen::genText(CPX *out, quint8 text)
{
	if (text == ' ') {
		return genWord(out);
	}
	//Look up character
	quint32 token = m_morseCode.asciiLookup(text);
	if (token > 0)
		return genToken(out, token);
	return 0;
}

quint32 MorseGen::genToken(CPX *out, quint16 token)
{
	quint32 len = 0;
	quint16 bit;
	bool hasHighBit = false;
	//Shift out each bit off token and ouput
	if (token > 0) {
		//Max bits in token is 9
		for (int i=0; i<9; i++) {
			bit = token & 0x100; //Check high order bit (9)
			if (!hasHighBit && bit == 0) {
				//Looking for high order start bit
				token = token << 1;
				continue;
			} else if (!hasHighBit && bit > 0) {
				token = token << 1;
				hasHighBit = true;
				continue;
			}
			//Found high order start bit, process rest of bits
			if (bit == 0) {
				len += genDot(out+len); //Handles genElement()
			} else {
				len += genDash(out+len); //Handles genElement()
			}
			token = token << 1;
		}
		len += genChar(out+len);
		return len;
	}
	return 0;
}

quint32 MorseGen::genDot(CPX *out)
{
	quint32 len = 0;
	if (m_lastSymbol==MorseCode::DOT || m_lastSymbol == MorseCode::DASH) {
		//Output element space
		len = genElement(out);
	}
	for (quint32 i=0; i<m_numSamplesDotBuf; i++) {
		out[len+i].real(m_dotSampleBuf[i].real());
		out[len+i].imag(m_dotSampleBuf[i].im);
	}
	m_lastSymbol = MorseCode::DOT;
	return len + m_numSamplesDotBuf;
}

quint32 MorseGen::genDash(CPX *out)
{
	quint32 len = 0;
	if (m_lastSymbol==MorseCode::DOT || m_lastSymbol == MorseCode::DASH) {
		//Output element space
		len = genElement(out);
	}
	for (quint32 i=0; i<m_numSamplesDashBuf; i++) {
		out[len+i].real(m_dashSampleBuf[i].real());
		out[len+i].imag(m_dashSampleBuf[i].im);
	}
	m_lastSymbol = MorseCode::DASH;
	return len + m_numSamplesDashBuf;
}

quint32 MorseGen::genElement(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesElementBuf; i++) {
		out[i].real(m_elementSampleBuf[i].real());
		out[i].imag(m_elementSampleBuf[i].im);
	}
	m_lastSymbol = MorseCode::EL_SPACE;
	return m_numSamplesElementBuf;
}

quint32 MorseGen::genChar(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesCharBuf; i++) {
		out[i].real(m_charSampleBuf[i].real());
		out[i].imag(m_charSampleBuf[i].im);
	}
	m_lastSymbol = MorseCode::CH_SPACE;
	return m_numSamplesCharBuf;
}

quint32 MorseGen::genWord(CPX *out)
{
	for (quint32 i=0; i<m_numSamplesWordBuf; i++) {
		out[i].real(m_wordSampleBuf[i].real());
		out[i].imag(m_wordSampleBuf[i].im);
	}
	m_lastSymbol = MorseCode::WORD_SPACE;
	return m_numSamplesWordBuf;

}

quint32 MorseGen::genTable(CPX *out, quint32 index)
{
	MorseSymbol *symbol;
	symbol = m_morseCode.tableLookup(index);
	if (symbol != NULL) {
		//qDebug()<<index<<" "<<symbol->chr<<" "<<symbol->display<<" "<<symbol->dotDash<<" "<<symbol->token;
		return genToken(out, symbol->token);
	}
	return 0;
}
