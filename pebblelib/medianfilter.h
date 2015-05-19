#ifndef MEDIANFILTER_H
#define MEDIANFILTER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

//Derived from Median filtering
//Saturday, October 2nd, 2010 by Nigel Jones
// http://embeddedgurus.com/stack-overflow/2010/10/median-filtering/
// http://www.embedded.com/design/programming-languages-and-tools/4399504/Better-Than-Average


class MedianFilter
{
public:
	MedianFilter(quint16 _filterSize);
	~MedianFilter();

	qint16 filter(qint16 datum);

	void test();
	void dump();
private:
	struct pair
	{
	  pair   *point; //Points to next smallest pair in the sorted list
	  qint16  value; //Values to sort
	};
	quint16 filterSize;
	const qint16 STOPPER = 0;  //Smaller than any datum

	pair *buffer; //Ring buffer of nwidth pairs in time order, links maintain sorted order
	pair *datpoint; //Pointer to oldest pair
	pair small; //Chain stopper
	pair big; //Pointer to head (largest) of linked list.
	pair *median;


};

#endif // MEDIANFILTER_H
