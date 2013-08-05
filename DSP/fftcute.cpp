// fft.cpp: implementation of the CFft class.
//  This is a somewhat modified version of Takuya OOURA's
//     original radix 4 FFT package.
//Copyright(C) 1996-1998 Takuya OOURA
//    (email: ooura@mmm.t.u-tokyo.ac.jp).
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#include <math.h>
#include "fftcute.h"
#include <QDebug>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CFft::CFft() :FFT()
{
    fftInputOverload = false;
    invert = false;
	m_LastFFTSize = 0;
    fftSize = 1024;
	m_pWorkArea = NULL;
	m_pSinCosTbl = NULL;
	m_pWindowTbl = NULL;

    FFTParams( 2048, false ,0.0, 1000);
    SetMovingAvgLimit( 1);
}

CFft::~CFft()
{							// free all resources
	FreeMemory();
}

void CFft::FreeMemory()
{
	if(m_pWorkArea)
	{
		delete m_pWorkArea;
		m_pWorkArea = NULL;
	}
	if(m_pSinCosTbl)
	{
		delete m_pSinCosTbl;
		m_pSinCosTbl = NULL;
	}
	if(m_pWindowTbl)
	{
		delete m_pWindowTbl;
		m_pWindowTbl = NULL;
	}
}

///////////////////////////////////////////////////////////////////
//FFT initialization and parameter setup function
///////////////////////////////////////////////////////////////////
void CFft::FFTParams( qint32 _size, bool _invert, double _dBCompensation, double _sampleRate)
{
    //Must call FFT base to properly init
    FFT::FFTParams(_size, _invert, _dBCompensation, _sampleRate);

    qint32 i;
    fftMutex.lock();

    if(m_LastFFTSize != fftSize)
	{
        m_LastFFTSize = fftSize;
		FreeMemory();
        m_pWindowTbl = new double[fftSize];
        m_pSinCosTbl = new double[fftSize/2];
        m_pWorkArea = new qint32[ (qint32)sqrt((double)fftSize)+2];
		m_pWorkArea[0] = 0;
        makewt(fftSize/2, m_pWorkArea, m_pSinCosTbl);

double WindowGain;
#if 0
		WindowGain = 1.0;
        for(i=0; i<fftSize; i++)	//Rectangle(no window)
			m_pWindowTbl[i] = 1.0*WindowGain;
#endif
#if 1
		WindowGain = 2.0;
        for(i=0; i<fftSize; i++)	//Hann
            m_pWindowTbl[i] = WindowGain*(.5  - .5 *cos( (K_2PI*i)/(fftSize-1) ));
#endif
#if 0
		WindowGain = 1.852;
        for(i=0; i<fftSize; i++)	//Hamming
            m_pWindowTbl[i] = WindowGain*(.54  - .46 *cos( (K_2PI*i)/(fftSize-1) ));
#endif
#if 0
		WindowGain = 2.8;
        for(i=0; i<fftSize; i++)	//Blackman-Nuttall
			m_pWindowTbl[i] = WindowGain*(0.3635819
                - 0.4891775*cos( (K_2PI*i)/(fftSize-1) )
                + 0.1365995*cos( (2.0*K_2PI*i)/(fftSize-1) )
                - 0.0106411*cos( (3.0*K_2PI*i)/(fftSize-1) ) );
#endif
#if 0
		WindowGain = 2.82;
        for(i=0; i<fftSize; i++)	//Blackman-Harris
			m_pWindowTbl[i] = WindowGain*(0.35875
                - 0.48829*cos( (K_2PI*i)/(fftSize-1) )
                + 0.14128*cos( (2.0*K_2PI*i)/(fftSize-1) )
                - 0.01168*cos( (3.0*K_2PI*i)/(fftSize-1) ) );
#endif
#if 0
		WindowGain = 2.8;
        for(i=0; i<fftSize; i++)	//Nuttall
			m_pWindowTbl[i] = WindowGain*(0.355768
                - 0.487396*cos( (K_2PI*i)/(fftSize-1) )
                + 0.144232*cos( (2.0*K_2PI*i)/(fftSize-1) )
                - 0.012604*cos( (3.0*K_2PI*i)/(fftSize-1) ) );
#endif
#if 0
		WindowGain = 1.0;
        for(i=0; i<fftSize; i++)	//Flat Top 4 term
			m_pWindowTbl[i] = WindowGain*(1.0
                    - 1.942604  * cos( (K_2PI*i)/(fftSize-1) )
                    + 1.340318 * cos( (2.0*K_2PI*i)/(fftSize-1) )
                    - 0.440811 * cos( (3.0*K_2PI*i)/(fftSize-1) )
                    + 0.043097  * cos( (4.0*K_2PI*i)/(fftSize-1) )
				);

#endif
	}
    fftMutex.unlock();
	ResetFFT();
}

