//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

#if (SIMD)
#include <pmmintrin.h>
#endif

//Forward declaration of SSE utils
extern void SSEScaleCPX(CPX * c, CPX * a, double b, int size);
extern void SSEAddCPX(CPX * c, CPX * a, CPX * b, int size);
extern void SSEMultCPX(CPX * c, CPX * a, CPX * b, int size);
extern void SSEMagCPX(CPX * c, CPX * a, int size);
extern void SSESqMagCPX(CPX * c, CPX * a, int size);


double CPX::phase()
{
    double tmp;
	//Special handling to avoid divide by 0
	//Alternative is to set re = tiny amount and continue to use atan()
	tmp = atan(im / ((re==0) ? ALMOSTZERO : re));
    //double tmp1 = atan(im/re);

	//Correct for errors if re and im are negative
	if (re < 0 && im < 0)
		tmp -= M_PI; //Subtract 180deg
	else if (re < 0 && im >= 0)
		tmp += M_PI;

	return tmp;

}

//Allocates a block of 16byte aligned memory, optimal for FFT and SIMD
CPX *CPX::memalign(int _numCPX)
{
	void * buf;
	size_t align = 16;
	size_t msz = sizeof(CPX) * _numCPX;
	//Many FFT libraries require 16 byte alignment for best performance
	//Especially if they use SIMD (SSE) instructions
	posix_memalign(&buf, align, msz);
	return (CPX*)buf;
}





//CPXBuf
CPXBuf::CPXBuf(int _size)
{
    size = _size;
    int msz = sizeof(CPX) * size;
	cpxBuffer = CPXBuf::malloc(msz); //16byte aligned
}
CPXBuf::~CPXBuf()
{
    if (cpxBuffer != NULL) {
		free (cpxBuffer);
    }
}
void CPXBuf::Clear()
{
    memset(cpxBuffer,0,sizeof(CPX) * size);
}

CPX *CPXBuf::malloc(int size)
{
	void * buf;
	size_t align = 16;
	size_t msz = sizeof(CPX) * size;
	//Many FFT libraries require 16 byte alignment for best performance
	//Especially if they use SIMD (SSE) instructions
	posix_memalign(&buf, align, msz);
	return (CPX*)buf;
}
void CPXBuf::scale(CPX *out, CPX *in, double a, int size)
{
	if(SIMD)
		return SSEScaleCPX(out,in,a,size);

	for (int i=0; i<size; i++)
		out[i] = in[i].scale(a);
}
void CPXBuf::add(CPX * out, CPX * in, CPX *in2, int size)
{
	if (SIMD)
		return SSEAddCPX(out,in,in2,size);

	for (int i=0; i<size; i++)
		out[i] = in[i] + in2[i];
}
void CPXBuf::mult(CPX * out, CPX * in, CPX *in2, int size)
{
	if(SIMD)
		return SSEMultCPX(out,in,in2,size);

	for (int i=0; i<size; i++)
		out[i] = in[i] * in2[i];
}
void CPXBuf::mag(CPX *out, CPX *in, int size)
{
	if (SIMD)
		return SSEMagCPX(out, in, size);
	for (int i=0; i<size; i++){
		out[i].re = in[i].mag();
		out[i].im = in[i].re;
	}
}
void CPXBuf::sqrMag(CPX *out, CPX *in, int size)
{
	if (SIMD)
		return SSESqMagCPX(out, in, size);
	for (int i = 0; i < size; i++) {
		out[i].re = in[i].sqrMag();
		out[i].im = in[i].re; //Keet this for ref?
	}
}
//Copy every 'by' samples from in to out
//decimate(out,in,2,numSamples) //Copy 0,2,4... to in
//decimate(out,in,3,numSamples) //COpy 0,3,6 ...
void CPXBuf::decimate(CPX *out, CPX *in, int by, int size)
{
    for (int i = 0, j = 0; i < size; i+=by, j++)
    {
        out[j] = in[i];
    }  
}
//
double CPXBuf::normSqr(CPX *in, int size)
{
    double sum = 0.0;
	for (int i=0; i<size; i++)
		sum += in[i].sqrMag();
	return sum;
}
double CPXBuf::norm(CPX *in, int size)
{
    double sum = 0.0;
	for (int i=0; i<size; i++)
		sum += in[i].sqrMag();
	return sqrt(sum/size);
}
//Returns max mag() in buffer
double CPXBuf::peak(CPX *in, int size)
{
    double maxMag=0.0;
	for (int i=0; i<size; i++)
		maxMag = std::max(in[i].mag(),maxMag);
	return maxMag;
}
double CPXBuf::peakPower(CPX *in, int size)
{
    double maxPower = 0.0;
	for (int i=0; i<size; i++)
		maxPower = std::max(in[i].sqrMag(),maxPower);
    return maxPower;
}

void CPXBuf::Scale(CPX *out, double a)
{
    if(SIMD)
        return SSEScaleCPX(out,cpxBuffer,a,size);

    for (int i=0; i<size; i++)
        out[i] = cpxBuffer[i].scale(a);

}

void CPXBuf::Add(CPX *out, CPX *in2)
{
    if (SIMD)
        return SSEAddCPX(out,cpxBuffer,in2,size);

    for (int i=0; i<size; i++)
        out[i] = cpxBuffer[i] + in2[i];

}

void CPXBuf::Mult(CPX *out, CPX *in2)
{
    if(SIMD)
        return SSEMultCPX(out,cpxBuffer,in2,size);

    for (int i=0; i<size; i++)
        out[i] = cpxBuffer[i] * in2[i];

}

void CPXBuf::Mag(CPX *out)
{
    if (SIMD)
        return SSEMagCPX(out, cpxBuffer, size);
    for (int i=0; i<size; i++){
        out[i].re = cpxBuffer[i].mag();
        out[i].im = cpxBuffer[i].re;
    }

}

