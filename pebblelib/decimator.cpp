#include "decimator.h"

//Use MatLab coefficients instead of cuteSDR
#define USE_MATLAB

Decimator::Decimator(quint32 _sampleRate, quint32 _bufferSize)
{
	//Comparing convolution options, vDsp with combining stages works best
	useVdsp = true;
	combineStages = true;
	if (combineStages)
		qDebug()<<"Decimator is combining stages when the same filter is re-used";
	else
		qDebug()<<"Decimator is using fixed decimate-by-two for each stage";
	if (useVdsp)
		qDebug()<<"Decimator is using Apple vDSP functions";
	else
		qDebug()<<"Decimator is using internal convolution functions";

	sampleRate = _sampleRate;
	bufferSize = _bufferSize;

	decimatedSampleRate = _sampleRate;
	decimationChain.clear();
	initFilters();

	workingBuf1 = CPX::memalign(bufferSize * 2);
	workingBuf2 = CPX::memalign(bufferSize * 2);

	if (useVdsp) {
		//Use osX Accelerate vector library
		quint32 splitBufSize = bufferSize * 2;
		splitComplexIn.realp = new double[splitBufSize];
		memset(splitComplexIn.realp,0,splitBufSize);

		splitComplexIn.imagp = new double[splitBufSize];
		memset(splitComplexIn.imagp,0,splitBufSize);

		splitComplexOut.realp = new double[splitBufSize];
		memset(splitComplexOut.realp,0,splitBufSize);

		splitComplexOut.imagp = new double[splitBufSize];
		memset(splitComplexOut.imagp,0,splitBufSize);
	}
}

Decimator::~Decimator()
{
	free (workingBuf1);
	free (workingBuf2);

	if (useVdsp) {
		delete [] splitComplexIn.realp;
		delete [] splitComplexIn.imagp;
		delete [] splitComplexOut.realp;
		delete [] splitComplexOut.imagp;
	}
	deleteFilters();
}

//Return decimation rate and chain from sampleRateIn to sampleRateOut(if specified) that also protects bw
//Testing sampleRateOut with hackrfdevice for higher sample rates

