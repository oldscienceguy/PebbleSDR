//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "demod.h"
#include <string.h>
#include "global.h"
#include "receiver.h"
#include "demod/demod_am.h"
#include "demod/demod_sam.h"
#include "demod/wfmdemod.h"


//Set up filter option lists
//Broadcast 15Khz 20 hz -15 khz
//Speech 3Khz 300hz to 3khz
//RTTY 250-1000hz
//CW 200-500hz
//PSK31 100hz

//Must be in same order as DEMODMODE
//Verified with CuteSDR values
//AM, SAM, FM are expressed in bandwidth, ie 16k am = -8000 to +8000 filter
//Rest are 0 relative

const Demod::DemodInfo Demod::demodInfo[] = {
    {dmAM, QStringList() << "20000" << "10000" << "5000", 0, -10000, 10000, 48000, 0, -120, 20},
    {dmSAM,QStringList() << "20000" << "10000" << "5000", 1, -10000, 10000, 48000, 0, -100, 200},
    {dmFMN,QStringList() << "30000" << "10000" << "7000", 0, -15000, 15000, 48000, 0, -100, 200}, //Check FM
    {dmFMM,QStringList(), 0, -100000, 100000, 100000, 0, -100, 200},
    {dmFMS,QStringList(), 0, -100000, 100000, 100000, 0, -100, 200},
    {dmDSB,QStringList() << "20000" << "10000" << "5000", 0, -10000, 10000, 48000, 0, -100, 200},
    {dmLSB,QStringList() << "10000" << "5000" << "2500" << "1500", 1, -20000, 0, 20000, 0, -100, 200},
    {dmUSB,QStringList() << "10000" << "5000" << "2500" << "1500", 1, 0, 20000, 20000, 0, -100, 200},
    {dmCWL,QStringList() << "1000" << "500" << "250" << "100" << "50", 1, -1000, 1000, 1000, 0, -100, 200}, //Check CW
    {dmCWU,QStringList() << "1000" << "500" << "250" << "100" << "50", 1, -1000, 1000, 1000, 0, -100, 200},
    {dmDIGL,QStringList() << "2000" << "1000" << "500" << "250" << "100",2,-20000, 0, 20000, 0, -100, 200},
    {dmDIGU,QStringList() << "2000" << "1000" << "500" << "250" << "100",2, 0, 20000, 20000, 0, -100, 200},
    {dmNONE,QStringList(), 0, 0, 0, 0, 0, -100, 200}

};

const float Demod::usDeemphasisTime = 75E-6; //Use for US & Korea FM
const float Demod::intlDeemphasisTime = 50E-6;  //Use for international FM

//New constructor as we move demods to sub classes
Demod::Demod(int _inputRate, int _numSamples) :
    SignalProcessing(_inputRate,_numSamples)
{

}

//Two input rates, one normal and one for wfm
Demod::Demod(int _inputRate, int _inputWfmRate, int ns) :
    SignalProcessing(_inputRate,ns)
{   
    inputSampleRate = _inputRate;
    inputWfmSampleRate = _inputWfmRate;

    SetDemodMode(dmAM, sampleRate, sampleRate);
	
	//FMN config, should these come from filter settings?
	fmLoLimit = -6000 * TWOPI/sampleRate;
	fmHiLimit = 6000 * TWOPI/sampleRate;
	fmBandwidth = 5000;

	pllPhase = 0.0;  //Tracks ref sig in PLL
	pllFrequency = 0.0;

    fmDCOffset = 0.0;
	fmIPrev = 0.0;
	fmQPrev = 0.0;

    //We're no longer decimating to audio in wfmdemod, so audio rate is same as input rate
    //This fixed bug where FM filters were not working because rate was wrong

    //Moving to subclasses for each demod, transition with instance for each demod, change to vptr later
    demodAM = new Demod_AM(sampleRate, numSamples);
    demodSAM = new Demod_SAM(sampleRate, numSamples);
    demodWFM = new Demod_WFM(sampleRate,numSamples);
    demodWFM->Init(inputWfmSampleRate, inputWfmSampleRate);
    ResetDemod();

}

Demod::~Demod()
{
}

