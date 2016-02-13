#include "decimator.h"

Decimator::Decimator(quint32 _sampleRate, quint32 _bufferSize) :
	ProcessStep(_sampleRate, _bufferSize)
{
	decimatedSampleRate = _sampleRate;
	initFilters();
}

Decimator::~Decimator()
{
	deleteFilters();
}


quint32 Decimator::buildDecimationChain(quint32 _sampleRate, quint32 _maxBandWidth)
{
	decimatedSampleRate = _sampleRate;
	maxBandWidth = _maxBandWidth;

	qDebug()<<"Building Decimator chain for sampleRate = "<<_sampleRate<<" max bw = "<<_maxBandWidth;
	while (decimatedSampleRate > minDecimatedSampleRate) {
		if(decimatedSampleRate >= (maxBandWidth / hb7->wPass) ) {		//See if can use CIC order 3
			decimationChain.append(hb7);
			qDebug()<<"hb7 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb11->wPass) ) {	//See if can use fixed 11 Tap Halfband
			decimationChain.append(hb11);
			qDebug()<<"hb11 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb15->wPass) ) {	//See if can use Halfband 15 Tap
			decimationChain.append(hb15);
			qDebug()<<"hb15 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb19->wPass) ) {	//See if can use Halfband 19 Tap
			decimationChain.append(hb19);
			qDebug()<<"hb19 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb23->wPass) ) {	//See if can use Halfband 23 Tap
			decimationChain.append(hb23);
			qDebug()<<"hb23 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb27->wPass) ) {	//See if can use Halfband 27 Tap
			decimationChain.append(hb27);
			qDebug()<<"hb27 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb31->wPass) ) {	//See if can use Halfband 31 Tap
			decimationChain.append(hb31);
			qDebug()<<"hb31 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb35->wPass) ) {	//See if can use Halfband 35 Tap
			decimationChain.append(hb35);
			qDebug()<<"hb35 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb39->wPass) ) {	//See if can use Halfband 39 Tap
			decimationChain.append(hb39);
			qDebug()<<"hb39 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb43->wPass) ) {	//See if can use Halfband 43 Tap
			decimationChain.append(hb43);
			qDebug()<<"hb43 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb47->wPass) ) {	//See if can use Halfband 47 Tap
			decimationChain.append(hb47);
			qDebug()<<"hb47 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb51->wPass) ) {	//See if can use Halfband 51 Tap
			decimationChain.append(hb51);
			qDebug()<<"hb51 = "<<decimatedSampleRate;
		} else if(decimatedSampleRate >= (maxBandWidth / hb59->wPass) ) {	//See if can use Halfband 51 Tap
			decimationChain.append(hb59);
			qDebug()<<"hb59 = "<<decimatedSampleRate;
		} else {
			qDebug()<<"Ran out of filters before minDecimatedSampleRate";
			break; //out of while
		}
		decimatedSampleRate /= 2.0;
	}
	qDebug()<<"Decimated sample rate = "<<decimatedSampleRate;
	return decimatedSampleRate;
}

//Should return CPX* to decimated buffer and number of samples in buffer
quint32 Decimator::process(CPX *_in, CPX *_out, quint32 _numSamples)
{
	CPX *next = _in;
	quint32 remainingSamples = _numSamples;
	for (int i=0; i<decimationChain.length(); i++) {
		remainingSamples = decimationChain[i]->process(next, _out, remainingSamples);
		next = _out;
	}

	return remainingSamples;
}

Decimator::HalfbandFilter::HalfbandFilter(quint16 _numTaps, float _wPass, double *_coeff)
{
	numTaps = _numTaps;
	wPass = _wPass;
	coeff = _coeff;
	//Big enough for 2x max samples we will handle
	delayBuffer = CPX::memalign(32768);
}

Decimator::HalfbandFilter::~HalfbandFilter()
{
	delete coeff;
}

//From cuteSdr, refactored and modified for Pebble
quint32 Decimator::HalfbandFilter::process(CPX *_in, CPX *_out, quint32 _numInSamples)
{
	quint32 numInSamples = _numInSamples;
	quint32 numOutSamples = 0;
	CPX* inBuffer = _in;
	CPX *outBuffer = _out;

	if(numInSamples < numTaps)	//safety net to make sure numInSamples is large enough to process
		return numInSamples / 2;

	//StartPerformance();
	//Copy input samples into 2nd half of delay buffer starting at position numTaps-1
	//1st half of delay buffer has samples from last call
	for(int i=0,j = numTaps - 1; i<numInSamples; i++, j++)
		delayBuffer[j] = inBuffer[i];

	//perform decimation FIR filter on even samples
	CPX acc;
	for(int i=0; i<numInSamples; i+=2) {
		acc.re = ( delayBuffer[i].re * coeff[0] );
		acc.im = ( delayBuffer[i].im * coeff[0] );
		//Fix from cuteSdr 1.10
		for(int j=0; j<numTaps; j+=2)	//only use even coefficients since odd are zero(except center point)
		{
			acc.re += ( delayBuffer[i+j].re * coeff[j] );
			acc.im += ( delayBuffer[i+j].im * coeff[j] );
		}
		//now multiply the center coefficient
		acc.re += ( delayBuffer[i+(numTaps-1)/2].re * coeff[(numTaps-1)/2] );
		acc.im += ( delayBuffer[i+(numTaps-1)/2].im * coeff[(numTaps-1)/2] );
		outBuffer[numOutSamples++] = acc;	//put output buffer

	}
	//need to copy last numInSamples - 1 input samples in buffer to beginning of buffer
	// for FIR wrap around management
	for(int i=0,j = numInSamples-numTaps+1; i<numTaps - 1; i++)
		delayBuffer[i] = inBuffer[j++];

	//StopPerformance(numInSamples);
	return numOutSamples;
}

