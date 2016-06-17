#ifndef FLDIGIFILTERS_H
#define FLDIGIFILTERS_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

/* Several Digital Filter classes used in fldigi
    Original Copyright (C) 2006-2008 Dave Freese, W1HKJ
    These filters are based on the
    gmfsk design and the design notes given in
    "Digital Signal Processing", A Practical Guid for Engineers and Scientists
      by Steven W. Smith.

    Also referenced in JSDR

    Adapted for Pebble independent of other Pebble Filters for use in integrating fldigi decoders
    Eventually should merge all implementations into one best of breed class

    -Rename complex to CPX.  Fldigi complex.h is almost identical to Pebble CPX
*/

#include "cpx.h"
#include "pebblelib_global.h"


class PEBBLELIBSHARED_EXPORT FldigiFilters
{
public:
    FldigiFilters();
};

class C_FIR_filter {
#define FIRBufferLen 4096
public:
    C_FIR_filter ();
    ~C_FIR_filter ();
    void init (int len, int dec, double *ifil, double *qfil);
    //freq is the normalized freq.  So a 1khz lp filter at 8ksps = 0.125
    void init_lowpass (int len, int dec, double freq );
    void init_bandpass (int len, int dec, double freq1, double freq2);
    void init_hilbert (int len, int dec);
    double *bp_FIR(int len, int hilbert, double f1, double f2);
    void dump();
    int run (const CPX &in, CPX &out);
    int Irun (const double &in, double &out);
    int Qrun (const double &in, double &out);
private:
    int length;
    int decimateratio;

    double *ifilter;
    double *qfilter;

    double ffreq;

    double ibuffer[FIRBufferLen];
    double qbuffer[FIRBufferLen];

    int pointer;
    int counter;

    CPX fu;

    inline double sinc(double x) {
        if (fabs(x) < 1e-10)
            return 1.0;
        else
            return sin(M_PI * x) / (M_PI * x);
    }
    inline double cosc(double x) {
        if (fabs(x) < 1e-10)
            return 0.0;
        else
            return (1.0 - cos(M_PI * x)) / (M_PI * x);
    }
    inline double hamming(double x) {
        return 0.54 - 0.46 * cos(2 * M_PI * x);
    }
    inline double mac(const double *a, const double *b, unsigned int size) {
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

protected:

};

//=====================================================================
// Moving average filter
//=====================================================================

class Cmovavg {
#define MAXMOVAVG 2048
public:
    Cmovavg(int filtlen);
    ~Cmovavg();
    double run(double a);
    void setLength(int filtlen);
    void reset();
private:
    double	*in;
    double	out;
    int		len, pint;
    bool	empty;
};


//=====================================================================
// Sliding FFT
//=====================================================================

class sfft {
#define K1 0.99999999999
public:
    sfft(int len, int first, int last);
    ~sfft();
    void run(const CPX& input, CPX * __restrict__ result, int stride );
private:
    int fftlen;
    int first;
    int last;
    int ptr;
    struct vrot_bins_pair ;
    vrot_bins_pair * __restrict__ vrot_bins ;
    CPX * __restrict__ delay;
    double k2;
};

//=============================================================================
// Goertzel DFT
//=============================================================================

class goertzel {
public:
    goertzel(int n, double freq, double sr);
    ~goertzel();
    void reset();
    void reset(int n, double freq, double sr);
    bool run(double v);
    double real();
    double imag();
    double mag();
private:
    int N;
    int count;
    double Q0;
    double Q1;
    double Q2;
    double k1;
    double k2;
    double k3;
    bool isvalid;
};

#endif // FLDIGIFILTERS_H