///////////////////////////////////////////////////////////////////
//  Resets the FFT buffers and averaging variables.
///////////////////////////////////////////////////////////////////
void CFft::ResetFFT()
{
    fftMutex.lock();
    FFT::ResetFFT();
    fftMutex.unlock();
}

#if 0
//////////////////////////////////////////////////////////////////////
// "InBuf[]" is first multiplied by a window function, checked for overflow
//	and then placed in the FFT input buffers and the FFT performed.
//	For real data there should be  fftSize/2 InBuf data points
//	For complex data there should be  fftSize InBuf data points
//////////////////////////////////////////////////////////////////////

//!!Replaced by FFTForward here for reference
qint32 CFft::PutInDisplayFFT(qint32 n, TYPECPX* InBuf)
{
    qint32 i;
	m_Overload = false;
    fftMutex.lock();
	double dtmp1;
	for(i=0; i<n; i++)
	{
		if( InBuf[i].re > OVER_LIMIT )	//flag overload if within OVLimit of max
			m_Overload = true;
		dtmp1 = m_pWindowTbl[i];
		//NOTE: For some reason I and Q are swapped(demod I/Q does not apear to be swapped)
		//possibly an issue with the FFT ?
		((TYPECPX*)m_pFFTInBuf)[i].im =  dtmp1 * (InBuf[i].re);//window the I data
		((TYPECPX*)m_pFFTInBuf)[i].re = dtmp1 * (InBuf[i].im);	//window the Q data
	}
	//Calculate the complex FFT
    bitrv2(fftSize*2, m_pWorkArea + 2, m_pFFTInBuf);
    CpxFFT(fftSize*2, m_pFFTInBuf, m_pSinCosTbl);
    fftMutex.unlock();
	return m_TotalCount;
}
#endif

///////////////////////////////////////////////////////////////////
//Interface for doing fast convolution filters.  Takes complex data
// in pInOutBuf and does fwd or rev FFT and places back in same buffer.
///////////////////////////////////////////////////////////////////

//This should be called by Pebble, returns FP and unfolds
//CuteSDR should call CPXFFT
void CFft::FFTForward(CPX * in, CPX * out, int size)
{
    if (!fftParamsSet)
        return;

    qint32 i;
    fftInputOverload = false;
    fftMutex.lock();

    double dtmp1;
    for(i=0; i<size; i++)
    {
        if( in[i].re > overLimit )	//flag overload if within OVLimit of max
            fftInputOverload = true;
        dtmp1 = m_pWindowTbl[i];
        //CuteSDR swapped I/Q here, but order is correct in Pebble
        workingBuf[i].re = dtmp1 * (in[i].re); //window the I data
        workingBuf[i].im = dtmp1 * (in[i].im); //window the Q data
    }

    bitrv2(fftSize*2, m_pWorkArea + 2, (TYPEREAL*)workingBuf);
    CpxFFT(fftSize*2, (TYPEREAL*)workingBuf, m_pSinCosTbl);

    CPXBuf::copy(freqDomain,workingBuf,size) ;

    //If out == NULL, just leave result in freqDomain buffer and let caller get it
    if (out != NULL)
        CPXBuf::copy(out, freqDomain, fftSize);

#if 0
    //Test Pebble restuls with original cuteSDR calculations
    for (int i=0; i<size; i++) {
        if (FFTPwrSumBuf[i] != m_pFFTSumBuf[i])
            qDebug()<<"FFT Sum []"<<i<<","<<FFTPwrSumBuf[i]<<","<<m_pFFTSumBuf[i];
        if (FFTAvgBuf[i] != m_pFFTAveBuf[i])
            qDebug()<<"FFT Avg []"<<i<<","<<FFTAvgBuf[i]<<","<<m_pFFTAveBuf[i];
        if (FFTPwrAvgBuf[i] != m_pFFTPwrAveBuf[i])
            qDebug()<<"FFT Pwr Avg []"<<i<<","<<FFTPwrAvgBuf[i]<<","<<m_pFFTPwrAveBuf[i];

    }
#endif

    fftMutex.unlock();

}

