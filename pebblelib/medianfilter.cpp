#include "medianfilter.h"
#include "QDebug"

MedianFilter::MedianFilter(quint16 _filterSize)
{
	filterSize = _filterSize;
	buffer = new pair[filterSize];
	for (int i=0; i<filterSize; i++)
		buffer[i] = {NULL,0};
	datpoint = &buffer[0]; //First datum in buffer[0]
	small = {NULL, STOPPER}; //Small marks the end of the sorted list, successor is always NULL
	big = {&small, 0}; //Value field in big is not used
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
qint16 MedianFilter::filter(qint16 datum)
{
	pair *successor;                              /* Pointer to successor of replaced data item */
	pair *scan;                                   /* Pointer used to scan down the sorted list */
	pair *scanold;                                /* Previous value of scan */
	qint16 i;

	if (datum == STOPPER) {
	  datum = STOPPER + 1;                             /* No stoppers allowed. */
	}

	//Replace oldest value in ring buffer with new value and wrap ring buffer pointer if needed
	if ( (++datpoint - &buffer[0]) >= filterSize) {
	  datpoint = &buffer[0];
	}

	//datpoint points to oldest value
	datpoint->value = datum; //Copy in new datum
	successor = datpoint->point; //Save pointer to old value's successor

	median = &big;                                     /* Median initially to first in chain */
	scanold = NULL;                                    /* Scanold initially null. */
	scan = &big;                                       /* Points to pointer to first (largest) datum in chain */

	/* Handle chain-out of first item in chain as special case */
	if (scan->point == datpoint) {
	  scan->point = successor;
	}

	scanold = scan;                                     /* Save this pointer and   */
	scan = scan->point ;                                /* step down chain */

	// Scan the sorted linked list and insert new value in sorted place
	for (i = 0 ; i < filterSize; ++i) {
		//Handle odd-numbered item in chain
		if (scan->point == datpoint) {
			scan->point = successor;                      /* Chain out the old datum.*/
		}
		//If datum is larger than scanned value,
		if (scan->value < datum) {
			datpoint->point = scanold->point;             /* Chain it in here.  */
			scanold->point = datpoint;                    /* Mark it chained in. */
			datum = STOPPER;
		};

		//Step median pointer down chain after doing odd-numbered element
		median = median->point;                       /* Step median pointer.  */
		if (scan == &small) {
			break;                                      /* Break at end of chain  */
		}
		scanold = scan;                               /* Save this pointer and   */
		scan = scan->point;                           /* step down chain */

		//Handle even-numbered item in chain
		if (scan->point == datpoint) {
			scan->point = successor;
		}

		if (scan->value < datum) {
			datpoint->point = scanold->point;
			scanold->point = datpoint;
			datum = STOPPER;
		}

		//Early exit if we get to smallest value before filterSize
		if (scan == &small) {
			break;
		}

		//Next loop
		scanold = scan;
		scan = scan->point;
	}

	return median->value;
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
	m.filter(4);
	m.dump();
	m.filter(4);
	m.dump();
}

void MedianFilter::dump()
{
	pair *ptr = datpoint;
	qDebug() << "Output in time order";
	for (int i=0; i<filterSize; i++) {
		qDebug() << ptr->value;
		if ( (++ptr - &buffer[0]) >= filterSize) {
		  ptr = &buffer[0];
		}
	}
	qDebug() << "Output in sorted order";
	ptr = big.point;
	while (ptr->point != NULL) {
		qDebug() << ptr->value;
		ptr = ptr->point;
	}
	qDebug() << "Median "<<median->value;
}