void CPXBuf::SqrMag(CPX *out)
{
    if (SIMD)
        return SSESqMagCPX(out, cpxBuffer, size);
    for (int i = 0; i < size; i++) {
        out[i].re = cpxBuffer[i].sqrMag();
        out[i].im = cpxBuffer[i].re; //Keet this for ref?
    }

}

void CPXBuf::Decimate(CPX *out, int by)
{
    for (int i = 0, j = 0; i < size; i+=by, j++)
    {
        out[j] = cpxBuffer[i];
    }

}

double CPXBuf::Norm()
{
    double sum = 0.0;
    for (int i=0; i<size; i++)
        sum += cpxBuffer[i].sqrMag();
    return sqrt(sum/size);

}

double CPXBuf::NormSqr()
{
    double sum = 0.0;
    for (int i=0; i<size; i++)
        sum += cpxBuffer[i].sqrMag();
    return sum;

}

double CPXBuf::Peak()
{
    double maxMag=0.0;
    for (int i=0; i<size; i++)
        maxMag = std::max(cpxBuffer[i].mag(),maxMag);
    return maxMag;

}

double CPXBuf::PeakPower()
{
    double maxPower = 0.0;
    for (int i=0; i<size; i++)
        maxPower = std::max(cpxBuffer[i].sqrMag(),maxPower);
	return maxPower;

}

CPXBuf &CPXBuf::operator +(CPXBuf a)
{
    for (int i=0; i<size; i++)
        cpxBuffer[i] = cpxBuffer[i] + a.Cpx(i);
    return *this;
}

#if (SIMD)
/*
SSE functions use Intel Streaming SIMD Extensions for performance
Search Intel site for compiler settings, usage, etc
*/
void DoSSEScaleCPX(CPX * c, CPX * a, double b) {
	__m128 x, y, z;

    x = _mm_load_ps((double *)a);
	y = _mm_load1_ps(&b);

	z = _mm_mul_ps(x, y);

    _mm_store_ps((double *)c, z);
}

void SSEScaleCPX(CPX * c, CPX * a, double b, int size)
{
	CPX * aa;
    double sb;
	CPX * cc;

	aa = a;
	sb = b;
	cc = c;

	for (int i = 0; i < size; i+=2) {
		DoSSEScaleCPX(cc, aa, sb);
		cc += 2;
		aa += 2;
	}
	if (size%2)
		*cc = (*aa).scale(sb);
}


void DoSSEAddCPX(CPX * c, CPX * a, CPX * b) {
	__m128 x, y, z;

    x = _mm_load_ps((double *)a);
    y = _mm_load_ps((double *)b);

	z = _mm_add_ps(x, y);

    _mm_store_ps((double *)c, z);
}

void SSEAddCPX(CPX * c, CPX * a, CPX * b, int size) 
{
	CPX * aa;
	CPX * bb;
	CPX * cc;

	aa = a;
	bb = b;
	cc = c;

	for (int i = 0; i < size; i+=2) {
		DoSSEAddCPX(cc, aa, bb);
		cc += 2;
		aa += 2;
		bb += 2;
	}
	if (size%2)
		*cc = *aa + *bb;
}

void DoSSEMultCPX(CPX * c, CPX * a, CPX * b) {

	__m128 x, y, yl, yh, t1, t2, z;

    x = _mm_load_ps((double *)a);
    y = _mm_load_ps((double *)b);

	yl = _mm_moveldup_ps(y);
	yh = _mm_movehdup_ps(y);

	t1 = _mm_mul_ps(x, yl);

	x = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2,3,0,1));

	t2 = _mm_mul_ps(x, yh);

	z = _mm_addsub_ps(t1, t2);

    _mm_store_ps((double *)c, z);
}

void SSEMultCPX(CPX * c, CPX * a, CPX * b, int size) 
{

	CPX * aa;
	CPX * bb;
	CPX * cc;

	aa = a;
	bb = b;
	cc = c;

	for (int i = 0; i < size; i+=2) {
		DoSSEMultCPX(cc, aa, bb);
		cc += 2;
		aa += 2;
		bb += 2;
	}
	if (size%2)
		*cc = *aa * *bb;
}

void DoSSEMagCPX(CPX * c, CPX * a) {
	__m128 x, t1, z;

    x = _mm_load_ps((double *)a);

	t1 = _mm_mul_ps(x, x);

	t1 = _mm_hadd_ps(t1, t1);

	t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(3,1,2,0));

	z = _mm_sqrt_ps(t1);

    _mm_store_ps((double *)c, z);
}

void SSEMagCPX(CPX * c, CPX * a, int size) {
	CPX * aa;
	CPX * cc;

	aa = a;
	cc = c;

	for (int i = 0; i < size; i+=2) {
		DoSSEMagCPX(cc, aa);
		cc += 2;
		aa += 2;
	}
	if (size%2) {
		cc->re = (*aa).sqrMag();
		cc->im = cc->re;
	}
}

void DoSSESqMagCPX(CPX * c, CPX * a) {
	__m128 x, t1;

    x = _mm_load_ps((double *)a);

	t1 = _mm_mul_ps(x, x);

	t1 = _mm_hadd_ps(t1, t1);

    _mm_store_ps((double *)c, t1);
}

void SSESqMagCPX(CPX * c, CPX * a, int size) {
	CPX * aa;
	CPX * cc;

	aa = a;
	cc = c;

	for (int i = 0; i < size; i+=2) {
		DoSSESqMagCPX(cc, aa);
		cc += 2;
		aa += 2;
	}
	if (size%2) {
		cc->re = (*aa).mag();
		cc->im = cc->re;
	}
}
#endif
