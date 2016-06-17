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
	m_inputSampleRate = _sampleRate;
	m_inputWfmSampleRate = _wfmSampleRate;

	setDemodMode(DeviceInterface::dmAM, sampleRate, sampleRate);
	
    //We're no longer decimating to audio in wfmdemod, so audio rate is same as input rate
    //This fixed bug where FM filters were not working because rate was wrong

    //Moving to subclasses for each demod, transition with instance for each demod, change to vptr later
	m_demodAM = new Demod_AM(m_inputSampleRate, numSamples);
	m_demodSAM = new Demod_SAM(m_inputSampleRate, numSamples);
	m_demodNFM = new Demod_NFM(m_inputSampleRate, numSamples);
	m_demodWFM = new Demod_WFM(m_inputWfmSampleRate, numSamples);
	resetDemod();

	m_dataUi = NULL;
}

Demod::~Demod()
{
}

//Move to plugin
void Demod::setupDataUi(QWidget *parent)
{
    if (parent == NULL) {
		m_outputOn = false;

        //We want to delete
		if (m_dataUi != NULL) {
			delete m_dataUi;
        }
		m_dataUi = NULL;
        return;
	} else if (m_dataUi == NULL) {
        //Create new one
		m_dataUi = new Ui::dataBand();
		m_dataUi->setupUi(parent);

        //Reciever/demod thread emits and display thread handles
		connect(this,SIGNAL(bandData(QString,QString,QString,QString)),this,SLOT(outputBandData(QString,QString,QString,QString)));

		m_outputOn = true;
    }
}

//We can get called with anything up to maxSamples, depending on earlier decimation steps
CPX * Demod::processBlock(CPX * in, int bufSize)
{

	switch(m_mode) {
		case DeviceInterface::dmAM: // AM
            //SimpleAM(in,out, bufSize);
			m_demodAM->processBlockFiltered(in,out, bufSize); //Sounds slightly better
            break;
		case DeviceInterface::dmSAM: // SAM
			m_demodSAM->processBlock(in, out, bufSize);
            break;
		case DeviceInterface::dmFMN: // FMN
            //demodNFM->ProcessBlockFM1(in, out, bufSize);
            //demodNFM->ProcessBlockFM2(in, out, bufSize);
			m_demodNFM->processBlockNCO(in, out, bufSize);
            break;
		case DeviceInterface::dmFMM: // FMW
			fmMono(in,out, bufSize);
            break;
		case DeviceInterface::dmFMS:
            //Will only work if sample rate is at least 192k
			fmStereo(in,out,bufSize);
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

void Demod::simpleUSB(CPX *in, CPX *out, int _numSamples)
{
	float tmp;
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
	{
		tmp = in[i].real()  + in[i].imag();
		out[i].real(tmp);
		out[i].imag(tmp);
	}
}
void Demod::simpleLSB(CPX *in, CPX *out, int _numSamples)
{
	float tmp;
    int ns = _numSamples;

    for (int i=0;i<ns;i++)
	{
		tmp = in[i].real()  - in[i].imag();
		out[i].real(tmp);
		out[i].imag(tmp);
	}
}

//CuteSDR algorithm
void Demod::fmMono( CPX * in, CPX * out, int bufSize)
{
    //in buf has already been decimated to inputWfmSampleRate
    //out buf will be decimated to audio rate in receive chain
	bufSize = m_demodWFM->processDataMono(bufSize,in,out);
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
void Demod::fmStereo(CPX * in, CPX * out, int bufSize)
{
	bufSize = m_demodWFM->processDataStereo(bufSize,in,out);

	m_rdsUpdate = false; //We don't update unless something new or changed
    //Do we have stereo lock
    int pilotLock =0;
	if (m_demodWFM->getStereoLock(&pilotLock))
        //Stereo lock has changed and pilotLock has status
		m_rdsUpdate = true;

    //This only updates when something there's new data and its different than last, when do we reset display
    tRDS_GROUPS rdsGroups;
	if (m_demodWFM->getNextRdsGroupData(&rdsGroups)) {
        //We have new data, is it valid
        if (rdsGroups.BlockA != 0) {
			m_rdsDecode.decodeRdsGroup(&rdsGroups);
            //If changed since last call, update
			if (m_rdsDecode.getRdsString(m_rdsString))
				m_rdsUpdate = true;
			if (m_rdsDecode.getRdsCallString(m_rdsCallString))
				m_rdsUpdate = true;
        }
    }
    char *shortData = (char*)"";
	if (m_rdsUpdate) {
		emit bandData((char*) (pilotLock ? "Stereo" : ""), m_rdsCallString, shortData, m_rdsString);
    }
    return;

}


//Pass filter changes to demodulators in case they need to stay in sync
void Demod::setBandwidth(double bandwidth)
{
	switch (m_mode) {
		case DeviceInterface::dmAM:
			m_demodAM->setBandwidth(bandwidth);
            break;
        default:
            break;
    }
}

void Demod::setDemodMode(DeviceInterface::DemodMode _mode, int _sourceSampleRate, int _audioSampleRate)
{
	m_mode = _mode;
	m_inputSampleRate = _sourceSampleRate;
	m_audioSampleRate = _audioSampleRate;

	switch (m_mode) {
		case DeviceInterface::dmFMM:
		case DeviceInterface::dmFMS:
            //FM Stereo testing
			m_rdsDecode.decodeReset(true);
            break;
        default:
            break;
	}

}

DeviceInterface::DemodMode Demod::demodMode() const
{
	return m_mode;
}

//Reset RDS and any other decoders after frequency change
void Demod::resetDemod()
{
	m_rdsDecode.decodeReset(true);
	m_rdsCallString[0] = 0;
	m_rdsString[0] = 0;
	m_rdsBuf[0] = 0;
	m_rdsUpdate = true; //Update display next loop
}

DeviceInterface::DemodMode Demod::stringToMode(QString m)
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
QString Demod::modeToString(DeviceInterface::DemodMode dm)
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


void Demod::outputBandData(QString status, QString callSign, QString shortData, QString longData)
{
	if (!m_outputOn || m_dataUi == NULL)
        return;
	m_dataUi->status->setText(status);
	m_dataUi->callSign->setText(callSign);
	m_dataUi->callSignData->setText(shortData);
	m_dataUi->dataEdit->setText(longData);
}
