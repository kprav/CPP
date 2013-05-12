/* Start.s 
 *	Assembly language assist for user programs running on top of Nachos.
 *
 *	Since we don't want to pull in the entire C library, we define
 *	what we need for a user program here, namely Start and the system
 *	calls.
 */

#define IN_ASM
#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main". 
 *
 * 	NOTE: This has to be first, so that it gets loaded at location 0.
 *	The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$0		
	jal	Exit	 /* if we return from main, exit(0) */
	.end __start

/* -------------------------------------------------------------
 * System call stubs:
 *	Assembly language assist to make system calls to the Nachos kernel.
 *	There is one stub per system call, that places the code for the
 *	system call into register r2, and leaves the arguments to the
 *	system call alone (in other words, arg1 is in r4, arg2 is 
 *	in r5, arg3 is in r6, arg4 is in r7)
 *
 * 	The return value is in r2. This follows the standard C calling
 * 	convention on the MIPS.
 * -------------------------------------------------------------
 */

	.globl Halt
	.ent	Halt
Halt:
	addiu $2,$0,SC_Halt
	syscall
	j	$31
	.end Halt

	.globl Exit
	.ent	Exit
Exit:
	addiu $2,$0,SC_Exit
	syscall
	j	$31
	.end Exit

	.globl Exec
	.ent	Exec
Exec:
	addiu $2,$0,SC_Exec
	syscall
	j	$31
	.end Exec

	.globl Join
	.ent	Join
Join:
	addiu $2,$0,SC_Join
	syscall
	j	$31
	.end Join

	.globl Create
	.ent	Create
Create:
	addiu $2,$0,SC_Create
	syscall
	j	$31
	.end Create

	.globl Open
	.ent	Open
Open:
	addiu $2,$0,SC_Open
	syscall
	j	$31
	.end Open

	.globl Read
	.ent	Read
Read:
	addiu $2,$0,SC_Read
	syscall
	j	$31
	.end Read

	.globl Write
	.ent	Write
Write:
	addiu $2,$0,SC_Write
	syscall
	j	$31
	.end Write

	.globl Close
	.ent	Close
Close:
	addiu $2,$0,SC_Close
	syscall
	j	$31
	.end Close

	.globl Fork
	.ent	Fork
Fork:
	addiu $2,$0,SC_Fork
	syscall
	j	$31
	.end Fork

	.globl Yield
	.ent	Yield
Yield:
	addiu $2,$0,SC_Yield
	syscall
	j	$31
	.end Yield

	.globl CreateLock
	.ent CreateLock
CreateLock:
	addiu $2,$0,SC_CreateLock
	syscall
	j	$31
	.end CreateLock
	
	.globl AcquireLock
	.ent	AcquireLock
AcquireLock:
	addiu $2,$0,SC_AcquireLock
	syscall
	j	$31
	.end AcquireLock

	.globl ReleaseLock
	.ent	ReleaseLock
ReleaseLock:
	addiu $2,$0,SC_ReleaseLock
	syscall
	j	$31
	.end ReleaseLock	

	.globl DeleteLock
	.ent	DeleteLock
DeleteLock:
	addiu $2,$0,SC_DeleteLock
	syscall
	j	$31
	.end DeleteLock

	.globl CreateCondition
	.ent	CreateCondition
CreateCondition:
	addiu $2,$0,SC_CreateCondition
	syscall
	j	$31
	.end CreateCondition

	.globl WaitCV
	.ent	WaitCV
WaitCV:
	addiu $2,$0,SC_WaitCV
	syscall
	j	$31
	.end WaitCV

	.globl SignalCV
	.ent	SignalCV
SignalCV:
	addiu $2,$0,SC_SignalCV
	syscall
	j	$31
	.end SignalCV

	.globl BroadcastCV
	.ent	BroadcastCV
BroadcastCV:
	addiu $2,$0,SC_BroadcastCV
	syscall
	j	$31
	.end BroadcastCV


	.globl DeleteCondition
	.ent	DeleteCondition
DeleteCondition:
	addiu $2,$0,SC_DeleteCondition
	syscall
	j	$31
	.end DeleteCondition	

	.globl TestMe
	.ent	TestMe
TestMe:
	addiu $2,$0,SC_TestMe
	syscall
	j	$31
	.end TestMe

	.globl CheckCondWaitQueue
	.ent	CheckCondWaitQueue
CheckCondWaitQueue:
	addiu $2,$0,SC_CheckCondWaitQueue
	syscall
	j	$31
	.end CheckCondWaitQueue

	.globl RandomFunction
	.ent	RandomFunction
RandomFunction:
	addiu $2,$0,SC_Random
	syscall
	j	$31
	.end RandomFunction

	.globl MemAllocate
	.ent	MemAllocate
MemAllocate:
	addiu $2,$0,SC_MemAllocate
	syscall
	j	$31
	.end MemAllocate

	.globl Concatenate
	.ent	Concatenate
Concatenate:
	addiu $2,$0,SC_Concatenate
	syscall
	j	$31
	.end Concatenate

	.globl PrintStuff
	.ent	PrintStuff
PrintStuff:
	addiu $2,$0,SC_PrintStuff
	syscall
	j	$31
	.end PrintStuff

	.globl WriteNum
	.ent	WriteNum
WriteNum:
	addiu $2,$0,SC_WriteNum
	syscall
	j	$31
	.end WriteNum

	.globl Delay
	.ent	Delay
Delay:
	addiu $2,$0,SC_Delay
	syscall
	j	$31
	.end Delay

	.globl CreateSharedInt
	.ent	CreateSharedInt
CreateSharedInt:
	addiu $2,$0,SC_CreateSharedInt
	syscall
	j	$31
	.end CreateSharedInt

	.globl GetSharedInt
	.ent	GetSharedInt
GetSharedInt:
	addiu $2,$0,SC_GetSharedInt
	syscall
	j	$31
	.end GetSharedInt

	.globl SetSharedInt
	.ent	SetSharedInt
SetSharedInt:
	addiu $2,$0,SC_SetSharedInt
	syscall
	j	$31
	.end SetSharedInt

	.globl GetOneIndex
	.ent	GetOneIndex
GetOneIndex:
	addiu $2,$0,SC_GetOneIndex
	syscall
	j	$31
	.end GetOneIndex

	.globl GetZeroIndex
	.ent	GetZeroIndex
GetZeroIndex:
	addiu $2,$0,SC_GetZeroIndex
	syscall
	j	$31
	.end GetZeroIndex

	.globl ArraySearch
	.ent	ArraySearch
ArraySearch:
	addiu $2,$0,SC_ArraySearch
	syscall
	j	$31
	.end ArraySearch

/* dummy function to keep gcc happy */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main

