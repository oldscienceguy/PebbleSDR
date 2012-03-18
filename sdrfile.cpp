#include "sdrfile.h"

SDRFile::SDRFile(Receiver *_receiver,SDRDEVICE dev,Settings *_settings): SDR(_receiver, dev,_settings)
{
}
SDRFile::~SDRFile(void) {}

bool SDRFile::Connect() {return true;}
bool SDRFile::Disconnect() {return true;}
double SDRFile::SetFrequency(double fRequested,double fCurrent) {return 0;}
void SDRFile::ShowOptions() {return;}
void SDRFile::Start() {return;}
void SDRFile::Stop() {return;}
double SDRFile::GetStartupFrequency() {return 0;}
int SDRFile::GetStartupMode() {return 0;}
double SDRFile::GetHighLimit() {return 0;}
double SDRFile::GetLowLimit() {return 0;}
double SDRFile::GetGain() {return 0;}
QString SDRFile::GetDeviceName() {return "SDRFile";}
