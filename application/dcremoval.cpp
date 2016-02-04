#include "dcremoval.h"

DCRemoval::DCRemoval(quint32 _sampleRate)
{
	double Q = 0.7071;
	dcHpFilter.InitHP(10, 1.0, _sampleRate);
}

void DCRemoval::Process(CPX* in, CPX *out, quint32 _numSamples) {
#if 1
	dcHpFilter.ProcessFilter(_numSamples,in, out);
#else
	//http://sam-koblenski.blogspot.com/2015/11/everyday-dsp-for-programmers-dc-and.html
	//Test DC removal with IIR hp filter
	//Smaller alpha = higher filter
	//We want DC + to be removed
	float alpha = 0.90; //Scale factor to widen or narrow filter
	float lastRE=0, lastIM = 0;
	float w_nIM, w_nRE;
	for (int i=0; i<numSamples; i++) {
		w_nRE = in[i].re  + alpha * lastRE;
		w_nIM = in[i].im  + alpha * lastIM;
		in[i].re = w_nRE - lastRE;
		in[i].im = w_nIM - lastIM;
		lastRE = w_nRE;
		lastIM = w_nIM;
	}
#endif

}
