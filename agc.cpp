//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "agc.h"
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
/*
dttsp uses these values
Attack time: 2ms
Decay time: 500ms
Hang time: 500ms
Slope: 1
Max Gain: 31622.8 (linear, not db)
Min Gain: 0.00001
Initial Gain: 1
*/
AGC::AGC(int r, int n):SignalProcessing(r,n)
{
	agcDelay = NULL;

	setAgcMode(OFF);
}

AGC::~AGC(void)
{
	delete agcDelay;
}
//Returns what we got, keep in sync with setAgcGainTop
int AGC::getAgcGainTop()
{
	return gainTop / 0.25;
}
//Values in = 0-100
//Increasing GainTop increases the possible signal gain, like an RF control
//GainTop indirectly controls hangThreshold
void AGC::setAgcGainTop(int g)
{
	//Trial and error to find something that sounds right
	gainTop = g * 0.25 ;
}
void AGC::setAgcMode(AGCMODE m)
{
	//Turn off ProcessBlock while we're making changes
	agcMode = OFF; 
	switch (m)
	{
	case OFF:
		
		break;
	case SLOW:
		modeHangTimeMs = 500;
		modeFastHangTimeMs = 100;
		modeDecayTimeMs = 500;
		modeAttackTimeMs = 2;
		break;
	case MED:
		modeHangTimeMs = 250;
		modeFastHangTimeMs = 100;
		modeDecayTimeMs = 250;
		modeAttackTimeMs = 2;
		break;
	case LONG:
		modeHangTimeMs = 750;
		modeFastHangTimeMs = 100;
		modeDecayTimeMs = 2000;
		modeAttackTimeMs = 2;
		break;
	case FAST:
		modeHangTimeMs = 100;
		modeFastHangTimeMs = 100;
		modeDecayTimeMs = 100;
		modeAttackTimeMs = 2;
		break;
	default:
		//Default constructor values, can we get rid of these?
		modeHangTimeMs = 480;
		modeFastHangTimeMs = 48;
		modeDecayTimeMs = 500;
		modeAttackTimeMs = 2;
	}
	//# samples for N ms hang
	hangSamples = sampleRate * modeHangTimeMs * 0.001;
	fastHangSamples = sampleRate * modeFastHangTimeMs * 0.001;
	hangCount = 0;

	//Decay controls the weighted average split on a falling signal
	decaySamples = modeDecayTimeMs * sampleRate * 0.001; //# samples @ samplerate to get decay time
	decayWeightNew =  1.0 - eVal(modeDecayTimeMs);
	decayWeightLast = 1.0 - decayWeightNew;
	fastDecayWeightNew =  1.0 - eVal(3.0);
	fastDecayWeightLast = 1.0 - fastDecayWeightNew;

	//Attack controls the weighted average split on a rising signal
	attackSamples = modeAttackTimeMs * sampleRate * 0.001;
	attackWeightNew =  1.0 - eVal(modeAttackTimeMs);
	attackWeightLast = 1.0 - attackWeightNew;
	fastAttackWeightNew =  1.0 - eVal(0.2);
	fastAttackWeightLast = 1.0 - fastAttackWeightNew;
	//delayTime controls how long we delay an input sample before we output it.  This allows us to detect a rapid
	//spike in the samples and adjust gain before the delayed sample is output.  In other words, it lets the 
	//AGC algorithm work on 'future' samples.
	delayTime = (int)(sampleRate * modeAttackTimeMs * 0.003); //Where does .003 come from?
	fastDelayTime = 72; //From dttsp
	//Delay line just needs to be large enough for delayTime plus safety
	if (agcDelay != NULL)
		delete agcDelay;
	agcDelay = new DelayLine(delayTime * 2, delayTime); 

	//Reset working variables so we start clean
	//These are not mode specfic
	gainFix = 10; //10/11/10: Not used

	//GainTop is user adjustable, but reset it every time we change mode
	//UI will have to ask for current value so we can update control
	gainTop = 2.5;
	gainBottom =  0.001; //dttsp 0.0001;
	//gainLimit is the max value our DAC can output, for floating point -1 to +1
	//If we use fixed point, Youngblood does in Nov/Dec QEX article, this would be 32768
	gainLimit = 0.5;
	gainNow = 1.0;

	modeHangWeight = 0.001;

	//Done, set AGC mode
	agcMode = m;
}

