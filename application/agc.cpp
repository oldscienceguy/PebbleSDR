//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "agc.h"

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
AGC::AGC(quint32 _sampleRate, quint32 _bufferSize):ProcessStep(_sampleRate,_bufferSize)
{
	m_agcDelay = NULL;


    //cuteSDR
	m_useHang = false;
	m_threshold = 0;
	m_slopeFactor = 0;
	m_decay = 0;
	m_sampleRate = 100.0; //Trigger init on first call to setup

	uiSliderValueDb = 0;
	setAgcMode(AGC_OFF, 1);

}

AGC::~AGC(void)
{
	delete m_agcDelay;
}

//Returns what we got, keep in sync with SetAgcThreshold
int AGC::getAgcThreshold()
{
	return uiSliderValueDb;
}
//Values in = 0-99
void AGC::setAgcThreshold(int g)
{
	uiSliderValueDb = g; //Could be gain or threshold, used differently

	//Recalc parameters for current mode
	setParameters(m_useHang, g, m_slopeFactor, m_decay);

}

void AGC::setAgcMode(AgcMode m, int threshold)
{
    bool useHang = false;
    int slopeFactor = 0; //0db to 10db
    int decay = 200; //20ms to 2000ms

	m_agcMode = m;
	switch (m_agcMode) {
		case AGC_OFF:
            break;
		case ACG_FAST:
            //hang time 100ms (fixed value from other algorithms, not used)
            decay = 100;
            break;
		case AGC_SLOW:
            //hang time 500ms
            decay = 500;
            break;
		case AGC_MED:
            //hang time 250ms
            decay = 250;
            break;
		case AGC_LONG:
            //hang time 750ms
            decay = 2000;
            break;
        default:
            break;
    }
	setParameters(useHang, threshold, slopeFactor, decay);
}

