#include "decimator.h"

//Use MatLab coefficients instead of cuteSDR
#define USE_MATLAB

Decimator::Decimator(quint32 _sampleRate, quint32 _bufferSize) :
	ProcessStep(_sampleRate, _bufferSize)
{
	useVdsp = false;
	decimatedSampleRate = _sampleRate;
	decimationChain.clear();
	initFilters();

	if (useVdsp) {
		//Testing vDSP
		splitComplexIn.realp = new double[_bufferSize * 2];
		splitComplexIn.imagp = new double[_bufferSize * 2];
		splitComplexOut.realp = new double[_bufferSize * 2];
		splitComplexOut.imagp = new double[_bufferSize * 2];
	}

}

Decimator::~Decimator()
{
	deleteFilters();
}


quint32 Decimator::buildDecimationChain(quint32 _sampleRate, quint32 _maxBandWidth)
{
	decimatedSampleRate = _sampleRate;
	maxBandWidth = _maxBandWidth;
	Decimator::HalfbandFilter *chain = NULL;
	Decimator::HalfbandFilter *prevChain = NULL;
	qDebug()<<"Building Decimator chain for sampleRate = "<<_sampleRate<<" max bw = "<<_maxBandWidth;
	while (decimatedSampleRate > minDecimatedSampleRate) {
		//Use cic3 for faster decimation
#if 1
		if(decimatedSampleRate >= (maxBandWidth / cic3->wPass) ) {		//See if can use CIC order 3
			//Passing NULL for coeff sets CIC3
			chain = new HalfbandFilter(cic3);
			qDebug()<<"cic3 = "<<decimatedSampleRate;
#else
		if(decimatedSampleRate >= (maxBandWidth / hb7->wPass) ) {		//See if can use CIC order 3
			chain = new HalfbandFilter(hb7);
			qDebug()<<"hb7 = "<<decimatedSampleRate;
#endif
		} else if(decimatedSampleRate >= (maxBandWidth / hb11->wPass) ) {	//See if can use fixed 11 Tap Halfband
			chain = new HalfbandFilter(hb11);
			qDebug()<<"hb11 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb15->wPass) ) {	//See if can use Halfband 15 Tap
			chain = new HalfbandFilter(hb15);
			qDebug()<<"hb15 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb19->wPass) ) {	//See if can use Halfband 19 Tap
			chain = new HalfbandFilter(hb19);
			qDebug()<<"hb19 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb23->wPass) ) {	//See if can use Halfband 23 Tap
			chain = new HalfbandFilter(hb23);
			qDebug()<<"hb23 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb27->wPass) ) {	//See if can use Halfband 27 Tap
			chain = new HalfbandFilter(hb27);
			qDebug()<<"hb27 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb31->wPass) ) {	//See if can use Halfband 31 Tap
			chain = new HalfbandFilter(hb31);
			qDebug()<<"hb31 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb35->wPass) ) {	//See if can use Halfband 35 Tap
			chain = new HalfbandFilter(hb35);
			qDebug()<<"hb35 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb39->wPass) ) {	//See if can use Halfband 39 Tap
			chain = new HalfbandFilter(hb39);
			qDebug()<<"hb39 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb43->wPass) ) {	//See if can use Halfband 43 Tap
			chain = new HalfbandFilter(hb43);
			qDebug()<<"hb43 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb47->wPass) ) {	//See if can use Halfband 47 Tap
			chain = new HalfbandFilter(hb47);
			qDebug()<<"hb47 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb51->wPass) ) {	//See if can use Halfband 51 Tap
			chain = new HalfbandFilter(hb51);
			qDebug()<<"hb51 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb59->wPass) ) {	//See if can use Halfband 51 Tap
			chain = new HalfbandFilter(hb59);
			qDebug()<<"hb59 = "<<decimatedSampleRate;
		} else {
			qDebug()<<"Ran out of filters before minDecimatedSampleRate";
			break; //out of while
		}

		decimationChain.append(chain);
		//Each chain knows what the next chain needs
		if (prevChain != NULL)
			prevChain->delayBufSizeNextStage = chain->numTaps-1;
		prevChain = chain;
		chain->delayBufSizeNextStage = 0; //Stop flag

		decimatedSampleRate /= 2.0;
	}
	qDebug()<<"Decimated sample rate = "<<decimatedSampleRate;
	return decimatedSampleRate;
}

//Should return CPX* to decimated buffer and number of samples in buffer
quint32 Decimator::process(CPX *_in, CPX *_out, quint32 _numSamples)
{
	quint32 remainingSamples = _numSamples;
	if (useVdsp) {
		//Stride for CPX must be 2 for vDsp legacy reasons
		int cpxStride = 2;
		int splitCpxStride = 1;

		//Leave space for delay buffer in first chain
		quint32 delaySize = decimationChain[0]->numTaps - 1;
		DSPDoubleSplitComplex tmp;
		tmp.realp = &splitComplexIn.realp[delaySize];
		tmp.imagp = &splitComplexIn.imagp[delaySize];
		vDSP_ctozD((DSPDoubleComplex*)_in,cpxStride,&tmp,splitCpxStride,_numSamples);

		HalfbandFilter *chain = NULL;
		for (int i=0; i<decimationChain.length(); i++) {
			chain = decimationChain[i];
			remainingSamples = chain->processVDsp(&splitComplexIn, &splitComplexOut, remainingSamples);
			//Copy out samples to in for next loop.  Inplace decimation not supported for vDsp
			if (chain->delayBufSizeNextStage != 0) {
				vDSP_zvmovD(&splitComplexOut,splitCpxStride,
							&splitComplexIn,splitCpxStride,remainingSamples);
			}
		}
		//Convert back to CPX*, skipping last chain delayBuffer
		tmp.realp = &splitComplexOut.realp[chain->numTaps-1];
		tmp.imagp = &splitComplexOut.imagp[chain->numTaps-1];
		vDSP_ztocD(&tmp,splitCpxStride,(DSPDoubleComplex*)_out,cpxStride,remainingSamples);
	} else {
		HalfbandFilter *chain = NULL;
		for (int i=0; i<decimationChain.length(); i++) {
			chain = decimationChain[i];
			//Decimate in place is ok
			remainingSamples = chain->process(_in, _in, remainingSamples);
		}
		CPX::copyCPX(_out, _in, remainingSamples);

	}
	return remainingSamples;
}

Decimator::HalfbandFilterDesign::HalfbandFilterDesign(quint16 _numTaps, double _wPass, double *_coeff) :
	numTaps(_numTaps), wPass(_wPass), coeff(_coeff)
{
}

Decimator::HalfbandFilter::HalfbandFilter(Decimator::HalfbandFilterDesign *_design) :
	HalfbandFilter(_design->numTaps, _design->wPass, _design->coeff)
{
}

Decimator::HalfbandFilter::HalfbandFilter(quint16 _numTaps, double _wPass, double *_coeff)
{
	numTaps = _numTaps;
	delayBufSize = numTaps - 1;
	wPass = _wPass;
	coeff = _coeff;
	//Big enough for 2x max samples we will handle
	delayBuffer = CPX::memalign(32768);
	CPX::clearCPX(delayBuffer,32678);

	if (_coeff == NULL)
		useCIC3 = true;
	else
		useCIC3 = false;
	xOdd = 0;
	xEven = 0;

	splitComplexAcc.realp = new double;
	splitComplexAcc.imagp = new double;
}

Decimator::HalfbandFilter::~HalfbandFilter()
{
	delete coeff;
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

quint32 Decimator::HalfbandFilter::process(const CPX *_in, CPX *_out, quint32 _numInSamples)
{
	if (useCIC3) {
		return processCIC3(_in, _out, _numInSamples);
	}
	//Testing
	//return processExp(_in, _out, _numInSamples);

	quint32 numInSamples = _numInSamples;
	quint32 numOutSamples = 0;
	const CPX* inBuffer = _in;
	CPX *outBuffer = _out;
	CPX *cpxDelay;
	quint32 midPoint = delayBufSize / 2; //5 for 11 tap filter

	//Not sure what good this does, since output is unchanged?
	if(numInSamples < numTaps)	//safety net to make sure numInSamples is large enough to process
		return numInSamples / 2;

	//global->perform.StartPerformance();
	//Copy input samples into 2nd half of delay buffer starting at position numTaps-1
	//1st half of delay buffer has samples from last call
	//Faster, using memcpy
	CPX::copyCPX(&delayBuffer[delayBufSize], inBuffer, numInSamples);

	//perform decimation FIR filter on even samples
	CPX acc;
	for(int i=0; i<numInSamples; i+=2) {
		//Todo: This looks like a bug in cuteSDR (1.19) that double processes delayBuffer[i]
		// Here and in first [i+j] loop where j=0 and i+j = i
		// cuteSDR 1.0 started j loop with j=2 which didn't have this problem
#if 0
		acc.re = ( delayBuffer[i].re * coeff[0] );
		acc.im = ( delayBuffer[i].im * coeff[0] );
#else
		acc.clear();
#endif
		cpxDelay = &delayBuffer[i]; //Oldest sample
#if 0
		acc = sse_dot(numInSamples,cpxDelay,coeff);
#else
		//Calculate dot-product (MAC)
		//only use even coefficients since odd are zero(except center point)
		for(int j=0; j<numTaps; j+=2) {

			//Fused multiply accumulate is actually slower than simple code
			//acc.re = fma(cpxDelay[j].re, coeff[j], acc.re);
			//acc.im = fma(cpxDelay[j].im, coeff[j], acc.im);
			acc.re += ( cpxDelay[j].re * coeff[j] );
			acc.im += ( cpxDelay[j].im * coeff[j] );
		}
#endif
		//now multiply the center coefficient
		acc.re += ( delayBuffer[i + midPoint].re * coeff[midPoint] );
		acc.im += ( delayBuffer[i + midPoint].im * coeff[midPoint] );
		outBuffer[numOutSamples].re = acc.re;	//put output buffer
		outBuffer[numOutSamples].im = acc.im;	//put output buffer
		numOutSamples++;

	}
	//need to copy last numTapSamples input samples in buffer to beginning of buffer
	// for FIR wrap around management
	CPX::copyCPX(delayBuffer,&inBuffer[numInSamples - delayBufSize], delayBufSize);

	//global->perform.StopPerformance(numInSamples);
	return numOutSamples;
}

//From cuteSDR
//Decimate by 2 CIC 3 stage
// -80dB alias rejection up to Fs * (.5 - .4985)

//Function performs decimate by 2 using polyphase decompostion
// implemetation of a CIC N=3 filter.
// InLength must be an even number
//returns number of output samples processed
quint32 Decimator::HalfbandFilter::processCIC3(const CPX *_in, CPX *_out, quint32 _numInSamples)
{
	quint32 numInSamples = _numInSamples;
	quint32 numOutSamples = 0;
	const CPX* inBuffer = _in;
	CPX *outBuffer = _out;

	CPX even,odd;
	//StartPerformance();
	for(int i=0; i<numInSamples; i+=2)
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

//Experiment using vDsp dot product functions
//_in has empty space at the beginning of the buffer that we use for delay samples
//Actual input samples start at _in[numDelaySamples]
quint32 Decimator::HalfbandFilter::processVDsp(const DSPDoubleSplitComplex *_in,
		DSPDoubleSplitComplex *_out, quint32 _numInSamples)
{
	quint32 numInSamples = _numInSamples;
	quint32 numOutSamples = 0;

	DSPDoubleSplitComplex tmp;
	//Copy delay samples to _in so we have continuous buffer
	//Todo: Switch delayBuffer to splitComplex so we can use vDsp for speed
	for (int i=0; i<delayBufSize; i++) {
		_in->realp[i] = delayBuffer[i].re;
		_in->imagp[i] = delayBuffer[i].im;
	}

	for (int i=0; i<numInSamples; i+=2) {
		//Multiply every other sample in _in (decimate by 2) * every coefficient

		tmp.realp = &_in->realp[i];
		tmp.imagp = &_in->imagp[i];
		//Brute force for all coeff, does not know or take advantage of zero coefficients
		vDSP_zrdotprD(&tmp,1,coeff,1,&splitComplexAcc,numTaps);
		//Leave room in next stage for delay samples
		_out->realp[numOutSamples + delayBufSizeNextStage] = *splitComplexAcc.realp;
		_out->imagp[numOutSamples + delayBufSizeNextStage] = *splitComplexAcc.imagp;
		numOutSamples++;
	}

	//Copy last numDelaySamples to delayBuffer for next iteration
	for (int i=0; i<delayBufSize; i++) {
		delayBuffer[i].re = _in->realp[i + numInSamples];
		delayBuffer[i].im = _in->imagp[i + numInSamples];
	}
	return numOutSamples;
}

//Experimental improvement over cuteSDR code which processes 1st coeff twice?
//Avoids copying entire inbuffer into delay buffer, we just need numTaps delay buffer
//Status: Not filtering correctly and takes longer than just copying in buffer each time
//Todo: Debug just to see what we have wrong in algorithm
/*
	For numTaps = 11
		samples in	samples in
	I	delayBuf	inBuf			midPoint
	---------------------------------------
	0	0,2,4,6,8	0				db[5]
	2	2,4,6,8		0,2				db[7]
	4	4,6,8		0,2,4			db[9]
	6	6,8			0,2,4,6			ib[1]
	8	8			0,2,4,6,8		ib[3]
	10				0,2,4,6,8,10	ib[5]
	12				2,4,6,8,10,12	ib[7]
*/
quint32 Decimator::HalfbandFilter::processExp(const CPX *_in, CPX *_out, quint32 _numInSamples)
{
	quint32 numInSamples = _numInSamples;
	quint32 numOutSamples = 0;
	const CPX* inBuffer = _in;
	CPX *outBuffer = _out;

	//Step 1: Process samples in delay buffer
	//For numTaps = 11, will process delay[0] to delay[8]

	//Step 2: Process samples in input buffer, except for final numTap samples.
	//	Total samples processed will be same as numInSamples

	//Step 3: Copy last numTap input samples to delay buffer
	//global->perform.StartPerformance();
	//Copy input samples into 2nd half of delay buffer starting at position numTaps-1

	//Delay buffer has samples from last call

	//perform decimation FIR filter on even samples
	//Avoid CPX copy from using local CPX and put directly in outBuffer
	CPX acc;
	quint32 delayIndex;
	quint32 midPoint = delayBufSize / 2; //5 for 11 tap filter
	//Process all but the last numTap samples, which get put in delay buffer for next time
	for(int i=0; i<numInSamples ; i+=2) {
		acc.clear();
		//only use even coefficients since odd are zero(except center point)
		for(int j=0; j<numTaps; j+=2) {
			if (i < delayBufSize) {
				//Oldest sample is in delay buffer, so we have to split the process
				if (i+j < delayBufSize) {
					delayIndex = i+j; // Oldest sample in delay buffer
					//qDebug()<<"1-delayBuffer "<<delayIndex;
					//First N samples are in delay buffer
					acc.re += ( delayBuffer[delayIndex].re * coeff[j] );
					acc.im += ( delayBuffer[delayIndex].im * coeff[j] );
				} else {
					//Remaining samples are in inBuffer
					delayIndex = (i+j) - delayBufSize;
					//qDebug()<<"2-inBuffer "<<delayIndex;
					acc.re += ( inBuffer[delayIndex].re * coeff[j] );
					acc.im += ( inBuffer[delayIndex].im * coeff[j] );
				}
			} else {
				//All of the delay samples are in inBuffer, delayBuffer not used
				delayIndex = (i+j) - delayBufSize;
				//qDebug()<<"3-inBuffer "<<delayIndex;
				acc.re += ( inBuffer[delayIndex].re * coeff[j] );
				acc.im += ( inBuffer[delayIndex].im * coeff[j] );
			}
		} //end j<numTaps

		//Add midpoint
		if (i < delayBufSize) {
			//midpoint can be in delayBuffer or inBuffer
			if (i < midPoint) {
				delayIndex = i + midPoint;
				//qDebug()<<"4-mid delayBuffer "<<delayIndex;
				//now multiply the center coefficient
				acc.re += ( delayBuffer[delayIndex].re * coeff[midPoint] );
				acc.im += ( delayBuffer[delayIndex].im * coeff[midPoint] );

			} else {
				delayIndex = i - midPoint;
				//qDebug()<<"5-mid inBuffer "<<delayIndex;
				//now multiply the center coefficient
				acc.re += ( inBuffer[delayIndex].re * coeff[midPoint] );
				acc.im += ( inBuffer[delayIndex].im * coeff[midPoint] );
			}
		} else {
			//midPoint is in inBuffer
			delayIndex = i - midPoint;
			//qDebug()<<"6-mid inBuffer "<<delayIndex;
			//now multiply the center coefficient
			acc.re += ( inBuffer[delayIndex].re * coeff[midPoint] );
			acc.im += ( inBuffer[delayIndex].im * coeff[midPoint] );
		}

		outBuffer[numOutSamples++] = acc;	//put output buffer

	}
	//Copy last numTapSamples from in buffer to delay buffer
	CPX::copyCPX(delayBuffer,&inBuffer[numInSamples - delayBufSize], delayBufSize);

	//global->perform.StopPerformance(numInSamples);
	return numOutSamples;

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
