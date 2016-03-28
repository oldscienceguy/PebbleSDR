#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <cstdlib> //NULL etc
#include <string.h>
#define _USE_MATH_DEFINES
#include "math.h"

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

//Ignore warnings about C++ 11 features
#pragma clang diagnostic ignored "-Wc++11-extensions"
//Ignore warnings about OS X version unsupported (QT 5.1 bug)
#pragma clang diagnostic ignored "-W#warnings"

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
	qint8 re;
	qint8 im;
};
class CPXU8 {
public:
	quint8 re;
	quint8 im;
};
class CPX16 {
public:
	qint16 re;
	qint16 im;
};
class CPXU16 {
public:
	quint16 re;
	quint16 im;
};
class CPXFLOAT {
public:
	float re;
	float im;
};

//Inline simple methods for performance
class PEBBLELIBSHARED_EXPORT CPX
{
public:

    inline CPX(double r=0.0, double i=0.0) : re(r), im(i) { }

	inline CPX(const CPX &cx) : re(cx.re), im(cx.im) {}

	inline CPX(double r) : re(r), im(0){}

    ~CPX(void){}

	double re, im;

	//Static methods that work on CPX* buffers, was in CPXBuf
	//Allocates a block of 16byte aligned memory, optimal for FFT and SIMD
	static CPX *memalign(int _numCPX);
	//Just copies in to out
	static void copyCPX(CPX *out, CPX *in, int size);
	static void copyCPX(CPX *out, const CPX *in, int size);

	//Clears buffer to zeros, equiv to CPX(0,0)
	static void clearCPX(CPX *out, int size);

	//scales in by a and returns in out, SIMD enabled
	static void scaleCPX(CPX * out, CPX * in, double a, int size);
	static void scaleCPX(CPX * out, const CPX * in, double a, int size);

	static void multCPX(CPX * out, CPX * in, CPX *in2, int size);
	static void multCPX(CPX * out, const CPX * in, const CPX *in2, int size);

	//adds in + in2 and returns in out, SIMD enabled
	static void addCPX(CPX * out, CPX * in, CPX *in2, int size);
	static void addCPX(CPX * out, const CPX * in, const CPX *in2, int size);

	//out.re = mag, out.im = original out.re SIMD enabled
	static void magCPX(CPX *out, const CPX *in, int size);

	//out.re = sqrMag, out.im = original out.reSIMD enabled
	static void sqrMagCPX(CPX *out, const CPX *in, int size);

	//Copy every N samples from in to out
	static void decimateCPX(CPX *out, const CPX *in, int by, int size);

	//Hypot version of norm
	static double normCPX(const CPX *in, int size);

	//Squared version of norm
	static double normSqrCPX(const CPX *in, int size);

	//Return max mag()
	static double peakCPX(const CPX *in, int size);
	static double peakPowerCPX(const CPX *in, int size);

	inline double real() const { return re; }
	inline void real(double R) {re=R;}
	inline double imag() const { return im; }
	inline void imag(double I) {im=I;}

    //For cases where we need consistent I/Q representation using complex
	inline double i() const {return re;} //In phase is typically in real
	inline double q() const {return im;} //Quad phase is typically in imag

	inline void clear() {re=0.0; im=0.0;}

    //Every operator should have ref (&) and normal version
	// Assignment
	inline CPX& operator= (const double r) {
		re = r;
		im = 0.;
		return *this;
	}

    //cpx1 = cpx2 + cpx3
	inline CPX operator +(const CPX a) const {
        return CPX (re + a.re ,im + a.im);}

    inline CPX& operator+=(const CPX& a) {
        re += a.re;
        im += a.im;
        return *this;
        }

    //cpx1 = cpx2 - cpx3
	inline CPX operator -(const CPX &a) const {
        return CPX (re - a.re, im - a.im);}

    inline CPX& operator-=(const CPX& a) {
        re -= a.re;
        im -= a.im;
        return *this;
    }

    //cpx1 = cpx2 * cpx3 (Convolution)
	//Commonly used in tight DSP loops.  Avoids overhead of operator* which creates a new CPX object
	inline void convolution(const CPX& cx1, const CPX& cx2) {
		re = ((cx1.re * cx2.re) - (cx1.im * cx2.im));
		im = ((cx1.re * cx2.im) + (cx1.im * cx2.re));
	}
	inline void convolution(const CPX& cx1, const CPX& cx2, double gain) {
		re = ((cx1.re * cx2.re) - (cx1.im * cx2.im)) * gain;
		im = ((cx1.re * cx2.im) + (cx1.im * cx2.re)) * gain;
	}

	inline CPX& operator*= (const CPX& a) {
        double temp = re * a.re - im * a.im;
        im = re * a.im + im * a.re;
        re = temp;
        return *this;
    }

