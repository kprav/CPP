// list.cc 
//
//     	Routines to manage a singly-linked list of "things".
//
// 	A "ListElement" is allocated for each item to be put on the
//	list; it is de-allocated when the item is removed. This means
//      we don't need to keep a "next" pointer in every object we
//      want to put on a list.
// 
//     	NOTE: Mutual exclusion must be provided by the caller.
//  	If you want a synchronized list, you must use the routines 
//	in synchlist.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "list.h"
#include "synch.h"
extern "C" { int bcopy(char *, char *, int); };

timeCounterClass::timeCounterClass()
{
	t1 = 0;
	t2 = 0;
}

timeCounterClass::timeCounterClass(int t)
{
	t1 = 0;
	t2 = t;
}

timeCounterClass::timeCounterClass(int t11, int t21)
{
	t1 = t11;
	t2 = t21;
}

// -------------------------------------------------------------------------------------------------------------------
// This is the constructor of the replyType class.
// -------------------------------------------------------------------------------------------------------------------
replyType::replyType(int i, int j, char *a)
{
	inPktHdr_from = i;		//From machine id
	inMailHdr_from = j;		//Reply-to mail box number
	bcopy(a,ack, MaxMailSize2); //copy the message from a to ack.
	machineID = i;			// machine id is same as the above inPktHdr_from
}


//----------------------------------------------------------------------
// ListElement::ListElement
// 	Initialize a list element, so it can be added somewhere on a list.
//
//	"itemPtr" is the item to be put on the list.  It can be a pointer
//		to anything.
//	"sortKey" is the priority of the item, if any.
//----------------------------------------------------------------------

ListElement::ListElement(void *itemPtr, int sortKey)
{
     item = itemPtr;
     key = sortKey;
     next = NULL;	// assume we'll put it at the end of the list 
}

//----------------------------------------------------------------------
// List::List
//	Initialize a list, empty to start with.
//	Elements can now be added to the list.
//----------------------------------------------------------------------

List::List()
{ 
    first = last = NULL; 
}

//----------------------------------------------------------------------
// List::~List
//	Prepare a list for deallocation.  If the list still contains any 
//	ListElements, de-allocate them.  However, note that we do *not*
//	de-allocate the "items" on the list -- this module allocates
//	and de-allocates the ListElements to keep track of each item,
//	but a given item may be on multiple lists, so we can't
//	de-allocate them here.
//----------------------------------------------------------------------

List::~List()
{ 
    while (Remove() != NULL)
	;	 // delete all the list elements
}

//----------------------------------------------------------------------
// List::Append
//      Append an "item" to the end of the list.
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the end.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void
List::Append(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it after last
	last->next = element;
	last = element;
    }
}

//----------------------------------------------------------------------
// List::Prepend
//      Put an "item" on the front of the list.
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the beginning.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void
List::Prepend(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it before first
	element->next = first;
	first = element;
    }
}

//----------------------------------------------------------------------
// List::Remove
//      Remove the first "item" from the front of the list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//----------------------------------------------------------------------

void *
List::Remove()
{
    return SortedRemove(NULL);  // Same as SortedRemove, but ignore the key
}

waitingQueueMember::waitingQueueMember(char *pkt, timeCounterClass t, int f, bool sendack)
{
	bcopy(pkt, packet, MaxMailSize2);
	time = t;
	whichServer = f;
	sendAckValid = sendack;
}

int isTimeGreater(timeCounterClass tS1, timeCounterClass tS2)
{
//	printf("In Funtion: %d | %d ||| %d | %d\n", tS1.t1, tS1.t2, tS2.t1, tS2.t2);
	if(tS1.t1>tS2.t1)
		return 1;
	else if(tS1.t1<tS2.t1)
		return 0;
	else if(tS1.t2 > tS2.t2) 
		return 1;
	else if(tS1.t2 == tS2.t2)
		return 2; //Tie!!
	else
		return 0;
}

