// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include "table.h"
//#include "serverutils.h"


#include <stdio.h>
#include <string.h>
#include <iostream>


#define MAXLOCKSCONDS 200
#define MAXL 200
using namespace std;

extern "C" { int bzero(char *, int); };
extern "C" { int bcopy(char *, char *, int); };


char procName[100];
SpaceId PIDCount = 0;
SpaceId threadPIDCount = 0;
Lock *processTableLock = new Lock("Process Table Lock");
Lock *lockcondlock = new Lock("lock and condition lock");
Lock *lockQueue[MAXLOCKSCONDS];
Lock *iptLock = new Lock("IPT Lock");
int currentLockID = 0;
Condition *condQueue[MAXLOCKSCONDS];
int someCounter = 0;

Lock *forkLock = new Lock("forkLock");
Lock *aliveServersLock = new Lock("aliveServersLock");
int *aliveServers;
List *sentMessagesQueue = new List();
Lock *sentMessagesLock = new Lock("sentMessagesLock");
int currentConditionID = 0;
int currentTLBSlot = 0;   // counter for TLB slot
int messageCounter = 0;
Lock *counterLock = new Lock("counterLock");


//Function to pick a server randomly - used by the client to send its request messages
int getRandServer() 
{
	int randServer;
	aliveServersLock->Acquire();
	do
	{
		randServer = (getTime()%1000+rand())%NServers;	
	}
	while (aliveServers[randServer]==0);
	aliveServersLock->Release();
//	printf("randServer = %d, Nservers = %d\n",randServer,NServers);

	return randServer; //Returns the machine ID of the random server that was picked
}

void waitForNSeconds(int n)
{
	timeCounterClass t1, t2, t3, tn;
	t1 = getTimeStamp();
	t2 = getTimeStamp();
	t3 = findDiffTime(t1, t2);
	tn.t1 = 0;
	tn.t2 = n*1000000;
	while(isTimeGreater(t3,tn)==0)
	{
		currentThread->Yield();
		t1 = getTimeStamp();
		t3 = findDiffTime(t1, t2);
	}

}

void clientMonitor(int n)
{
	int toMachine;
	PacketHeader inPktHdr, outPktHdr;
	MailHeader outMailHdr, inMailHdr;
	bool success;
	bool nothingToSend;
	timeCounterClass t;
	waitingQueueMember *wMember;
	while(1)
	{
		sentMessagesLock->Acquire();
		nothingToSend = sentMessagesQueue->IsEmpty();
		sentMessagesLock->Release();
		if(nothingToSend)
		{
			printf("Wait for 1 second\n");
			waitForNSeconds(1);
		}
		else
		{
			sentMessagesLock->Acquire();
			wMember = (waitingQueueMember *)sentMessagesQueue->Remove();
			t = wMember->time;  //The unique identifier.
			sentMessagesQueue->Prepend((void *)wMember);
			sentMessagesLock->Release();

			printf("wait for %d seconds before resending the message\n",n);
			waitForNSeconds(n);
			
			sentMessagesLock->Acquire();
			wMember = (waitingQueueMember *)sentMessagesQueue->SearchEqual(t);
			
			
			if(wMember!=NULL)
			{
				toMachine = wMember->whichServer;
				aliveServersLock->Acquire();
				int isAlive = aliveServers[toMachine];
				aliveServersLock->Release();
				if(!isAlive)
					toMachine = getRandServer();
				
				printf("[%d]RESENDING UNANSWERED MESSAGE %s TO %d\n",wMember->time.t2, wMember->packet+19, toMachine);

				do
				{
					
					outPktHdr.to = toMachine;
					outMailHdr.to = 0;
					outMailHdr.from = wMember->time.t2;
					outMailHdr.length = MaxMailSize;

					success = postOffice->Send(outPktHdr, outMailHdr, wMember->packet);
					if(!success)
					{
						printf("[%d]Sending to server %d failed.. Marking it as dead!\n", outMailHdr.from, toMachine);
						aliveServersLock->Acquire();
						aliveServers[toMachine] = 0;
						aliveServersLock->Release();
						// Get a new machine to send.
						toMachine = getRandServer();
					}
				}
				while(!success);
				printf("[%d]Message sent successfully to %d\n", outMailHdr.from, toMachine);
				sentMessagesQueue->Append((void *)wMember);
				
			}
			sentMessagesLock->Release();
		}
	}
}



//This function runs at the beginning of the user program.
//This has been called in progtest.cc

