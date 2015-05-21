#include "medianfilter.h"
#include "QDebug"

MedianFilter::MedianFilter(quint16 _filterSize)
{
	filterSize = _filterSize;
	buffer = new pair[filterSize];
	//Initialize linked list
	for (int i=0; i<filterSize; i++) {
		buffer[i].point = NULL;
		buffer[i].value = 0;
	}

	//head is initialized to location where first value is stored
	pNewData = buffer;

	head.point = &tail; //Smallest
	head.value = 0;

	tail.point = NULL;
	tail.value = 0;

	//Special handling for even sized filters since there is no precise point where exactly half are bigger and half are smaller
	//Defined handling for this is to take the average of the 2 possible median points
	if (filterSize % 2 == 0)
		evenFilterSize = true;
	else
		evenFilterSize = false;

	isValid = false;
	isValidCounter = 0;

	lastNewData = 0;
}

MedianFilter::~MedianFilter()
{
	delete buffer;
}

/*
 * Continuous maintenance of a ring buffer (time order) and sorted linked list (value order)
 * Remove the oldest value from the linked list
 * Merge the new value into the sorted linked list
 *
*/
qint16 MedianFilter::filter(qint16 newData)
{
	pair *pPrev;
	pair *pCur;
	pair *oldDataPoint;

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
		//Handle odd-numbered item in chain
		if( pCur->point == pNewData )
			//Chain out the old node
			pCur->point = oldDataPoint;

		//If newData is >= scanned value, then chain in and we're done with insert
		if( (newData >= pCur->value) ) {
			//The new data node points to what the previous node pointed to
			pNewData->point = pPrev->point;
			//And the previous node points to the new data node
			pPrev->point = pNewData;
			//So pPrev -> pNewData -> pPrev.point
			break; //Don't need to process rest of list
		}

		//Are we at end of linked list?
		if ( pCur == &tail )
			break;

		pPrev = pCur ;
		pCur = pCur->point;
	}
	return median();
}

qint16 MedianFilter::median()
{
	if (!isValid)
		return lastNewData;

	pMedian = head.point;
	//We only need to iterate through half of filter, once we find median node we're done
	for (int i=0; i<filterSize / 2; i++)
		pMedian = pMedian->point;

	if (evenFilterSize) {
		//Average 2 center nodes
		return (pMedian->value + pMedian->point->value) / 2;
	} else {
		//Just return center node
		return pMedian->value;
	}
}

void MedianFilter::test()
{
	MedianFilter m(10);
	m.dump();
	m.filter(4);
	m.dump();
	m.filter(3);
	m.dump();
	m.filter(6);
	m.dump();
	m.filter(7);
	m.dump();
	m.filter(0);
	m.dump();
	m.filter(10);
	m.dump();
	m.filter(6);
	m.dump();
	m.filter(1);
	m.dump();
	m.filter(5);
	m.dump();
	m.filter(4);
	m.dump();
}

void MedianFilter::dump()
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
