//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "demod.h"
#include <string.h>
#include "global.h"
#include "receiver.h"
#include "demod/demod_am.h"
#include "demod/demod_sam.h"
#include "demod/demod_wfm.h"
#include "demod/demod_nfm.h"


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
	
    //We're no longer decimating to audio in wfmdemod, so audio rate is same as input rate
    //This fixed bug where FM filters were not working because rate was wrong

    //Moving to subclasses for each demod, transition with instance for each demod, change to vptr later
    demodAM = new Demod_AM(sampleRate, numSamples);
    demodSAM = new Demod_SAM(sampleRate, numSamples);
    demodWFM = new Demod_WFM(inputWfmSampleRate,numSamples);
    demodNFM = new Demod_NFM(inputWfmSampleRate,numSamples);
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
            demodNFM->ProcessBlock(in, out, bufSize);
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

    switch (mode) {
        case dmSAM:
            break;
        case dmFMN:
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
            break;
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