void *
List::Search(timeCounterClass givenTime)
{
	ListElement *thisElement, *thisBefore, *minElement, *minBefore;
	waitingQueueMember *minMember, *thisMember;
	int g;

	if(first==NULL)
	{
		printf("List empty\n");
		return NULL;
	}
	
	if(first == last)
	{
		minElement = first;
		minMember = (waitingQueueMember *)minElement->item;
		printf("QUEUE : %s - (%d, %d) - %d \n", minMember->packet+19, minMember->time.t1, minMember->time.t2, minMember->whichServer);
		g = isTimeGreater(minMember->time, givenTime);
		if((g==0)||(g==2))
		{
			first = NULL;
			last = NULL;
			printf("returning : %s - (%d, %d) - %d \n", minMember->packet+19, minMember->time.t1, minMember->time.t2, minMember->whichServer);
			return ((void *)minMember);
		}
		else
			return NULL;
	}
	else
	{
		minElement = first;
		thisElement = first;
		thisMember = (waitingQueueMember *)thisElement->item;
		printf("QUEUE : %s - (%d, %d) - %d | ", thisMember->packet+19, thisMember->time.t1, thisMember->time.t2, thisMember->whichServer);
		while(1)
		{
			thisBefore = thisElement;
			thisElement = thisElement->next;

			thisMember = (waitingQueueMember *)thisElement->item;
			minMember =  (waitingQueueMember *)minElement->item;
			printf("%s - (%d, %d) - %d | ", thisMember->packet+19, thisMember->time.t1, thisMember->time.t2, thisMember->whichServer);
			if(isTimeGreater(thisMember->time, minMember->time)==0)
			{
				minElement = thisElement;
				minBefore = thisBefore;
			}
			else if(isTimeGreater(thisMember->time, minMember->time)==2)
			{
				if(thisMember->whichServer < minMember->whichServer)
				{
					minElement = thisElement;
					minBefore = thisBefore;
				}
			}

			if(thisElement == last)
				break;
		}
		
		minMember = (waitingQueueMember *)minElement->item;
		g = isTimeGreater(minMember->time, givenTime);
		if((g==0)||(g==2))
		{
			if(minElement == first)
			{
				first = first->next;
			}
			else if(minElement == last) 
			{
				minBefore->next = NULL;
				last = minBefore;
			}
			else
				minBefore->next = minElement->next;
			printf("returning : %s - (%d, %d) - %d \n", minMember->packet+19, minMember->time.t1, minMember->time.t2, minMember->whichServer);
			return ((void *)minMember);
		}
		else
		{
			printf("No search result found\n");
			return NULL;
		}
	}	
}

void *
List::SearchEqual(timeCounterClass givenTime)
{
	ListElement *thisElement, *thisBefore, *minElement, *minBefore;
	waitingQueueMember *minMember, *thisMember;
	int g;

	if(first==NULL)
	{
		//printf("List empty\n");
		return NULL;
	}
	
	if(first == last)
	{
		minElement = first;
		minMember = (waitingQueueMember *)minElement->item;
	//	printf("QUEUE : %s - (%d, %d) - %d \n", minMember->packet+19, minMember->time.t1, minMember->time.t2, minMember->whichServer);
		g = isTimeGreater(minMember->time, givenTime);
		if(g==2)
		{
			first = NULL;
			last = NULL;
			//printf("SE returning : %s - (%d, %d) - %d \n", minMember->packet+19, minMember->time.t1, minMember->time.t2, minMember->whichServer);
			//printf("SE given     : (%d, %d) \n", givenTime.t1, givenTime.t2);
			return ((void *)minMember);
		}
		else
			return NULL;
	}
	else
	{
		minElement = NULL;
		thisElement = first;
		thisBefore = NULL;
		thisMember = (waitingQueueMember *)thisElement->item;
		//printf("QUEUE : %s - (%d, %d) - %d | ", thisMember->packet+19, thisMember->time.t1, thisMember->time.t2, thisMember->whichServer);
		while(1)
		{
			
			if(isTimeGreater(givenTime, thisMember->time)==2)
			{
				minElement = thisElement;
				minBefore = thisBefore;
				break;
			}

			if(thisElement == last)
				break;

			thisBefore = thisElement;
			thisElement = thisElement->next;
			thisMember = (waitingQueueMember *)thisElement->item;			
		//	printf("%s - (%d, %d) - %d | ", thisMember->packet+19, thisMember->time.t1, thisMember->time.t2, thisMember->whichServer);
			
		}
		
		if(minElement!=NULL)
		{
			minMember = (waitingQueueMember *)minElement->item;
		
			if(minElement == first)
			{
				first = first->next;
			}
			else if(minElement == last) 
			{
				minBefore->next = NULL;
				last = minBefore;
			}
			else
				minBefore->next = minElement->next;
			//printf("returning : %s - (%d, %d) - %d \n", minMember->packet+19, minMember->time.t1, minMember->time.t2, minMember->whichServer);
			//printf("SE given     : (%d, %d) \n", givenTime.t1, givenTime.t2);
			return ((void *)minMember);
		}
		else
		{
		//	printf("No search result found\n");
			return NULL;
		}
	}
}