	inline CPX operator* (const CPX& a) const {
        return CPX(re * a.re - im * a.im,  re * a.im + im * a.re);
    }

    //cpx1 = cpx2 * double (Same as scale)
	inline CPX operator* (double a) const {
        return CPX(re * a, im * a);
    }

    inline CPX& operator*=(double a) {
        re *= a;
        im *= a;
        return *this;
    }

    //Same as '*'
	inline CPX scale (double a) const {
        return *this * a;
    }


    //cpx1 = cpx2 / cpx3
    inline CPX& operator/=(const CPX& y) {
        double temp, denom = y.re*y.re + y.im*y.im;
        if (denom == 0.0) denom = 1e-10;
        temp = (re * y.re + im * y.im) / denom;
        im = (im * y.re - re * y.im) / denom;
        re = temp;
        return *this;
    }

    inline CPX operator/(const CPX& y) const {
        double denom = y.re*y.re + y.im*y.im;
        if (denom == 0.0) denom = 1e-10;
        return CPX((re * y.re + im * y.im) / denom,  (im * y.re - re * y.im) / denom);
    }

    //fldigi uses
    // Z = (complex conjugate of X) * Y
    // Z1 = x1 + jy1, or Z1 = |Z1|exp(jP1)
    // Z2 = x2 + jy2, or Z2 = |Z2|exp(jP2)
    // Z = (x1 - jy1) * (x2 + jy2)
    // or Z = |Z1|*|Z2| exp (j (P2 - P1))
    inline CPX& operator%=(const CPX& y) {
        double temp = re * y.re + im * y.im;
        im = re * y.im - im * y.re;
        re = temp;
        return *this;
    }

	inline CPX operator%(const CPX& y) const {
        CPX z;
        z.re = re * y.re + im * y.im;
        z.im = re * y.im - im * y.re;
        return z;
    }

    // n = |Z| * |Z|
    inline double norm() const {
        return (re * re + im * im);
    }

    //Squared version of magnitudeMagnitude = re^2 + im^2
	inline double sqrMag() const {
        return re * re + im * im;
    }

    // Z = x + jy
    // Z = |Z|exp(jP)
    // arg returns P
    inline double arg() const {
        return atan2(im, re);
    }

    //Alternative implementation from Pebble
    //Convert to Polar phase()
    //This is also the arg() function (see wikipedia)
    /*
    inline double arg() {
        return this->phase();
    }
    */


	inline bool operator ==(CPX a) const {
		return re == a.re && im == a.im;
	}

	inline bool operator !=(CPX a) const {
		return re != a.re || im != a.im;
	}
	
	//Complex conjugate (see wikipedia).  im has opposite sign of original
	inline CPX conj() const {
        return CPX(re,-im);
    }

	//In Polar notation, signals are represented by magnitude and phase (angle)
	//Complex and Polar notation are interchangable.  See Steve Smith pg 162
	//Convert to Polar mag()
    // n = |Z|
    inline double mag() const {
		return sqrt(sqrMag());
    }

	//Used in GNURadio and some texts
	inline double abs() const {
		return sqrt(re*re + im*im);
	}

	double phase() const;

	//Convert Cartesion to Polar
	//WARNING: CPX IS USED TO KEEP POLAR, BUT VALUES ARE NOT REAL/IMAGINARY, THEY ARE MAG/PHASE
	//NAME VARS cpx... and pol... TO AVOID CONFUSION
	inline CPX CartToPolar() const {
		return CPX(mag(),phase());
	}

	//re has Mag, im has Phase
	inline CPX PolarToCart() const {
		return CPX(re * cos(im), re * sin(im));
	}

};

//Tells Qt it can use memmove on this type instead of repeated copy constructors
Q_DECLARE_TYPEINFO(CPX, Q_MOVABLE_TYPE);

#if 0
//Specializations of QT container classes
//Problem with QVector is that we can't replace its allocator with a 16byte aligned one
//See std::vector for alternative
//Could add additional methods if useful
class CPXVector : public QVector<CPX>
{
public:
	CPXVector(int _size) : QVector(_size) {}
};
#endif
#if 0
//QCircularBuffer could not be resolved in QT5.5. #include <Qt3D> not found even with Qt+=3dcore
class CPXCircularBuffer : public QCircularBuffer<CPX>
{
	CPXCircularBuffer(int _size) : QCircularBuffer(_size) {}
};
#endif

#if 0
//Never used as intended and deprecated, use CPX:: static methods for array operations instead
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

    //Conversion operators
    //Explicit converter allows CPXBuf to be use anywhere we need a CPX*
    explicit operator const CPX* () const {return cpxBuffer;}

    //Array operators don't work on pointers, ie CPXBuf *, use Cpx() instead
    inline CPX & operator [](int i) {return cpxBuffer[i];}

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

private:
    CPX *cpxBuffer;
    int size;


};
#endif

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