void CFft::FFTMagnForward(CPX *in, int size, double baseline, double correction, double *fbr)
{
    // Not used, will be replaced by spectrum
}

void CFft::FFTSpectrum(CPX *in, int size)
{
    if (!fftParamsSet)
        return;
    FFTForward(in,workingBuf,size); //No need to copy to out, leave in freqDomain

    //We to unfold here because CalcPowerAverages expects things in most neg to most pos order
    //See fftooura for spectrum folding model, cuteSDR uses same
    // FFT output index N/2 to N-1 is frequency output -Fs/2 to 0hz (negative frequencies)
    for( int unfolded = 0, folded = size/2 ; folded < size; unfolded++, folded++) {
        freqDomain[unfolded] = workingBuf[folded]; //folded = 1024 to 2047 unfolded = 0 to 1023
    }
    // FFT output index 0 to N/2-1 is frequency output 0 to +Fs/2 Hz  (positive frequencies)
    for( int unfolded = size/2, folded = 0; unfolded < size; unfolded++, folded++) {
        freqDomain[unfolded] = workingBuf[folded]; //folded = 0 to 1023 unfolded = 1024 to 2047
    }

    CalcPowerAverages(freqDomain,size);
}

void CFft::FFTInverse(CPX * in, CPX * out, int size)
{
    if (!fftParamsSet)
        return;

    CPXBuf::copy(workingBuf,in, size);  //In-place functions, use workingBuf to keep other buffers intact

    bitrv2conj(fftSize*2, m_pWorkArea + 2, (TYPEREAL*)workingBuf);
    cftbsub(fftSize*2, (TYPEREAL*)workingBuf, m_pSinCosTbl);
    //in and out are same buffer so we need to copy to freqDomain buffer to be consistent
    CPXBuf::copy(timeDomain, workingBuf, fftSize);

    if (out != NULL)
        CPXBuf::copy(out, timeDomain, fftSize);

}


