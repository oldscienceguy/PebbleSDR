#ifndef MEDIANFILTER_H
#define MEDIANFILTER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"
#include "QDebug"

//Derived from Median filtering
//Saturday, October 2nd, 2010 by Nigel Jones
// http://embeddedgurus.com/stack-overflow/2010/10/median-filtering/
// http://www.embedded.com/design/programming-languages-and-tools/4399504/Better-Than-Average

/*
 * Median filters for noise reduction are typically 3, 5, 7, or 9 nodes
 * But this call can also be used to track the median power in an FFT sample
 * This is used as the noise floor
 */

template <class templateType> class MedianFilter
{
public:
	MedianFilter(quint16 _filterSize);
	~MedianFilter();

	templateType filter(templateType newData);
	templateType median();

	void test();
	void dump();
private:
	struct pair
	{
	  pair   *point; //Points to next smallest pair in the sorted list
	  templateType  value; //Values to sort
	};
	quint16 filterSize;
	bool evenFilterSize; //Used for special handling for even sized filters
	bool isValid; //Results are valid if we've processed at least filterSize samples
	quint16 isValidCounter; //Counts # entries up to filterSize
	templateType lastNewData; //Last value inserted, used by median() if !isValid

	pair *buffer; //Ring buffer of nwidth pairs in time order, links maintain sorted order
	pair head; //Head (largest) of linked list.  Fixed node, not in buffer
	pair tail; //Smallest item.  Fixed node, not it buffer

	pair *pNewData; //Pointer to oldest pair, increments in ring buffer order
	pair *pMedian;
	bool isPMedianValid;


};

template <class templateType>
MedianFilter<templateType>::MedianFilter(quint16 _filterSize)
{
	filterSize = _filterSize;
	if (filterSize < 3)
		filterSize = 3; //Minimum size where median is defined

	buffer = new pair[filterSize];
	//Initialize linked list
	for (int i=0; i<filterSize; i++) {
		buffer[i].point = NULL;
		buffer[i].value = NAN; //We don't want an actual value to interfere with sort while buffer is populated
	}

	//head is initialized to location where first value is stored
	pNewData = buffer;

	head.point = &tail; //Smallest
	head.value = NAN;

	tail.point = NULL;
	tail.value = NAN;

	//Special handling for even sized filters since there is no precise point where exactly half are bigger and half are smaller
	//Defined handling for this is to take the average of the 2 possible median points
	if (filterSize % 2 == 0)
		evenFilterSize = true;
	else
		evenFilterSize = false;

	isValid = false;
	isValidCounter = 0;

	lastNewData = NAN;
	isPMedianValid = false;
}

template <class templateType>
MedianFilter<templateType>::~MedianFilter()
{
	delete buffer;
}

