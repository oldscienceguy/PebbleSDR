#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <cstdlib> //NULL etc
#include <string.h>
#define _USE_MATH_DEFINES
#include "math.h"
#include <complex>

//Moved from defs.h
//Very small number that is used in place of zero in cases where we want to avoid divide by zero error
#define ALMOSTZERO 1e-200

//Use where ever possible
#define PI	3.14159265358979323846264338328
#define ONEPI	3.14159265358979323846264338328
#define TWOPI   6.28318530717958647692528676656
#define FOURPI 12.56637061435917295385057353312

//For imported code compatibility
#define TYPEREAL double
#define TYPECPX	CPX
#define TYPESTEREO16 CPX16
#define TYPEMONO16 qint16


//Used in several places where we need to decrement by one, modulo something
//Defined here because the bit mask logic may be confusing to some
//Equivalent to ((A - 1) % B)
#define DECMOD(A,B) ((A+B) & B)

//If SIMD is true, then all CPX arrays are byte aligned for use with fftw
//and SIMD platform routines are used wherever possible.
//Just use normal CPX functions, SIMD will then be automatic
#ifndef SIMD
#define SIMD 0 //Default to off, causing problems on some platforms
#endif

#include "pebblelib_global.h"


//HackRF format
class CPX8 {
public:
	qint8 real() {return re;}
	void real(qint8 r) {re = r;}
	qint8 imag() {return im;}
	void imag(qint8 i) {im = i;}
private:
	qint8 re;
	qint8 im;
};
class CPXU8 {
public:
	quint8 real() {return re;}
	void real(quint8 r) {re = r;}
	quint8 imag() {return im;}
	void imag(quint8 i) {im = i;}
private:
	quint8 re;
	quint8 im;
};
class CPX16 {
public:
	qint16 real() {return re;}
	void real(qint16 r) {re = r;}
	qint16 imag() {return im;}
	void imag(qint16 i) {im = i;}
private:
	qint16 re;
	qint16 im;
};
class CPXU16 {
public:
	quint16 real() {return re;}
	void real(quint16 r) {re = r;}
	quint16 imag() {return im;}
	void imag(quint16 i) {im = i;}
private:
	quint16 re;
	quint16 im;
};
class CPXFLOAT {
public:
	float real() {return re;}
	void real(float r) {re = r;}
	float imag() {return im;}
	void imag(float i) {im = i;}
private:
	float re;
	float im;
};


//C++ Alias syntax
using CPX = std::complex<double>;

#if 0
	Functions declared in std::complex
	abs(...) The absolute value of a complex number is its magnitude
	arg(...) Returns the phase angle (or angular component) of the complex number x, expressed in radians
	norm(...) The norm value of a complex number is its squared magnitude.  same as abs()^2
	conj(...) The conjugate of a complex number (real,imag) is (real,-imag)
	proj(...) Returns the projection of the complex number x onto the Riemann sphere
	polar(...) Returns a complex number defined by its polar components rho (mag) and theta(angle)

	// 26.3.8 transcendentals:
	acos(...);
	asin(...);
	atan(...);
	acosh(...);
	asinh(...);
	atanh(...);
	cos (...);
	cosh (...);
	exp (...);
	log (...);
	log10(...);
	pow(...);
	sin (...);
	sinh (...);
	sqrt (...);
	tan (...);
	tanh (...);

#endif

#if 0
	Regex syntax used for major change from CPX (.re & .im) to std::complex
	//Doesn't catch continued lines with no ';', check with .\re\s*= after for stragglers
	\.re\s*=\s*(.+)(;)  replace with .real(\1);
	\->re\s*=\s*(.+)(;)
	\.re\s*\+=\s*(.+)(;) manual replace with foo.real(foo.real() + \1)
	\->re\s*\+=\s*(.+)(;)
	\.re\s*\*=\s*(.+)(;)
	\->re\s*\*=\s*(.+)(;)
	\.re\s*\-=\s*(.+)(;)
	\->re\s*\-=\s*(.+)(;)
	\.re\s*\\=\s*(.+)(;)
	\->re\s*\\=\s*(.+)(;)
	//Do this last after all '='. Don't replace within cpx.h class
	\.re replace with .real()  WholeWord, CaseSens, RegExp
	or just look for remaining usage on .re in cpx.h.  Doesn't find all usage!!!

#endif

namespace CpxUtil {
	//Note: std::literals (only if _LIBCPP_STD_VER > 14) defines i, il operators as simply {0.0, arg}
	//	This allows the use of complex notation like '4i' or 'i(4)' or {0.0, 4}

	//Using 'j' as it appears in DSP math and matlab, move to CPX as constant eventually
	//std::complex<double> c_j = -1; // {-1, 0} {re,im}
	//Just doing sqrt(-1) will result in a complex number with Nan, which is invalid
	//c_j = sqrt(c_j); //Ensure we get sqrt(cpx) {6.123233995736766e-17,1}
	//square it and we get a value really close to original {-1, 0}
	//c_j = c_j * c_j; //{-1, 1.2246467991473532e-16}
	//Conclusion, just define c_j without doing all the math
	const CPX c_j = {6.123233995736766e-17,1};

	//adds in + in2 and returns in out
	void addCPX(CPX * out, CPX * in, CPX *in2, int size);
	void addCPX(CPX * out, const CPX * in, const CPX *in2, int size);
	//Allocates a block of 16byte aligned memory, optimal for FFT
	CPX *memalign(int _numCPX);
	//Just copies in to out
	void copyCPX(CPX *out, CPX *in, int size);
	void copyCPX(CPX *out, const CPX *in, int size);

