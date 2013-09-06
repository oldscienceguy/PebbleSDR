#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
//class IIRFilter
//{
//public:
//	IIRFilter(void);
//	~IIRFilter(void);
//};
class IIRFilter 
{

// IIR filter design code based on a Pascal program
// listed in "Digital Signal Processing with Computer Applications"
// by P. Lynn and W. Fuerst (Prentice Hall)
public:
  static const int LP = 1;
  static const int HP = 2;
  static const int BP = 3;
  static const int BUTTERWORTH = 4;
  static const int CHEBYSHEV = 5;

  IIRFilter(int sampleRate);

  float filter(float f,float *xv, float*yv); //Execute filter
  void filter(CPX &c);
  void filter(CPX *c,int len);
void setPrototype(int p);
int getPrototype();
int getFilterType();
void setFilterType(int ft);
void setOrder(int n);
int getOrder();
void setRate(float r);
float getRate();
void setFreq1(float f);
float getFreq1();
void setFreq2(float f);
float getFreq2();
void setRipple(float r);
float getRipple();
float getPReal(int i);
float getPImag(int i);
float getZero(int i);
float getACoeff(int i);
float getBCoeff(int i);
float sqr(float x);
double sqr(double x);
bool odd(int n);
void design();
//Returns an array[freqPoints] of gain figures for use in graphing filter behavior
//Also normalizes coefficients for 0db gain
float* filterGain();

private:
  int order;
  int prototype, filterType, freqPoints;
  float fp1, fp2, fN, ripple, rate;
  double *pReal;
  double *pImag;
  double *z;
  float *aCoeff;
  float *bCoeff;
  float gain;
  bool ready; //Keeps us from calling filter with an inconsistent set of data
  //Working buffers to execute filter, we need 2 sets for complex. 1 for re, 1 for im
  float *xv1;
	float *yv1;
	float *xv2;
	float *yv2;
  void locatePolesAndZeros(); 
};