float Decimator::buildDecimationChain(quint32 _sampleRateIn, quint32 _protectBw, quint32 _sampleRateOut)
{
	decimatedSampleRate = _sampleRateIn;
	protectBandWidth = _protectBw;
	quint32 minSampleRateOut = _sampleRateOut > 0 ? _sampleRateOut : minDecimatedSampleRate;
	HalfbandFilter *chain = NULL;
	HalfbandFilter *prevChain = NULL;
	qDebug()<<"Building Decimator chain for sampleRate = "<<_sampleRateIn<<" max bw = "<<_protectBw;
	decimationChain.clear(); //in case we're called more than once
	while (decimatedSampleRate > minSampleRateOut) {
		//Use cic3 for faster decimation
#if 1
		if(decimatedSampleRate >= (protectBandWidth / cic3->wPass) ) {		//See if can use CIC order 3
			//Passing NULL for coeff sets CIC3
			chain = new HalfbandFilter(cic3);
			qDebug()<<"cic3 = "<<decimatedSampleRate;
#else
		//Compare 7 tap halfband to cic for 1st stage
		if(decimatedSampleRate >= (protectBandWidth / hb7->wPass) ) {		//See if can use CIC order 3
			chain = new HalfbandFilter(hb7);
			qDebug()<<"hb7 = "<<decimatedSampleRate;
#endif
		} else if(decimatedSampleRate >= (protectBandWidth / hb11->wPass) ) {	//See if can use fixed 11 Tap Halfband
			chain = new HalfbandFilter(hb11);
			qDebug()<<"hb11 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb15->wPass) ) {	//See if can use Halfband 15 Tap
			chain = new HalfbandFilter(hb15);
			qDebug()<<"hb15 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb19->wPass) ) {	//See if can use Halfband 19 Tap
			chain = new HalfbandFilter(hb19);
			qDebug()<<"hb19 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb23->wPass) ) {	//See if can use Halfband 23 Tap
			chain = new HalfbandFilter(hb23);
			qDebug()<<"hb23 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb27->wPass) ) {	//See if can use Halfband 27 Tap
			chain = new HalfbandFilter(hb27);
			qDebug()<<"hb27 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb31->wPass) ) {	//See if can use Halfband 31 Tap
			chain = new HalfbandFilter(hb31);
			qDebug()<<"hb31 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb35->wPass) ) {	//See if can use Halfband 35 Tap
			chain = new HalfbandFilter(hb35);
			qDebug()<<"hb35 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb39->wPass) ) {	//See if can use Halfband 39 Tap
			chain = new HalfbandFilter(hb39);
			qDebug()<<"hb39 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb43->wPass) ) {	//See if can use Halfband 43 Tap
			chain = new HalfbandFilter(hb43);
			qDebug()<<"hb43 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb47->wPass) ) {	//See if can use Halfband 47 Tap
			chain = new HalfbandFilter(hb47);
			qDebug()<<"hb47 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb51->wPass) ) {	//See if can use Halfband 51 Tap
			chain = new HalfbandFilter(hb51);
			qDebug()<<"hb51 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (protectBandWidth / hb59->wPass) ) {	//See if can use Halfband 51 Tap
			chain = new HalfbandFilter(hb59);
			qDebug()<<"hb59 = "<<decimatedSampleRate;
		} else {
			qDebug()<<"Ran out of filters before minimum sample rate";
			break; //out of while
		}

		if (!combineStages || prevChain == NULL || prevChain->numTaps != chain->numTaps) {
			//New filter, add to chain
			decimationChain.append(chain);
			//Each chain knows what the next chain needs
			chain->sampleRateIn = decimatedSampleRate;
			prevChain = chain;
		} else {
			//If we're using the same filter for more than one step, we can save significant time
			//By doing all the decimation in the same stage, rather than calling the stage repeatedly
			//For a 2.048e6 sample rate and 20khz bandwidth, this reduced time almost 50%
			//From 223us to 123us by reusing hb11
			prevChain->decimate *= 2;
			prevChain->sampleRateIn /= 2;
		}

		decimatedSampleRate /= 2;
	}
	qDebug()<<"Decimated sample rate = "<<decimatedSampleRate;
	return decimatedSampleRate;
}

//Should return CPX* to decimated buffer and number of samples in buffer
quint32 Decimator::process(CPX *_in, CPX *_out, quint32 _numSamples)
{
	mutex.lock();
	if (decimationChain.isEmpty()) {
		//No decimation, just return
		CPX::copyCPX(_out,_in,_numSamples);
		mutex.unlock();
		return _numSamples;
	}
	quint32 remainingSamples = _numSamples;

	if (useVdsp) {
		//Stride for CPX must be 2 for vDsp legacy reasons
		int cpxStride = 2;
		int splitCpxStride = 1;

		//Convert CPX to DoubleSplitComplex
		vDSP_ctozD((DSPDoubleComplex*)_in,cpxStride, &splitComplexIn, splitCpxStride, _numSamples);
#if 0
		//Test conversion
		for (int i=0; i<_numSamples; i++) {
			if (_in[i].re != splitComplexIn.realp[i])
				qDebug()<<"Real pointers !=";
			if (_in[i].im != splitComplexIn.imagp[i])
				qDebug()<<"Imag pointers !=";
		}
#endif

		HalfbandFilter *chain = NULL;
		DSPDoubleSplitComplex* nextIn = &splitComplexIn;
		DSPDoubleSplitComplex* nextOut = &splitComplexOut;
		DSPDoubleSplitComplex* lastOut;

		for (int i=0; i<decimationChain.length(); i++) {
			chain = decimationChain[i];
			//qDebug()<<"Remaining Samples "<<remainingSamples<<" decimate "<<chain->decimate<<" taps "<<chain->numTaps;
			remainingSamples = chain->processVDsp(nextIn, nextOut, remainingSamples);
			//Swap in and out
			lastOut = nextOut;
			nextOut = nextIn;
			nextIn = lastOut;
		}
		//Convert back to CPX*
		vDSP_ztocD(lastOut,splitCpxStride,(DSPDoubleComplex*)_out,cpxStride,remainingSamples);
#if 0
		//Test conversion
		for (int i=0; i<remainingSamples; i++) {
			if (_out[i].re != splitComplexOut.realp[i])
				qDebug()<<"Real pointers !=";
			if (_out[i].im != splitComplexOut.imagp[i])
				qDebug()<<"Imag pointers !=";
		}
#endif

	} else {
		HalfbandFilter *chain = NULL;
		CPX::copyCPX(workingBuf1, _in, _numSamples);
		CPX* nextIn = workingBuf1;
		CPX* nextOut = workingBuf2;
		CPX* lastOut;
		for (int i=0; i<decimationChain.length(); i++) {
			chain = decimationChain[i];

			remainingSamples = chain->process(nextIn, nextOut, remainingSamples);
			//Swap in and out
			lastOut = nextOut;
			nextOut = nextIn;
			nextIn = lastOut;
		}
		CPX::copyCPX(_out, lastOut, remainingSamples);

	}
	mutex.unlock();
	return remainingSamples;
}

HalfbandFilterDesign::HalfbandFilterDesign(quint16 _numTaps, double _wPass, double *_coeff) :
	numTaps(_numTaps), wPass(_wPass), coeff(_coeff)
{
}

HalfbandFilter::HalfbandFilter(HalfbandFilterDesign *_design) :
	HalfbandFilter(_design->numTaps, _design->wPass, _design->coeff)
{
}

HalfbandFilter::HalfbandFilter(quint16 _numTaps, double _wPass, double *_coeff)
{
	numTaps = _numTaps;

	wPass = _wPass;
	coeff = _coeff;

	if (_coeff == NULL)
		useCIC3 = true;
	else
		useCIC3 = false;
	xOdd = 0;
	xEven = 0;

	lastXVDsp.realp = new double[maxResultLen];
	lastXVDsp.imagp = new double[maxResultLen];
	//Make sure at least numTaps is set to zero before first call or we can get nan errors from garbage in the buffer
	memset(lastXVDsp.realp,0,maxResultLen);
	memset(lastXVDsp.imagp,0,maxResultLen);

	lastX = CPX::memalign(maxResultLen);
	CPX::clearCPX(lastX, maxResultLen);
	tmpX = CPX::memalign(maxResultLen); //Largest output size we'll see
	CPX::clearCPX(tmpX,maxResultLen);
	decimate = 2;
}

HalfbandFilter::~HalfbandFilter()
{
	delete [] lastXVDsp.realp;
	delete [] lastXVDsp.imagp;
	free (lastX);
	free (tmpX);
}

//Derived from cuteSdr, refactored and modified for Pebble
//From Lyons 10.12 pg 547 z-Domain transfer function for 11 tap halfband filter
//	z = samples ih the delay buffer, z is the most recent sample, z-1 is previous sample#, z-2 is earlier ...
//	h(0) = 1st coeff, h(1) = 2nd coeff ...
// H(z) = h(0)z + h(1)z-1 + h(2)z-2 + h(3)z-3 + h(4)z-4 + h(5)z-5 + h(6)z-6 +
//	h(7)z-7 + h(8)z-8 +h(9)-9 + h(10)-10
//Because every other coeff, except for middle, is zero in a half band filter, we can skip these
// H(z) = h(0)z + h(2)z-2 + h(4)z-4 + h(6)z-6 + h(8)z-8 +  h(10)-10
//Then we add the non-zero middle term that was skipped
// H(z) += h(5)z-5

/*
   Converntions: x(n) = sample(n) h(k) = coefficient[k] y(n) = output(n)
   Output of convolution is numSamples + numTaps -1, output is larger than numSamples
   h(0) is the coefficient that's applied to the oldest sample
   h(k) is the coefficient that's applied to the current sample
   x(0) is the oldest sample, x(numSamples-1) is the newest

   Convolution buffer management options for N tap filter
   1. Single buffer with numTaps-1 samples from the previous buffer at the beginning
		| numTaps-1 delay samples | numSamples new samples |
   2. Overlap/Add
   3. Overlap/Save
*/
//Convolution utility functions for reference, move to CPX
//Basic algorithm from many texts and sources, no delay buffer from last x[]
quint32 HalfbandFilter::convolve(const double *x, quint32 xLen, const double *h,
	quint32 hLen, double *y)
{
	quint32 yLen = xLen + hLen;

	quint32 kmin, kmax;
	for (quint32 n = 0; n < yLen; n++) {
		y[n] = 0;

		//kmin and kmax index the coefficient to be applied to each sample
		//k can never be less than zero or greater than the number of coefficients - 1
		//This handles the case where we are processing the first and last samples
		kmin = (n >= hLen - 1) ? n - (hLen - 1) : 0;
		kmax = (n < xLen - 1) ? n : xLen - 1;

		for (quint32 k = kmin; k <= kmax; k++) {
			y[n] += x[k] * h[n - k];
		}
	}
	return yLen;
}

//We have last samples in , buffer management #1 above
//Overlap/Save
quint32 HalfbandFilter::convolveOS(const CPX *x, quint32 xLen, const double *h,
	quint32 hLen, CPX *y, quint32 ySize, quint32 decimate)
{
	Q_UNUSED(ySize);
	quint32 dLen = hLen - 1; //Size of delay buffer
	quint32 yCnt = 0;
	if (xLen < hLen) {
		//We must have at least enough delayed samples in x to process 1 convolution
		//We can get to this case when we have a very high initial sample rate like 10msps with 2048 samples
		//The last stage may only have 16 samples for a 35 tap filter
		//Filter won't work, so simple decimate
		for (quint32 i=0; i<xLen; i+=decimate) {
			y[yCnt].re = x[i].re;
			y[yCnt].im = x[i].im;
			yCnt++;
		}
		//We don't have enough samples to fill all of lastX
		//So make sure the samples we do have are at the end of lastX and zero anything earlier
		quint32 saveSamples = dLen - xLen; //# x samples to save in lastX
		for (quint32 i=0; i<hLen; i++) {
			if (i < saveSamples) {
				lastX[i].re = 0;
				lastX[i].im = 0;
			} else {
				lastX[i].re = x[i].re;
				lastX[i].im = x[i].im;
			}
		}
		return yCnt;
	}


	//Create a unified buffer with lastX[] and x[]
	//First part of lastX already has last hLen samples;
	CPX::copyCPX(&lastX[dLen], x, xLen);

	for (quint32 n = 0; n < xLen; n += decimate) {
		y[yCnt].clear();

		//in this scenario, we always have hLen samples from the previous call, so no bounds check needed
		//We don't know if we have halfband filter, so need to process all the coefficients
		//If we have halfband, then we only need to process even number coeff plus the mid point
		for (quint32 k = 0; k < hLen; k++) {
			if (h[k] != 0) {
				//Skip zero coefficients since they won't add anything to accumulator
				y[yCnt].re += lastX[n+k].re * h[k];
				y[yCnt].im += lastX[n+k].im * h[k];
			}
		}
		yCnt++;
	}
	//Save the last hLen -1 samples for next call
	CPX::copyCPX(lastX, &x[xLen - dLen], dLen);

	return yCnt;
}

//Overlap/add version
//First hLen samples are partially convolved because we don't have a delay buffer with previous samples
//Last hLen results of convolution come after xLen samples and are saved
//These saved results are then added to the first hLen samples of the next pass
/*
	Diagram
	(TBD)
*/
quint32 HalfbandFilter::convolveOA(const CPX *x, quint32 xLen, const double *h,
	quint32 hLen, CPX *y, quint32 ySize, quint32 decimate)
{
	Q_UNUSED(ySize);

	quint32 dLen = hLen - 1; //Size of delay buffer
	quint32 yLen = xLen + dLen; //Size of result buffer

	quint32 yCnt = 0; //#of samples in y[], needed if decimate != 1
	if (xLen < hLen) {
		//We must have at least enough delayed samples in x to process 1 convolution
		//We can get to this case when we have a very high initial sample rate like 10msps with 2048 samples
		//The last stage may only have 16 samples for a 35 tap filter
		//Filter won't work, so simple decimate
		for (quint32 i=0; i<xLen; i+=decimate) {
			y[yCnt].re = x[i].re;
			y[yCnt].im = x[i].im;
			yCnt++;
		}
		//We don't have enough samples to fill all of lastX
		//So make sure the samples we do have are at the end of lastX and zero anything earlier
		quint32 saveSamples = dLen - xLen; //# x samples to save in lastX
		for (quint32 i=0; i<hLen; i++) {
			if (i < saveSamples) {
				lastX[i].re = 0;
				lastX[i].im = 0;
			} else {
				lastX[i].re = x[i].re;
				lastX[i].im = x[i].im;
			}
		}
		return yCnt;
	}

	CPX *yOut = tmpX;
#if 0
	if (yLen <= ySize) {
		//Use temporary buffer to convolve because we need xLen + hLen - 1 output samples
		//y[] may not be large enough
		yOut = y;
	}
#endif

	for (quint32 n = 0; n < yLen; n += decimate) {
		yOut[yCnt].clear();

/*
	For hLen = 3, dLen = 2 (hLen - 1), xLen = 8, yLen = 10 (xLen + dLen)
	h(0) h(1) h(2)	x(0)..x(7)

	n =     0    1    2 //Special handling #1, missing older samples at beginning of x
	d0 d1 x(0) x(1) x(2) x(3)
		  y(0) = x(0) * h(2) //missing d(0) d(1)
			   y(1) = x(0) * h(1) + x(1) * h(2) //missing d(0)
					y(2) = x(0) * h(0) + x(1) * h(1) + x(2) * h(2) //normal delay = 2 pattern

	n =     4    5    6 //Default handling, we have older samples in x

	n =         7    8    9 //Special handling, conjugation at end of x in prep for next loop
	x(5) x(6) x(7) d(0) d(1)
			  y(7) = x(5) * h(0) + x(6) * h(1) + x(7) * h(2) //Last normal pattern
				   d(0) = x(6) * h(0) + x(7) * h(1) //Last 2 samples
						d(1) = x(7) * h(0) // Last sample

	Next loop, d(0) and d(1) have convolve from previous samples
	y(0) += last d(0) // add missing 2 delay samples
	y(1) += last d(1) // add missing 1 delay sample

*/
		//Default handling, older samples are in x
		quint32 xOld = n - dLen;
		quint32 kMin = 0; //First coefficient to use
		quint32 kMax = dLen; //Last coefficient index
		if (n < dLen) {
			//Special handling #1
			xOld = 0;
			kMin = dLen - n;
			//kMax not changed
			//qDebug()<<n<<" "<<xOld<<" "<<kMin<<" "<<kMax;
		} else if (n >= xLen) {
			//Special handling #2
			//kMin not changed
			kMax = dLen - (n-xLen) - 1;
			//qDebug()<<n<<" "<<xOld<<" "<<kMin<<" "<<kMax;
		}

		const CPX *oldest = &x[xOld];
		for (quint32 k = kMin, j=0; k <= kMax; k++,j++) {
			//qDebug()<<n<<"x["<<xOld+j<<"]*h["<<k<<"] yCnt="<<yCnt;

			if (h[k] != 0) {
				//Skip zero coefficients since they won't add anything to accumulator
				yOut[yCnt].re += oldest[j].re * h[k];
				yOut[yCnt].im += oldest[j].im * h[k];
			}
		}
		yCnt++;
	}

	//Add overlap from last run
	for (quint32 i=0; i<dLen; i++) {
		yOut[i].re += lastX[i].re;
		yOut[i].im += lastX[i].im;
	}

	//Save last overlap results, remember we're decimated
	CPX::copyCPX(lastX,&yOut[xLen / decimate], dLen);

	//And return in out
	if (yOut != y)
		CPX::copyCPX(y,tmpX,yCnt);

	return xLen / decimate; //yCnt will have extra results that we save to lastX
}

quint32 HalfbandFilter::convolveVDsp1(const DSPDoubleSplitComplex *x, quint32 xLen, const double *h,
		quint32 hLen, DSPDoubleSplitComplex *y, quint32 ySize, quint32 decimate)
{
	Q_UNUSED(ySize);

	quint32 dLen = hLen - 1; //Size of delay buffer
	//yLen is the final size of the decimated and filtered output
	quint32 yLen = xLen / decimate;
	Q_UNUSED(yLen);

	quint32 yCnt = 0;

	if (xLen < hLen) {
		//We must have at least enough delayed samples in x to process 1 convolution
		//We can get to this case when we have a very high initial sample rate like 10msps with 2048 samples
		//The last stage may only have 16 samples for a 35 tap filter
		//Filter won't work, so simple decimate
		for (quint32 i=0; i<xLen; i+=decimate) {
			y->realp[yCnt] = x->realp[i];
			y->imagp[yCnt] = x->imagp[i];
			yCnt++;
		}
		//We don't have enough samples to fill all of lastX
		//So make sure the samples we do have are at the end of lastX and zero anything earlier
		quint32 saveSamples = dLen - xLen; //# x samples to save in lastX
		for (quint32 i=0; i<hLen; i++) {
			if (i < saveSamples) {
				lastXVDsp.realp[i] = 0;
				lastXVDsp.imagp[i] = 0;
			} else {
				lastXVDsp.realp[i] = x->realp[i];
				lastXVDsp.imagp[i] = x->imagp[i];
			}
		}
		return yCnt;
	}

	//We can't index split complex directly, but we can create temp ptr to position in realp and imagp
	DSPDoubleSplitComplex tmpIn;
	DSPDoubleSplitComplex tmpOut;

	//Create continuous buffer in lastXVDsp
	tmpOut.realp = &lastXVDsp.realp[dLen];
	tmpOut.imagp = &lastXVDsp.imagp[dLen];
	vDSP_zvmovD(x,1,&tmpOut,1,xLen);
#if 0
		//Test conversion
		for (int i=0; i<xLen; i++) {
			if (x->realp[i] != lastXVDsp.realp[i+dLen])
				qDebug()<<"Real pointers !=";
			if (x->imagp[i] != lastXVDsp.imagp[i+dLen])
				qDebug()<<"Imag pointers !=";
		}
#endif

	for (quint32 n=0; n<xLen; n+=decimate) {
		//Multiply every other sample in _in (decimate by 2) * every coefficient
		tmpIn.realp = &lastXVDsp.realp[n];
		tmpIn.imagp = &lastXVDsp.imagp[n];
		tmpOut.realp = &y->realp[yCnt];
		tmpOut.imagp = &y->imagp[yCnt];
		//Brute force for all coeff, does not know or take advantage of zero coefficients
		vDSP_zrdotprD(&tmpIn,1,h,1,&tmpOut,hLen);
#if 0
		//Checking dot product results, ok
		CPX acc;
		acc.clear();
		for (int i=0; i<hLen; i++) {
			acc.re += tmpIn.realp[i] * coeff[i];
			acc.im += tmpIn.imagp[i] * coeff[i];
		}
		if (acc.re != tmpOut.realp[0])
			qDebug()<<"RE"<<acc.re<<" "<<tmpOut.realp[0];
		if (acc.im != tmpOut.imagp[0])
			qDebug()<<"IM"<<acc.im<<" "<<tmpOut.imagp[0];
#endif
		yCnt++;
	}

	//Copy last dLen samples to lastX for next iteration
	tmpIn.realp = &x->realp[xLen - dLen];
	tmpIn.imagp = &x->imagp[xLen - dLen];

	vDSP_zvmovD(&tmpIn, 1, &lastXVDsp, 1, dLen);

	return yCnt;

}

//Use vDSP_zrdesampD() to do complete filtering
quint32 HalfbandFilter::convolveVDsp2(const DSPDoubleSplitComplex *x, quint32 xLen, const double *h, quint32 hLen,
		DSPDoubleSplitComplex *y, quint32 ySize, quint32 decimate)
{
	Q_UNUSED(ySize);
	quint32 dLen = hLen - 1; //Size of delay buffer
	//yLen is the final size of the decimated and filtered output
	quint32 yLen = xLen / decimate;
	quint32 yCnt = 0;

	if (xLen < hLen) {
		//We must have at least enough delayed samples in x to process 1 convolution
		//We can get to this case when we have a very high initial sample rate like 10msps with 2048 samples
		//The last stage may only have 16 samples for a 35 tap filter
		//Filter won't work, so simple decimate
		for (quint32 i=0; i<xLen; i+=decimate) {
			y->realp[yCnt] = x->realp[i];
			y->imagp[yCnt] = x->imagp[i];
			yCnt++;
		}
		//We don't have enough samples to fill all of lastX
		//So make sure the samples we do have are at the end of lastX and zero anything earlier
		quint32 saveSamples = dLen - xLen; //# x samples to save in lastX
		for (quint32 i=0; i<hLen; i++) {
			if (i < saveSamples) {
				lastXVDsp.realp[i] = 0;
				lastXVDsp.imagp[i] = 0;
			} else {
				lastXVDsp.realp[i] = x->realp[i];
				lastXVDsp.imagp[i] = x->imagp[i];
			}
		}
		return yCnt;
	}


	//We can't index split complex directly, but we can create temp ptr to position in realp and imagp
	DSPDoubleSplitComplex tmpIn;
	DSPDoubleSplitComplex tmpOut;

	//Create continuous buffer in lastXVDsp
	tmpOut.realp = &lastXVDsp.realp[dLen];
	tmpOut.imagp = &lastXVDsp.imagp[dLen];
	vDSP_zvmovD(x,1,&tmpOut,1,xLen);

/*
	Pseudo code from Apple doc
	N = yLen, A = in, F = h, C = y DF = decimate
	for (n = 0; n < N; ++n)
	{
		sum = 0;
		for (p = 0; p < P; ++p)
			sum += A[n * DF + p] * F[p];
		C[n] = sum;
	}
*/
	vDSP_zrdesampD(&lastXVDsp, decimate, h, y, yLen, hLen);

	//We make sure we have enough samples to copy to lastX delay buffer before we get here
	//Copy last dLen samples to lastX for next iteration
	tmpIn.realp = &x->realp[xLen - dLen];
	tmpIn.imagp = &x->imagp[xLen - dLen];
	vDSP_zvmovD(&tmpIn, 1, &lastXVDsp, 1, dLen);

	return yLen;


}

quint32 HalfbandFilter::process(CPX *_in, CPX *_out, quint32 _numInSamples)
{
	if (useCIC3) {
		return processCIC3(_in, _out, _numInSamples);
	}
	quint32 outLen;
	outLen = convolveOS(_in,_numInSamples,coeff,numTaps,_out, maxResultLen, decimate);
	//Overlapp add is slower
	//outLen = convolveOA(_in,_numInSamples,coeff,numTaps,_out, maxResultLen, decimate);

#if 0
	//Todo: Results between OS and OA don't compare yet
	//Compare results
	outLen = convolveOS(_in,_numInSamples,coeff,numTaps,delayBuffer, decimate);
	for (int i=0; i<outLen; i++) {
		if (delayBuffer[i] != _out[i]) {
			qDebug()<<"i = "<<i<<" "<<delayBuffer[i].re<<" != "<<_out[i].re;
			qDebug()<<"i = "<<i<<" "<<delayBuffer[i].im<<" != "<<_out[i].im;
		}
	}
#endif

	return outLen;

}

//From cuteSDR
//Decimate by 2 CIC 3 stage
// -80dB alias rejection up to Fs * (.5 - .4985)

//Function performs decimate by 2 using polyphase decompostion
// implemetation of a CIC N=3 filter.
// InLength must be an even number
//returns number of output samples processed
quint32 HalfbandFilter::processCIC3(const CPX *_in, CPX *_out, quint32 _numInSamples)
{
	quint32 numInSamples = _numInSamples;
	quint32 numOutSamples = 0;
	const CPX* inBuffer = _in;
	CPX *outBuffer = _out;

	CPX even,odd;
	//StartPerformance();
	for(quint32 i=0; i<numInSamples; i += decimate)
	{	//mag gn=8
		even = inBuffer[i];
		odd = inBuffer[i+1];
		outBuffer[numOutSamples].re = .125*( odd.re + xEven.re + 3.0*(xOdd.re + even.re) );
		outBuffer[numOutSamples].im = .125*( odd.im + xEven.im + 3.0*(xOdd.im + even.im) );
		xOdd = odd;
		xEven = even;
		numOutSamples++;
	}
	//StopPerformance(numInSamples);
	return numOutSamples;

}

quint32 HalfbandFilter::processCIC3(const DSPDoubleSplitComplex *_in, DSPDoubleSplitComplex *_out, quint32 _numInSamples)
{
	quint32 numOutSamples = 0;

	CPX even,odd;
	for(quint32 i=0; i<_numInSamples; i += decimate)
	{	//mag gn=8
		even.re = _in->realp[i];
		even.im = _in->imagp[i];
		odd.re = _in->realp[i+1];
		odd.im = _in->imagp[i+1];
		_out->realp[numOutSamples] = .125*( odd.re + xEven.re + 3.0*(xOdd.re + even.re) );
		_out->imagp[numOutSamples] = .125*( odd.im + xEven.im + 3.0*(xOdd.im + even.im) );
		xOdd = odd;
		xEven = even;
		numOutSamples++;
	}
	return numOutSamples;
}

//Experiment using vDsp dot product functions
//_in has empty space at the beginning of the buffer that we use for delay samples
//Actual input samples start at _in[numDelaySamples]
quint32 HalfbandFilter::processVDsp(const DSPDoubleSplitComplex *_in,
		DSPDoubleSplitComplex *_out, quint32 _numInSamples)
{
	if (useCIC3) {
		return processCIC3(_in, _out, _numInSamples);
	}

	//VDSP Dot Product
	//return convolveVDsp1(_in, _numInSamples, coeff, numTaps, _out, maxResultLen, decimate);
	//VDSP filter
	return convolveVDsp2(_in, _numInSamples, coeff, numTaps, _out, maxResultLen, decimate);
}

void Decimator::deleteFilters()
{
	delete (cic3);
	delete (hb7);
	delete (hb11);
	delete (hb15);
	delete (hb19);
	delete (hb23);
	delete (hb27);
	delete (hb31);
	delete (hb35);
	delete (hb39);
	delete (hb43);
	delete (hb47);
	delete (hb51);
	delete (hb59);
	for (int i=0; i<decimationChain.length(); i++) {
		delete decimationChain[i];
	}
}

//Halfband decimating filters are generated in Matlab with order = numTaps-1 and wPass
//Matlab saves to .h file where we copy and paste below
//Doubles are only 15 decimal place precision, so Matlab has more precise data and slightly better modeling
void Decimator::initFilters()
{
	double *coeff;

	cic3 = new HalfbandFilterDesign(0, 0.0030, NULL);

	hb7 = new HalfbandFilterDesign(7, 0.0030, new double[7] {
			-0.03125208195057,                 0,   0.2812520818582,               0.5,
			  0.2812520818582,                 0, -0.03125208195057
	});

#ifndef USE_MATLAB
	coeff = new double[11] {
			0.0060431029837374152,
			0.0,
			-0.049372515458761493,
			0.0,
			0.29332944952052842,
			0.5,
			0.29332944952052842,
			0.0,
			-0.049372515458761493,
			0.0,
			0.0060431029837374152,
		};
#else
		coeff = new double[11] {
			0.006043102983737,                 0, -0.04937251545876,                 0,
			  0.2933294495205,               0.5,   0.2933294495205,                 0,
			-0.04937251545876,                 0, 0.006043102983737
		};
#endif

	hb11 = new HalfbandFilterDesign(11, 0.0500, coeff);

#ifndef USE_MATLAB
		coeff = new double[15] {
			-0.001442203300285281,
			0.0,
			0.013017512802724852,
			0.0,
			-0.061653278604903369,
			0.0,
			0.30007792316024057,
			0.5,
			0.30007792316024057,
			0.0,
			-0.061653278604903369,
			0.0,
			0.013017512802724852,
			0.0,
			-0.001442203300285281
		};
#else
		coeff = new double[15] {
			-0.001442203300197,                 0,  0.01301751280274,                 0,
			 -0.06165327860496,                 0,   0.3000779231603,               0.5,
			   0.3000779231603,                 0, -0.06165327860496,                 0,
			  0.01301751280274,                 0,-0.001442203300197
		};
#endif
	hb15 = new HalfbandFilterDesign(15, 0.0980, coeff);

#ifndef USE_MATLAB
		coeff = new double[19] {
			0.00042366527106480427,
			0.0,
			-0.0040717333369021894,
			0.0,
			0.019895653881950692,
			0.0,
			-0.070740034412329067,
			0.0,
			0.30449249772844139,
			0.5,
			0.30449249772844139,
			0.0,
			-0.070740034412329067,
			0.0,
			0.019895653881950692,
			0.0,
			-0.0040717333369021894,
			0.0,
			0.00042366527106480427
		};
#else
		coeff = new double[19] {
			0.0004236652712401,                 0, -0.00407173333702,                 0,
				0.019895653882,                 0, -0.07074003441233,                 0,
			   0.3044924977284,               0.5,   0.3044924977284,                 0,
			 -0.07074003441233,                 0,    0.019895653882,                 0,
			 -0.00407173333702,                 0,0.0004236652712401
		};
#endif
	hb19 = new HalfbandFilterDesign(19, 0.1434, coeff);

#ifndef USE_MATLAB
		coeff = new double[23] {
			-0.00014987651418332164,
			0.0,
			0.0014748633283609852,
			0.0,
			-0.0074416944990005314,
			0.0,
			0.026163522731980929,
			0.0,
			-0.077593699116544707,
			0.0,
			0.30754683719791986,
			0.5,
			0.30754683719791986,
			0.0,
			-0.077593699116544707,
			0.0,
			0.026163522731980929,
			0.0,
			-0.0074416944990005314,
			0.0,
			0.0014748633283609852,
			0.0,
			-0.00014987651418332164
		};
#else
		coeff = new double[23] {
			-0.0001498765142888,                 0, 0.001474863328452,                 0,
			-0.007441694499077,                 0,  0.02616352273205,                 0,
			 -0.07759369911661,                 0,   0.3075468371979,               0.5,
			   0.3075468371979,                 0, -0.07759369911661,                 0,
			  0.02616352273205,                 0,-0.007441694499077,                 0,
			 0.001474863328452,                 0,-0.0001498765142888
	};
#endif
	hb23 = new HalfbandFilterDesign(23, 0.1820, coeff);

#ifndef USE_MATLAB
		coeff = new double[27] {
			0.000063730426952664685,
			0.0,
			-0.00061985193978569082,
			0.0,
			0.0031512504783365756,
			0.0,
			-0.011173151342856621,
			0.0,
			0.03171888754393197,
			0.0,
			-0.082917863582770729,
			0.0,
			0.3097770473566307,
			0.5,
			0.3097770473566307,
			0.0,
			-0.082917863582770729,
			0.0,
			0.03171888754393197,
			0.0,
			-0.011173151342856621,
			0.0,
			0.0031512504783365756,
			0.0,
			-0.00061985193978569082,
			0.0,
			0.000063730426952664685
		};
#else
		coeff = new double[27] {
			6.373042719017e-05,                 0,-0.0006198519400048,                 0,
			 0.003151250478532,                 0, -0.01117315134303,                 0,
			  0.03171888754407,                 0, -0.08291786358287,                 0,
			   0.3097770473567,               0.5,   0.3097770473567,                 0,
			 -0.08291786358287,                 0,  0.03171888754407,                 0,
			 -0.01117315134303,                 0, 0.003151250478532,                 0,
			-0.0006198519400048,                 0,6.373042719017e-05
	};
#endif
	hb27 = new HalfbandFilterDesign(27, 0.2160, coeff);

#ifndef USE_MATLAB
		coeff = new double[31] {
			-0.000030957335326552226,
			0.0,
			0.00029271992847303054,
			0.0,
			-0.0014770381124258423,
			0.0,
			0.0052539088990950535,
			0.0,
			-0.014856378748476874,
			0.0,
			0.036406651919555999,
			0.0,
			-0.08699862567952929,
			0.0,
			0.31140967076042625,
			0.5,
			0.31140967076042625,
			0.0,
			-0.08699862567952929,
			0.0,
			0.036406651919555999,
			0.0,
			-0.014856378748476874,
			0.0,
			0.0052539088990950535,
			0.0,
			-0.0014770381124258423,
			0.0,
			0.00029271992847303054,
			0.0,
			-0.000030957335326552226
		};
#else
		coeff = new double[31] {
			-3.095733538676e-05,                 0, 0.000292719928498,                 0,
			-0.001477038112406,                 0, 0.005253908899031,                 0,
			 -0.01485637874838,                 0,  0.03640665191945,                 0,
			 -0.08699862567944,                 0,   0.3114096707604,               0.5,
			   0.3114096707604,                 0, -0.08699862567944,                 0,
			  0.03640665191945,                 0, -0.01485637874838,                 0,
			 0.005253908899031,                 0,-0.001477038112406,                 0,
			 0.000292719928498,                 0,-3.095733538676e-05
	};
#endif
	hb31 = new HalfbandFilterDesign(31, 0.2440, coeff);

#ifndef USE_MATLAB
		coeff = new double[35] {
			0.000017017718072971716,
			0.0,
			-0.00015425042851962818,
			0.0,
			0.00076219685751140838,
			0.0,
			-0.002691614694785393,
			0.0,
			0.0075927497927344764,
			0.0,
			-0.018325727896057686,
			0.0,
			0.040351004914363969,
			0.0,
			-0.090198224668969554,
			0.0,
			0.31264689763504327,
			0.5,
			0.31264689763504327,
			0.0,
			-0.090198224668969554,
			0.0,
			0.040351004914363969,
			0.0,
			-0.018325727896057686,
			0.0,
			0.0075927497927344764,
			0.0,
			-0.002691614694785393,
			0.0,
			0.00076219685751140838,
			0.0,
			-0.00015425042851962818,
			0.0,
			0.000017017718072971716
		};
#else
		coeff = new double[35] {
			1.70177181004e-05,                 0,-0.0001542504284999,                 0,
		   0.0007621968574243,                 0,-0.002691614694624,                 0,
			0.007592749792507,                 0, -0.01832572789579,                 0,
			 0.04035100491411,                 0, -0.09019822466878,                 0,
			   0.312646897635,               0.5,    0.312646897635,                 0,
			-0.09019822466878,                 0,  0.04035100491411,                 0,
			-0.01832572789579,                 0, 0.007592749792507,                 0,
		   -0.002691614694624,                 0,0.0007621968574243,                 0,
		   -0.0001542504284999,                 0, 1.70177181004e-05
	};
#endif
	hb35 = new HalfbandFilterDesign(35, 0.2680, coeff);

#ifndef USE_MATLAB
		coeff = new double[39] {
			-0.000010175082832074367,
			0.0,
			0.000088036416015024345,
			0.0,
			-0.00042370835558387595,
			0.0,
			0.0014772557414459019,
			0.0,
			-0.0041468438954260153,
			0.0,
			0.0099579126901608011,
			0.0,
			-0.021433527104289002,
			0.0,
			0.043598963493432855,
			0.0,
			-0.092695953625928404,
			0.0,
			0.31358799113382152,
			0.5,
			0.31358799113382152,
			0,
			-0.092695953625928404,
			0.0,
			0.043598963493432855,
			0.0,
			-0.021433527104289002,
			0.0,
			0.0099579126901608011,
			0.0,
			-0.0041468438954260153,
			0.0,
			0.0014772557414459019,
			0.0,
			-0.00042370835558387595,
			0.0,
			0.000088036416015024345,
			0.0,
			-0.000010175082832074367
		};
#else
		coeff = new double[39] {
			-1.017508271229e-05,                 0,8.803641592052e-05,                 0,
			-0.0004237083555277,                 0, 0.001477255741435,                 0,
			-0.004146843895457,                 0, 0.009957912690223,                 0,
			 -0.02143352710436,                 0,   0.0435989634935,                 0,
			 -0.09269595362598,                 0,   0.3135879911338,               0.5,
			   0.3135879911338,                 0, -0.09269595362598,                 0,
			   0.0435989634935,                 0, -0.02143352710436,                 0,
			 0.009957912690223,                 0,-0.004146843895457,                 0,
			 0.001477255741435,                 0,-0.0004237083555277,                 0,
			8.803641592052e-05,                 0,-1.017508271229e-05
	};
#endif
	hb39 = new HalfbandFilterDesign(39, 0.2880, coeff);

#ifndef USE_MATLAB
		coeff = new double[43] {
			0.0000067666739082756387,
			0.0,
			-0.000055275221547958285,
			0.0,
			0.00025654074579418561,
			0.0,
			-0.0008748125689163153,
			0.0,
			0.0024249876017061502,
			0.0,
			-0.0057775190656021748,
			0.0,
			0.012299834239523121,
			0.0,
			-0.024244050662087069,
			0.0,
			0.046354303503099069,
			0.0,
			-0.094729903598633314,
			0.0,
			0.31433918020123208,
			0.5,
			0.31433918020123208,
			0.0,
			-0.094729903598633314,
			0.0,
			0.046354303503099069,
			0.0,
			-0.024244050662087069,
			0.0,
			0.012299834239523121,
			0.0,
			-0.0057775190656021748,
			0.0,
			0.0024249876017061502,
			0.0,
			-0.0008748125689163153,
			0.0,
			0.00025654074579418561,
			0.0,
			-0.000055275221547958285,
			0.0,
			0.0000067666739082756387
		};
#else
		coeff = new double[43] {
			6.766673911034e-06,                 0,-5.527522154637e-05,                 0,
			0.0002565407457896,                 0,-0.0008748125689126,                 0,
			 0.002424987601706,                 0,-0.005777519065606,                 0,
			  0.01229983423953,                 0, -0.02424405066209,                 0,
			   0.0463543035031,                 0, -0.09472990359863,                 0,
			   0.3143391802012,               0.5,   0.3143391802012,                 0,
			 -0.09472990359863,                 0,   0.0463543035031,                 0,
			 -0.02424405066209,                 0,  0.01229983423953,                 0,
			-0.005777519065606,                 0, 0.002424987601706,                 0,
			-0.0008748125689126,                 0,0.0002565407457896,                 0,
			-5.527522154637e-05,                 0,6.766673911034e-06
	};
#endif
	hb43 = new HalfbandFilterDesign(43, 0.3060, coeff);

#ifndef USE_MATLAB
		coeff = new double[47] {
			-0.0000045298314172004251,
			0.0,
			0.000035333704512843228,
			0.0,
			-0.00015934776420643447,
			0.0,
			0.0005340788063118928,
			0.0,
			-0.0014667949695500761,
			0.0,
			0.0034792089350833247,
			0.0,
			-0.0073794356720317733,
			0.0,
			0.014393786384683398,
			0.0,
			-0.026586603160193314,
			0.0,
			0.048538673667907428,
			0.0,
			-0.09629115286535718,
			0.0,
			0.31490673428547367,
			0.5,
			0.31490673428547367,
			0.0,
			-0.09629115286535718,
			0.0,
			0.048538673667907428,
			0.0,
			-0.026586603160193314,
			0.0,
			0.014393786384683398,
			0.0,
			-0.0073794356720317733,
			0.0,
			0.0034792089350833247,
			0.0,
			-0.0014667949695500761,
			0.0,
			0.0005340788063118928,
			0.0,
			-0.00015934776420643447,
			0.0,
			0.000035333704512843228,
			0.0,
			-0.0000045298314172004251
		};
#else
		coeff = new double[47] {
			-4.529831444832e-06,                 0,3.533370457971e-05,                 0,
			-0.0001593477643404,                 0,0.0005340788065404,                 0,
			-0.001466794969894,                 0, 0.003479208935549,                 0,
			-0.007379435672603,                 0,  0.01439378638532,                 0,
			 -0.02658660316082,                 0,  0.04853867366844,                 0,
			 -0.09629115286572,                 0,   0.3149067342856,               0.5,
			   0.3149067342856,                 0, -0.09629115286572,                 0,
			  0.04853867366844,                 0, -0.02658660316082,                 0,
			  0.01439378638532,                 0,-0.007379435672603,                 0,
			 0.003479208935549,                 0,-0.001466794969894,                 0,
			0.0005340788065404,                 0,-0.0001593477643404,                 0,
			3.533370457971e-05,                 0,-4.529831444832e-06
	};
#endif
	hb47 = new HalfbandFilterDesign(47, 0.3200, coeff);

#ifndef USE_MATLAB
		coeff = new double[51] {
			0.0000033359253688981639,
			0.0,
			-0.000024584155158361803,
			0.0,
			0.00010677777483317733,
			0.0,
			-0.00034890723143173914,
			0.0,
			0.00094239127078189603,
			0.0,
			-0.0022118302078923137,
			0.0,
			0.0046575030752162277,
			0.0,
			-0.0090130973415220566,
			0.0,
			0.016383673864361164,
			0.0,
			-0.028697281101743237,
			0.0,
			0.05043292242400841,
			0.0,
			-0.097611898315791965,
			0.0,
			0.31538104435015801,
			0.5,
			0.31538104435015801,
			0.0,
			-0.097611898315791965,
			0.0,
			0.05043292242400841,
			0.0,
			-0.028697281101743237,
			0.0,
			0.016383673864361164,
			0.0,
			-0.0090130973415220566,
			0.0,
			0.0046575030752162277,
			0.0,
			-0.0022118302078923137,
			0.0,
			0.00094239127078189603,
			0.0,
			-0.00034890723143173914,
			0.0,
			0.00010677777483317733,
			0.0,
			-0.000024584155158361803,
			0.0,
			0.0000033359253688981639
		};
#else
		coeff = new double[51] {
			3.335925497904e-06,                 0,-2.458415527314e-05,                 0,
			 0.000106777774933,                 0,-0.0003489072315249,                 0,
			0.0009423912708838,                 0, -0.00221183020802,                 0,
			  0.00465750307538,                 0,-0.009013097341723,                 0,
			  0.01638367386459,                 0, -0.02869728110197,                 0,
			   0.0504329224242,                 0, -0.09761189831592,                 0,
			   0.3153810443502,               0.5,   0.3153810443502,                 0,
			 -0.09761189831592,                 0,   0.0504329224242,                 0,
			 -0.02869728110197,                 0,  0.01638367386459,                 0,
			-0.009013097341723,                 0,  0.00465750307538,                 0,
			 -0.00221183020802,                 0,0.0009423912708838,                 0,
			-0.0003489072315249,                 0, 0.000106777774933,                 0,
			-2.458415527314e-05,                 0,3.335925497904e-06
	};
#endif
	hb51 = new HalfbandFilterDesign(51, 0.3332, coeff);

	//Testing
	coeff = new double[59] {
			3.506682689989e-05,                 0,-0.000121561534634,                 0,
			0.0003173663988414,                 0,-0.0006986749344128,                 0,
			 0.001372715712337,                 0,-0.002481541656365,                 0,
			 0.004208439930571,                 0,-0.006790677852846,                 0,
			  0.01054942754858,                 0, -0.01596257974904,                 0,
			   0.0238504368495,                 0,   -0.035900032588,                 0,
			  0.05647045609946,                 0,  -0.1016394489401,                 0,
			   0.3167963405688,               0.5,   0.3167963405688,                 0,
			  -0.1016394489401,                 0,  0.05647045609946,                 0,
			   -0.035900032588,                 0,   0.0238504368495,                 0,
			 -0.01596257974904,                 0,  0.01054942754858,                 0,
			-0.006790677852846,                 0, 0.004208439930571,                 0,
			-0.002481541656365,                 0, 0.001372715712337,                 0,
			-0.0006986749344128,                 0,0.0003173663988414,                 0,
			-0.000121561534634,                 0,3.506682689989e-05

	};
	hb59 = new HalfbandFilterDesign(59, 0.400, coeff);
}
