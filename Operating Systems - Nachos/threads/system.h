// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "synch.h"
//#include "serverutils.h"

#define MAXPROCESSES 100

//typedef int SpaceId;


// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock


#ifdef USER_PROGRAM
#include "machine.h"
#include "process.h"
extern Machine* machine;	// user program memory and registers
typedef int SpaceId;
extern IPTEntry *ipt;
extern int replacementScheme;
extern OpenFile *swapFile;
//extern Lock *iptLock;
extern Lock *swapLock;
extern BitMap *iptMap;
extern BitMap *swapMap;
extern processTable pTable[MAXPROCESSES];
extern int fifoPolicy[NumPhysPages];
extern int fifoStart;
extern int fifoEnd;
extern Table *memorySimulator;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
#include "serverutils.h"
extern PostOffice* postOffice;
extern int NServers;
extern int myMachineID;
extern int part3;
#endif

#endif // SYSTEM_H

