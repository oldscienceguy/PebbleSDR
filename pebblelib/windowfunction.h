#ifndef WINDOWFUNCTION_H
#define WINDOWFUNCTION_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

class WindowFunction
{
public:
	enum WINDOWTYPE {RECTANGULAR=1, HANNING, WELCH, PARZEN, BARTLETT, HAMMING, BLACKMAN2, BLACKMAN3,
		BLACKMAN4, EXPONENTIAL,RIEMANN, BLACKMANHARRIS, BLACKMAN, NONE};

	WindowFunction(WINDOWTYPE _type, int _numSamples);

	~WindowFunction();

	//Regenerate window function
	void generate(WINDOWTYPE _type);

	double *window;
	CPX *windowCpx; //CPX version, same coefficients in re and im
	int numSamples;
	WINDOWTYPE windowType;
};

#endif // WINDOWFUNCTION_H
