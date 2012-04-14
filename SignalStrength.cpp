//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "SignalStrength.h"

SignalStrength::SignalStrength(int sr, int ns):
	SignalProcessing(sr,ns)
{
	instValue = 27.0; //If correction = -40, the 27 = -13 or full scale
	avgValue = -127.0;
    extValue = -127.0;
	//Todo: This should be adj per receiver and user should be able to calibrate with known signal
	//SRV9 -40
	//SREnsemble 
	correction = -40.0;  //Adj to -127db to -13db

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
CPX * SignalStrength::ProcessBlock(CPX *in, int squelch)
{
	//Squelch values are -100db to 0db

	//Same as TotalPower, except here we modify out if below squelch
    float tmp = 0.0;
	float watts = 0.0;
	//double squelchWatts = dBm_2_Watts(squelch);
	//sum(re^2 + im^2)
    for (int i = 0; i < numSamples; i++) {
		//Total power of this sample pair
        watts = cpxToWatts(in[i]);
		tmp += watts;
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
	
    instValue = powerToDb(tmp) + correction;

	//Weighted average 90/10
    avgValue = 0.9 * avgValue + 0.1 * instValue;
	return out;
}
float SignalStrength::instFValue() {
    if (instValue > -13)
        return -13.0;
    else
        return instValue;
}

float SignalStrength::avgFValue() {
    if (avgValue > -13)
        return -13.0;
    else
        return avgValue;
}

float SignalStrength::extFValue(){
    if (extValue > -18)
        return -18.0;
    else
        return extValue; //with or Without correction?
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