void initprogram(int startValue)
{
	// Populate the IPT for initialization
	
	PIDCount = startValue;
	int i = PIDCount++;
	currentThread->PID = i;
	currentThread->isMainThread = 1;
	currentThread->threadID = 0;
	// let me set the pid of other processes to -1.
	for(int k=0;k<MAXPROCESSES;k++)
		pTable[k].PID = -1;
	//Update the process table for the first process.
	pTable[i].PID = i;
	pTable[i].parentPID = -1; //No parent!
	pTable[i].parentSpace = NULL;
//	pTable[i].space = space;
	pTable[i].childProcessCount = 0;
	for(int k=0;k<MAXPCHILDREN;k++)
	{
		pTable[i].childProcessDetailsArray[k].childPID=-1;
		//setting child address space too null is optional and not required. Any change here must reflect at Exec_Syscall
	}
	pTable[i].threadCount = 0;
	pTable[i].parentThread = NULL;
	pTable[i].waitForExiting = new Condition("root parent exit condition");
	pTable[i].exitStatus = TRUE;
	aliveServers = new int[NServers];
	for(int j=0;j<NServers;j++)
		aliveServers[j] = 1;

	Thread *clientMonitorThread = new Thread("Client monitor thread");
	if(part3)
		clientMonitorThread->Fork(clientMonitor,10);
}

// Function to set the space pointer for the first process first main thread.
void setMainSpace(int pid, AddrSpace *space)
{
	pTable[pid].space = space;
}



// Code starts
int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;
	if(vaddr < currentThread->space->getNumPages()*PageSize)
	{
	    while ( n >= 0 && n < len) {
			result = FALSE;
			while(result == FALSE)
			      result = machine->ReadMem( vaddr, 1, paddr );
	        buf[n++] = *paddr;
	        vaddr++;
		}

	    delete paddr;
		return len;
	}
	else
		return -1;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in
	if(vaddr < (currentThread->space->getNumPages())*PageSize)
	{
	    while ( n >= 0 && n < len) {
		  // Note that we check every byte's address
		  result = FALSE;
		  while(result == FALSE)
		      result = machine->WriteMem( vaddr, 1, (int)(buf[n]) );
    		n++;
	 

	      vaddr++;
	    }

		return n;
	}
	else
	{
		printf("vaddr outside!");
		return -1;
	}
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output
	int return_value = -1;
    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to write: data not written\n");
			    delete[] buf;
			    return;
			}
		}			
    

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
		  if(buf[ii]=='\0')
			  break;
		  printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }
    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

// Function helpful in sending and receiving.
void sendIt(char *data, char *buffer, int type)
{
	PacketHeader inPktHdr, outPktHdr;
	MailHeader outMailHdr, inMailHdr;
	int fromBox = currentThread->threadID%100 + 2;			// We have used the threadID for each thread as a means to find the mailbox it listens to
	int toMachine = getRandServer();						// get a random 'living' server to send.
	outMailHdr.to = 0;										//Always send to mailbox 0
	outMailHdr.from = fromBox;								// reply-to mailbox.
	outMailHdr.length = MaxMailSize;
	bool success;
	int sentMessageIndex, receivedMessageIndex;

	//Append the counter to data and then send it!
	counterLock->Acquire();
	sentMessageIndex = messageCounter;
	messageCounter = (messageCounter + 1)%1000;
	counterLock->Release();
	printf("[%d]Going to send : %s, messageIndex = %d to %d\n",fromBox, data+19,sentMessageIndex, toMachine);
	appendNumber(data,sentMessageIndex);

	do
	{
		outPktHdr.to = toMachine; //Server machine id
		success = postOffice->Send(outPktHdr, outMailHdr, data);
		if(!success)
		{
			printf("[%d]Sending to server %d failed. Marking it as dead!\n", fromBox, toMachine);
			aliveServersLock->Acquire();
			aliveServers[toMachine] = 0;
			aliveServersLock->Release();
			// Get a new machine to send.
			toMachine = getRandServer();
		}
	}
	while(!success);
	// Message sending succeeded!
	// We never know whether we are going to get back the reply immediately or not.
	// So lets append this message that we sent to a queue.
	waitingQueueMember *wMember, *pMember;
	timeCounterClass t;
	t.t1 = type*1000+sentMessageIndex;
	t.t2 = fromBox;
	wMember = new waitingQueueMember(data, t, toMachine, TRUE);
	sentMessagesLock->Acquire();
	sentMessagesQueue->Append((void *)wMember);
	sentMessagesLock->Release();
	do
	{
		postOffice->Receive(fromBox, &inPktHdr, &inMailHdr, buffer);
		receivedMessageIndex = getNumber(buffer);
	}
	while (receivedMessageIndex != sentMessageIndex);
	printf("[%d]Received Message = %s index = %d from %d\n",fromBox, buffer+19,receivedMessageIndex,inPktHdr.from);
	sentMessagesLock->Acquire();
	pMember = (waitingQueueMember *)sentMessagesQueue->SearchEqual(t);
	sentMessagesLock->Release();
	if(pMember == NULL)
		printf("[%d]Something went wrong in handling the queue\n",fromBox);		//Should never be printed.

}

