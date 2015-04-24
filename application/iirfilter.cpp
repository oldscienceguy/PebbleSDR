//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "iirfilter.h"
#include <string.h>
#include <QtDebug>
#include <limits>
#include "cmath" //For std::abs() that supports float.  Include last so it overrides abs(int)

void IIRFilter::locatePolesAndZeros(){
    // determines poles and zeros of IIR filter
    // based on bilinear transform method
    pReal  = new double[order + 1];
    pImag  = new double[order + 1];
    z      = new double[order + 1];
    double ln10 = log(10.0);
    for(int k = 1; k <= order; k++) {
      pReal[k] = 0.0;
      pImag[k] = 0.0;
    }
    // Butterworth, Chebyshev parameters
    int n = order;
    if (filterType == BP) n = n/2;
    int ir = n % 2;
    int n1 = n + ir;
    int n2 = (3*n + ir)/2 - 1;
    double f1;
    switch (filterType) {
      case LP: f1 = fp2;       break;
      case HP: f1 = fN - fp1;  break;
      case BP: f1 = fp2 - fp1; break;
      default: f1 = 0.0;
    }
    double tanw1 = tan(0.5*M_PI*f1/fN);
    double tansqw1 = sqr(tanw1);
    // Real and Imaginary parts of low-pass poles
    double t, a = 1.0, r = 1.0, i = 1.0;
	double b3;
    for (int k = n1; k <= n2; k++) {
      t = 0.5*(2*k + 1 - ir)*M_PI/(double)n;
      switch (prototype) {
        case BUTTERWORTH:
          b3 = 1.0 - 2.0*tanw1*cos(t) + tansqw1;
          r = (1.0 - tansqw1)/b3;
          i = 2.0*tanw1*sin(t)/b3;
          break;
        case CHEBYSHEV:
		
          double d = 1.0 - exp(-0.05*ripple*ln10);
          double e = 1.0 / sqrt(1.0 / sqr(1.0 - d) - 1.0);
          double x = pow(sqrt(e*e + 1.0) + e, 1.0/(double)n);
          a = 0.5*(x - 1.0/x);
          double b = 0.5*(x + 1.0/x);
          double c3 = a*tanw1*cos(t);
          double c4 = b*tanw1*sin(t);
          double c5 = sqr(1.0 - c3) + sqr(c4);
          r = 2.0*(1.0 - c3)/c5 - 1.0;
          i = 2.0*c4/c5;
          break;
      }
      int m = 2*(n2 - k) + 1;
      pReal[m + ir]     = r;
	  pImag[m + ir]     = std::abs(i);
      pReal[m + ir + 1] = r;
	  pImag[m + ir + 1] = - std::abs(i);
    }
    if (odd(n)) {
      if (prototype == BUTTERWORTH) r = (1.0 - tansqw1)/(1.0 + 2.0*tanw1+tansqw1);
      if (prototype == CHEBYSHEV)   r = 2.0/(1.0 + a*tanw1) - 1.0;
      pReal[1] = r;
      pImag[1] = 0.0;
    }
	double aa;
	double f4;
	double f5;
    switch (filterType) {
      case LP:
        for (int m = 1; m <= n; m++)
          z[m]= -1.0;
        break;
      case HP:
        // low-pass to high-pass transformation
        for (int m = 1; m <= n; m++) {
          pReal[m] = -pReal[m];
          z[m]     = 1.0;
        }
        break;
      case BP:
        // low-pass to bandpass transformation
        for (int m = 1; m <= n; m++) {
          z[m]  =  1.0;
          z[m+n]= -1.0;
        }
        f4 = 0.5*M_PI*fp1/fN;
        f5 = 0.5*M_PI*fp2/fN;
        /*
        check this bit ... needs value for gp to adjust critical freqs
        if (prototype == BUTTERWORTH) {
          f4 = f4/Math.exp(0.5*Math.log(gp)/n);
          f5 = fN - (fN - f5)/Math.exp(0.5*Math.log(gp)/n);
        }
        */
        aa = cos(f4 + f5)/cos(f5 - f4);
        double aR, aI, h1, h2, p1R, p2R, p1I, p2I;
        for (int m1 = 0; m1 <= (order - 1)/2; m1++) {
          int m = 1 + 2*m1;
          aR = pReal[m];
          aI = pImag[m];
		  if (std::abs(aI) < 0.0001) {
            h1 = 0.5*aa*(1.0 + aR);
            h2 = sqr(h1) - aR;
            if (h2 > 0.0) {
              p1R = h1 + sqrt(h2);
              p2R = h1 - sqrt(h2);
              p1I = 0.0;
              p2I = 0.0;
            }
            else {
              p1R = h1;
              p2R = h1;
			  p1I = sqrt(std::abs(h2));
              p2I = -p1I;
            }
          }
          else {
            double fR = aa*0.5*(1.0 + aR);
            double fI = aa*0.5*aI;
            double gR = sqr(fR) - sqr(fI) - aR;
            double gI = 2*fR*fI - aI;
			double sR = sqrt(0.5*std::abs(gR + sqrt(sqr(gR) + sqr(gI))));
            double sI = gI/(2.0*sR);
            p1R = fR + sR;
            p1I = fI + sI;
            p2R = fR - sR;
            p2I = fI - sI;
          }
          pReal[m]   = p1R;
          pReal[m+1] = p2R;
          pImag[m]   = p1I;
          pImag[m+1] = p2I;
        } // end of m1 for-loop
        if (odd(n)) {
          pReal[2] = pReal[n+1];
          pImag[2] = pImag[n+1];
        }
        for (int k = n; k >= 1; k--) {
          int m = 2*k - 1;
          pReal[m]   =   pReal[k];
          pReal[m+1] =   pReal[k];
		  pImag[m]   =   std::abs(pImag[k]);
		  pImag[m+1] = - std::abs(pImag[k]);
        }
        break;
      default:
		  break;
    }
  }


  IIRFilter::IIRFilter(int sampleRate) {
    // initial (default) settings
    prototype = BUTTERWORTH;
    filterType = LP;
    order = 1;
    ripple = 1.0f;
    rate = sampleRate;
    fN = 0.5f*rate;
    fp1 = 0.0f;
    fp2 = 1000.0f;
	freqPoints=250;
	ready = false;
	xv1 = NULL;
    yv1 = NULL;
	xv2 = NULL;
    yv2 = NULL;
	//8pole 50-3050 1.069763695e+03
	//4 pole 0-3000 1.071238871e+03
	//Gain is critical and must be matched to coefficients.  Otherwise filter generates infinity values
	gain = 1.071238871e+03; //Hack until we can figure out how to calc this
  }

  void IIRFilter::setPrototype(int p) {
    prototype = p;
	ready = false;
  }

  int IIRFilter::getPrototype() {
    return prototype;
  }

  int IIRFilter::getFilterType() {
    return filterType;
  }

  void IIRFilter::setFilterType(int ft) {
    filterType = ft;
    if ((filterType == BP) && odd(order)) order++;
	ready = false;
  }

  void IIRFilter::setOrder(int n) {
    order = abs(n);
    if ((filterType == BP) && odd(order)) order++;
	ready = false;
  }

  int IIRFilter::getOrder() {
    return order;
  }

  void IIRFilter::setRate(float r) {
    rate = r;
    fN = 0.5f * rate;
	ready = false;
  }

  float IIRFilter::getRate() {
    return rate;
  }

  void IIRFilter::setFreq1(float f) {
    fp1 = f;
	ready = false;
  }

  float IIRFilter::getFreq1() {
    return fp1;
  }

  void IIRFilter::setFreq2(float f) {
    fp2 = f;
	ready = false;
  }

  float IIRFilter::getFreq2() {
    return fp2;
  }

  void IIRFilter::setRipple(float r) {
    ripple = r;
	ready = false;
  }

  float IIRFilter::getRipple() {
    return ripple;
  }

  float IIRFilter::getPReal(int i) {
    // returns real part of filter pole with index i
    return (float)pReal[i];
  }

  float IIRFilter::getPImag(int i) {
    // returns imaginary part of filter pole with index i
    return (float)pImag[i];
  }

  float IIRFilter::getZero(int i) {
    // returns filter zero with index i
    return (float)z[i];
  }

  float IIRFilter::getACoeff(int i) {
    // returns IIR filter numerator coefficient with index i
    return (float)aCoeff[i];
  }

  float IIRFilter::getBCoeff(int i) {
    // returns IIR filter denominator coefficient with index i
    return (float)bCoeff[i];
  }

  float IIRFilter::sqr(float x) {
    return x * x;
  }

  double IIRFilter::sqr(double x) {
    return x * x;
  }

  bool IIRFilter::odd(int n) {
    // returns true if n odd
    return (n % 2) != 0;
  }


  //todo: change to float from double to get same precision as York
  void IIRFilter::design() {
  //Working memory for filter()
	  //Init here because order has been set
	xv1 = new float[order + 1];
    yv1 = new float[order + 1];
	xv2 = new float[order + 1];
    yv2 = new float[order + 1];
    aCoeff = new float[order + 1];
    bCoeff = new float[order + 1];
    float *newA = new float[order + 1];
    float *newB = new float[order + 1];
    locatePolesAndZeros(); // find filter poles and zeros
    // compute filter coefficients from pole/zero values
    for (int i = 0; i <= order; i++) {
      aCoeff[i] = 0.0;
      bCoeff[i] = 0.0;
	  xv1[i]=0;
	  xv2[i]=0;
	  yv1[i]=0;
	  yv2[i]=0;
    }
     aCoeff[0]= 1.0;
    bCoeff[0]= 1.0;
   int k = 0;
    int n = order;
    int pairs = n/2;
    if (odd(order)) {
     // first subfilter is first order
      aCoeff[1] = - z[1];
      bCoeff[1] = - pReal[1];
      k = 1;
    }
    for (int p = 1; p <= pairs; p++) {
      int m = 2*p - 1 + k;
      double alpha1 = - (z[m] + z[m+1]);
      double alpha2 = z[m]*z[m+1];
      double beta1  = - 2.0*pReal[m];
      double beta2  = sqr(pReal[m]) + sqr(pImag[m]);
      newA[1] = aCoeff[1] + alpha1*aCoeff[0];
      newB[1] = bCoeff[1] + beta1 *bCoeff[0];
      for (int i = 2; i <= n; i++) {
        newA[i] = aCoeff[i] + alpha1*aCoeff[i-1] + alpha2*aCoeff[i-2];
        newB[i] = bCoeff[i] + beta1 *bCoeff[i-1] + beta2 *bCoeff[i-2];
      }
      for (int i = 1; i <= n; i++) {
        aCoeff[i] = newA[i];
        bCoeff[i] = newB[i];
      }
    }
	//RL: Make table look like York, reverse order and invert sign
	float *tmp = new float[order+1];
	for (int i=0; i<=order; i++)
		tmp[i] = -bCoeff[order-i];
	bCoeff = tmp;

	ready=true; //Ok to call filter()
	//Testing
	if (false)
	{
		
		for (int i=0; i<=order; i++)
		{
			qDebug()<<"aCoeff:["<<i<<"] = "<<aCoeff[i]<<" : "<<"bCoeff:["<<i<<"] = "<<bCoeff[i];
		}
	}
  }


  //Calc filter gain at uniform intervals and normalize coefficients so we don't have to factor gain
  //RL: Doesn't work, filters only work when we add York gain
  float *IIRFilter::filterGain() {
    // filter gain at uniform frequency intervals
    float *g = new float[freqPoints+1];
    double theta, s, c, sac, sas, sbc, sbs;
    float gMax = -100.0f;
    float sc = 10.0f/(float)log(10.0f);
    double t = M_PI / freqPoints;
    for (int i = 0; i <= freqPoints; i++) {
      theta = i*t;
      if (i == 0) theta = M_PI*0.0001;
      if (i == freqPoints) theta = M_PI*0.9999;
      sac = 0.0f;
      sas = 0.0f;
      sbc = 0.0f;
      sbs = 0.0f;
      for (int k = 0; k <= order; k++) {
        c = cos(k*theta);
        s = sin(k*theta);
        sac += c*aCoeff[k];
        sas += s*aCoeff[k];
        sbc += c*bCoeff[k];
        sbs += s*bCoeff[k];
      }
      g[i] = sc*(float)log((sqr(sac) + sqr(sas))/(sqr(sbc) + sqr(sbs)));
	  gMax = gMax>g[i]?gMax: g[i];
    }
    // normalise to 0 dB maximum gain
    for (int i=0; i<=freqPoints; i++) g[i] -= gMax;
    // normalise numerator (a) coefficients
	//float normFactor = (float)pow(10.0, -0.05*gMax);
	//RL: We don't need to normalize for York filter algorithm.
    //for (int i=0; i<=order; i++) aCoeff[i] *= normFactor;
    return g;
  }

  void IIRFilter::filter(CPX *cpx,int len)
  {
	  for (int i=0; i<len; i++)
		  filter(cpx[i]);

  }

  void IIRFilter::filter(CPX &c)
  {
	  if (!ready)
		  return;
	  c.re = filter(c.re,xv1,yv1);
	  c.im = filter(c.im,xv2,yv2);
  }
  float IIRFilter::filter(float input, float *xv, float*yv)
{
	if (!ready)
		return 0;
    //Shift array n=n+1.  This leaves us with xv[order] open for a new value
	memcpy(&xv[0], &xv[1], order*sizeof(float));
   xv[order] = input / gain;
    //Same for yv
	memcpy(&yv[0], &yv[1], order*sizeof(float));
	//Calc new yv[order] value, save in yv array, and return it as filtered value
    yv[order] = 0;
	//For order N, we have N+1 elements in array.  Last N values and most recent
    for (int i = 0; i < order; i++)
    {
        yv[order] += (aCoeff[i] * xv[i]);
		yv[order] += (bCoeff[i] * yv[i]);
    }
	yv[order] += (aCoeff[order] * xv[order]);
    return yv[order];
}


