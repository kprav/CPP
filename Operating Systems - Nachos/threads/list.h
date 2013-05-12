// list.h 
//	Data structures to manage LISP-like lists.  
//
//      As in LISP, a list can contain any type of data structure
//	as an item on the list: thread control blocks, 
//	pending interrupts, etc.  That is why each item is a "void *",
//	or in other words, a "pointers to anything".
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef LIST_H
#define LIST_H

#include "copyright.h"
#include "utility.h"
#define MaxMailSize2 40

// The following class defines a "list element" -- which is
// used to keep track of one item on a list.  It is equivalent to a
// LISP cell, with a "car" ("next") pointing to the next element on the list,
// and a "cdr" ("item") pointing to the item on the list.
//
// Internal data structures kept public so that List operations can
// access them directly.

class ListElement {
   public:
     ListElement(void *itemPtr, int sortKey);	// initialize a list element

     ListElement *next;		// next element on list, 
				// NULL if this is the last
     int key;		    	// priority, for a sorted list
     void *item; 	    	// pointer to item on the list
};

// The following class defines a "list" -- a singly linked list of
// list elements, each of which points to a single item on the list.
//
// By using the "Sorted" functions, the list can be kept in sorted
// in increasing order by "key" in ListElement.
class timeCounterClass
{
	public:
		int t1;
		int t2;
	timeCounterClass();
	timeCounterClass(int);
	timeCounterClass(int, int);
};

class waitingQueueMember
{
	public:
		char packet[MaxMailSize2];
		timeCounterClass time;
		int whichServer; //To break ties!
		bool sendAckValid;
		waitingQueueMember(char *, timeCounterClass, int, bool);
};

// -----------------------------------------------------------------------------------------------------------------
//	This class is used to store the replies that are to be sent back. For example, when a lock is not available to
//	be acquired, we	make an object of this class and append it to the waiting queue in that particular lock. 
// -----------------------------------------------------------------------------------------------------------------
class replyType
{
  public:
	int inPktHdr_from;
	int inMailHdr_from;
	char ack[MaxMailSize2];
	int machineID;
	replyType(int, int, char *);
};

int isTimeGreater(timeCounterClass, timeCounterClass);
class List {
  public:
    List();			// initialize the list
    ~List();			// de-allocate the list
    void Prepend(void *item); 	// Put item at the beginning of the list
    void Append(void *item); 	// Put item at the end of the list
    void *Remove(); 	 	// Take item off the front of the list
    void *Search(timeCounterClass t);     // Search for list entry containing key and return it.
	void *SearchEqual(timeCounterClass t);
	bool isPresent(int i, int j);
    void Mapcar(VoidFunctionPtr func);	// Apply "func" to every element 
					// on the list
    bool IsEmpty();		// is the list empty? 
    void displayMembers();

    // Routines to put/get items on/off list in order (sorted by key)
    void SortedInsert(void *item, int sortKey);	// Put item into list
    void *SortedRemove(int *keyPtr); 	  	// Remove first item from list

  private:
    ListElement *first;  	// Head of the list, NULL if list is empty
    ListElement *last;		// Last element of list
};

#endif // LIST_H