//System call to create a lock
int CreateLock_Syscall(unsigned int vaddr, int len)
{
	char *buf;
	int rv;	
	if(!(buf = new char[len+1]))
	{
		printf("%s","Error allocating kernel buffer in CreateLock\n");
		return -1;
	}
	if(copyin(vaddr, len, buf)==-1)
	{
		printf("%s","Bad pointer passed to CreateLock\n");
		delete[] buf;
		return -1;
	}
	buf[len]='\0';
#ifndef NETWORK
	lockcondlock->Acquire();
	Lock *lock = new Lock(buf);
	lockQueue[currentLockID] = lock;
	rv = currentLockID;
	DEBUG('y',"Lock Variable %s Created \n",buf);
	currentLockID++;
	lockcondlock->Release();
	//delete[] buf; 
	return rv;
#else
	
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100 + 2;
	timeCounterClass time;// = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_CreateLock,myMachineID, fromBox,0,0,0,&time,buf);

	sendIt(data, buffer, SC_CreateLock);

	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3,msg);
	if(syscallType!=SC_CreateLock)
	  {
	    printf("[Createlock]Something went wrong, syscallType = %d\n",syscallType);
	  }
	//	  printf("lock successfully created, id = %d\n",parameter1);
	return parameter1; //Return the lock ID to the client

	fflush(stdout);

#endif
}

//Acquire lock system call.
void AcquireLock_Syscall(int lockID)
{
  
#ifndef NETWORK
  Lock *lock;

	if((lockID>=0)&&(lockID<currentLockID))
	{
		lock = lockQueue[lockID];
		lock->Acquire();
		DEBUG('y',"Lock %s Acquired \n",lock->getName());
	}
	else
		printf("Error :  LockID = %d is either negative or does not exist : %d \n",lockID,currentLockID);

#else

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_AcquireLock,myMachineID, fromBox,lockID,0,0,&time,"Acquire");
	
	sendIt(data, buffer, SC_AcquireLock);

	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_AcquireLock)
	  {
	    printf("[AcquireLock] Something went wrong, syscall got = %d\n", syscallType);
	  }
	if(parameter1==1)
	{
	  //  printf("Lock successfully Acquired \n");
	}
	else
	  printf("Lock acquire unsuccessful, server replies : %s\n", msg);

	fflush(stdout);

#endif
}

void ReleaseLock_Syscall(int lockID)
{
#ifndef NETWORK
	Lock *lock;
	lock = lockQueue[lockID];
	lock->Release();
	DEBUG('y',"Lock %s Released \n",lock->getName());
#else

	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_ReleaseLock,myMachineID, fromBox,lockID,0,0,&time,"Release");
	sendIt(data, buffer, SC_ReleaseLock);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_ReleaseLock)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1==1)
	{
	  // printf("Lock successfully released\n");
	}
	else
	  printf("Lock release unsuccessful. Server replies : %s\n", msg);

	fflush(stdout);

#endif
}

void DeleteLock_Syscall(int lockID)
{
#ifndef NETWORK
	Lock *lock;
	lock = lockQueue[lockID];
	lock->~Lock();
	DEBUG('y',"Lock Deleted \n");	
#else


	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_DeleteLock,myMachineID, fromBox,lockID,0,0,&time,"DeleteLock");
	sendIt(data, buffer, SC_DeleteLock);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_DeleteLock)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1==1)
	{
	//  printf("Lock deleted successfully\n");
	}
	else
	  printf("Lock deletion unsuccessful. Server replies : %s\n", msg);

	fflush(stdout);

#endif
}

int CreateCondition_Syscall(unsigned int vaddr, int len)
{
	char *buf;
	int rv, condID;
	if(!(buf = new char[len+1]))
	{
		printf("%s","Error allocating kernel buffer in CreateCondition\n");
		return -1;
	}
	if(copyin(vaddr, len, buf)==-1)
	{
		printf("%s","Bad pointer passed to CreateCondition\n");
		delete[] buf;
		return -1;
	}
	buf[len]='\0';

#ifndef NETWORK
	/* Now	create a condition variable,
			associate a number with it
			put it in the global linked list
			return the number which is the condition variable id.
	*/    

	lockcondlock->Acquire();
	Condition *condition = new Condition(buf);
	DEBUG('y',"Condition Variable %s Created \n",buf);
	condQueue[currentConditionID] = condition;
	
	rv = currentConditionID;	
	currentConditionID++;
	lockcondlock->Release();
	//delete[] buf;
	//delete[] lock;
	return rv;
#else


	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_CreateCondition,myMachineID, fromBox,0,0,0,&time,buf);
	sendIt(data, buffer, SC_CreateCondition);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_CreateCondition)
	  {
	    printf("Something went wrong\n");
	  }
	condID = parameter1;
	fflush(stdout);
	return condID;

#endif

}

void WaitCV_Syscall(int condID,int lockID)
{
#ifndef NETWORK
	Condition *condition;
	Lock *lock;
	if(((lockID<0)||(lockID>=currentLockID)))
	{
		printf("lockID = %d is negative or doesn't exist : %d\n",lockID,currentLockID);
	}
	else if(((condID<0)||(condID>=currentConditionID)))
	{
		printf("conditionID =%d is negative or doesn't exist : %d\n",condID, currentConditionID);
	}
	else
	{ 
		condition = condQueue[condID];
		lock = lockQueue[lockID];
//	    lockcondlock->Acquire();
//		lockcondlock->Release();
		condition->Wait(lock);
	}

#else


	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_WaitCV,myMachineID, fromBox,condID,lockID,0,&time,"Wait");
	sendIt(data, buffer, SC_WaitCV);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_WaitCV)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1==1)
	{
	//  printf("Finished Waiting\n");
	}
	else
	  printf("Error in waiting, Server replies : %s\n", msg);

	fflush(stdout);

