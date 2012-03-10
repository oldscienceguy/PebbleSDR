//GPL license and attributions are in gpl.h and terms are included in this file by reference

#include "fftbasic.h"

FFTBasic::FFTBasic(int sr, int fc):SignalProcessing(sr, fc)
{
	m_fWinCoeff = new float(numSamples);
}

FFTBasic::~FFTBasic()
{

}

//Adapted from FFT.C by Don Cross
void FFTBasic::FFT(float *RealIn, float *ImagIn, float *RealOut, float *ImagOut, unsigned NumSamples, unsigned NumBits, bool InverseTransform)
{
	unsigned i, j, k, n;
	int BlockSize, BlockEnd;

	double angle_numerator = 2.0 * M_PI;
	double tr, ti;	// temp real, temp imaginary;

	if (InverseTransform) angle_numerator = -angle_numerator;

	// Do simultaneous data copy and bit-reversal ordering into outputs...

	for ( i=0; i < NumSamples; i++ )
	{
		j = ReverseBits ( i, NumBits );
		RealOut[j] = RealIn[i];
		ImagOut[j] = ImagIn[i];
	}

	// Do the FFT itself
	BlockEnd = 1;
	for ( BlockSize = 2; BlockSize <= (int)NumSamples; BlockSize <<= 1 )
	{
		double delta_angle = angle_numerator / (double)BlockSize;
		double sm2 = sin ( -2 * delta_angle );
		double sm1 = sin ( -delta_angle );
		double cm2 = cos ( -2 * delta_angle );
		double cm1 = cos ( -delta_angle );
		double w = 2 * cm1;
		double ar[3], ai[3];

		for ( i=0; i < NumSamples; i += BlockSize )
		{
			ar[2] = cm2;
			ar[1] = cm1;

			ai[2] = sm2;
			ai[1] = sm1;

			for ( j=i, n=0; n < (unsigned)BlockEnd; j++, n++ )
			{
				ar[0] = w*ar[1] - ar[2];
				ar[2] = ar[1];
				ar[1] = ar[0];

				ai[0] = w*ai[1] - ai[2];
				ai[2] = ai[1];
				ai[1] = ai[0];

				k = j + BlockEnd;
				tr = ar[0]*RealOut[k] - ai[0]*ImagOut[k];
				ti = ar[0]*ImagOut[k] + ai[0]*RealOut[k];

				RealOut[k] = (float)(RealOut[j] - tr);
				ImagOut[k] = (float)(ImagOut[j] - ti);

				RealOut[j] += (float)tr;
				ImagOut[j] += (float)ti;
			}
		}

		BlockEnd = BlockSize;
	}

	// Need to normalize if inverse transform...
	if (InverseTransform)
	{
		float denom = (float)NumSamples;

		for ( i=0; i < NumSamples; i++ )
		{
			RealOut[i] /= denom;
			ImagOut[i] /= denom;
		}
	}
}

unsigned FFTBasic::ReverseBits(unsigned int index, unsigned int NumBits)
{

	unsigned rev = 0;
	for (unsigned i=0; i < NumBits; i++ )
	{
		rev = (rev << 1) | (index & 1);
		index >>= 1;
	}

	return rev;
}

// Generate Blackman window coefficients
void FFTBasic::GetWindowCoeff()
{
	double K;

	K = 2.0*M_PI/(double)numSamples;

	for (int i = 0; i < numSamples; i++)
		m_fWinCoeff[i] = (float)(0.42 - (0.5*cos(K*(double)i)) + (0.08*cos(2*K*(double)i)));
}

void FFTBasic::WindowData(float *RealIn, float *ImagIn, int NumSamples)
{

	for (int i = 0; i <  NumSamples; i++)
		{
			RealIn[i] *= m_fWinCoeff[i];
			ImagIn[i] *= m_fWinCoeff[i];
		}
}
