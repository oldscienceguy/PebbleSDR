#include "dcremoval.h"

DCRemoval::DCRemoval(quint32 _sampleRate, quint32 _bufferSize) : ProcessStep(_sampleRate, _bufferSize)
{
	//Not sure what effect Q has on DC filter
	double Q = 0.7071;
	//Filter out 10hz and lower
	dcHpFilter.InitHP(10, Q, _sampleRate);
}

CPX* DCRemoval::process(CPX* in, quint32 _numSamples) {
	if (!enabled) {
		copyCPX(out, in, _numSamples);
		return out;
	}
#if 1
	dcHpFilter.ProcessFilter(_numSamples,in, out);
	return out;
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
		in[i].real(w_nRE - lastRE);
		in[i].imag(w_nIM - lastIM);
		lastRE = w_nRE;
		lastIM = w_nIM;
	}
#endif

}