#endif
}

void SignalCV_Syscall(int condID,int lockID)
{
#ifndef NETWORK
	Condition *condition;
	Lock *lock;
	if(((lockID<0)||(lockID>=currentLockID)))
	{
		printf("lockID = %d is negative or doesn't exist : %d\n",lockID,currentLockID);
	}
	else if(((condID<0)||(condID>=currentConditionID)))
	{
		printf("conditionID =%d is negative or doesn't exist : %d\n",condID, currentConditionID);
	} 
	else
	{
		condition = condQueue[condID];
		lock = lockQueue[lockID];
		condition->Signal(lock);
	}
#else

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_SignalCV,myMachineID, fromBox,condID,lockID,0,&time,"Signal");
	sendIt(data, buffer, SC_SignalCV);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_SignalCV)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1==1)
	{
	//  printf("Finished Signaling\n");
	}
	else
	  printf("Error in Signaling, Server replies : %s\n", msg);

	fflush(stdout);

#endif
}

void BroadcastCV_Syscall(int condID,int lockID)
{
#ifndef NETWORK
	Condition *condition;
	Lock *lock;
	if((lockID<0)||(lockID>=currentLockID))
	{
		printf("lockID = %d is negative or doesn't exist : %d\n",lockID,currentLockID);
	}
	else if(((condID<0)||(condID>=currentConditionID)))
	{
		printf("conditionID =%d is negative or doesn't exist : %d\n",condID, currentConditionID);
	}
	else
	{ 
		condition = condQueue[condID];
		lock = lockQueue[lockID];
		condition->Broadcast(lock);
	}
#else

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_BroadcastCV, myMachineID, fromBox,condID,lockID,0,&time,"Broadcast");
	sendIt(data, buffer, SC_BroadcastCV);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_BroadcastCV)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1==1)
	{
	//   printf("Finished Broadcasting\n");
	}
	else
	  printf("Error in Broadcasting, Server replies : %s\n", msg);

	fflush(stdout);

#endif
}

int CheckCondWaitQueue_Syscall(int condID)
{
#ifndef NETWORK
	Condition *condition;
	condition = condQueue[condID];
	int rv;
	rv = condition->isSomebodyWaiting();
	return rv;
#else

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_CheckCondWaitQueue, myMachineID, fromBox,condID,0,0,&time,"isSomebodyWaiting");
	sendIt(data, buffer, SC_CheckCondWaitQueue);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_CheckCondWaitQueue)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1 == -1)
		printf("Error in checking\n");
	fflush(stdout);
	return parameter1;
#endif
}
	
void DeleteCondition_Syscall(int condID)
{
#ifndef NETWORK

	Condition *condition;
	if((condID<0)||(condID>=currentConditionID))
	{
		printf("Error in deleting : condition ID %d doesn't exist or is negative\n",condID);
	}
	else
	{
	condition = condQueue[condID];	
	condition->~Condition();
	DEBUG('y',"Condition Variable Deleted \n");	
	}

#else


	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	 int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_DeleteCondition,myMachineID, fromBox,condID,0,0,&time,"DeleteCond");
	sendIt(data, buffer, SC_DeleteCondition);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_DeleteCondition)
	  {
	    printf("Something went wrong\n");
	  }
	if(parameter1==1)
	{
	//  printf("Finished Deleting\n");
	}
	else
	  printf("Error in Deletion, Server replies : %s\n", msg);

	fflush(stdout);

#endif
}

//Syscall to create a shared integer
int CreateSharedInt_Syscall(unsigned int vaddr, int len, int arraySize) //name, length of name, length of array
{
	char *buf;
	int rv, condID;
	if(!(buf = new char[len+1]))
	{
		printf("%s","Error allocating kernel buffer in CreateCondition\n");
		return -1;
	}
	if(copyin(vaddr, len, buf)==-1)
	{
		printf("%s","Bad pointer passed to CreateCondition\n");
		delete[] buf;
		return -1;
	}
	buf[len]='\0';
#ifdef NETWORK

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_CreateSharedInt,myMachineID, fromBox,arraySize,0,0,&time,buf); //Form a packet with message type CLIENTREQ
	sendIt(data, buffer, SC_CreateSharedInt);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_CreateSharedInt)
	  {
	    printf("Something went wrong\n");
	  }
	fflush(stdout);
	if(parameter1 == -1)
	  printf("Error in allocating a shared integer\n");
	//	else
	  //	printf("Successfully created a shared integer\n");
	return parameter1;

#else
	printf("Call this while running from the network directory\n");