CPX* AGC::processBlock(CPX *pInData)
{
	if (m_agcMode == AGC_OFF) {
        //manual gain just multiply by m_ManualGain
        for(int i=0; i<numSamples; i++)
        {
			out[i].real(m_manualIFGainAmp * pInData[i].real());
			out[i].imag(m_manualIFGainAmp * pInData[i].imag());
        }
        return out;
    }

    double gain;
    double mag;
    CPX delayedin;
    //m_Mutex.lock();
    for(int i=0; i<numSamples; i++)
    {
        CPX in = pInData[i];	//get latest input sample
        //Get delayed sample of input signal
		delayedin = m_sigDelayBuf[m_sigDelayPtr];
        //put new input sample into signal delay buffer
		m_sigDelayBuf[m_sigDelayPtr++] = in;
		if( m_sigDelayPtr >= m_delaySamples)	//deal with delay buffer wrap around
			m_sigDelayPtr = 0;

        //double dmag = 0.5* log10(  (dsig.real()*dsig.real()+dsig.imag()*dsig.imag())/(MAX_AMPLITUDE*MAX_AMPLITUDE) + 1e-16);	//clamped to -160dBfs
        //pOutData[i].real(3000*dmag);
#if 1
        mag = fabs(in.real());
        double mim = fabs(in.imag());
        if(mim>mag)
            mag = mim;
        mag = log10( mag + MIN_CONSTANT ) - log10(MAX_AMPLITUDE);		//0==max  -8 is min==-160dB
#else
        mag = in.real()*in.real()+in.imag()*in.imag();
        //mag is power so 0.5 factor takes square root of power
        mag = 0.5* log10( mag/(MAX_AMPLITUDE*MAX_AMPLITUDE) + 1e-16);	//clamped to -160dBfs
#endif
        //pOutData[i].real(3000*mag);
        //pOutData[i].real(1500*log10( ((delayedin.real()*delayedin.real())+(delayedin.imag()*delayedin.imag()))/(MAX_AMPLITUDE*MAX_AMPLITUDE) + 1e-16););

        //create a sliding window of 'm_WindowSamples' magnitudes and output the peak value within the sliding window
		double tmp = m_magBuf[m_magBufPos];	//get oldest mag sample from buffer into tmp
		m_magBuf[m_magBufPos++] = mag;			//put latest mag sample in buffer;
		if( m_magBufPos >= m_windowSamples)		//deal with magnitude buffer wrap around
			m_magBufPos = 0;
		if(mag > m_peak)
        {
			m_peak = mag;	//if new sample is larger than current peak then use it, no need to look at buffer values
        }
        else
        {
			if(tmp == m_peak)		//tmp is oldest sample pulled out of buffer
            {	//if oldest sample pulled out was last peak then need to find next highest peak in buffer
				m_peak = -8.0;		//set to lowest value to find next max peak
                //search all buffer for maximum value and set as new peak
				for(int i=0; i<m_windowSamples; i++)
                {
					tmp = m_magBuf[i];
					if(tmp > m_peak)
						m_peak = tmp;
                }
            }
        }

        //pOutData[i].imag(3000*m_Peak);

		if(m_useHang)
        {	//using hang timer mode
			if(m_peak>m_attackAvg)	//if power is rising (use m_AttackRiseAlpha time constant)
				m_attackAvg = (1.0-m_attackRiseAlpha)*m_attackAvg + m_attackRiseAlpha*m_peak;
            else					//else magnitude is falling (use  m_AttackFallAlpha time constant)
				m_attackAvg = (1.0-m_attackFallAlpha)*m_attackAvg + m_attackFallAlpha*m_peak;

			if(m_peak>m_decayAvg)	//if magnitude is rising (use m_DecayRiseAlpha time constant)
            {
				m_decayAvg = (1.0-m_decayRiseAlpha)*m_decayAvg + m_decayRiseAlpha*m_peak;
				m_hangTimer = 0;	//reset hang timer
            }
            else
            {	//here if decreasing signal
				if(m_hangTimer<m_hangTime)
					m_hangTimer++;	//just inc and hold current m_DecayAve
                else	//else decay with m_DecayFallAlpha which is RELEASE_TIMECONST
					m_decayAvg = (1.0-m_decayFallAlpha)*m_decayAvg + m_decayFallAlpha*m_peak;
            }
        }
        else
        {	//using exponential decay mode
            // perform average of magnitude using 2 averagers each with separate rise and fall time constants
			if(m_peak>m_attackAvg)	//if magnitude is rising (use m_AttackRiseAlpha time constant)
				m_attackAvg = (1.0-m_attackRiseAlpha)*m_attackAvg + m_attackRiseAlpha*m_peak;
            else					//else magnitude is falling (use  m_AttackFallAlpha time constant)
				m_attackAvg = (1.0-m_attackFallAlpha)*m_attackAvg + m_attackFallAlpha*m_peak;

            //pOutData[i].imag(3000*m_AttackAve);
			if(m_peak>m_decayAvg)	//if magnitude is rising (use m_DecayRiseAlpha time constant)
				m_decayAvg = (1.0-m_decayRiseAlpha)*m_decayAvg + m_decayRiseAlpha*(m_peak);
            else					//else magnitude is falling (use m_DecayFallAlpha time constant)
				m_decayAvg = (1.0-m_decayFallAlpha)*m_decayAvg + m_decayFallAlpha*(m_peak);
            //pOutData[i].imag(3000*m_DecayAve);
        }
        //use greater magnitude of attack or Decay Averager
		if(m_attackAvg>m_decayAvg)
			mag = m_attackAvg;
        else
			mag = m_decayAvg;

        //pOutData[i].imag(3000*mag);
        //calc gain depending on which side of knee the magnitude is on
		if(mag<=m_knee)		//use fixed gain if below knee
			gain = m_fixedGain;
        else				//use variable gain if above knee
			gain = AGC_OUTSCALE * pow(10.0, mag*(m_gainSlope - 1.0) );
        //pOutData[i].real(.5*gain);
        out[i].real(delayedin.real() * gain);
        out[i].imag(delayedin.imag() * gain);
    }

        //m_Mutex.unlock();

    return out;
}

/*
 * Reference A discussion on the Automatic Gain Control (AGC) requirements of the SDR1000 By Phil Harman VK6APH
 *Threshold and ManualGain are set by the same slider, depending on AGC active
 *If AGC is not active, then slider controls manual gain
 *Manual gain ranges from 0dB to 100dB
 *If AGC is active, then slider controls AGC threshold
 *Threshold specifies where this AGC becomes active (the knee).  Below this value AGC is off (use manual gain), above it is on
 *Threshold ranges from -100db to 0db.  (default -20)
 *At -100 AGC is on for everything and signal will probably be overloaded
 *At 0db AGC is effectively off expect for limiting strongest signals
 *
 *AGC slope controls what happens when AGC kicks in above the knee (threshold), ie how output is reduced below maximum output level
 *Slope range is 0 to 10db (default is 0)
 *
 *Decay is the rate that AGC reacts to changing signals (FAST,MED,SLOW,LONG)
 *Decay range is 20ms to 5000ms (default is 200ms)
 *Sets the attack and decay filtering values
 *
 *2 algorithms can be selected, Hangtime timer and exponential decay
 *Hang time uses an adjustable (m_HangTimer) timer to control the delay on decreasing signal before AGC is updated
 *Shorter hang time = faster but jagged response, Longer hang time = slower but smoother response
 *Hang time is calculated as factor of decay, but could also be specified as fixed values associated with FAST,MED,SLOW,LONG as we used to do
 *
 *Exponential decay uses averages instead of hang timer to control responsiveness of AGC
*/



