//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "string.h"
#include "butterworth.h"

	//Predefined filters
Butterworth::Butterworth(int p,int z,int fc,int fc2,int fs,float g,int t[],float y[])
{
	nPoles = p;
    nZeros = z;
    corner1 = fc;
    corner2 = fc2;
    sampleRate = fs;
    gain = g;
    xTbl = t;
    yTbl = y;
	xv = new float[nZeros + 1];
    yv = new float[nPoles + 1];
}

Butterworth::~Butterworth(void)
{
}


//Todo: Check to make sure xv maps nPoles and xy nZeros, no diff if they are equal
//This can be slightly optimized for a specific set of values, but then not generic for any set
float Butterworth::Filter(float input)
{
    if (nPoles == 0)
        return 0; //Should never get here, but in case

    //Shift array n=n-1
	memcpy(xv, &xv[1], nZeros*sizeof(float));
    //xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5];
    xv[nZeros] = input / gain;

	memcpy(yv, &yv[1], nPoles*sizeof(float));
    //yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5];
    yv[nPoles] = 0;
    for (int i = 0; i < nPoles; i++)
    {
        yv[nPoles] += ((xTbl[i] * xv[i]) + (yTbl[i] * yv[i]));
    }
    //xTbl has one more element than yTbl
    yv[nPoles] += (xTbl[nPoles] * xv[nPoles]);
    return yv[nPoles];
}
