//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "signalstrength.h"

SignalStrength::SignalStrength(int sr, int ns):
	SignalProcessing(sr,ns)
{
    instValue = global->minDb;
    avgValue = global->minDb;
    extValue = global->minDb;
	//Todo: This should be adj per receiver and user should be able to calibrate with known signal
	//SRV9 -40
	//SREnsemble 
    //Set this using testbench signal to fixed db output and make sure instDb is same
    correction = 0.0;

}

SignalStrength::~SignalStrength(void)
{
}
/*
From Lynn book pg 443-445: 
Avg power for any given freq = 1/2 * (B^2 + C^2) 'This is mean of the squares'
Total power = sum of power for each freq in fourier series
or
Total power = mean square value of time domain waveform

RMS (Root Mean Square) = sqrt(mean of the squares)

ProcessBlock is called after we've mixed and filtered our samples, so *in contains desired signal
Compute the total power of all the samples, convert to dB save
Background thread will pick up instValue or avgValue and display
*/

#if 0
if(SampleRate != m_SampleRate)
{	//need to recalculate any values dependent on sample rate
    m_SampleRate = SampleRate;
    m_AttackAlpha = (1.0-exp(-1.0/(m_SampleRate*ATTACK_TIMECONST)) );
    m_DecayAlpha = (1.0-exp(-1.0/(m_SampleRate*DECAY_TIMECONST)) );
//qDebug()<<"SMeter vals "<<m_AttackAlpha << m_DecayAlpha << SampleRate;
}
for(int i=0; i<length; i++)
{
    //calculate instantaeous power magnitude of pInData which is I*I + Q*Q
    TYPECPX in = pInData[i];
    //convert I/Q magnitude to dB power
    TYPEREAL mag = 10.0*log10((in.re*in.re+in.im*in.im)/ MAX_PWR + 1e-50);
    //calculate attack and decay average
    m_AttackAve = (1.0-m_AttackAlpha)*m_AttackAve + m_AttackAlpha*mag;
    m_DecayAve = (1.0-m_DecayAlpha)*m_DecayAve + m_DecayAlpha*mag;
    if(m_AttackAve>m_DecayAve)
    {	//if attack average is greater then must be increasing signal
        m_AverageMag = m_AttackAve;	//use attack average value
        m_DecayAve = m_AttackAve;	//force decay average to attack average
    }
    else
    {	//is decreasing strength so use decay average
        m_AverageMag = m_DecayAve;	//use decay average value
    }
    if(mag > m_PeakMag)
        m_PeakMag = mag;		//save new peak (reset when read )
}


#endif

CPX * SignalStrength::ProcessBlock(CPX *in, int squelch)
{
	//Squelch values are -100db to 0db

	//Same as TotalPower, except here we modify out if below squelch
    float tmp = 0.0;
    float pwr = 0.0;
    float db = 0.0;
	//double squelchWatts = dBm_2_Watts(squelch);
	//sum(re^2 + im^2)
    for (int i = 0; i < numSamples; i++) {
		//Total power of this sample pair
        pwr = cpxToWatts(in[i]); //Power in watts
        db = powerToDb(pwr); //Watts to db
        //Verified 8/13 comparing testbench gen output at all db levels
        instValue = db;
        //Weighted average 90/10
        avgValue = 0.99 * avgValue + 0.01 * instValue;

		//we clearing whole buffer so audio is either on or off
		//Sample by sample gives us gradual threshold, which sounds strange
		if (avgValue < squelch) {
		//if (watts < squelchWatts) {
			out[i].re = 0;
			out[i].im = 0;
		} else {
			out[i] = in[i];
		}
    }
	
	return out;
}
float SignalStrength::instFValue() {
    float temp = instValue + correction;
    temp = temp > global->maxDb ? global->maxDb : temp;
    temp = temp < global->minDb ? global->minDb : temp;
    return temp;
}

float SignalStrength::avgFValue() {
    float temp = avgValue + correction;
    temp = temp > global->maxDb ? global->maxDb : temp;
    temp = temp < global->minDb ? global->minDb : temp;
    return temp;
}

//Used for testing with other values
float SignalStrength::extFValue(){
    return extValue;
}
void SignalStrength::setExtValue(float v)
{
    extValue = v;
}

char SignalStrength::instCValue() {
    return (char)(instValue + correction + (float)SPECDBMOFFSET);
}

char SignalStrength::avgCValue() {
    return (char)(avgValue + correction + (float)SPECDBMOFFSET);
}

void SignalStrength::setCorrection(const float value) {
    correction = value;
}
