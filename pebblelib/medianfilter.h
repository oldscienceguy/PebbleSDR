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
	qint16 median();

	void test();
	void dump();
private:
	struct pair
	{
	  pair   *point; //Points to next smallest pair in the sorted list
	  qint16  value; //Values to sort
	};
	quint16 filterSize;
	bool evenFilterSize; //Used for special handling for even sized filters
	bool isValid; //Results are valid if we've processed at least filterSize samples
	quint16 isValidCounter; //Counts # entries up to filterSize
	qint16 lastNewData; //Last value inserted, used by median() if !isValid

	pair *buffer; //Ring buffer of nwidth pairs in time order, links maintain sorted order
	pair head; //Head (largest) of linked list.  Fixed node, not in buffer
	pair tail; //Smallest item.  Fixed node, not it buffer

	pair *pNewData; //Pointer to oldest pair, increments in ring buffer order
	pair *pMedian;


};

#endif // MEDIANFILTER_H