/*
Doug Smith, 9-7, for Digital AGC block diagram
QEX Youngblook, Nov/Dec 2002 pg 9 (Best description of how algorithm works

AGC 101
No AGC occurs until after agcThreshold is exceeded.
After threshold is exceeded, audio level is allowed to slowly increase by 5-10db.
A slowly moving weighted average is updated with every sample and used to calculate the agcGain to apply.
AttackTime controls how quickly it responds on the rise. From ARRL: 1 or 2 ms is usually good
HangTime controls how long AGC is maintained, too low and AGC can 'pump'. From ARRL: 100 to 1000ms

*/
CPX * AGC::ProcessBlock(CPX *in)
{
	//We always apply some gain, even if AGC is disabled, to bring audio up to level where we can control
	//with user gain control
	//if (agcMode == OFF || agcDelay == NULL) {
	//	CPXBuf::scale(out,in,gainFix,numSamples);
	//	return out;
	//}
	//10/11/10: I think this just confuses gain management.  Everything handled in final gain step
	if (agcMode == OFF)
		return in;

	float magNow; 
	//hangThreshold is a weighted value between gainTop and gainBottom
	//gainTop could be set by user via slider to affect overall gain of AGC
	if (modeHangWeight > 0)
		hangThreshold = gainTop * modeHangWeight + gainBottom * (1.0 - modeHangWeight);
	else
		hangThreshold = 0.0;

	for (int i=0; i<numSamples; i++)
	{
		//Add sample to delay line
		agcDelay->NewSample(in[i]);

		//Power in current sample
		//Dttsp uses true polar mag (sqrt(a^2 +b^2)), sdrmax uses sqrMag (a^2 + b^2)
		magNow = in[i].mag() * 1.1; 
		
		if (magNow == 0)
			magNow = gainNow;
		else
			//Calc grain relative to some base
			//dttsp uses a base of GainLimit
			//Youngblood limits this to 6db (50%) of DAC max
			magNow = (gainLimit * 0.5) / magNow;

		//Are we below threshold where AGC kicks in?
		if (magNow < hangThreshold) {
			//No AGC, Stop hangTime counter
			hangCount = 0; 
		}

		if (magNow >= gainNow) {
			//Signal level is increasing, Decrease gain (decay) if not in hangTime period
			if (hangCount > 0)
				//Leave gainNow unchanged, just count down hang time
				hangCount --;
			else
								gainNow = (decayWeightLast * gainNow) + (decayWeightNew * min(magNow, gainTop));

		} else {
			//Signal level is decreasing, Increase gain (attack)
			hangCount = hangSamples;
			//Weighted average of last gain and current sample 
						gainNow =  (attackWeightLast * gainNow) + (attackWeightNew * max(magNow,gainBottom));
		}

		//Keep gain between gainTop and gainBottom
				gainNow = max(min(gainNow, gainTop), gainBottom);
		float slope = 1;
				float scale = min(gainNow * slope, gainTop);
		
		//agcDelay handles ring buffer duties and gives us a delayed sample
		out[i] = agcDelay->NextDelay(0).scale(scale);

	}

	return out;
}

/*
Yougblood algorithm from Nov/Dec 2002 QEX
Works on the entire buffer instead of one sample at a time
Fixed attack/decay times of 1ms
If AGC has been triggered, then do AGC for agcHang loops
NOTE: Never completed this experiment, just used it to test some ideas
*/
CPX * AGC::ProcessBlock2(CPX *in)
{
	if (agcMode == OFF)
		return in;

	CPX pol;

	//Test polar conversion, should not hear any difference even though before/after values may vary
	//at extreme limits of precision
#if (0)
	for (int i=0; i<numSamples; i++)
	{
		pol = in[i].CartToPolar();
		pol.re *= 2; //Increase mag, Should get slightly louder, but no distortion
		out[i] = pol.PolarToCart();
	}
	return out;
#endif

	if (modeHangWeight > 0)
		hangThreshold = gainTop * modeHangWeight + gainBottom * (1.0 - modeHangWeight);
	else
		hangThreshold = 0.0;


	//float gainHang =0; //TBD
	float gainStep;
	gainLimit = 0.001; //Why is 1.0 too large?

	//Attack/Delay times are fixed in this algorithm
	attackSamples = .001 * sampleRate; //1ms
	decaySamples = .001 * sampleRate; //1ms

	//Are we executing an AGC event?  If so, apply AGC for N buffers, based on hang time


	//We'll use out buffer to keep the gain that should be applied to each sample 
	//Calc the polar mag() of each sample and find the largest
	float peak = 0;
	float level = 0;
	for (int i=0; i<numSamples; i++) 
	{
		out[i] = in[i].CartToPolar();
				peak = max(peak,out[i].re); //Max magnitude
		level += out[i].re; //Total power in buffer
	}
	//out[] now has polar equivalents of in[], re=mag and im=phase

	//If level is above agc threshold, do agc attack, hang, decay
	if (level > hangThreshold)
		hangCount = hangSamples;

	if (peak != 0) {
		//If no peak, don't change gainNow
		//AGC gain factor with 6db of headroom (50% of max DAC value)
		//Algorithm values result in TOO MUCH GAIN
		//Trial and error shows .001 as gain Limit works ok.  Why??
		gainNow = (gainLimit/2) / peak;
		//gainNow = std::min(gainNow,gainHang);
	}  
	//gain never exceeds gainTop
		gainNow = min(gainNow,gainTop);

	//NOTE: We are scaling the magnitude (re in polar notation) NOT the signal
	//Phase is not changed
	//We'll convert back to Cartesian when done
	if (gainNow < gainPrev) {
		//AGC gain is decreasing, ramp down over decay time
		gainStep = (gainPrev - gainNow) / decaySamples; //increment per sample
		for (int i=0; i<numSamples; i++)
		{
			if (i<decaySamples)
				//ramp down
				out[i].re *= (gainPrev - (gainStep * (i+1)));
			else
				//rest of buffer is flat
				out[i].re *= gainNow;
		}
	} else if (gainNow > gainPrev) {
		//AGC gain is increasing, ramp up over attack time
		gainStep = (gainNow - gainPrev) / attackSamples;
		for (int i=0; i<numSamples; i++)
		{
			if (i<attackSamples)
				out[i].re *= (gainPrev + (gainStep * (i+1)));
			else
				out[i].re *= gainNow;
		}

	} else {
		//Gain is unchanged, no ramp
		for (int i=0; i<numSamples; i++)
			out[i].re *= gainNow;
	}
	
	gainPrev = gainNow;

	//Convert back to Cartesian
	for (int i=0; i<numSamples; i++) {
		pol = out[i];
		out[i] = pol.PolarToCart();
	}

	//CPXBuf::limit(....) do we need to scan out buffer and make sure no values are greater than 1?
	return out;
}
