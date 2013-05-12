// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  
// This code was taken from my programming assignment in the course CSCI402x
// The copyright is stated above. 
// The function ListSize() that returns the current size of the list was 
// written by me

#include <stdio.h>

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

class List {
  public:
    List();			// initialize the list
    ~List();			// de-allocate the list    
    void Append(void *item); 	// Put item at the end of the list
    void *Remove(); 	 	// Take item off the front of the list
    bool IsEmpty();		// is the list empty?             
	int ListSize();			//return the current size of the list
  private:
    ListElement *first;  	// Head of the list, NULL if list is empty
    ListElement *last;		// Last element of list
	int size;				// stores current size of the list
};