#endif
}

int GetSharedInt_Syscall(int integerID, int position) //id, position
{
#ifdef NETWORK

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_GetSharedInt,myMachineID, fromBox,integerID,position,0,&time,"GetSharedInt");
	sendIt(data, buffer, SC_GetSharedInt);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_GetSharedInt)
	{
	    printf("Something went wrong\n");
	}
	if(parameter1 == -1)
	{
		printf("Getting unsuccesful\n");
	}
	else
	{
	  		printf("Successfully got a shared integer\n");
	}
	fflush(stdout);
	return parameter1;

#else
	printf("Call this while running from the network directory\n");
#endif
}

void SetSharedInt_Syscall(int integerID, int position, int value) //id, position, value to set
{
#ifdef NETWORK

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_SetSharedInt,myMachineID, fromBox,integerID,position,value,&time,"SetSharedInt");
	sendIt(data, buffer, SC_SetSharedInt);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_SetSharedInt)
	  {
	    printf("Something went wrong\n");
	  }
	else if(parameter1 != 1)
	{
		 printf("Setting integer value unsuccessful, integerID = %d, position = %d and value = %d \n", integerID, position, value);
	}
	else
	{
			//	printf("Successfully set a shared integer\n");
	}
	fflush(stdout);

	return;

#else
	printf("Call this function (SetSharedInt) while running from the network directory\n");
#endif
}

int GetOneIndex_Syscall(int integerID) //id
{
#ifdef NETWORK

	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_GetOneIndex,myMachineID, fromBox,integerID,0,0,&time,"GetOneIndex");
	sendIt(data, buffer, SC_GetOneIndex);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_GetOneIndex)
	  {
	    printf("Something went wrong\n");
	  }

	fflush(stdout);

	return parameter1;
#else
	printf("Call this function (GetOneIndex) while running from the network directory\n");

#endif
}

int GetZeroIndex_Syscall(int integerID) //id
{
#ifdef NETWORK


	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_GetZeroIndex,myMachineID, fromBox,integerID,0,0,&time,"GetZeroIndex");
	sendIt(data, buffer, SC_GetZeroIndex);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_GetZeroIndex)
	  {
	    printf("Something went wrong\n");
	  }

	fflush(stdout);

	return parameter1;
#else
	printf("Call this function (GetZeroIndex) while running from the network directory\n");

#endif
}

int ArraySearch_Syscall(int integerID, int skipIndex, int equalityValue) //id
{
#ifdef NETWORK


	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	int fromBox = currentThread->threadID%100+2;
	timeCounterClass time = getTimeStamp();
	formPacket(data, CLIENTREQ, SC_ArraySearch,myMachineID, fromBox,integerID,skipIndex,equalityValue,&time,"GetZeroIndex");
	sendIt(data, buffer, SC_ArraySearch);
	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3, msg);
	if(syscallType!=SC_ArraySearch)
	  {
	    printf("Something went wrong\n");
	  }

	fflush(stdout);

	return parameter1;
#else
	printf("Call this function (SearchArray) while running from the network directory\n");

#endif
}


void exec_thread(int what)
{
    currentThread->space->InitRegisters();		// set the initial register values
    currentThread->space->RestoreState();		// load page table register
    machine->Run();								// jump to the user progam
}

SpaceId Exec_Syscall(int vaddr, int len)
{	
	char *buf;
	int rv;	
	if(!(buf = new char[len+1]))
	{
		printf("%s","Error allocating kernel buffer in CreateLock\n");
		return -1;
	}
	if(copyin(vaddr, len, buf)==-1)
	{
		printf("%s","Bad pointer passed to CreateLock\n");
		delete[] buf;
		return -1;
	}
	buf[len]='\0';
	OpenFile *executable = fileSystem->Open(buf);
	AddrSpace *space;
	
	if (executable == NULL) {
	  printf("Unable to open file %s\n", buf);
	  return -1;
	}

	int i;
    processTableLock->Acquire();
	i = PIDCount++;
	space = new AddrSpace(executable,i);
	procName[i*4+0] = 'P';
	procName[i*4+1] =  (char) (i + 48);
	procName[i*4+2] = 'T';
	procName[i*4+3] = '\0';
	Thread *newThread = new Thread(procName+4*i);
	newThread->space = space;
	newThread->PID = i;
	newThread->threadID = 0;
	newThread->isMainThread = 1;
	rv = i;
	int parentPID = currentThread->PID;
	pTable[i].PID = i;
	pTable[i].parentSpace = currentThread->space;
	pTable[i].space = space;
	pTable[i].childProcessCount = 0;
	//Initialize the childProcessDetailsArray
	for(int k=0;k<MAXPCHILDREN;k++)
	{
		pTable[i].childProcessDetailsArray[k].childPID=-1;
		//setting child address space too null is optional and not required.
	}
	pTable[i].threadCount = 0;
	pTable[i].parentThread = NULL;
	pTable[i].waitForExiting = new Condition("root parent exit condition");
	pTable[i].exitStatus = TRUE;

	int c = pTable[parentPID].childProcessCount++;
	for(int j=0;j<=c;j++)
	{
		if(pTable[parentPID].childProcessDetailsArray[j].childPID==-1)
		{
			// use this spot.
			pTable[parentPID].childProcessDetailsArray[j].childPID = i;
			pTable[parentPID].childProcessDetailsArray[j].childSpace = space;
			break;
		}
	}

	processTableLock->Release();
	newThread->Fork(exec_thread,1); 
	return rv;
}