void Demod::SetupDataUi(QWidget *parent)
{
    if (parent == NULL) {
        outputOn = false;

        //We want to delete
        if (dataUi != NULL) {
            delete dataUi;
        }
        dataUi = NULL;
        return;
    } else if (dataUi == NULL) {
        //Create new one
        dataUi = new Ui::dataBand();
        dataUi->setupUi(parent);

        //Reciever/demod thread emits and display thread handles
        connect(this,SIGNAL(BandData(char*,char*,char*)),this,SLOT(OutputBandData(char*,char*,char*)));

        outputOn = true;
    }
}

//We can get called with anything up to maxSamples, depending on earlier decimation steps
CPX * Demod::ProcessBlock(CPX * in, int bufSize)
{

    switch(mode) {
        case dmAM: // AM
            //SimpleAM(in,out, bufSize);
            demodAM->ProcessBlockFiltered(in,out, bufSize); //Sounds slightly better
            break;
        case dmSAM: // SAM
            demodSAM->ProcessBlock(in, out, bufSize);
            break;
        case dmFMN: // FMN
			//SimpleFM(in,out);
            SimpleFM2(in,out, bufSize); //5/12 Working well for NFM
            //PllFMN(in,out, bufSize); //6/8/12 Scaled output, now sounds better than SimpleFM2
            break;
        case dmFMM: // FMW
            FMMono(in,out, bufSize);
            break;
        case dmFMS:
            //Will only work if sample rate is at least 192k
            FMStereo(in,out,bufSize);
            break;

		//We've already filtered the undesired sideband @ BPFilter
		//So we don't need additional demod
		//
		case dmUSB:
            //SimpleUSB(in,out,bufSize); //Only if we still have both sidebands
			//break;
		
		case dmLSB:
            //SimpleLSB(in,out, bufSize); //Only if we still have both sidebands
			//break;

		//Passthrough, no demod
		default:
			return in;
            break;
    }
	return out;
}