///////////////////////////////////////////////////////////////////
// Nitty gritty fft routines by Takuya OOURA(Updated to his new version 4-18-02)
// Routine calculates real FFT
///////////////////////////////////////////////////////////////////
void CFft::rftfsub(qint32 n, double *a, qint32 nc, double *c)
{
qint32 j, k, kk, ks, m;
double wkr, wki, xr, xi, yr, yi;

    m = n >> 1;
    ks = 2 * nc/m;
    kk = 0;
	for (j = 2; j < m; j += 2 ) 
	{
		k = n - j;
		kk += ks;
		wkr = 0.5 - c[nc - kk];
		wki = c[kk];
		xr = a[j] - a[k];
		xi = a[j + 1] + a[k + 1];
		yr = wkr * xr - wki * xi;
		yi = wkr * xi + wki * xr;
		a[j] -= yr;
		xi = a[j]*a[j];
		a[j+1] -= yi;
		xi += ( a[j+1]*a[j+1]);
		a[k] += yr;
		xr = a[k]*a[k];
		a[k+1] -= yi;
		xr += (a[k+1]*a[k+1]);

#if 0
		//xr is real power  xi is imag power terms
		//perform moving average on power up to m_AveSize then do exponential averaging after that
		if(m_TotalCount <= m_AveSize)
		{
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + xi;
			m_pFFTSumBuf[k] = m_pFFTSumBuf[k] + xr;
		}
		else
		{
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + xi;
			m_pFFTSumBuf[k] = m_pFFTSumBuf[k] - m_pFFTPwrAveBuf[k] + xr;
		}
		m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(double)m_AveCount;
		m_pFFTPwrAveBuf[k] = m_pFFTSumBuf[k]/(double)m_AveCount;

		m_pFFTAveBuf[j] = log10(m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
		m_pFFTAveBuf[k] = log10(m_pFFTPwrAveBuf[k] + m_K_C) + m_K_B;
#endif

	}

	a[0] *= a[0];						//calc DC term
	xr = a[m]*a[m]+a[m+1]*a[m+1];		//calculate N/4(middle) term

#if 0
	//xr is real power  a[0] is imag power terms
	//perform moving average on power up to m_AveSize then do exponential averaging after that
	if(m_TotalCount <= m_AveSize)
	{
		m_pFFTSumBuf[0] = m_pFFTSumBuf[0] + a[0];
		m_pFFTSumBuf[n/2] = m_pFFTSumBuf[n/2] + xr;
	}
	else
	{
		m_pFFTSumBuf[0] = m_pFFTSumBuf[0] - m_pFFTPwrAveBuf[0] + a[0];
		m_pFFTSumBuf[n/2] = m_pFFTSumBuf[n/2] - m_pFFTPwrAveBuf[n/2] + xr;
	}
	m_pFFTPwrAveBuf[0] = m_pFFTSumBuf[0]/(double)m_AveCount;
	m_pFFTPwrAveBuf[n/2] = m_pFFTSumBuf[n/2]/(double)m_AveCount;

	m_pFFTAveBuf[0] = log10(m_pFFTPwrAveBuf[0] + m_K_C) + m_K_B;
	m_pFFTAveBuf[n/2] = log10(m_pFFTPwrAveBuf[n/2] + m_K_C) + m_K_B;
#endif

}


///////////////////////////////////////////////////////////////////
// Routine calculates complex FFT
///////////////////////////////////////////////////////////////////
void CFft::CpxFFT(qint32 n, double *a, double *w)
{
qint32 j, j1, j2, j3, l;
double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }

#if 0
    //Do not remove, left for reference
    //Logic moved to FFT so it can be used with any FFT implementation

	//n = 2*FFTSIZE 
	n = n>>1;
	//now n = FFTSIZE

    //Slightly confusing
    // l is incrementing over CPX samples in a Real array[0-1096]
    // j is incrementing over Real power output array [0-2048]
	// FFT output index 0 to N/2-1
	// is frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term ) 
	for( l=0,j=n/2; j<n; l+=2,j++)
	{
		x0r = (a[l]*a[l]) + (a[l+1]*a[l+1]);
		//perform moving average on power up to m_AveSize then do exponential averaging after that
		if(m_TotalCount <= m_AveSize)
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + x0r;
		else
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + x0r;
		m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(double)m_AveCount;
		m_pFFTAveBuf[j] = log10( m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
	}
	// FFT output index N/2 to N-1  (times 2 since complex samples)
	// is frequency output -Fs/2 to 0  
	for( l=n,j=0; j<n/2; l+=2,j++)
	{
		x0r = (a[l]*a[l]) + (a[l+1]*a[l+1]);
		//perform moving average on power up to m_AveSize then do exponential averaging after that
		if(m_TotalCount <= m_AveSize)
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + x0r;
		else
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + x0r;
		m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(double)m_AveCount;
		m_pFFTAveBuf[j] = log10( m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
	}
#endif

}

///////////////////////////////////////////////////////////////////
/* -------- initializing routines -------- */
///////////////////////////////////////////////////////////////////
void CFft::makewt(qint32 nw, qint32 *ip, double *w)
{
qint32 j, nwh;
double delta, x, y;
    
    ip[0] = nw;
    ip[1] = 1;
    if (nw > 2) {
        nwh = nw >> 1;
        delta = atan(1.0) / nwh;
        w[0] = 1;
        w[1] = 0;
        w[nwh] = cos(delta * nwh);
        w[nwh + 1] = w[nwh];
        if (nwh > 2) {
            for (j = 2; j < nwh; j += 2) {
                x = cos(delta * j);
                y = sin(delta * j);
                w[j] = x;
                w[j + 1] = y;
                w[nw - j] = y;
                w[nw - j + 1] = x;
            }
            bitrv2(nw, ip + 2, w);
        }
    }
}

///////////////////////////////////////////////////////////////////
void CFft::makect(qint32 nc, qint32 *ip, double *c)
{
qint32 j, nch;
double delta;
    
	ip[1] = nc;
    if (nc > 1) {
        nch = nc >> 1;
        delta = atan(1.0) / nch;
        c[0] = cos(delta * nch);
        c[nch] = 0.5 * c[0];
        for (j = 1; j < nch; j++) {
            c[j] = 0.5 * cos(delta * j);
            c[nc - j] = 0.5 * sin(delta * j);
        }
    }
}

///////////////////////////////////////////////////////////////////
/* -------- child routines -------- */
///////////////////////////////////////////////////////////////////
void CFft::bitrv2(qint32 n, qint32 *ip, double *a)
{
qint32 j, j1, k, k1, l, m, m2;
double xr, xi, yr, yi;
    
    ip[0] = 0;
    l = n;
    m = 1;
    while ((m << 3) < l) {
        l >>= 1;
        for (j = 0; j < m; j++) {
            ip[m + j] = ip[j] + l;
        }
        m <<= 1;
    }
    m2 = 2 * m;
    if ((m << 3) == l) {
        for (k = 0; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 -= m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
            j1 = 2 * k + m2 + ip[k];
            k1 = j1 + m2;
            xr = a[j1];
            xi = a[j1 + 1];
            yr = a[k1];
            yi = a[k1 + 1];
            a[j1] = yr;
            a[j1 + 1] = yi;
            a[k1] = xr;
            a[k1 + 1] = xi;
        }
    } else {
        for (k = 1; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////
void CFft::cftfsub(qint32 n, double *a, double *w)
{
qint32 j, j1, j2, j3, l;
double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}

///////////////////////////////////////////////////////////////////
void CFft::cft1st(qint32 n, double *a, double *w)
{
qint32 j, k1, k2;
double wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    x0r = a[0] + a[2];
    x0i = a[1] + a[3];
    x1r = a[0] - a[2];
    x1i = a[1] - a[3];
    x2r = a[4] + a[6];
    x2i = a[5] + a[7];
    x3r = a[4] - a[6];
    x3i = a[5] - a[7];
    a[0] = x0r + x2r;
    a[1] = x0i + x2i;
    a[4] = x0r - x2r;
    a[5] = x0i - x2i;
    a[2] = x1r - x3i;
    a[3] = x1i + x3r;
    a[6] = x1r + x3i;
    a[7] = x1i - x3r;
    wk1r = w[2];
    x0r = a[8] + a[10];
    x0i = a[9] + a[11];
    x1r = a[8] - a[10];
    x1i = a[9] - a[11];
    x2r = a[12] + a[14];
    x2i = a[13] + a[15];
    x3r = a[12] - a[14];
    x3i = a[13] - a[15];
    a[8] = x0r + x2r;
    a[9] = x0i + x2i;
    a[12] = x2i - x0i;
    a[13] = x0r - x2r;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[10] = wk1r * (x0r - x0i);
    a[11] = wk1r * (x0r + x0i);
    x0r = x3i + x1r;
    x0i = x3r - x1i;
    a[14] = wk1r * (x0i - x0r);
    a[15] = wk1r * (x0i + x0r);
    k1 = 0;
    for (j = 16; j < n; j += 16) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        x0r = a[j] + a[j + 2];
        x0i = a[j + 1] + a[j + 3];
        x1r = a[j] - a[j + 2];
        x1i = a[j + 1] - a[j + 3];
        x2r = a[j + 4] + a[j + 6];
        x2i = a[j + 5] + a[j + 7];
        x3r = a[j + 4] - a[j + 6];
        x3i = a[j + 5] - a[j + 7];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 4] = wk2r * x0r - wk2i * x0i;
        a[j + 5] = wk2r * x0i + wk2i * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 2] = wk1r * x0r - wk1i * x0i;
        a[j + 3] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 6] = wk3r * x0r - wk3i * x0i;
        a[j + 7] = wk3r * x0i + wk3i * x0r;
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        x0r = a[j + 8] + a[j + 10];
        x0i = a[j + 9] + a[j + 11];
        x1r = a[j + 8] - a[j + 10];
        x1i = a[j + 9] - a[j + 11];
        x2r = a[j + 12] + a[j + 14];
        x2i = a[j + 13] + a[j + 15];
        x3r = a[j + 12] - a[j + 14];
        x3i = a[j + 13] - a[j + 15];
        a[j + 8] = x0r + x2r;
        a[j + 9] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 12] = -wk2i * x0r - wk2r * x0i;
        a[j + 13] = -wk2i * x0i + wk2r * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 10] = wk1r * x0r - wk1i * x0i;
        a[j + 11] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 14] = wk3r * x0r - wk3i * x0i;
        a[j + 15] = wk3r * x0i + wk3i * x0r;
    }
}

