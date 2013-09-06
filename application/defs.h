#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

//Very small number that is used in place of zero in cases where we want to avoid divide by zero error
#define ALMOSTZERO 1e-200

//Use where ever possible
#define PI	3.14159265358979323846264338328
#define ONEPI	3.14159265358979323846264338328
#define TWOPI   6.28318530717958647692528676656
#define FOURPI 12.56637061435917295385057353312 

//Used in several places where we need to decrement by one, modulo something
//Defined here because the bit mask logic may be confusing to some
//Equivalent to ((A - 1) % B)
#define DECMOD(A,B) ((A+B) & B)

#if(0)
//min max causes all sorts of grief switching complilers and libraries
//Define locally in any file that needs it
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#endif


