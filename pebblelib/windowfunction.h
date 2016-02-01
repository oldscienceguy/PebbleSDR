#ifndef WINDOWFUNCTION_H
#define WINDOWFUNCTION_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

class WindowFunction
{
public:
	enum WINDOWTYPE {RECTANGULAR=1,HANNING,WELCH,PARZEN,BARTLETT,HAMMING,BLACKMAN2,BLACKMAN3,
		BLACKMAN4,EXPONENTIAL,RIEMANN,BLACKMANHARRIS,BLACKMAN};

	WindowFunction(WINDOWTYPE _type, int _numSamples);

	~WindowFunction();

	double *window;
	int numSamples;
	WINDOWTYPE windowType;
};

#endif // WINDOWFUNCTION_H