void kernel_thread(int vaddr)
{
	machine->WriteRegister(PCReg,vaddr);
	machine->WriteRegister(NextPCReg,vaddr+4);
	currentThread->space->RestoreState();
	machine->WriteRegister(StackReg,(currentThread->stackPos)*PageSize-16);
	machine->Run();
}


SpaceId Fork_Syscall(int vaddr)
{
	unsigned int numPages, newNumPages,i,j;
	int k,thisPID,rv;
	AddrSpace *space;
	forkLock->Acquire();
	Thread *forkThread = new Thread("Thread to be Forked");
	forkThread->isMainThread = 0;
	numPages = currentThread->space->getNumPages();
	newNumPages = numPages + 8;
	// Make a new pagetable
	TranslationEntry *pageTable;
	TranslationEntry *newPageTable = new TranslationEntry[newNumPages];
	space = currentThread->space;
	pageTable = space->getPageTable();
	// Copy the existing page table to the new pages table.
	for(i=0;i<numPages;i++)
	{
		newPageTable[i].virtualPage = pageTable[i].virtualPage;
		newPageTable[i].physicalPage = pageTable[i].physicalPage;
		newPageTable[i].valid = pageTable[i].valid;
		newPageTable[i].dirty = pageTable[i].dirty;
		newPageTable[i].use = pageTable[i].use;
		newPageTable[i].readOnly = pageTable[i].readOnly;
	}
	for(i=numPages;i<newNumPages;i++)
	{
		int *p = new int;
		*p = 1;
		k = memorySimulator->Put((void *)p);
		if(k==-1)
		{
			printf("Main Memory Full\n");
			exit(0);
		}
		else
		{
			j = (unsigned int)k;
			newPageTable[i].virtualPage = i;
			newPageTable[i].physicalPage = j;
			newPageTable[i].valid = TRUE;
			newPageTable[i].dirty = FALSE;
			newPageTable[i].use = FALSE;
			newPageTable[i].readOnly = FALSE;
			bzero(&machine->mainMemory[PageSize*j],PageSize);
		}
	}
	forkThread->space = space;	
	forkThread->PID = currentThread->PID;
	forkThread->stackPos = newNumPages;
	thisPID = currentThread->PID;
	//Update the Process Table
	processTableLock->Acquire();
	pTable[thisPID].parentThread = currentThread;
	rv = ++pTable[thisPID].threadCount;
	forkThread->threadID = rv;
	pTable[thisPID].exitStatus = FALSE;
	processTableLock->Release();

	space->setPageTable(newPageTable,newNumPages);
	machine->pageTable = newPageTable;
	machine->pageTableSize = newNumPages;
	delete[] pageTable;
	forkLock->Release();
	forkThread->Fork(kernel_thread,vaddr);
	return rv;

}

