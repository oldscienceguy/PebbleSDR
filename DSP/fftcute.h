// fftcute.h: interface for the CFft class from cuteSDR source
//
//  This is a somewhat modified version of Takuya OOURA's
//     original radix 4 FFT package.
// A C++ wrapper around his code plus some specialized methods
// for displaying power vs frequency
//
//Copyright(C) 1996-1998 Takuya OOURA
//    (email: ooura@mmm.t.u-tokyo.ac.jp).
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef FFT_CUTE
#define FFT_CUTE

#include "fft.h"
#include "filters/datatypes.h"
#include <QMutex>

class CFft : public FFT
{
public:
	CFft();
	virtual ~CFft();
    void FFTParams(qint32 _size, bool _invert, double _dBCompensation, double _SampleFreq);
    void FFTForward(CPX * in, CPX * out, int size);
    void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
    void FFTInverse(CPX * in, CPX * out, int size);

	void ResetFFT();

private:
	void FreeMemory();
	void makewt(qint32 nw, qint32 *ip, double *w);
	void makect(qint32 nc, qint32 *ip, double *c);
	void bitrv2(qint32 n, qint32 *ip, double *a);
	void cftfsub(qint32 n, double *a, double *w);
	void rftfsub(qint32 n, double *a, qint32 nc, double *c);
	void CpxFFT(qint32 n, double *a, double *w);
	void cft1st(qint32 n, double *a, double *w);
	void cftmdl(qint32 n, qint32 l, double *a, double *w);
	void bitrv2conj(int n, int *ip, TYPEREAL *a);
	void cftbsub(int n, TYPEREAL *a, TYPEREAL *w);

	qint32 m_LastFFTSize;

	qint32* m_pWorkArea;

	double* m_pSinCosTbl;
	double* m_pWindowTbl;
#if 0
    //Replaced in FFT
    qint32 m_AveCount;
    qint32 m_TotalCount;
    double m_K_C;
    double m_K_B;
    double dBCompensation;
    double* m_pFFTPwrAveBuf;
	double* m_pFFTAveBuf;
	double* m_pFFTSumBuf;
	double* m_pFFTInBuf;
    //replaced with fftMutex in fft.h
    //QMutex m_Mutex;		//for keeping threads from stomping on each other
#endif

};

#endif // FFT_CUTE
