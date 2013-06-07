#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <cstdlib> //NULL etc
#include <string.h>
#define _USE_MATH_DEFINES
#include "math.h"
#include "defs.h"



//If SIMD is true, then all CPX arrays are byte aligned for use with fftw
//and SIMD platform routines are used wherever possible.
//Just use normal CPX functions, SIMD will then be automatic
#ifndef SIMD
#define SIMD 1
#endif

//Inline simple methods for performance
class CPX
{
public:
	inline CPX(void)
		{re=0; im=0;}

    inline CPX(double r, double i)
		{re = r;im = i;}

	inline CPX(const CPX &cx)
		{re=cx.re; im=cx.im;}

    ~CPX(void){}

    double re, im;
	inline void clear() {re = im = 0;}

	inline CPX operator +(CPX a)
		{return CPX (re + a.re ,im + a.im);}

	inline CPX operator -(CPX a)
		{return CPX (re - a.re, im - a.im);}

	//Convolution
	CPX operator *(CPX y);
	//Same as scale
    inline CPX operator *(double a)
		{ return CPX(re * a, im * a);}

	CPX operator /(CPX y);
	bool operator ==(CPX a)
	{return re == a.re && im == a.im;}

	bool operator !=(CPX a)
	{return re != a.re || im != a.im;}

	//Scale re*a, im*a
    inline CPX scale(double a)
		{return CPX(re * a, im * a);}
	
	//Complex conjugate (see wikipedia).  im has opposite sign of original
	inline CPX conj()
	{return CPX(re,-im);}

	//In Polar notation, signals are represented by magnitude and phase (angle)
	//Complex and Polar notation are interchangable.  See Steve Smith pg 162
	//Convert to Polar mag()
    inline double mag()
	{return sqrt(re*re + im*im);}//hypot(re,im);}
	//Convert to Polar phase()
	//This is also the arg() function (see wikipedia)
    inline double arg()
	{return this->phase();}
    double phase();
	//Convert Cartesion to Polar
	//WARNING: CPX IS USED TO KEEP POLAR, BUT VALUES ARE NOT REAL/IMAGINARY, THEY ARE MAG/PHASE
	//NAME VARS cpx... and pol... TO AVOID CONFUSION
	inline CPX CartToPolar()
	{return CPX(mag(),phase());}
	//re has Mag, im has Phase
	inline CPX PolarToCart()
	{return CPX(re * cos(im), re * sin(im));}

	//Squared version of magnitudeMagnitude = re^2 + im^2
    inline double sqrMag()
		{return re*re + im*im;}
};

//Methods for operating on an array of CPX
class CPXBuf
{
public:
    CPXBuf(int _size);
    ~CPXBuf(void);
    void Clear();
    CPX * Ptr() {return cpxBuffer;}
    //Returning a reference allows these to be used as lvalues (assignment) or rvalues (data)
    inline CPX & Cpx(int i) {return cpxBuffer[i];}
    inline double & Re(int i) {return cpxBuffer[i].re;}
    inline double & Im(int i) {return cpxBuffer[i].im;}

    //Don't use array operator which won't work on CPXBuf *, use Cpx() instead
    //inline CPX & operator [](int i) {return cpxBuffer[i];}

    inline void Copy(CPX *out)
        {memcpy(out,cpxBuffer,sizeof(CPX) * size);}

    //scales in by a and returns in out, SIMD enabled
    void Scale(CPX * out, double a);

    //adds in + in2 and returns in out, SIMD enabled
    void Add(CPX * out, CPX *in2);
    inline CPXBuf &operator + (CPXBuf a);

    //SIMD enabled
    void Mult(CPX * out, CPX *in2);

    //out.re = mag, out.im = original out.re SIMD enabled
    void Mag(CPX *out);

    //out.re = sqrMag, out.im = original out.reSIMD enabled
    void SqrMag(CPX *out);

    //Copy every N samples from in to out
    void Decimate(CPX *out, int by);

    //Hypot version of norm
    double Norm();

    //Squared version of norm
    double NormSqr();

    //Return max mag()
    double Peak();
    double PeakPower();

	//Static for now
	//Returns 16byte aligned pointer if SIMD math enabled, you must use CPXBuf::free(...) to delete
	static CPX *malloc(int size);
	static void free(CPX *memory);

	//Just copies in to out
	static inline void copy(CPX *out, CPX *in, int size)
		{memcpy(out,in,sizeof(CPX) * size);}
	//Clears buffer to zeros, equiv to CPX(0,0)
	static inline void clear(CPX *out, int size)
		{memset(out,0,sizeof(CPX) * size); }
	//scales in by a and returns in out, SIMD enabled
    static void scale(CPX * out, CPX * in, double a, int size);
	//adds in + in2 and returns in out, SIMD enabled
	static void add(CPX * out, CPX * in, CPX *in2, int size);
	//SIMD enabled
	static void mult(CPX * out, CPX * in, CPX *in2, int size);
	//out.re = mag, out.im = original out.re SIMD enabled
	static void mag(CPX *out, CPX *in, int size);
	//out.re = sqrMag, out.im = original out.reSIMD enabled
	static void sqrMag(CPX *out, CPX *in, int size);
	//Copy every N samples from in to out
	static void decimate(CPX *out, CPX *in, int by, int size);
	//Hypot version of norm
    static double norm(CPX *in, int size);
	//Squared version of norm
    static double normSqr(CPX *in, int size);
	//Return max mag()
    static double peak(CPX *in, int size);
    static double peakPower(CPX *in, int size);

private:
    CPX *cpxBuffer;
    int size;


};

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