void Exit_Syscall(int status)
{
	 DEBUG('y',"started exit command\n");
     
	 int pid = currentThread->PID;
	 if(status!=0)
		 	printf("Exit Value = %d\n",status);

	 if(currentThread->isMainThread!=1)
	 {
	   processTableLock->Acquire();
		 if(pTable[pid].threadCount>1)
		 {
			 
 			 DEBUG('y',"Thread of Process %d going to finish, ThreadCount = %d. Finish.\n",pid,pTable[pid].threadCount);
			 pTable[pid].threadCount--;
			 processTableLock->Release();
			 
			 currentThread->Finish();
		 }
		 else if(pTable[pid].threadCount==1) //can't be equal to zero.
		 {
			 //wake up my parent thread.
			 pTable[pid].exitStatus = TRUE;
 			 DEBUG('y',"Last Thread of Process %d going to finish, ThreadCount = %d. Finish.\n",pid,pTable[pid].threadCount);
			 pTable[pid].waitForExiting->Signal(processTableLock);
			 DEBUG('y',"going to wake up parent\n");
			 
			 pTable[pid].threadCount--; //will become zero now (after the decrement)
			 processTableLock->Release();
			 currentThread->Finish();
		 }
		 
	 }
	 else
	 {
	   processTableLock->Acquire();
		 if(pTable[pid].exitStatus==FALSE)
			 DEBUG('y',"Going to sleep\n");
		 while(pTable[pid].exitStatus==FALSE)
			 pTable[pid].waitForExiting->Wait(processTableLock);
		 // exitStatus is true which means that there are no other child threads of this main thread.
		 //some child processes are running. update their process table entry also
		 for(int childPID = 0; childPID < pTable[pid].childProcessCount;childPID++)
		 {
			 pTable[childPID].parentPID = -1;
			 pTable[childPID].parentSpace = NULL;
		 }
		 //Now also take care of the parent process.
		 int parentPID = pTable[pid].parentPID;
		 if(parentPID!=-1)
		 {
			 //So i have a parent. Update its process table too.
			 // find where i am in the parent's process table.
			 for(int k=0;k<pTable[parentPID].childProcessCount;k++)
			 {
				 if(pTable[parentPID].childProcessDetailsArray[k].childPID==pid)
				 {
					 //Got the entry. Delete this one. Instead, put the last entry into this place.
					 //But this shouldn't be the last one.
					 int count = pTable[parentPID].childProcessCount;					
					 pTable[parentPID].childProcessDetailsArray[k].childPID = pTable[parentPID].childProcessDetailsArray[count-1].childPID;
					 pTable[parentPID].childProcessDetailsArray[k].childSpace = pTable[parentPID].childProcessDetailsArray[count-1].childSpace;
					 pTable[parentPID].childProcessDetailsArray[count-1].childPID = -1;
					 pTable[parentPID].childProcessDetailsArray[count-1].childSpace = NULL;
					 break;
				 }				  
			 }
			 pTable[parentPID].childProcessCount--;
			 //Finally I got to take care of myself!
		 }
		 pTable[pid].PID = -1;

		 //Do the cleaning up
		 //i am the main thread of the exiting process. So check whether my process is the last process or not
		 int isLastProcess = TRUE;
		 for(int k=0;k<MAXPROCESSES;k++)
		 {
			 if(k!=pid)
			 {
				if(pTable[k].PID!=-1)
				 {
					isLastProcess = FALSE;
					break;
				 }
			 }
		 }
	
		 if(isLastProcess)
		 {
			 // Remove AddressSpace and halt
			 DEBUG('y',"Last Process %d going to terminate\n",currentThread->PID);	
			 processTableLock->Release();
			 currentThread->space->~AddrSpace();
			 interrupt->Halt();
		 }
		 else
		 {
			 //Remove AddressSpace and do a finish
			 DEBUG('y',"Process %d going to finish (not the last process)\n",currentThread->PID);
			 processTableLock->Release();
			 currentThread->space->~AddrSpace();
			 currentThread->Finish();
		 }		 	 
	 }
}


void WriteNum_Syscall(int i)
{
	printf("%d",i);
}



/* Just a Test user system call */
void TestMe_Syscall(int p, int toMach)
{

	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int syscallType, parameter1, parameter2, parameter3;
	char msg[MaxMailSize];
	
	timeCounterClass time;// = getTimeStamp();
	//int randNo = rand()%50;
	printf("Sending %2d\n", p);
	int fromBox = currentThread->threadID%100 + 2;
	formPacket(data, CLIENTREQ, SC_TestMe,myMachineID, fromBox, p,0,0,&time,"TestMe");
	
	sendIt(data, buffer,SC_TestMe);

	
	//printf("from Server = %d\n", inPktHdr.from);
	


	parsePacket(buffer, &syscallType, &parameter1, &parameter2, &parameter3,msg);
	if(syscallType!=SC_TestMe)
	  {
	    printf("[Createlock]Something went wrong, syscallType = %d \n",syscallType);
	  }
	//	  printf("lock successfully created, id = %d\n",parameter1);
	printf("Getting %d\n\n", parameter1);

	fflush(stdout);

}

void Delay_Syscall(int i)
{
	Delay(i);
}

int RandomFunction_Syscall(int i)
{
	return (1+(getTime()+rand())%i);
}

int MemAllocate_Syscall(int vaddr, int s)
{
	
	int *a = new int;
	machine->Translate(vaddr, a, 1, FALSE);
	int *b = new int[s];
	*a = (int)b;
	return 1;
}

void Concatenate_Syscall(int vaddr,int len,int a, int vaddr2)
{
	char *buf;		
	if(!(buf = new char[len+2]))
	{
		printf("%s","Error allocating kernel buffer in Concatenate\n");
		return;
	}
	if(copyin(vaddr, len, buf)==-1)
	{
		printf("%s","Bad pointer passed to Concatenate\n");
		delete[] buf;
		return;
	}
	buf[len]='\0';
	for (int ii=0;ii<=len;ii++ )
	{
		if(buf[ii]=='\0')
		{
			buf[ii]=a+48;
			buf[ii+1]='\0';
			break;
		}
	}
	if(copyout(vaddr2, len+2, buf)==-1)
	{
		printf("%s","Bad pointer for outputing in Concatenate\n");
	}
}

void Print_Syscall(int ID)
{
	// Again, another test system call	
}

