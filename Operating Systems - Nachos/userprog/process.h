#include "copyright.h"
#include "synch.h"
#include "list.h"
#include "syscall.h"

#define MAXPCHILDREN 100
#define MAXTCHILDREN 100

#define TRUE 1
#define FALSE 0

//typedef int SpaceId;

class childProcessDetails
{
	public:
		SpaceId childPID;
		AddrSpace *childSpace;
//		Condition *jsp;
//		int indicator;
};

struct processTable
{
	public:
		SpaceId PID;
		AddrSpace *parentSpace;
		AddrSpace *space;
		SpaceId parentPID;
		int childProcessCount;
		class childProcessDetails childProcessDetailsArray[MAXPCHILDREN];
		int threadCount; //starts from 0
		Thread *parentThread;
		Condition *waitForExiting;
		int exitStatus;

};


/*
class childProcessDetails
{
	public:
		SpaceId childPID;
		AddrSpace *childSpace;
//		Condition *jsp;
//		int indicator;
};

class threadDetails
{
	public:
		SpaceId threadID;
		int childThreadCount;
		Thread *parentThread;
		Condition *waitForExiting;
		int exitStatus;	//1 - can exit and 0 - wait for child to finish;
};

class processTable
{
	public:
		SpaceId PID;
		AddrSpace *parentSpace;
		int childProcessCount;
		childProcessDetails childProcessDetailsArray[MAXPCHILDREN];
		threadDetails threadDetailsArray[MAXTCHILDREN];
		int threadCount;
}pTable[MAXPROCESSES];
*/