///////////////////////////////////////////////////////////////////
void CFft::cftmdl(qint32 n, qint32 l, double *a, double *w)
{
qint32 j, j1, j2, j3, k, k1, k2, m, m2;
double wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    m = l << 2;
    for (j = 0; j < l; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x0r - x2r;
        a[j2 + 1] = x0i - x2i;
        a[j1] = x1r - x3i;
        a[j1 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
    }
    wk1r = w[2];
    for (j = m; j < l + m; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x2i - x0i;
        a[j2 + 1] = x0r - x2r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j1] = wk1r * (x0r - x0i);
        a[j1 + 1] = wk1r * (x0r + x0i);
        x0r = x3i + x1r;
        x0i = x3r - x1i;
        a[j3] = wk1r * (x0i - x0r);
        a[j3 + 1] = wk1r * (x0i + x0r);
    }
    k1 = 0;
    m2 = 2 * m;
    for (k = m2; k < n; k += m2) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        for (j = k; j < l + k; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = wk2r * x0r - wk2i * x0i;
            a[j2 + 1] = wk2r * x0i + wk2i * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        for (j = k + m; j < l + (k + m); j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = -wk2i * x0r - wk2r * x0i;
            a[j2 + 1] = -wk2i * x0i + wk2r * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
    }
}

void CFft::bitrv2conj(int n, int *ip, TYPEREAL *a)
{
	int j, j1, k, k1, l, m, m2;
	TYPEREAL xr, xi, yr, yi;

	ip[0] = 0;
	l = n;
	m = 1;
	while ((m << 3) < l) {
		l >>= 1;
		for (j = 0; j < m; j++) {
			ip[m + j] = ip[j] + l;
		}
		m <<= 1;
	}
	m2 = 2 * m;
	if ((m << 3) == l) {
		for (k = 0; k < m; k++) {
			for (j = 0; j < k; j++) {
				j1 = 2 * j + ip[k];
				k1 = 2 * k + ip[j];
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 += 2 * m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 -= m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 += 2 * m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
			}
			k1 = 2 * k + ip[k];
			a[k1 + 1] = -a[k1 + 1];
			j1 = k1 + m2;
			k1 = j1 + m2;
			xr = a[j1];
			xi = -a[j1 + 1];
			yr = a[k1];
			yi = -a[k1 + 1];
			a[j1] = yr;
			a[j1 + 1] = yi;
			a[k1] = xr;
			a[k1 + 1] = xi;
			k1 += m2;
			a[k1 + 1] = -a[k1 + 1];
		}
	} else {
		a[1] = -a[1];
		a[m2 + 1] = -a[m2 + 1];
		for (k = 1; k < m; k++) {
			for (j = 0; j < k; j++) {
				j1 = 2 * j + ip[k];
				k1 = 2 * k + ip[j];
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 += m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
			}
			k1 = 2 * k + ip[k];
			a[k1 + 1] = -a[k1 + 1];
			a[k1 + m2 + 1] = -a[k1 + m2 + 1];
		}
	}
}

void CFft::cftbsub(int n, TYPEREAL *a, TYPEREAL *w)
{
	int j, j1, j2, j3, l;
	TYPEREAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

	l = 2;
	if (n > 8) {
		cft1st(n, a, w);
		l = 8;
		while ((l << 2) < n) {
			cftmdl(n, l, a, w);
			l <<= 2;
		}
	}
	if ((l << 2) == n) {
		for (j = 0; j < l; j += 2) {
			j1 = j + l;
			j2 = j1 + l;
			j3 = j2 + l;
			x0r = a[j] + a[j1];
			x0i = -a[j + 1] - a[j1 + 1];
			x1r = a[j] - a[j1];
			x1i = -a[j + 1] + a[j1 + 1];
			x2r = a[j2] + a[j3];
			x2i = a[j2 + 1] + a[j3 + 1];
			x3r = a[j2] - a[j3];
			x3i = a[j2 + 1] - a[j3 + 1];
			a[j] = x0r + x2r;
			a[j + 1] = x0i - x2i;
			a[j2] = x0r - x2r;
			a[j2 + 1] = x0i + x2i;
			a[j1] = x1r - x3i;
			a[j1 + 1] = x1i - x3r;
			a[j3] = x1r + x3i;
			a[j3 + 1] = x1i + x3r;
		}
	} else {
		for (j = 0; j < l; j += 2) {
			j1 = j + l;
			x0r = a[j] - a[j1];
			x0i = -a[j + 1] + a[j1 + 1];
			a[j] += a[j1];
			a[j + 1] = -a[j + 1] - a[j1 + 1];
			a[j1] = x0r;
			a[j1 + 1] = x0i;
		}
	}
}