void Halt_Syscall()
{
	interrupt->Halt();
}

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Find out the type of the exception (applicable to syscall)
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
	switch (type) {
	    default:
		DEBUG('z', "Unknown syscall - shutting down.\n");
	    case SC_Halt:
		DEBUG('z', "Shutdown, initiated by user program.\n");
		Halt_Syscall();
		break;
	    case SC_Create:
		DEBUG('z', "Create syscall.\n");
		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Open:
		DEBUG('z', "Open syscall.\n");
		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Write:
		DEBUG('z', "Write syscall.\n");
		Write_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Read:
		DEBUG('z', "Read syscall.\n");
		rv = Read_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Close:
		DEBUG('z', "Close syscall.\n");
		Close_Syscall(machine->ReadRegister(4));
		break;

		case SC_CreateLock:
			DEBUG('z', "CreateLock syscall.\n");
			rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;

		case SC_AcquireLock:
			DEBUG('z', "AcquireLock syscall.\n");		    
			AcquireLock_Syscall(machine->ReadRegister(4));			
			break;

		case SC_ReleaseLock:
			DEBUG('z', "ReleaseLock syscall.\n");		    
			ReleaseLock_Syscall(machine->ReadRegister(4));			
			break;

		case SC_DeleteLock:
			DEBUG('z', "DeleteLock syscall.\n");		    
			DeleteLock_Syscall(machine->ReadRegister(4));			
			break;		

		case SC_CreateCondition:
			DEBUG('z', "CreateCondition syscall.\n");		    
			rv = CreateCondition_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));			
			break;

		case SC_WaitCV:
			DEBUG('z', "WaitCV syscall.\n");		    
			WaitCV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));			
			break;

		case SC_SignalCV:
			DEBUG('z', "SignalCV syscall.\n");		    
			SignalCV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));			
			break;

		case SC_BroadcastCV:
			DEBUG('z', "BroadcastCV syscall.\n");		    
			BroadcastCV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));			
			break;

		case SC_DeleteCondition:
			DEBUG('z', "DeleteCondition syscall.\n");		    
			DeleteCondition_Syscall(machine->ReadRegister(4));			
			break;

		case SC_Yield:
			DEBUG('z',"Yield syscall.\n");
			currentThread->Yield();
			break;

		case SC_TestMe:
			DEBUG('z',"User test syscall.\n");
			TestMe_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));
			break;

		case SC_CheckCondWaitQueue:
			DEBUG('z',"CheckCondWaitQueue syscall.\n");
			rv = CheckCondWaitQueue_Syscall(machine->ReadRegister(4));
			break;

		case SC_Exec:
			DEBUG('z',"Exec syscall.\n");
			rv = Exec_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));
			break;

		case SC_Fork:
			DEBUG('z',"Fork syscall.\n");
			rv = Fork_Syscall(machine->ReadRegister(4));
			break;

		case SC_Exit:
			DEBUG('z',"Exit syscall.\n");
			Exit_Syscall(machine->ReadRegister(4));
			break;

		case SC_Random:
			DEBUG('z',"Random Number Generator syscall.\n");
			rv = RandomFunction_Syscall(machine->ReadRegister(4));
			break;
			
		case SC_MemAllocate:
			DEBUG('z',"Dynamic Memory Allocation Syscall\n");
			rv = MemAllocate_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));
			break;

		case SC_Concatenate:
			DEBUG('z',"Dynamic Memory Allocation Syscall\n");
			Concatenate_Syscall(machine->ReadRegister(4),machine->ReadRegister(5),machine->ReadRegister(6),machine->ReadRegister(7));
			break;

		case SC_PrintStuff:
			DEBUG('z',"Print syscall.\n");
			Print_Syscall(machine->ReadRegister(4));
			break;

		case SC_WriteNum:
			DEBUG('z',"Print syscall.\n");
			WriteNum_Syscall(machine->ReadRegister(4));
			break;
		
		case SC_Delay:
			DEBUG('z', "Delay Syscall\n");
			Delay_Syscall(machine->ReadRegister(4));
			break;

		case SC_CreateSharedInt:
			DEBUG('z', "CreateSharedInt syscall\n");
			rv = CreateSharedInt_Syscall(machine->ReadRegister(4),machine->ReadRegister(5),machine->ReadRegister(6));
			break;

		case SC_GetSharedInt:
			DEBUG('z', "GetSharedInt syscall\n");
			rv = GetSharedInt_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));
			break;

		case SC_SetSharedInt:
			DEBUG('z', "ClearSharedInt syscall\n");
			SetSharedInt_Syscall(machine->ReadRegister(4),machine->ReadRegister(5),machine->ReadRegister(6));
			break;

		case SC_GetOneIndex:
			DEBUG('z',"GetOneIndex syscall\n");
			rv = GetOneIndex_Syscall(machine->ReadRegister(4));
			break;
		
		case SC_GetZeroIndex:
			DEBUG('z',"GetOneIndex syscall\n");
			rv = GetZeroIndex_Syscall(machine->ReadRegister(4));
			break;

		case SC_ArraySearch:
			DEBUG('z',"ArraySearch syscall\n");
			rv = ArraySearch_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
			break;

	}

	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
    }
	else if(which == PageFaultException){
		printf("Page Fault occurred!!\n");
	}
	else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}
