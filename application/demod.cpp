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

//Must be in same order as DeviceInterface::DEMODMODE
//Verified with CuteSDR values
//AM, SAM, FM are expressed in bandwidth, ie 16k am = -8000 to +8000 filter
//Rest are 0 relative

const Demod::DemodInfo Demod::demodInfo[] = {
	{DeviceInterface::dmAM, QStringList() << "20000" << "10000" << "5000", 0, -10000, 10000, 10000, 0, -120, 20},
	{DeviceInterface::dmSAM,QStringList() << "20000" << "10000" << "5000", 1, -10000, 10000, 10000, 0, -100, 200},
	{DeviceInterface::dmFMN,QStringList() << "30000" << "10000" << "7000", 0, -15000, 15000, 15000, 0, -100, 200},
	{DeviceInterface::dmFMM,QStringList(), 0, -100000, 100000, 100000, 0, -100, 200},
	{DeviceInterface::dmFMS,QStringList(), 0, -100000, 100000, 100000, 0, -100, 200},
	{DeviceInterface::dmDSB,QStringList() << "20000" << "10000" << "5000", 0, -10000, 10000, 10000, 0, -100, 200},
	{DeviceInterface::dmLSB,QStringList() << "10000" << "5000" << "2500" << "1500", 1, -20000, 0, 20000, 0, -100, 200},
	{DeviceInterface::dmUSB,QStringList() << "10000" << "5000" << "2500" << "1500", 1, 0, 20000, 20000, 0, -100, 200},
	{DeviceInterface::dmCWL,QStringList() << "1000" << "500" << "250" << "100" << "50", 1, -1000, 1000, 1000, 0, -100, 200}, //Check CW
	{DeviceInterface::dmCWU,QStringList() << "1000" << "500" << "250" << "100" << "50", 1, -1000, 1000, 1000, 0, -100, 200},
	{DeviceInterface::dmDIGL,QStringList() << "2000" << "1000" << "500" << "250" << "100",2,-20000, 0, 20000, 0, -100, 200},
	{DeviceInterface::dmDIGU,QStringList() << "2000" << "1000" << "500" << "250" << "100",2, 0, 20000, 20000, 0, -100, 200},
	{DeviceInterface::dmNONE,QStringList(), 0, 0, 0, 0, 0, -100, 200}

};

//New constructor as we move demods to sub classes
Demod::Demod(quint32 _sampleRate, quint32 _bufferSize) :
	ProcessStep(_sampleRate,_bufferSize)
{

}

//Two input rates, one normal and one for wfm
Demod::Demod(quint32 _sampleRate, quint32 _wfmSampleRate, quint32 _bufferSize) :
	ProcessStep(_sampleRate,_bufferSize)
{   
	inputSampleRate = _sampleRate;
	inputWfmSampleRate = _wfmSampleRate;

	SetDemodMode(DeviceInterface::dmAM, sampleRate, sampleRate);
	
    //We're no longer decimating to audio in wfmdemod, so audio rate is same as input rate
    //This fixed bug where FM filters were not working because rate was wrong

    //Moving to subclasses for each demod, transition with instance for each demod, change to vptr later
    demodAM = new Demod_AM(inputSampleRate, numSamples);
    demodSAM = new Demod_SAM(inputSampleRate, numSamples);
    demodNFM = new Demod_NFM(inputSampleRate, numSamples);
    demodWFM = new Demod_WFM(inputWfmSampleRate, numSamples);
    ResetDemod();

    dataUi = NULL;
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
        connect(this,SIGNAL(BandData(QString,QString,QString,QString)),this,SLOT(OutputBandData(QString,QString,QString,QString)));

        outputOn = true;
    }
}

