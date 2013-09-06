#ifndef FFTBASIC_H
#define FFTBASIC_H
#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference

/*
	Basic FFT function from RxDisp
	Adapted from FFT.C by Don Cross and Doug Craig
	Converted to QT by Rich Landsman
	12/18/10: Compiles but not tested or used anyplace yet, added to project for reference
	Todo: Convert to CPX and test
*/

#include "SignalProcessing.h"

class FFTBasic : public SignalProcessing
{
public:
	FFTBasic(int sr, int fc);
	void WindowData(float *, float *, int);
	float *m_fWinCoeff;
	void GetWindowCoeff();
	void FFT(float *, float *, float *, float *, unsigned int, unsigned int, bool);
	virtual ~FFTBasic();

private:
	unsigned ReverseBits(unsigned int, unsigned int);

};

#endif // FFTBASIC_H