void AGC::setParameters(bool useHang, int threshold, int slopeFactor, int decay)
{
	if (m_agcMode == AGC::AGC_OFF) {
		//Threshold is dbGain 0 to N
		//RL: We're converting a db gain figure to amplitude so we can scale with it
		//Slider value is normally 0db to 99db, which is way to much simple gain
		//Scale it back to a level where we're unlikely to overload
		m_manualIFGainAmp = DB::dBToAmplitude(threshold / 5);
		return;
	} else {
		//Threshold is agc -120 to 0;
		threshold = -threshold; //Convert for agc calculations
		m_manualIFGainAmp = 1; //No gain
	}

	if((useHang == m_useHang) &&
		(threshold == m_threshold) &&
		(slopeFactor == m_slopeFactor) &&
		(decay == m_decay) )
    {
        return;		//just return if no parameter changed
    }
    //m_Mutex.lock();
	m_useHang = useHang;
	m_threshold = threshold;
	m_slopeFactor = slopeFactor;
	m_decay = decay;
    //sampleRate doesn't change in Pebble, call once to init
	if(	m_sampleRate != sampleRate)
    {	//clear out delay buffer and init some things if sample rate changes
		m_sampleRate = sampleRate;
        for(int i=0; i<MAX_DELAY_BUF; i++)
        {
			m_sigDelayBuf[i].real(0.0);
			m_sigDelayBuf[i].imag(0.0);
			m_magBuf[i] = -16.0;
        }
		m_sigDelayPtr = 0;
		m_hangTimer = 0;
		m_peak = -16.0;
		m_decayAvg = -5.0;
		m_attackAvg = -5.0;
		m_magBufPos = 0;
    }

    //calculate parameters for AGC gain as a function of input magnitude
	m_knee = (double)m_threshold/20.0;
	m_gainSlope = m_slopeFactor/(100.0);
    //m_FixedGain = AGC_OUTSCALE * pow(10.0, m_Knee*(m_GainSlope - 1.0) );	//fixed gain value used below knee threshold
	m_fixedGain = AGC_OUTSCALE * pow(10.0, m_knee*(m_gainSlope - 1.0) );	//fixed gain value used below knee threshold
//qDebug()<<"m_Knee = "<<m_Knee<<" m_GainSlope = "<<m_GainSlope<< "m_FixedGain = "<<m_FixedGain;

    //calculate fast and slow filter values.
	m_attackRiseAlpha = (1.0-exp(-1.0/(m_sampleRate*ATTACK_RISE_TIMECONST)) );
	m_attackFallAlpha = (1.0-exp(-1.0/(m_sampleRate*ATTACK_FALL_TIMECONST)) );

	m_decayRiseAlpha = (1.0-exp(-1.0/(m_sampleRate * (double)m_decay*.001*DECAY_RISEFALL_RATIO)) );	//make rise time DECAY_RISEFALL_RATIO of fall
	m_hangTime = (int)(m_sampleRate * (double)m_decay * .001);

	if(m_useHang)
		m_decayFallAlpha = (1.0-exp(-1.0/(m_sampleRate * RELEASE_TIMECONST)) );
    else
		m_decayFallAlpha = (1.0-exp(-1.0/(m_sampleRate * (double)m_decay *.001)) );

	m_delaySamples = (int)(m_sampleRate*DELAY_TIMECONST);
	m_windowSamples = (int)(m_sampleRate*WINDOW_TIMECONST);

    //clamp Delay samples within buffer limit
	if(m_delaySamples >= MAX_DELAY_BUF-1)
		m_delaySamples = MAX_DELAY_BUF-1;

    //m_Mutex.unlock();
}




#if 0
//Returns what we got, keep in sync with SetAgcThreshold
int AGC::getAgcThreshold()
{
	return gainTop / 0.25;
}
//Values in = 0-100
//Increasing GainTop increases the possible signal gain, like an RF control
//GainTop indirectly controls hangThreshold
void AGC::setAgcThreshold(int g)
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
	//	scaleCPX(out,in,gainFix,numSamples);
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
		pol.real(pol.real() * 2); //Increase mag, Should get slightly louder, but no distortion
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
				out[i].real(out[i].real() * (gainPrev - (gainStep * (i+1))));
			else
				//rest of buffer is flat
				out[i].real(out[i].real() * gainNow);
		}
	} else if (gainNow > gainPrev) {
		//AGC gain is increasing, ramp up over attack time
		gainStep = (gainNow - gainPrev) / attackSamples;
		for (int i=0; i<numSamples; i++)
		{
			if (i<attackSamples)
				out[i].real(out[i].real() * (gainPrev + (gainStep * (i+1))));
			else
				out[i].real(out[i].real() * gainNow);
		}

	} else {
		//Gain is unchanged, no ramp
		for (int i=0; i<numSamples; i++)
			out[i].real(out[i].real() * gainNow);
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
#endif
