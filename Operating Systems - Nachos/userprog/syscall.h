/* syscalls.h 
 * 	Nachos system call interface.  These are Nachos kernel operations
 * 	that can be invoked from user programs, by trapping to the kernel
 *	via the "syscall" instruction.
 *
 *	This file is included by user programs and by the Nachos kernel. 
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"


/* system call codes -- used by the stubs to tell the kernel which system call
 * is being asked for
 */
#define SC_Halt		0
#define SC_Exit		1
#define SC_Exec		2
#define SC_Join		3
#define SC_Create	4
#define SC_Open		5
#define SC_Read		6
#define SC_Write	7
#define SC_Close	8
#define SC_Fork		9
#define SC_Yield	10
#define SC_CreateLock	11
#define SC_DeleteLock	12
#define SC_CreateCondition	13
#define SC_DeleteCondition	14
#define SC_AcquireLock	15
#define SC_ReleaseLock	16
#define SC_WaitCV		17
#define SC_SignalCV	18
#define SC_BroadcastCV	19
#define SC_TestMe	20
#define SC_CheckCondWaitQueue 21
#define SC_Random 22
#define SC_MemAllocate 23
#define SC_Concatenate 24
#define SC_PrintStuff 25
#define SC_WriteNum 26
#define SC_Delay	27
#define SC_CreateSharedInt 28
#define SC_GetSharedInt 29
#define SC_SetSharedInt 30
#define SC_GetOneIndex	31
#define SC_GetZeroIndex 32
#define SC_ArraySearch 33

#define MAXFILENAME 256

#ifndef IN_ASM

/* The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the 
 * procedure; an assembly language stub stuffs the system call code
 * into a register, and traps to the kernel.  The kernel procedures
 * are then invoked in the Nachos kernel, after appropriate error checking, 
 * from the system call entry point in exception.cc.
 */

/* Stop Nachos, and print out performance stats */
void Halt();		
 

/* Address space control operations: Exit, Exec, and Join */

/* This user program is done (status = 0 means exited normally). */
void Exit(int status);	

/* A unique identifier for an executing user program (address space) */
typedef int SpaceId;	
 
/* Run the executable, stored in the Nachos file "name", and return the 
 * address space identifier
 */
SpaceId Exec(char *name, int pid);
 
/* Only return once the the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id); 	
 

/* File system operations: Create, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * If this assignment is done before doing the file system assignment,
 * note that the Nachos file system has a stub implementation, which
 * will work for the purposes of testing out these routines.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId;	

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  
 
/* Create a Nachos file, with "name" */
void Create(char *name, int size);

/* Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file.
 */
OpenFileId Open(char *name, int size);

/* Write "size" bytes from "buffer" to the open file. */
void Write(char *buffer, int size, OpenFileId id);

/* Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id);

void WriteNum(int id);
/* Close the file, we're done reading and writing to it. */
void Close(OpenFileId id);

/* Create a Lock */
int CreateLock(char *lockName,int len);

/* Acquire a Lock */
void AcquireLock(int lockID);

/* Release a lock */
void ReleaseLock(int lockID);

/* Delete a Lock */
void DeleteLock(int lockID);

/* Create a Condition Variable */
int CreateCondition(char *ConditionName,int len);

/* Wait on a Condition Variable */
int WaitCV(int condID, int lockID);

/* Signal a thread waiting on a Condition Variable*/
void SignalCV(int condID,int lockID);

/* Broadcast all threads waiting on a condition variable */
void BroadcastCV(int condID,int lockID);

/* Delete a Condition */
void DeleteCondition(int condID);

/* Check if a condition variable wait queue is empty or not */
int CheckCondWaitQueue(int condID);

/* A system call to print */
void Print(int);

/* Generate a random integer */
int RandomFunction(int);

/* Allocate Memory Dynamically */
int MemAllocate(int *, int);

/* Concatenate Strings */
void Concatenate(char *,int, int, char *);

/* User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 */

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 */
void Fork(void (*func)());

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void Yield();		

int CreateSharedInt(char *, int, int); /*name, length of name, length of array */
int GetSharedInt(int, int); /* id, position */
void SetSharedInt(int, int, int); /* id, position, value to set */
int GetOneIndex(int); /*id */
int GetZeroIndex(int); /*id */
int ArraySearch(int, int, int); /*id, skipindex, equalityValue*/
#endif /* IN_ASM */

#endif /* SYSCALL_H */
