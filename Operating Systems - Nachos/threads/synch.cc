// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

Lock::Lock(char* debugName) 
{
  name = debugName;         //name of the lock
  lockState = FREE;         //intially set the state of the lock to be free
  lockOwnerThread = NULL;   //intially the thread the owns the lock if NULL
  lockWaitQueue = new List; //create a wait queue for the lock
  howMany = 0;
}

Lock::~Lock() 
{
  delete lockWaitQueue; //delete the lock's wait queue
}

void Lock::Acquire() //code block to acquire a lock
{    
  IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts
  
  if (lockState == FREE)
    {
      lockState = BUSY;                  //if the lock is free, make it busy
      lockOwnerThread = currentThread;   //make the current thread, the lock owner thread
	//  printf("%s Acquired!! - %d %d\n",name,currentThread->PID, currentThread->threadID);
//	  printf("Lock %s Acquired by %s\n", name,currentThread->getName());
//	printf("acquired %d %s\n",currentThread->PID, name);
    }
  else
    {
//	  printf("acquire being put to sleep %d %s\n",currentThread->PID, name);
	  howMany += 1;
	//  printf("%s Acquire waiting!! - %d %d\n",name,currentThread->PID, currentThread->threadID);
	  //  printf("1. howMany = %d | name = %s\n",howMany,name);
      //printf ("\n Lock '%s' not available currently for thread \n",name);
      lockWaitQueue->Append((void *)currentThread);  //if lock is not free, then add the current thread to the lock's wait queue
      currentThread->Sleep();                        //put the current thread to sleep
    }
  (void) interrupt->SetLevel(oldLevel); //enable interrupts
}

void Lock::Release() //code block to release a lock
{
  IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts
  Thread *thread;
  if ( !isHeldByCurrentThread() ) //check it the current thread is the owner of the lock
  {
      printf ("\n The current Thread %s is not the owner of the lock '%s' \n",currentThread->getName(),name);
  }
  else
  {	  
    if ((thread = (Thread *)lockWaitQueue->Remove()) != NULL) //remove the thread from the lock's wait queue
	{
	  scheduler->ReadyToRun(thread);                        //put the thread in the ready queue
	  lockOwnerThread = thread;                             //make it the owner of the lock
	  howMany = howMany -1 ;
	//  printf("%s Released!! - %d %d\n",name, currentThread->PID, currentThread->threadID);
	//  printf("Making %d %d the owner of %s\n",thread->PID, thread->threadID, name);
	}
	else
	{
//		  howMany = 0;
	//  printf("%s Released\n", name);
	  lockOwnerThread = NULL;                              //if there are no waiting threads, then clear the lock ownership
	  lockState = FREE;                                    //set the lock state to be free
	}
  }  
  (void) interrupt->SetLevel(oldLevel);  //enable interrupts
}

bool Lock::isHeldByCurrentThread() //code block to check if the current thread is the lock owner
{
  if (lockOwnerThread == currentThread)
    return 1;   //return 1 if the current thread is the lock owner thread
  else
    return 0;
}

Condition::Condition(char* debugName)  
{
  name = debugName;          //name of the condition variable
  condWaitQueue = new List;  //create a wait queue for the condition variable
  trackCondLock = NULL;      //variable to track which lock is being used with the condition variable
}

Condition::~Condition()
{
  delete condWaitQueue;     //delete the condition variable wait queue
}

void Condition::Wait(Lock* conditionLock) //code block for a thread waiting on a condition variable
{
  IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interuppts

  if (trackCondLock == NULL)	
    trackCondLock = conditionLock;              //store the lock being used with the condition variable
  //  printf("Waiting on condition : %s and lock %s\n",name,conditionLock->getName());
  condWaitQueue->Append((void *)currentThread); //append the current thread to the condition thread wait queue
  conditionLock->Release();                     //release the lock
  currentThread->Sleep();                       //put the current thread to sleep
  conditionLock->Acquire();                     //acquire the lock once the thread is woken up
  (void) interrupt->SetLevel(oldLevel);	        //enable the interrupts
}

void Condition::Signal(Lock* conditionLock)  //code block to signal a thread waiting on a condition variable
{    
  IntStatus oldLevel = interrupt->SetLevel(IntOff);  //disable interrupts
  Thread *thread;
  if (condWaitQueue->IsEmpty())
    (void) interrupt->SetLevel(oldLevel);	     //if no threads are waiting currently, restore interrupts and return
  else {		
    if (conditionLock != NULL && conditionLock->isHeldByCurrentThread()) { //if a thread is waiting, check if it is the lock owner
      if (conditionLock != trackCondLock) {
	printf ("\n The singnaler's lock doesn't match the waiter's lock \n"); //restore interrupts and return if the signaller thread is not the lock owner
      }
      else {
	//	   printf("Signaling on condition : %s and lock %s\n",name,conditionLock->getName());
	thread = (Thread *)condWaitQueue->Remove(); //remove a waiting thread from the condition variable wait queue               
	if (thread != NULL)
	  scheduler->ReadyToRun(thread);	    //put the thread in the ready queue          
	if (condWaitQueue->IsEmpty()) {                    
	  trackCondLock = NULL;                     //clear the lock ownership if therea re no more waiters
	}
      } 
    }
  }
  (void) interrupt->SetLevel(oldLevel);             //enable interrupts
}

void Condition::Broadcast(Lock* conditionLock)  //code block to signal all the threads waiting
{
  while (!(condWaitQueue->IsEmpty())) {	         
    Signal(conditionLock);		        //keep signalling until the condition variable wait queue becomes empty
  }
}

bool Condition::isSomebodyWaiting()   //code block to check if there is any thread waiting in the condition variable wait queue       
{
  return !(condWaitQueue->IsEmpty()); //return 1 if there is a thread waiting, else return 0
}
