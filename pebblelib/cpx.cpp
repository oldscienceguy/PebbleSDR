//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

double CpxUtil::phaseCpx(CPX &cpx)
{
    double tmp;
	//Special handling to avoid divide by 0
	//Alternative is to set re = tiny amount and continue to use atan()
	tmp = atan(cpx.imag() / ((cpx.real()==0) ? ALMOSTZERO : cpx.real()));
    //double tmp1 = atan(im/re);

	//Correct for errors if re and im are negative
	if (cpx.real() < 0 && cpx.imag() < 0)
		tmp -= M_PI; //Subtract 180deg
	else if (cpx.real() < 0 && cpx.imag() >= 0)
		tmp += M_PI;

	return tmp;

}

//Allocates a block of 16byte aligned memory, optimal for FFT and SIMD
CPX *CpxUtil::memalign(int _numCPX)
{
	void * buf;
	size_t align = 16;
	size_t msz = sizeof(CPX) * _numCPX;
	//Many FFT libraries require 16 byte alignment for best performance
	//Especially if they use SIMD (SSE) instructions
	posix_memalign(&buf, align, msz);
	return (CPX*)buf;
}

//Todo: Remove non constant versions
void CpxUtil::copyCPX(CPX *out, CPX *in, int size)
{
	memcpy(out,in,sizeof(CPX) * size);
}
void CpxUtil::copyCPX(CPX *out, const CPX *in, int size)
{
	memcpy(out,in,sizeof(CPX) * size);
}

void CpxUtil::multCPX(CPX * out, const CPX * in, const CPX *in2, int size)
{
	for (int i=0; i<size; i++)
		out[i] = in[i] * in2[i];
}

void CpxUtil::multCPX(CPX * out, CPX * in, CPX *in2, int size)
{
	for (int i=0; i<size; i++)
		out[i] = in[i] * in2[i];
}

void CpxUtil::scaleCPX(CPX *out, const CPX *in, double a, int size)
{
	for (int i=0; i<size; i++)
		out[i] = scaleCpx(in[i],a);
}

void CpxUtil::scaleCPX(CPX *out, CPX *in, double a, int size)
{
	for (int i=0; i<size; i++)
		out[i] = scaleCpx(in[i], a);
}

void CpxUtil::addCPX(CPX * out, const CPX * in, const CPX *in2, int size)
{
	for (int i=0; i<size; i++)
		out[i] = in[i] + in2[i];
}

void CpxUtil::addCPX(CPX * out, CPX * in, CPX *in2, int size)
{
	for (int i=0; i<size; i++)
		out[i] = in[i] + in2[i];
}

void CpxUtil::magCPX(CPX *out, CPX *in, int size)
{
	for (int i=0; i<size; i++){
		out[i].real(magCpx(in[i]));
		out[i].imag(in[i].real());
	}
}

void CpxUtil::sqrMagCPX(CPX *out, CPX *in, int size)
{
	for (int i = 0; i < size; i++) {
		out[i].real(sqrMagCpx(in[i]));
		out[i].imag(in[i].real()); //Keet this for ref?
	}
}

void CpxUtil::clearCPX(CPX *out, int size)
{
	memset(out,0,sizeof(CPX) * size);
}


//Copy every 'by' samples from in to out
//decimate(out,in,2,numSamples) //Copy 0,2,4... to in
//decimate(out,in,3,numSamples) //COpy 0,3,6 ...
void CpxUtil::decimateCPX(CPX *out, const CPX *in, int by, int size)
{
    for (int i = 0, j = 0; i < size; i+=by, j++)
    {
        out[j] = in[i];
    }  
}
//
double CpxUtil::normSqrCPX(CPX *in, int size)
{
    double sum = 0.0;
	for (int i=0; i<size; i++)
		sum += sqrMagCpx(in[i]);
	return sum;
}

double CpxUtil::normCPX(CPX *in, int size)
{
    double sum = 0.0;
	for (int i=0; i<size; i++)
		sum += sqrMagCpx(in[i]);
	return sqrt(sum/size);
}

//Returns max mag() in buffer
double CpxUtil::peakCPX(CPX *in, int size)
{
	double maxMag=0.0;
	for (int i=0; i<size; i++)
		maxMag = std::max(magCpx(in[i]),maxMag);
	return maxMag;
}
double CpxUtil::peakPowerCPX(CPX *in, int size)
{
	double maxPower = 0.0;
	for (int i=0; i<size; i++)
		maxPower = std::max(sqrMagCpx(in[i]),maxPower);
	return maxPower;
}
