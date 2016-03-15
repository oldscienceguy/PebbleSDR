#include "windowfunction.h"

//Returns window[] with standard window coefficients
//From PowerSDR, SDRMax and DSP books in reference

/*
	From http://www.ap.com/kb/show/170
	Table 1 Window Scaling Factors
	Window						Noise Power Bandwidth aka Equivalent Noise Bandwidth
	Blackman Harris 3-term		1.73
	Blackman Harris 4-term		2.00
	Dolph-Chebychev 150dB		2.37
	Dolph-Chebychev 200dB		2.73
	Dolph-Chebychev 250dB		3.05
	Equiripple					2.63
	Flat-top					3.83
	Gaussian					2.21
	Hamming						1.36
	Hann						1.50
	None (rectangular)			1.00
	None, move to bin center	1.00
	Rife-Vincent 4-term			2.31
	Rife-Vincent 5-term			2.62

	From http://www.bores.com/courses/advanced/windows/files/windows.pdf
	Coherent power gain is the reduction in signal power caused by the window function
	Coherent power gain = sum (window coefficients) ^ 2 //Doesn't match tables

	From http://dsp-book.narod.ru/HFTSP/8579ch07.pdf which has full table and is a great window function ref
	Coherent power gain = 1/N * sum(window coefficients) //Gives same results as tables

*/
WindowFunction::WindowFunction(WINDOWTYPE _type, int _numSamples)
{
	windowType = _type;
	numSamples = _numSamples;
	window = new double[numSamples];
	windowCpx = CPX::memalign(numSamples);
	regenerate (_type);
}

WindowFunction::~WindowFunction()
{
	delete []window;
	free (windowCpx);
}

void WindowFunction::regenerate(WindowFunction::WINDOWTYPE _type)
{
	int i, j, midn, midp1, midm1;
	float freq, rate, sr1, angle, expn, expsum, cx, two_pi;
	midn = numSamples / 2;
	midp1 = (numSamples + 1) / 2;
	midm1 = (numSamples - 1) / 2;
	two_pi = TWOPI; //8.0 * atan(1.0);
	freq = two_pi / numSamples;
	rate = 1.0 /  midn;
	angle = 0.0;
	expn = log(2.0) / midn + 1.0;
	expsum = 1.0;

	double sumCoeff;

	switch (_type)
	{
		case RECTANGULAR: // RECTANGULAR_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 1.0;
			for (i = 0; i < numSamples; i++) {
				window[i] = 1.0;
				sumCoeff += window[i];
				windowCpx[i] = 1.0;
			}
			coherentGain = sumCoeff/numSamples; //SB 1
			break;
		case HANNING:	// HANNING_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 1.5; //For Hann, same as hanning?
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				window[j] = (window[i] = 0.5 - 0.5 * cos(angle));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples; //SB 0.50
			break;
		//http://www.recordingblogs.com/sa/tabid/88/Default.aspx?topic=Welch+window
		case WELCH: // WELCH_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 1.0; //Don't know
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				window[j] = (window[i] = 1.0 - (float)sqrt((float)((i - midm1) / midp1)));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples; //SB 0.67
			break;
		//http://www.recordingblogs.com/sa/tabid/88/Default.aspx?topic=Parzen+window
		case PARZEN: // PARZEN_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 1.0; //Don't know
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				window[j] = (window[i] = 1.0 - ((float)fabs((float)(i - midm1) / midp1)));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples; //SB 0.69
			break;
		//http://www.recordingblogs.com/sa/tabid/88/Default.aspx?topic=Coherent+gain
		case BARTLETT: // BARTLETT_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 1.0; //Don't know
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += rate) {
				window[j] = (window[i] = angle);
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples; //SB 0.50
			break;
		//Hamming Window: Raised cosine with a platform ("Digital Filters and their Design" pg 119)
		//Lynn forumula 5.22 shows 0.54 + 0.46, online reference show 0.54 - 0.46
		//Error in Lynn forumula, see Widipedia reference for all window functions
		case HAMMING: // HAMMING_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 1.36; //From http://www.ap.com/kb/show/170
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				window[j] = (window[i] = 0.54 - 0.46 * cos(angle));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples; //SB 0.54
			break;
		//Window function Blackman
		//RL: Used this algorithm effectively with DSPGUIDE code
		case BLACKMAN:
			sumCoeff = 0;
			for (i = 0; i<numSamples; i++) {
				window[i] = coherentGain - 0.5 * cos((TWOPI * i) / midm1) + 0.08 * cos((FOURPI * i) / midm1);
				sumCoeff += window[i];
				windowCpx[i] = window[i];
			}
			coherentGain = sumCoeff/numSamples; //SB 0.42
			break;
		case BLACKMAN2:	// BLACKMAN2_WINDOW
			sumCoeff = 0;
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				cx = cos(angle);
				window[j] = (window[i] = (.34401 + (cx * (-.49755 + (cx * .15844)))));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples;
			break;
		case BLACKMAN3: // BLACKMAN3_WINDOW
			sumCoeff = 0;
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				cx = cos(angle);
				window[j] = (window[i] = (.21747 + (cx * (-.45325 + (cx * (.28256 - (cx * .04672)))))));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples;
			break;
		case BLACKMAN4: // BLACKMAN4_WINDOW
			sumCoeff = 0;
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				cx = cos(angle);
				window[j] = (window[i] =
							(.084037 +
							(cx *
							(-.29145 +
							(cx *
							(.375696 + (cx * (-.20762 + (cx * .041194)))))))));
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples;
			break;
		case EXPONENTIAL: // EXPONENTIAL_WINDOW
			sumCoeff = 0;
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				window[j] = (window[i] = expsum - 1.0);
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
				expsum *= expn;
			}
			coherentGain = sumCoeff/numSamples;
			break;
		case RIEMANN: // RIEMANN_WINDOW
			sumCoeff = 0;
			sr1 = two_pi / numSamples;
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				if (i == midn) window[j] = (window[i] = 1.0);
				else {
					cx = sr1 * (midn - i);
					window[i] = sin(cx) / cx;
					window[j] = window[i];
				}
				sumCoeff += window[i];
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			coherentGain = sumCoeff/numSamples;
			break;
		//Min 4 sample BlackmanHarris (a0 .. a3)
		case BLACKMANHARRIS: // BLACKMANHARRIS_WINDOW
			sumCoeff = 0;
			equivalentNoiseBW = 2.00; //From table
			{
				float
						a0 = 0.35875F,
						a1 = 0.48829F,
						a2 = 0.14128F,
						a3 = 0.01168F;


				for (i = 0; i<numSamples;i++)
				{
					window[i] = a0 - a1* cos(two_pi*(i+0.5)/numSamples)
							+ a2* cos(2.0*two_pi*(i+0.5)/numSamples)
							- a3* cos(3.0*two_pi*(i+0.5)/numSamples);
					sumCoeff += window[i];
					windowCpx[i] = window[i];
				}
				coherentGain = sumCoeff/numSamples; //SB 0.36
			}
			break;
		case NONE: //Do nothing
			break;
		default:
			return;
	}

}