//We can get called with anything up to maxSamples, depending on earlier decimation steps
CPX * Demod::ProcessBlock(CPX * in, int bufSize)
{

    switch(mode) {
		case DeviceInterface::dmAM: // AM
            //SimpleAM(in,out, bufSize);
            demodAM->ProcessBlockFiltered(in,out, bufSize); //Sounds slightly better
            break;
		case DeviceInterface::dmSAM: // SAM
            demodSAM->ProcessBlock(in, out, bufSize);
            break;
		case DeviceInterface::dmFMN: // FMN
            //demodNFM->ProcessBlockFM1(in, out, bufSize);
            //demodNFM->ProcessBlockFM2(in, out, bufSize);
            demodNFM->ProcessBlockNCO(in, out, bufSize);
            break;
		case DeviceInterface::dmFMM: // FMW
            FMMono(in,out, bufSize);
            break;
		case DeviceInterface::dmFMS:
            //Will only work if sample rate is at least 192k
            FMStereo(in,out,bufSize);
            break;

		//We've already filtered the undesired sideband @ BPFilter
		//So we don't need additional demod
		//
		case DeviceInterface::dmUSB:
            //SimpleUSB(in,out,bufSize); //Only if we still have both sidebands
			//break;
		
		case DeviceInterface::dmLSB:
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
    bufSize = demodWFM->ProcessDataMono(bufSize,in,out);
    return;
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

    rdsUpdate = false; //We don't update unless something new or changed
    //Do we have stereo lock
    int pilotLock =0;
    if (demodWFM->GetStereoLock(&pilotLock))
        //Stereo lock has changed and pilotLock has status
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
    char *shortData = (char*)"";
    if (rdsUpdate) {
        emit BandData((char*) (pilotLock ? "Stereo" : ""), rdsCallString, shortData, rdsString);
    }
    return;

}


//Pass filter changes to demodulators in case they need to stay in sync
void Demod::SetBandwidth(double bandwidth)
{
    switch (mode) {
		case DeviceInterface::dmAM:
            demodAM->SetBandwidth(bandwidth);
            break;
        default:
            break;
    }
}

void Demod::SetDemodMode(DeviceInterface::DemodMode _mode, int _sourceSampleRate, int _audioSampleRate)
{
    mode = _mode;
    inputSampleRate = _sourceSampleRate;
    audioSampleRate = _audioSampleRate;

    switch (mode) {
		case DeviceInterface::dmFMM:
		case DeviceInterface::dmFMS:
            //FM Stereo testing
            rdsDecode.DecodeReset(true);
            break;
        default:
            break;
	}

}

DeviceInterface::DemodMode Demod::DemodMode() const
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

DeviceInterface::DemodMode Demod::StringToMode(QString m)
{
	if (m == "AM") return DeviceInterface::dmAM;
	else if (m == "SAM") return DeviceInterface::dmSAM;
	else if (m == "LSB") return DeviceInterface::dmLSB;
	else if (m == "USB") return DeviceInterface::dmUSB;
	else if (m == "DSB") return DeviceInterface::dmDSB;
	else if (m == "FM-Mono") return DeviceInterface::dmFMM;
	else if (m == "FM-Stereo") return DeviceInterface::dmFMS;
	else if (m == "FMN") return DeviceInterface::dmFMN;
	else if (m == "CWL") return DeviceInterface::dmCWL;
	else if (m == "CWU") return DeviceInterface::dmCWU;
	else if (m == "DIGL") return DeviceInterface::dmDIGL;
	else if (m == "DIGU") return DeviceInterface::dmDIGU;
	else if (m == "NONE") return DeviceInterface::dmNONE;
	else return DeviceInterface::dmAM; //default
}
QString Demod::ModeToString(DeviceInterface::DemodMode dm)
{
	if (dm == DeviceInterface::dmAM) return "AM";
	else if (dm == DeviceInterface::dmSAM) return "SAM";
	else if (dm == DeviceInterface::dmLSB) return "LSB";
	else if (dm == DeviceInterface::dmUSB) return "USB";
	else if (dm == DeviceInterface::dmDSB) return "DSB";
	else if (dm == DeviceInterface::dmFMM) return "FM-Mono";
	else if (dm == DeviceInterface::dmFMS) return "FM-Stereo";
	else if (dm == DeviceInterface::dmFMN) return "FMN";
	else if (dm == DeviceInterface::dmCWL) return "CWL";
	else if (dm == DeviceInterface::dmCWU) return "CWU";
	else if (dm == DeviceInterface::dmDIGL) return "DIGL";
	else if (dm == DeviceInterface::dmDIGU) return "DIGU";
	else if (dm == DeviceInterface::dmNONE) return "NONE";
	else return "AM"; //default
}


void Demod::OutputBandData(QString status, QString callSign, QString shortData, QString longData)
{
    if (!outputOn || dataUi == NULL)
        return;
    dataUi->status->setText(status);
    dataUi->callSign->setText(callSign);
    dataUi->callSignData->setText(shortData);
    dataUi->dataEdit->setText(longData);
}