	//Clears buffer to zeros, equiv to CPX(0,0)
	void clearCPX(CPX *out, int size);

	//scales in by a and returns in out
	void scaleCPX(CPX * out, CPX * in, double a, int size);
	void scaleCPX(CPX * out, const CPX * in, double a, int size);

	void multCPX(CPX * out, CPX * in, CPX *in2, int size);
	void multCPX(CPX * out, const CPX * in, const CPX *in2, int size);

	//out.real( mag, out.im = original out.re )
	void magCPX(CPX *out, CPX *in, int size);

	//out.real( sqrMag, out.im = original out.re)
	void sqrMagCPX(CPX *out, CPX *in, int size);

	//Copy every N samples from in to out
	void decimateCPX(CPX *out, const CPX *in, int by, int size);

	//Hypot version of norm
	double normCPX(CPX *in, int size);

	//Squared version of norm
	double normSqrCPX(CPX *in, int size);

	//Return max mag()
	double peakCPX(CPX *in, int size);
	double peakPowerCPX(CPX *in, int size);

	//CPX class methods moved
	inline void clearCpx(CPX &cpx) {cpx.real(0); cpx.imag(0);}

	//cpx1 = cpx2 * cpx3 (Convolution)
	//Commonly used in tight DSP loops.  Avoids overhead of operator* which creates a new CPX object
	inline void convolutionCpx(CPX &cpx, const CPX& cx1, const CPX& cx2) {
		cpx.real(((cx1.real() * cx2.real()) - (cx1.imag() * cx2.imag())));
		cpx.imag(((cx1.real() * cx2.imag()) + (cx1.imag() * cx2.real())));
	}

	inline void convolutionCpx(CPX &cpx, const CPX& cx1, const CPX& cx2, double gain) {
		cpx.real(((cx1.real() * cx2.real()) - (cx1.imag() * cx2.imag())) * gain);
		cpx.imag(((cx1.real() * cx2.imag()) + (cx1.imag() * cx2.real())) * gain);
	}

	//Same as '*'
	inline CPX scaleCpx(const CPX &cpx, double a) {
		return cpx * a;
	}

	//Alternative implementation from Pebble
	//Convert to Polar phase()
	//This is also the arg() function (see wikipedia)
	/*
	inline double argCpx(CPX &cpx) {
		return phaseCpx(cpx);
	}
	*/

	//Squared version of magnitudeMagnitude = re^2 + im^2
	inline double sqrMagCpx(CPX &cpx){
		return cpx.real() * cpx.real() + cpx.imag() * cpx.imag();
	}

	//In Polar notation, signals are represented by magnitude and phase (angle)
	//Complex and Polar notation are interchangable.  See Steve Smith pg 162
	//Convert to Polar mag()
	// n = |Z|
	inline double magCpx(CPX &cpx){
		return sqrt(sqrMagCpx(cpx));
	}


	double phaseCpx(CPX &cpx);

	//Convert Cartesion to Polar
	//WARNING: CPX IS USED TO KEEP POLAR, BUT VALUES ARE NOT REAL/IMAGINARY, THEY ARE MAG/PHASE
	//NAME VARS cpx... and pol... TO AVOID CONFUSION
	inline CPX cartToPolarCpx(CPX &cpx){
		return CPX(magCpx(cpx),phaseCpx(cpx));
	}

	//re has Mag, im has Phase
	inline CPX polarToCartCpx(CPX &cpx){
		return CPX(cpx.real() * cos(cpx.imag()), cpx.real() * sin(cpx.imag()));
	}


}

using namespace CpxUtil;


// MinGW doesn't have a aligned_malloc???

inline void * malloc16(int size)
{
    void *p;
    void **p1;

    if((p = malloc(size+31)) == NULL)
        return NULL;
    p1 = (void **)(((long)p + 31) & (~15));
    p1[-1] = p;
    return (void *)p1;
}



inline void free16(void *memory)
{
    if (memory != NULL)
        free(((void**)memory)[-1]);

}

//Aliases for cuteSDR compatiblity
#define TYPEREAL double
#define TYPECPX	CPX


#if 0
//MAC utility from fldigi
// MAC = sum += (a*b)
// This handles MAC with a delay, but relies on caller to track ptr. See DelayLine class for safe implementation
// a is a linear buffer [0 to len]
// b is a circular buffer [0 to len] with ptr indicating current position
inline 	CPX cpxMac (const CPX *a, const CPX *b, int ptr, int len) {
        CPX z;
        ptr %= len;
        for (int i = 0; i < len; i++) {
            z += a[i] * b[ptr];
            ptr = (ptr + 1) % len;
        }
        return z;
    }
inline double dMac(const double *a, const double *b, unsigned int size) {
    double sum = 0.0;
    double sum2 = 0.0;
    double sum3 = 0.0;
    double sum4 = 0.0;
    // Reduces read-after-write dependencies : Each subsum does not wait for the others.
    // The CPU can therefore schedule each line independently.
    for (; size > 3; size -= 4, a += 4, b+=4)
    {
        sum  += a[0] * b[0];
        sum2 += a[1] * b[1];
        sum3 += a[2] * b[2];
        sum4 += a[3] * b[3];
    }
    for (; size; --size)
        sum += (*a++) * (*b++);
    return sum + sum2 + sum3 + sum4 ;
}
#endif