void Demod::SimpleUSB(CPX *in, CPX *out, int _numSamples)
{
	float tmp;
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
	{
		tmp = in[i].re  + in[i].im;
		out[i].re = out[i].im = tmp;
	}
}
void Demod::SimpleLSB(CPX *in, CPX *out, int _numSamples)
{
	float tmp;
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
	{
		tmp = in[i].re  - in[i].im;
		out[i].re = out[i].im = tmp;
	}
}
void Demod::SimplePhase(CPX *in, CPX *out, int _numSamples)
{
	float tmp;
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
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

/*
  Another comment on FM from compDSP
    That is a pretty heavyweight method. Here is more efficient way:
    FM Output = I*dQ/dt - Q*dI/dt
    You may have to take care about the input amplitude limiting.
*/

void Demod::SimpleFM(CPX *in, CPX *out, int _numSamples)
{
	float tmp;
	float I,Q; //To make things more readable
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
	{
		I = in[i].re;
		Q = in[i].im;

		tmp = (Q * fmIPrev - I * fmQPrev) / (I * fmIPrev + Q * fmQPrev); //This seems to work better, less static, better audio
		//tmp = (Q * Ip - I * Qp) / (I * I + Q * Q);

		//Output volume is very low, scaling by even 100 doesn't do anything?
        out[i].re = out[i].im = tmp * .0005;
		fmIPrev = I;
		fmQPrev = Q;
	}

}
//From Doug Smith QEX article, Eq 21,  based on phase delta
//From Erio Blossom FM article in Linux Journal Sept 2004
void Demod::SimpleFM2(CPX *in, CPX *out, int _numSamples)
{
	CPX prod;
    int ns = _numSamples;

    //Based on phase delta between samples, so we always need last sample from previous run
    static CPX lastCpx(0,0);
    for (int i=0; i<ns; i++)
	{
		//The angle between to subsequent samples can be calculated by multiplying one by the complex conjugate of the other
		//and then calculating the phase (arg() or atan()) of the complex product
        prod = in[i] * lastCpx.conj();
		//Scale demod output to match am, usb, etc range
        out[i].re = out[i].im = prod.phase() *.0005;
        lastCpx = in[i];
	}
}

//dttsp PLL algorithm
CPX Demod::PLL(CPX sig, float loLimit, float hiLimit)
{
	CPX z;
    CPX pllSample;
    float difference;

	//Todo: See if we can use NCO here
	//This is the generated signal to sync with
    z.re = cos(pllPhase);
    z.im = sin(pllPhase);

    //Complex mult of incoming signal and NCO
    pllSample = z * sig;

	// For SAM we need the magnitude (hypot)
	// For FM we need the freq difference
	if (mode == dmSAM)
        difference = sig.mag() * pllSample.phase();
	else
        difference = pllSample.phase();

    
    pllFrequency += pllBeta * difference;

	//Keep the PLL within our limits
    if (pllFrequency < loLimit)
        pllFrequency = loLimit;
    if (pllFrequency > hiLimit)
        pllFrequency = hiLimit;

    pllPhase += pllFrequency + pllAlpha * difference;

	//Next reference signal
    while (pllPhase >= TWOPI)
        pllPhase -= TWOPI;
    while (pllPhase < 0)
        pllPhase += TWOPI;

    return pllSample;
}

/*
  FM Peak Deviation is max freq change above and below the carrier
  FM applications use peak deviations based on channel spacing
  75 kHz (200 kHz spacing) FM Broadcast leaves 25khz between channel on hi and lo side of carrier
  5 kHz (25 kHz spacing) NBFM
  2.25 kHz (12.5 kHz spacing) NBFM like FRS, Amateur, etc
  2 kHz (8.33 kHz spacing) NBFM
*/
//Uses Doug Smith phase delta equations
//PLL algorithm from dttsp
//See CuteSDR by Moe Wheatley for similar examples
/*
  I think this is the way this works
  PLL tries to follow the changing frequency of the input signal
  The amount of correction for each sample is the frequency change
*/
void Demod::PllFMN(  CPX * in, CPX * out, int _numSamples )
{
    int ns = _numSamples;

    //All these can be calculated once, not each call
    //time constant for DC removal filter
    const float fmDcAlpha = (1.0 - exp(-1.0 / (sampleRate * 0.001)) );
    const float fmPllZeta = .707;  //PLL Loop damping factor
    const float fmPllBW = fmBandwidth /2;// 3000.0;
    float norm = TWOPI/sampleRate;	//to normalize Hz to radians

    //Original alpha/beta calculations
    //pllAlpha = 0.3 * fmBandwidth * TWOPI/sampleRate;
    //pllBeta = pllAlpha * pllAlpha * 0.25;

    const float fmPllAlpha = 2.0 * fmPllZeta * fmPllBW * norm;
    const float fmPllBeta = (fmPllAlpha * fmPllAlpha) / (4.0 * fmPllZeta * fmPllZeta);

    const float fmPllRange = fmPllBW; //15000.0;	//maximum deviation limit of PLL
    const float fmPllLoLimit = -fmPllRange * norm; //PLL will not be allowed to exceed this range
    const float fmPllHiLimit = fmPllRange * norm;


    CPX pllNCO; //PLL NCO current value
    CPX delay;
    float phaseError;

    for (int i = 0; i < ns; i++) {
        //Todo: See if we can use NCO here
        //This is the generated signal to sync with (NCO)
        pllNCO.re = cos(pllPhase);
        pllNCO.im = sin(pllPhase);

        //Shift signal by PLL.  Should be same as CPX operator * ie pll * in[i]
        delay.re = pllNCO.re * in[i].re - pllNCO.im * in[i].im;
        delay.im = pllNCO.re * in[i].im + pllNCO.im * in[i].re;

        // same as -atan2(tmp.im, tmp.re), but with special handling in cpx class
        phaseError = -delay.phase();
        //phaseError = -atan2(delay.im,delay.re);

        //phaseError is the delta from last sample, ie demod value.  Rest is cleanup
        pllFrequency += fmPllBeta * phaseError / 100;  //Scale down to avoid overlaod

        //Keep the PLL within our limits
        if (pllFrequency < fmPllLoLimit)
            pllFrequency = fmPllLoLimit;
        if (pllFrequency > fmPllHiLimit)
            pllFrequency = fmPllHiLimit;

        //Next value for NCO
        pllPhase += pllFrequency + fmPllAlpha * phaseError;

        //LP filter the NCO frequency term to get DC offset value
        fmDCOffset = (1.0 - fmDcAlpha) * fmDCOffset + fmDcAlpha * pllFrequency;
        //Change in frequency - dc term is our demodulated output
        out[i].re = out[i].im = (pllFrequency - fmDCOffset);

    }
    //fmod will keep pllPhase <= TWOPI
    pllPhase = fmod(pllPhase, TWOPI);

}

//CuteSDR algorithm
void Demod::FMMono( CPX * in, CPX * out, int bufSize)
{
    //in buf has already been decimated to inputWfmSampleRate
    //out buf will be decimated to audio rate in receive chain
#if 1
    bufSize = demodWFM->ProcessDataMono(bufSize,in,out);
    return;
#else
    //Alternate methods works as of 5/18/13, but use wfmDemod
    //LP filter to elimate everything above FM bandwidth
    //Only if sample width high enough (2x) for 75khz filter to work
    if (inputWfmSampleRate >= 2*75000)
        fmMonoLPFilter.ProcessFilter(bufSize, in, in);

#if 1
    CPX d0;
    static CPX d1;

    for (int i=0; i<bufSize; i++)
    {
        //Condensed version of FM2 algorithm comparing sample with previous sample
        //Note: Have to divide by 100 to get signal in right range, different than CuteSDR
        d0 = in[i];
        out[i].re = out[i].im = atan2( (d1.re*d0.im - d0.re*d1.im), (d1.re*d0.re + d1.im*d0.im))/100;
        d1 = d0;
    }
#else
    SimpleFM2(in,out, bufSize);
#endif


    //19khz notch filter to get rid of pilot
    fmPilotNotchFilter.ProcessFilter(bufSize, out, out);	//notch out 19KHz pilot

    //15khz low pass filter to cut off audio >15khz
    fmAudioLPFilter.ProcessFilter(bufSize, out, out);


    //50 or 75uSec de-emphasis one pole filter
    FMDeemphasisFilter(bufSize, out,out);
#endif
}

/*
  FM Stereo notes
  FM stereo channels are L+R and L-R so mono receivers can just get the L+R channel
  Add the L-R to L+R to get left channel
  Sub the L-R from L+R to get right channel
  L+R channel is modulated directly 30-15khz baseband
  L-R channel is modulated on Double Sideband Supressed Carrier 38khz at 23khz to 53khz
  19khz pilot tone is used to get correct phase for 38khz supressed carrier

  FM Spectrum summary
  30hz - 15khz  L+R
  23khz - 38khz L-R lower sideband
  38khz         Dbl SB suppressed carrier
  38khz - 53khx L-R upper sideband
  57khz         RDBS
  58.65khz - 76.65khz   Directband
  92khz Audos subcarrier

*/
void Demod::FMStereo(CPX * in, CPX * out, int bufSize)
{
    bufSize = demodWFM->ProcessDataStereo(bufSize,in,out);

    //Do we have stereo lock
    int pilotLock =0;
    if (demodWFM->GetStereoLock(&pilotLock))
        rdsUpdate = true;

    //This only updates when something there's new data and its different than last, when do we reset display
    tRDS_GROUPS rdsGroups;
    if (demodWFM->GetNextRdsGroupData(&rdsGroups)) {
        //We have new data, is it valid
        if (rdsGroups.BlockA != 0) {
            rdsDecode.DecodeRdsGroup(&rdsGroups);
            //If changed since last call, update
            if (rdsDecode.GetRdsString(rdsString))
                rdsUpdate = true;
            if (rdsDecode.GetRdsCallString(rdsCallString))
                rdsUpdate = true;
        }
    }
    if (rdsUpdate) {
        emit BandData((char*) (pilotLock ? "Stereo" : ""), rdsCallString, rdsString);
    }
    return;

}

/*
  1 pole filter to delete high freq 'boost' (pre-emphasis) added before transmission
*/
void Demod::FMDeemphasisFilter(int _bufSize, CPX *in, CPX *out)
{
    int bufSize = _bufSize;

    static float avgRe = 0.0;
    static float avgIm = 0.0;

    for(int i=0; i<bufSize; i++)
    {
        avgRe = (1.0-fmDeemphasisAlpha)*avgRe + fmDeemphasisAlpha*in[i].re;
        avgIm = (1.0-fmDeemphasisAlpha)*avgIm + fmDeemphasisAlpha*in[i].im;
        out[i].re = avgRe*2.0;
        out[i].im = avgIm*2.0;
    }
}

//Pass filter changes to demodulators in case they need to stay in sync
void Demod::SetBandwidth(double bandwidth)
{
    switch (mode) {
        case dmAM:
            demodAM->SetBandwidth(bandwidth);
            break;
        default:
            break;
    }
}

void Demod::SetDemodMode(DEMODMODE _mode, int _sourceSampleRate, int _audioSampleRate)
{
    mode = _mode;
    inputSampleRate = _sourceSampleRate;
    audioSampleRate = _audioSampleRate;

	pllFrequency = 0.0;
    pllPhase = 0.0;
    switch (mode) {
        case dmSAM:
            break;
        case dmFMN:
            pllAlpha = 0.3 * fmBandwidth * TWOPI/sampleRate;
            pllBeta = pllAlpha * pllAlpha * 0.25;
            fmDCOffset = 0.0;
            break;
        case dmFMM:
        case dmFMS:
            //FM Stereo testing
            rdsDecode.DecodeReset(true);

            //Moe Wheatley filters
            //IIR filter freq * 2 must be below sampleRate or algorithm won't work
            fmMonoLPFilter.InitLP(75000,1.0,inputWfmSampleRate);
            //FIR version
            //fmMonoLPFilter.InitLPFilter(0, 1.0, 60.0, 75000, 1.4*75000.0, sourceSampleRate); //FIR version
            //Create narrow BP filter around 19KHz pilot tone with Q=500
            fmPilotBPFilter.InitBP(19000, 500, inputWfmSampleRate);
            //create LP filter to roll off audio
            fmAudioLPFilter.InitLPFilter(0, 1.0, 60.0, 15000.0, 1.4*15000.0, inputWfmSampleRate);
            //create 19KHz pilot notch filter with Q=5
            fmPilotNotchFilter.InitBR(19000, 5, inputWfmSampleRate);

            fmDeemphasisAlpha = (1.0-exp(-1.0/(inputWfmSampleRate * usDeemphasisTime)) );
            break;
        default:
            pllAlpha = 0.3 * 500.0 * TWOPI / sampleRate;
            pllBeta = pllAlpha * pllAlpha * 0.25;
	}

}

DEMODMODE Demod::DemodMode() const
{
    return mode;
}

//Reset RDS and any other decoders after frequency change
void Demod::ResetDemod()
{
    rdsDecode.DecodeReset(true);
    rdsCallString[0] = 0;
    rdsString[0] = 0;
    rdsBuf[0] = 0;
    rdsUpdate = true; //Update display next loop
}

DEMODMODE Demod::StringToMode(QString m)
{
	if (m == "AM") return dmAM;
	else if (m == "SAM") return dmSAM;
	else if (m == "LSB") return dmLSB;
	else if (m == "USB") return dmUSB;
	else if (m == "DSB") return dmDSB;
    else if (m == "FM-Mono") return dmFMM;
    else if (m == "FM-Stereo") return dmFMS;
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
    else if (dm == dmFMM) return "FM-Mono";
    else if (dm == dmFMS) return "FM-Stereo";
    else if (dm == dmFMN) return "FMN";
	else if (dm == dmCWL) return "CWL";
	else if (dm == dmCWU) return "CWU";
	else if (dm == dmDIGL) return "DIGL";
	else if (dm == dmDIGU) return "DIGU";
	else if (dm == dmNONE) return "NONE";
	else return "AM"; //default
}


void Demod::OutputBandData(char *status, char *callSign, char *callSignData)
{
    if (!outputOn || dataUi == NULL)
        return;
    dataUi->status->setText(status);
    dataUi->callSign->setText(callSign);
    //dataUi->callSignData->setText(callSignData);
    dataUi->dataEdit->setText(callSignData);
}