/*
 * Continuous maintenance of a ring buffer (time order) and sorted linked list (value order)
 * Remove the oldest value from the linked list
 * Merge the new value into the sorted linked list
 *
*/
template <class templateType>
templateType MedianFilter<templateType>::filter(templateType newData)
{
	pair *pPrev;
	pair *pCur;
	pair *oldDataPoint;
	quint16 numChecked = 0;

	if (!isValid && ++isValidCounter == filterSize)
		isValid = true;

	lastNewData = newData;

	//pNewData points to last value we added, increment to point to oldest value
	//Replace oldest value in ring buffer with new value and wrap ring buffer pointer if needed
	if ( (++pNewData - &buffer[0]) >= filterSize) {
		pNewData = &buffer[0]; //Wrap
	}

	pNewData->value = newData; //Copy in new newData
	oldDataPoint = pNewData->point; //Save what replaced node linked to so we can update

	/*
	 * Comments in re-linking refer to the following example
	 * Pre-Insertion			New max A'		New B'			New Min C'
	 *				head.point	head.point		head.point		head.point
	 *				|			|				|				|
	 *				A			A'				A'				A'
	 *				|			|				|				|
	 *				|			A				A				A
	 *				|			|				|				|
	 *				B			B				B'				B'
	 *				|			|				|				|
	 *				|			|				B				B
	 *				|			|				|				|
	 *				C			C				C				C
	 *				|			|				|				|
	 *				|			|				|				C'
	 *				|			|				|				|
	 *				tail.point	tail.point		tail.point		tail.point
	 *				|			|				|				|
	 *				NULL		NULL			NULL			NULL
	 */
	//Median initially points to first in chain
	pMedian = &head;
	isPMedianValid = false; //Will be set to true if we reach midpoint during our insert

	pPrev = NULL;
	//Points to pointer to first (largest) newData in chain
	pCur = &head;

	//If the node (pCur) being checked is linked to the node that was replaced (pNewData)
	//Then the pCur is set to the replaced node's linking pointer originally was (oldDataPoint)
	//Ex: A->B->C, pCur == A, B was replaced (pNewData), then A->C and B is linked elsewhere
	//Handle special case of pBiggest being replaced, new value may be bigger or smaller
	if( pCur->point == pNewData )
		//Chain out the old node
		pCur->point = oldDataPoint;

	pPrev = pCur;
	//Next node
	pCur = pCur->point ;

	//iterate through the chain, normal loop exit via break
	for(int i=0; i<filterSize; i++ ) {
		numChecked++;
		//Handle odd-numbered item in chain
		if( pCur->point == pNewData )
			//Chain out the old node
			pCur->point = oldDataPoint;

		//If newData is >= scanned value, then chain in and we're done with insert
		//Special case for buffer elements that are being populated for the first time
		//These are initialized with NAN which can be tested by isnan()
		if( isnan(pCur->value) || newData >= pCur->value) {
			//The new data node points to what the previous node pointed to
			pNewData->point = pPrev->point;
			//And the previous node points to the new data node
			pPrev->point = pNewData;
			//So pPrev -> pNewData -> pPrev.point
			break; //Don't need to process rest of list
		}

		pMedian = pMedian->point; //Step pointer
		//Have we reached mid point yet?
		if (numChecked > filterSize / 2)
			isPMedianValid == true;

		//Are we at end of linked list?
		if ( pCur == &tail )
			break;

		pPrev = pCur ;
		pCur = pCur->point;
	}
	return median();
}

template <class templateType>
templateType MedianFilter<templateType>::median()
{
	if (!isValid)
		return lastNewData;

	pMedian = head.point;
	//We only need to iterate through half of filter, once we find median node we're done

	if (evenFilterSize) {
		//Average 2 center nodes
		if (!isPMedianValid)
			//We only have to iterate if the last insert didn't process and least half nodes
			for (int i=0; i<(filterSize / 2) -1 ; i++)
				pMedian = pMedian->point;
		//pMedian points to the first of the 2 nodes to be averaged
		return (pMedian->value + pMedian->point->value) / 2;
	} else {
		//Just return center node
		if (!isPMedianValid)
			//We only have to iterate if the last insert didn't process and least half nodes
			for (int i=0; i<filterSize / 2 ; i++)
				pMedian = pMedian->point;
		return pMedian->value;
	}
}

template <class templateType>
void MedianFilter<templateType>::test()
{
	templateType v;
	templateType maxValue;
	templateType minValue;
	//Test Complex values
	//maxValue = 1; minValue = -1;
	maxValue = 100; minValue = 0;
	for (int i=0; i<filterSize; i++) {
		//Will generate templateType relevant data in the range of minValue to maxValu
		v = maxValue + (rand() / ( RAND_MAX / (minValue - maxValue) ) );
		filter(v);
		dump();
	}
}

template <class templateType>
void MedianFilter<templateType>::dump()
{
	pair *ptr;
	if (isValid)
		qDebug()<<"Valid";
	else
		qDebug()<<"Not valid";
	qDebug() <<"head "<<head.point<<" "<<head.value;
	qDebug() << "Median "<< median();
	qDebug() <<"pNewData "<<pNewData<<" "<<pNewData->value;

	qDebug() << "Output in array order";
	for (int i=0; i<filterSize; i++) {
		qDebug() << &buffer[i] << " " <<buffer[i].value;
	}

	qDebug() << "Output in time order";
	ptr = pNewData;
	for (int i=0; i<filterSize; i++) {
		qDebug() << ptr << " " <<ptr->value;
		if ( (++ptr - &buffer[0]) >= filterSize) {
		  ptr = &buffer[0];
		}
	}
	qDebug() << "Output in sorted order";
	ptr = head.point;
	while (ptr != &tail) {
		qDebug() << ptr << ptr->value;
		ptr = ptr->point;
	}
}

#endif // MEDIANFILTER_H
