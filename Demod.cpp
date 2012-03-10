//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "Demod.h"
#include <string.h>

Demod::Demod(int sr, int ns) :
	SignalProcessing(sr,ns)   
{   
    setDemodMode(dmAM);
	
	//SAM config
	samLoLimit = -2000 * TWOPI/sampleRate; //PLL range
	samHiLimit = 2000 * TWOPI/sampleRate;
	samBandwidth = 300;
	samLockCurrent = 0.5;
	samLockPrevious = 1.0;
	samDc = 0.0;

	//FMN config, should these come from filter settings?
	fmLoLimit = -6000 * TWOPI/sampleRate;
	fmHiLimit = 6000 * TWOPI/sampleRate;
	fmBandwidth = 5000;

	pllPhase = 0.0;  //Tracks ref sig in PLL
	pllFrequency = 0.0;

	fmAfc = 0.0;
	fmIPrev = 0.0;
	fmQPrev = 0.0;

	amDc = 0.0;
	amSmooth = 0.0;
}

Demod::~Demod()
{
}

CPX * Demod::ProcessBlock(CPX * in)
{
	//float tmp;

    switch(mode) {
        case dmAM: // AM
			//SimpleAM(in,out);
			SimpleAMSmooth(in,out); //Sounds slightly better
            break;
        case dmSAM: // SAM
            PllSAM(in, out);
            break;
        case dmFMN: // FMN
			//SimpleFM(in,out);
			SimpleFM2(in,out);
			//PllFMN(in,out);
            break;
        case dmFMW: // FMW
			//SimpleFM2(in,out);
			PllFMW(in,out);
            break;

		//We've already filtered the undesired sideband @ BPFilter
		//So we don't need additional demod
		//
		case dmUSB:
			//SimpleUSB(in,out); //Only if we still have both sidebands
			//break;
		
		case dmLSB:
			//SimpleLSB(in,out); //Only if we still have both sidebands
			//break;

		//Passthrough, no demod
		default:
			return in;
            break;
    }
	return out;
}

void Demod::SimpleAM(CPX *in, CPX *out)
{
	float tmp;
	for (int i=0;i<numSamples;i++)
	{
		//Just return the magnitude of each sample
		//tmp = sqrt(in[i].re * in[i].re + in[i].im * in[i].im);
		//10/11/10: putting 50% of mag in re and im yields the same signal power
		//as USB or LSB.  Without this, we get AM distortion at the same gain
		tmp = in[i].mag() * 0.5;
		out[i].re = out[i].im = tmp;
	}
}
//dttsp algorithm with smoothing
void Demod::SimpleAMSmooth(CPX *in, CPX *out)
{
	double current;
	for (int i = 0; i < numSamples; i++)
	{
		current = in[i].mag();
		//Smooth with weighted average
		amDc = 0.9999f * amDc + 0.0001f * current;
		amSmooth = 0.5f * amSmooth + 0.5f * (current - amDc);
		//smooth *= .5 so we keep overall signal magnitude unchanged
		out[i].re = out[i].im = amSmooth * 0.5;
	}

}
void Demod::SimpleUSB(CPX *in, CPX *out)
{
	float tmp;
	for (int i=0;i<numSamples;i++)
	{
		tmp = in[i].re  + in[i].im;
		out[i].re = out[i].im = tmp;
	}
}
void Demod::SimpleLSB(CPX *in, CPX *out)
{
	float tmp;
	for (int i=0;i<numSamples;i++)
	{
		tmp = in[i].re  - in[i].im;
		out[i].re = out[i].im = tmp;
	}
}
void Demod::SimplePhase(CPX *in, CPX *out)
{
	float tmp;
	for (int i=0;i<numSamples;i++)
	{
		tmp = tan(in[i].re  / in[i].im);
		out[i].re = out[i].im = tmp;
	}
}

//Doug Smith Eq22.  I think this is what SDRadio also used
//Note: Not sure about divisor, Doug uses I^2 + Q^2, Ireland uses I*Ip + Q*Qp
/*
These 3 formulas are all derivations of the more complex phase delta algorithm (SimpleFM2) and
are easier to compute.  
Ip & Qp are the previous sample, I & Q are the current sample
1) (Q * Ip - I * Qp) / (I * Ip + Q * Qp)
2) (Q * Ip - I * Qp) / (I^2 + Q^2)
3) (Q * Ip + I * Qp) / (I^2 + Q^2) //COMMAND by Andy Talbot RSGB, haven't tested adding vs sub yet
*/
void Demod::SimpleFM(CPX *in, CPX *out)
{
	float tmp;
	float I,Q; //To make things more readable
	for (int i=0;i<numSamples;i++)
	{
		I = in[i].re;
		Q = in[i].im;

		tmp = (Q * fmIPrev - I * fmQPrev) / (I * fmIPrev + Q * fmQPrev); //This seems to work better, less static, better audio
		//tmp = (Q * Ip - I * Qp) / (I * I + Q * Q);

		//Output volume is very low, scaling by even 100 doesn't do anything?
		out[i].re = out[i].im = tmp/100;
		fmIPrev = I;
		fmQPrev = Q;
	}

}
//From Doug Smith QEX article, Eq 21,  based on phase delta
//From Erio Blossom FM article in Linux Journal Sept 2004
void Demod::SimpleFM2(CPX *in, CPX *out)
{
	CPX prod;
	for (int i=1;i<numSamples;i++)
	{
		//The angle between to subsequent samples can be calculated by multiplying one by the complex conjugate of the other
		//and then calculating the phase (arg() or atan()) of the complex product
		prod = in[i] * in[i-1].conj();
		//Scale demod output to match am, usb, etc range
		out[i].re = out[i].im = prod.phase() / 100;
	}
}