bool 
List::isPresent(int i, int j)
{
	int this_messageIndex;
	int this_machineIdentifier;
	ListElement *thisElement;
	waitingQueueMember *thisMember;
	if(first==NULL)
		return FALSE;
	else
	{
		thisElement = first;
		while(thisElement!=NULL)
		{
				thisMember = (waitingQueueMember *)thisElement->item;
				this_messageIndex = thisMember->packet[MaxMailSize2-3]*100+thisMember->packet[MaxMailSize2-2]*10+thisMember->packet[MaxMailSize2-1];
				this_machineIdentifier = (thisMember->packet[3]-46)*1000+(thisMember->packet[4]-46)*100+(thisMember->packet[5]-46)*10+(thisMember->packet[6]-46);
				if((this_messageIndex == i)&&(this_machineIdentifier==j))
					return TRUE;

				thisElement = thisElement->next;
		}
		return FALSE;
	}
}

// A function to display all the members in a list. 
// This function is but specific because it reads only a list of locks and can print them
// So it can't print out the list of threads if stored as a list.

void 
List::displayMembers()
{

	/*
	ListElement *element;
	Lock *lock;
	element = first;
	int condn = 1;
	int runOnce = 0;
	int displayCount = 0;
	DEBUG('l',"in list display members\n");
	do{		
		lock = (Lock *)element->item;
		printf(" %s | ",lock->getName());
		
		if(runOnce == 1)
			condn = 0;
		if(element==last)
		{
			runOnce = 1;
		}
		else
		{
			element = element->next;
		}
	//	printf("in display members : %d",displayCount);
		displayCount++;
	}while(condn);
//	printf("\n"); */
}
//----------------------------------------------------------------------
// List::Mapcar
//	Apply a function to each item on the list, by walking through  
//	the list, one element at a time.
//
//	Unlike LISP, this mapcar does not return anything!
//
//	"func" is the procedure to apply to each element of the list.
//----------------------------------------------------------------------

void
List::Mapcar(VoidFunctionPtr func)
{
    for (ListElement *ptr = first; ptr != NULL; ptr = ptr->next) {
       DEBUG('l', "In mapcar, about to invoke %x(%x)\n", func, ptr->item);
       (*func)((int)ptr->item);
    }
}

//----------------------------------------------------------------------
// List::IsEmpty
//      Returns TRUE if the list is empty (has no items).
//----------------------------------------------------------------------

bool
List::IsEmpty() 
{ 
    if (first == NULL)
        return TRUE;
    else
	return FALSE; 
}

//----------------------------------------------------------------------
// List::SortedInsert
//      Insert an "item" into a list, so that the list elements are
//	sorted in increasing order by "sortKey".
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, walk through the list, one element at a time,
//	to find where the new item should be placed.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//	"sortKey" is the priority of the item.
//----------------------------------------------------------------------

void
List::SortedInsert(void *item, int sortKey)
{
    ListElement *element = new ListElement(item, sortKey);
    ListElement *ptr;		// keep track

    if (IsEmpty()) {	// if list is empty, put
        first = element;
        last = element;
    } else if (sortKey < first->key) {	
		// item goes on front of list
	element->next = first;
	first = element;
    } else {		// look for first elt in list bigger than item
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (sortKey < ptr->next->key) {
		element->next = ptr->next;
	        ptr->next = element;
		return;
	    }
	}
	last->next = element;		// item goes at end of list
	last = element;
    }
}

//----------------------------------------------------------------------
// List::SortedRemove
//      Remove the first "item" from the front of a sorted list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//	Sets *keyPtr to the priority value of the removed item
//	(this is needed by interrupt.cc, for instance).
//
//	"keyPtr" is a pointer to the location in which to store the 
//		priority of the removed item.
//----------------------------------------------------------------------

void *
List::SortedRemove(int *keyPtr)
{
    ListElement *element = first;
    void *thing;

    if (IsEmpty()) 
	return NULL;

    thing = first->item;
    if (first == last) {	// list had one item, now has none 
        first = NULL;
	last = NULL;
    } else {
        first = element->next;
    }
    if (keyPtr != NULL)
        *keyPtr = element->key;
    delete element;
    return thing;
}

