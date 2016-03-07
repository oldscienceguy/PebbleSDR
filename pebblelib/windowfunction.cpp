#include "windowfunction.h"

//Returns window[] with standard window coefficients
//From PowerSDR, SDRMax and DSP books in reference

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

	switch (_type)
	{
		case RECTANGULAR: // RECTANGULAR_WINDOW
			coherentGain = 1.0;
			for (i = 0; i < numSamples; i++) {
				window[i] = 1.0;
				windowCpx[i] = 1.0;
			}
			break;
		case HANNING:	// HANNING_WINDOW
			coherentGain = 0.50;
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				window[j] = (window[i] = 0.5 - 0.5 * cos(angle));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		//http://www.recordingblogs.com/sa/tabid/88/Default.aspx?topic=Welch+window
		case WELCH: // WELCH_WINDOW
			coherentGain = 0.67;
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				window[j] = (window[i] = 1.0 - (float)sqrt((float)((i - midm1) / midp1)));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		//http://www.recordingblogs.com/sa/tabid/88/Default.aspx?topic=Parzen+window
		case PARZEN: // PARZEN_WINDOW
			coherentGain = 0.69;
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				window[j] = (window[i] = 1.0 - ((float)fabs((float)(i - midm1) / midp1)));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		//http://www.recordingblogs.com/sa/tabid/88/Default.aspx?topic=Coherent+gain
		case BARTLETT: // BARTLETT_WINDOW
			coherentGain = 0.50;
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += rate) {
				window[j] = (window[i] = angle);
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		//Hamming Window: Raised cosine with a platform ("Digital Filters and their Design" pg 119)
		//Lynn forumula 5.22 shows 0.54 + 0.46, online reference show 0.54 - 0.46
		//Error in Lynn forumula, see Widipedia reference for all window functions
		case HAMMING: // HAMMING_WINDOW
			coherentGain = 0.54;
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				window[j] = (window[i] = coherentGain - 0.46 * cos(angle));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		//Window function Blackman
		//RL: Used this algorithm effectively with DSPGUIDE code
		case BLACKMAN:
			coherentGain = 0.42;
			for (i = 0; i<numSamples; i++) {
				window[i] = coherentGain - 0.5 * cos((TWOPI * i) / midm1) + 0.08 * cos((FOURPI * i) / midm1);
				windowCpx[i] = window[i];
			}
			break;
		case BLACKMAN2:	// BLACKMAN2_WINDOW
			coherentGain = 0.42;//???
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				cx = cos(angle);
				window[j] = (window[i] = (.34401 + (cx * (-.49755 + (cx * .15844)))));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		case BLACKMAN3: // BLACKMAN3_WINDOW
			coherentGain = 0.42;//???
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
				cx = cos(angle);
				window[j] = (window[i] = (.21747 + (cx * (-.45325 + (cx * (.28256 - (cx * .04672)))))));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		case BLACKMAN4: // BLACKMAN4_WINDOW
			coherentGain = 0.42;//???
			for (i = 0, j = numSamples - 1, angle = 0.0; i <= midn; i++, j--, angle += freq)
			{
				cx = cos(angle);
				window[j] = (window[i] =
							(.084037 +
							(cx *
							(-.29145 +
							(cx *
							(.375696 + (cx * (-.20762 + (cx * .041194)))))))));
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		case EXPONENTIAL: // EXPONENTIAL_WINDOW
			coherentGain = 1.0; //???
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				window[j] = (window[i] = expsum - 1.0);
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
				expsum *= expn;
			}
			break;
		case RIEMANN: // RIEMANN_WINDOW
			coherentGain = 1.0; //???
			sr1 = two_pi / numSamples;
			for (i = 0, j = numSamples - 1; i <= midn; i++, j--) {
				if (i == midn) window[j] = (window[i] = 1.0);
				else {
					cx = sr1 * (midn - i);
					window[i] = sin(cx) / cx;
					window[j] = window[i];
				}
				windowCpx[i] = window[i];
				windowCpx[j] = window[j];
			}
			break;
		case BLACKMANHARRIS: // BLACKMANHARRIS_WINDOW
			coherentGain = 0.36;
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
					windowCpx[i] = window[i];
				}
			}
			break;
		case NONE: //Do nothing
			break;
		default:
			return;
	}

}