//Halfband decimating filters are generated in Matlab with order = numTaps-1 and wPass
//Matlab saves to .h file where we copy and paste below
//Doubles are only 15 decimal place precision, so Matlab has more precise data and slightly better modeling
void Decimator::initFilters()
{
	double *coeff;

	coeff = new double[7] {
			-0.03125208195057,                 0,   0.2812520818582,               0.5,
			  0.2812520818582,                 0, -0.03125208195057
	};
	hb7 = new HalfbandFilter(7, 0.0030, coeff);

	coeff = new double[11] {
	  //0.0060431029837374152,
		0.006043102983737,                 0, -0.04937251545876,                 0,
		  0.2933294495205,               0.5,   0.2933294495205,                 0,
		-0.04937251545876,                 0, 0.006043102983737
	};
	hb11 = new HalfbandFilter(11, 0.0500, coeff);

	coeff = new double[15] {
		//  -0.001442203300285281,
			-0.001442203300197,                 0,  0.01301751280274,                 0,
			 -0.06165327860496,                 0,   0.3000779231603,               0.5,
			   0.3000779231603,                 0, -0.06165327860496,                 0,
			  0.01301751280274,                 0,-0.001442203300197
	};
	hb15 = new HalfbandFilter(15, 0.0980, coeff);

	coeff = new double[19] {
		//  0.00042366527106480427,
			0.0004236652712401,                 0, -0.00407173333702,                 0,
				0.019895653882,                 0, -0.07074003441233,                 0,
			   0.3044924977284,               0.5,   0.3044924977284,                 0,
			 -0.07074003441233,                 0,    0.019895653882,                 0,
			 -0.00407173333702,                 0,0.0004236652712401
	};
	hb19 = new HalfbandFilter(19, 0.1434, coeff);

	coeff = new double[23] {
		//  -0.00014987651418332164,
			-0.0001498765142888,                 0, 0.001474863328452,                 0,
			-0.007441694499077,                 0,  0.02616352273205,                 0,
			 -0.07759369911661,                 0,   0.3075468371979,               0.5,
			   0.3075468371979,                 0, -0.07759369911661,                 0,
			  0.02616352273205,                 0,-0.007441694499077,                 0,
			 0.001474863328452,                 0,-0.0001498765142888
	};
	hb23 = new HalfbandFilter(23, 0.1820, coeff);

	coeff = new double[27] {
		//  0.000063730426952664685,
			6.373042719017e-05,                 0,-0.0006198519400048,                 0,
			 0.003151250478532,                 0, -0.01117315134303,                 0,
			  0.03171888754407,                 0, -0.08291786358287,                 0,
			   0.3097770473567,               0.5,   0.3097770473567,                 0,
			 -0.08291786358287,                 0,  0.03171888754407,                 0,
			 -0.01117315134303,                 0, 0.003151250478532,                 0,
			-0.0006198519400048,                 0,6.373042719017e-05
	};
	hb27 = new HalfbandFilter(27, 0.2160, coeff);

	coeff = new double[31] {
		//  -0.000030957335326552226,
			-3.095733538676e-05,                 0, 0.000292719928498,                 0,
			-0.001477038112406,                 0, 0.005253908899031,                 0,
			 -0.01485637874838,                 0,  0.03640665191945,                 0,
			 -0.08699862567944,                 0,   0.3114096707604,               0.5,
			   0.3114096707604,                 0, -0.08699862567944,                 0,
			  0.03640665191945,                 0, -0.01485637874838,                 0,
			 0.005253908899031,                 0,-0.001477038112406,                 0,
			 0.000292719928498,                 0,-3.095733538676e-05
	};
	hb31 = new HalfbandFilter(31, 0.2440, coeff);

	coeff = new double[35] {
		//  0.000017017718072971716,
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
	hb35 = new HalfbandFilter(35, 0.2680, coeff);

	coeff = new double[39] {
		//  -0.000010175082832074367,
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
	hb39 = new HalfbandFilter(39, 0.2880, coeff);

	coeff = new double[43] {
		//  0.0000067666739082756387,
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
	hb43 = new HalfbandFilter(43, 0.3060, coeff);

	coeff = new double[47] {
		//  -0.0000045298314172004251,
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
	hb47 = new HalfbandFilter(47, 0.3200, coeff);

	coeff = new double[51] {
		//  0.0000033359253688981639,
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
	hb51 = new HalfbandFilter(51, 0.3332, coeff);

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
	hb59 = new HalfbandFilter(59, 0.400, coeff);
}

void Decimator::deleteFilters()
{
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
}