//dttsp PLL algorithm
CPX Demod::PLL(CPX sig, float loLimit, float hiLimit)
{
	CPX z;
	CPX delay;
    float difference;
	//Todo: See if we can use NCO here
	//This is the generated signal to sync with
    z.re = cos(pllPhase);
    z.im = sin(pllPhase);

	//delay.re = z.re * sig.re + z.im * sig.im;
	//delay.im = -z.im * sig.re + z.re * sig.im;

	delay = z * sig;

	// For SAM we need the magnitude (hypot)
	// For FM we need the freq difference
	if (mode == dmSAM)
		difference = sig.mag() * delay.phase();
	else
		difference = delay.phase();

    
    pllFrequency += beta * difference;

	//Keep the PLL within our limits
    if (pllFrequency < loLimit)
        pllFrequency = loLimit;
    if (pllFrequency > hiLimit)
        pllFrequency = hiLimit;

    pllPhase += pllFrequency + alpha * difference;

	//Next reference signal
    while (pllPhase >= TWOPI)
        pllPhase -= TWOPI;
    while (pllPhase < 0)
        pllPhase += TWOPI;

	return delay;
}
void Demod::PllSAM(  CPX * in, CPX * out ) 
{
	CPX delay;

    for (int i = 0; i < numSamples; i++) {
		delay = PLL(in[i],samLoLimit,samHiLimit);
		//Basic am demod
        samLockCurrent = 0.999f * samLockCurrent + 0.001f * fabs(delay.im);
        samLockPrevious = samLockCurrent;
        samDc = (0.9999f * samDc) + (0.0001f * delay.re);
		out[i].re = out[i].im = (delay.re - samDc) *.50 ;
        
    }
}

//Uses Doug Smith phase delta equations
//PLL algorithm from dttsp
void Demod::PllFMN(  CPX * in, CPX * out ) 
{
    float cvt = ((0.45 * sampleRate) / ONEPI) / fmBandwidth;

    for (int i = 0; i < numSamples; i++) {
		PLL(in[i],fmLoLimit,fmHiLimit);
        fmAfc = 0.9999 * fmAfc + 0.0001 * pllFrequency;
		out[i].re  = out[i].im = (pllFrequency - fmAfc) * cvt / 100;
    }
}

//Not working yet.  Can't just use FMN with wider BPF, I think we have to do more post-demod processing
//To get signal separated from stereo channel information
void Demod::PllFMW(  CPX * in, CPX * out ) 
{
	SimpleFM(in,out);
	//CPXBuf::copy(out, in, numSamples);
}

void Demod::setDemodMode(DEMODMODE _mode) 
{
    mode = _mode;
	pllFrequency = 0.0;
	pllPhase = 0.0;
	if (mode == dmSAM) {
		alpha = 0.3 * samBandwidth * TWOPI/sampleRate;
        beta = alpha * alpha * 0.25;
		samLockCurrent = 0.5;
		samLockPrevious = 1.0;
		samDc = 0.0;
	} else if (mode == dmFMN || mode == dmFMW) {
		alpha = 0.3 * fmBandwidth * TWOPI/sampleRate;
        beta = alpha * alpha * 0.25;
		fmAfc = 0.0;
	} else {
		alpha = 0.3 * 500.0 * TWOPI / sampleRate;
		beta = alpha * alpha * 0.25;
	}

}

DEMODMODE Demod::demodMode() const 
{
    return mode;
}

DEMODMODE Demod::StringToMode(QString m)
{
	if (m == "AM") return dmAM;
	else if (m == "SAM") return dmSAM;
	else if (m == "LSB") return dmLSB;
	else if (m == "USB") return dmUSB;
	else if (m == "DSB") return dmDSB;
	else if (m == "FMW") return dmFMW;
	else if (m == "FMN") return dmFMN;
	else if (m == "CWL") return dmCWL;
	else if (m == "CWU") return dmCWU;
	else if (m == "DIGL") return dmDIGL;
	else if (m == "DIGU") return dmDIGU;
	else if (m == "NONE") return dmNONE;
	else return dmAM; //default
}
QString Demod::ModeToString(DEMODMODE dm)
{
	if (dm == dmAM) return "AM";
	else if (dm == dmSAM) return "SAM";
	else if (dm == dmLSB) return "LSB";
	else if (dm == dmUSB) return "USB";
	else if (dm == dmDSB) return "DSB";
	else if (dm == dmFMW) return "FMW";
	else if (dm == dmFMN) return "FMN";
	else if (dm == dmCWL) return "CWL";
	else if (dm == dmCWU) return "CWU";
	else if (dm == dmDIGL) return "DIGL";
	else if (dm == dmDIGU) return "DIGU";
	else if (dm == dmNONE) return "NONE";
	else return "AM"; //default
}
