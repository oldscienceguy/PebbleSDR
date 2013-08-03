//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SignalProcessing.h"
#include "string.h"

SignalProcessing::SignalProcessing(int sr, int ns)
{
	sampleRate = sr;
	numSamples = ns;
	numSamplesX2 = ns*2; //Frequency used
	out = CPXBuf::malloc(numSamples);
}

SignalProcessing::~SignalProcessing(void)
{
	if (out != NULL)
		CPXBuf::free(out);
}

//Returns the total power in the sample buffer, using Lynn formula
double SignalProcessing::totalPower(CPX *in, int bsize)
{
	float tmp = 0.0;
	//sum(re^2 + im^2)
    for (int i = 0; i < bsize; i++) {
		//Total power of this sample pair
        tmp += cpxToWatts(in[i]);
    }
	return tmp;
}
/*
db tutorial from Steven Smith
'bel' means power has changed by factor of 10
'decibel' is 0.1 of a bell, so 10db means power changed by factor of 10
60db = power changed by 10^3 = 1000
40db = power changed by 10^2 = 100
20db = power changed by 10^1 = 10
0db = power changed by 10^0 = 1
*/
//Power same as cpx.mag()
//Convert Power to Db
double SignalProcessing::powerToDb(double p)
{
    //For our purposes -127db is the lowest we'll ever see.  Handle special case of 0 directly
    if (p==0)
        return global->minDb;

	//Std equation for decibles is A(db) = 10 * log10(P2/P1) where P1 is measured power and P2 is compared power
    //Voltage = 20 * log10(V2/V1)
	//  + ALMOSTZERO avoid problem if p==0 but does not impact result
    return  qBound(global->minDb, 10.0 * log10(p + ALMOSTZERO), global->maxDb);
}
double SignalProcessing::dbToPower(double db)
{
	return pow(10, db/10.0);
}
//Steven Smith pg 264
double SignalProcessing::amplitudeToDb(double a)
{
    return qBound(global->minDb, 20.0 * log10(a + ALMOSTZERO), global->maxDb);
}
//Steven Smith pg 264
double SignalProcessing::dbToAmplitude(double db)
{
	return pow(10, db/20.0);
}
/*
	One S unit = 6db
	s0 = -127, s1=-121, s2=-115, ... s9 = -73db (-93dbVHF)
	+10 = -63db, +20 = -53, ... +60 = -13db
*/
int SignalProcessing::dbToSUnit(double db)
{
	int unit = db/6;
	return unit;
}
double SignalProcessing::dBm_2_Watts(double dBm)
{
	return pow(10.0, (dBm - 30.0) / 10.0);
}

double SignalProcessing::watts_2_dBm(double watts)
{
	if (watts < 10.0e-32) 
		watts = 10.0e-32;
    return qBound(global->minDb, (10.0 * log10(watts)) + 30.0, global->maxDb);
}

double SignalProcessing::dBm_2_RMSVolts(double dBm, double impedance)
{
	return sqrt(dBm_2_Watts(dBm) * impedance);
}

double SignalProcessing::rmsVolts_2_dBm(double volts, double impedance)
{
	return watts_2_dBm((volts * volts) / impedance);
}




DelayLine::DelayLine(int s, int d) 
{
	size = s;
	delay = d;
	buffer = CPXBuf::malloc(size);
	CPXBuf::clear(buffer,size);
	head = 0;
	last = 0;
}
DelayLine::~DelayLine()
{
	if (buffer != NULL) CPXBuf::free(buffer);
}
void DelayLine::NewSample(CPX c)
{
	buffer[head] = c;
	last = head;
	//buffer order is newest to oldest, so head is decremented modulo size
	head = (head==0) ? size-1 : --head;
}
//Next sample in delay line, mod size of delay line to wrap (ring buffer)
CPX DelayLine::NextDelay(int i)
{
	//Last new value, plus delay, plus passed index, mod line size
	int next = (last + delay + i) % size;
	return buffer[next];
}
CPX DelayLine::operator [] (int i)
{
	return buffer[i];
}
//Convolution sum or Multiply And Accumulate (MAC)
//Key component for filter math
CPX DelayLine::MAC(double *coeff, int numCoeff)
{
	mutex.lock();
	int next;
	CPX mac(0,0);
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) % size;
		//This is a MAC operation and is NOT the same as cpx1 * cpx2 (see cpx.h)
		mac.re += buffer[next].re * coeff[i];
		mac.im += buffer[next].im * coeff[i];
	}
	mutex.unlock();
	return mac;
}
CPX DelayLine::MAC(CPX *coeff, int numCoeff)
{
	mutex.lock();
	int next;
	CPX mac(0,0);
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) % size;
		//This is a MAC operation and is NOT the same as cpx1 * cpx2 (see cpx.h)
		mac.re += buffer[next].re * coeff[i].re;
		mac.im += buffer[next].im * coeff[i].im;
	}
	mutex.unlock();
	return mac;
}
#if (0)
void LMS(int numCoeff)
{
	int next;
	float sos = 0;
	//Compute the average squared value
	for (int i = 0; i < numCoeff; i++)
	{
		next = (last + delay + i) & mask;
		sos += buffer[next].re;
	}
	float avgSOS = sos / numCoeff;

}
#endif
