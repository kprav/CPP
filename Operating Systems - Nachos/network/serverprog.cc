#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
//#include "serverutils.h"
#define MAXSERVERS 20
#define HBT_PERIOD 5

int currentSLockID = 0;
int currentSCondID = 0;
int currentIntegerID = 0;
PacketHeader outPktHdr, inPktHdr;
MailHeader outMailHdr, inMailHdr;
char buffer[MaxMailSize];
char data[MaxMailSize];
bool success;
bool sendAckValid = FALSE;
timeCounterClass timeStampTable[MAXSERVERS];
int alive[MAXSERVERS];
Lock *aliveLock = new Lock("aliveLock at server");
List *waitingQueue = new List();
int messageIndex;
int repliedMessageCounter = 0;
List *repliedMessageQueue = new List();
//timeStampTable = new timeCounterClass[NServers];
//alive = new int[NServers];

NetLocksClass *NetLocks = new NetLocksClass[MAX];
NetCondsClass *NetConds = new NetCondsClass[MAX];
sharedIntsClass *sharedInts = new sharedIntsClass[MAX];

int syscallType, parameter1, parameter2, parameter3;
char msg[MaxMailSize];

//function to display time.
void displayTime(timeCounterClass t)
{
	printf("(%d, %d)",t.t1, t.t2);
}

// function to update the timestamp table.
// update the timestamp entry only if the incoming entry is more than existing entry.

void updateTimeStampTable(int who, timeCounterClass timeStamp)
{
	int greater;
	greater = isTimeGreater(timeStampTable[who],timeStamp);
	if(greater == 0) //so not a tie and not greater. we don't bother about tie now.
		timeStampTable[who] = timeStamp;
	printf("TIMESTAMP TABLE");
	for(int i=0;i<NServers;i++)
	{
		printf(" | ");displayTime(timeStampTable[i]);
	}
	printf("|\n");
}

//function to find the minimum time in the time stamp table.
// it takes into consideration which all servers are alive and which are dead
// we will disregard the time stamp of dead servers
int findMinTime()
{
	int i, greater, minWho;
	timeCounterClass min;
	aliveLock->Acquire();
	for (i=0;i<NServers ;i++ )
	{
		if(alive[i])
			break;
	}
	// we just found the first alive server. 
	min = timeStampTable[i];
	minWho = i;
	//printf("In findMinTime, i = %d | alive[0] = %d and alive[1] = %d ", i, alive[0],alive[1]);
	for(i=0;i<NServers;i++)
	{
		//alive[i] = 1;
		if(alive[i])
		{
			greater = isTimeGreater(timeStampTable[i],min);  
			if(greater == 0)
			{
				min = timeStampTable[i];
				minWho = i;
			}
		}
	}
	aliveLock->Release();
	//printf("minWho = %d\n",minWho);
	return minWho;
}

// -------------------------------------------------------------------------------------------------------------------------
//	This function is used to send acknowledgements - basically messages. Once the packet has been formed by some function,
//	it calls this function with the appropriate values of inPktHdr_from, inMailHdr_from and the acknowledgement. 
//	This function then creates the necessary headers and then calls the postoffice Send.
//  The funciton will send only if the global value sendAckValid is true or not. This parameter is set in the StartServer 
//  function.
// -------------------------------------------------------------------------------------------------------------------------

void SendAck(int inPktHdr_from, int inMailHdr_from, char *ack_local)
{

	printf("In SendAck function : sending %s\n",ack_local + 19);
	 // Send acknowledgement
	
	printf("sending acknowledgement to %d\n",inPktHdr_from);
	outPktHdr.to = inPktHdr_from;
	outMailHdr.to = inMailHdr_from;
	outMailHdr.length = MaxMailSize;
	appendNumber(ack_local, messageIndex);
	timeCounterClass t(messageIndex, inPktHdr_from);

	waitingQueueMember *wMember = new waitingQueueMember(ack_local, t, 0, TRUE);
	repliedMessageQueue->Append((void *)wMember);
	repliedMessageCounter++;
	if(repliedMessageCounter>=500)
	{
		repliedMessageQueue->Remove();
		repliedMessageCounter--;
	}

	if(sendAckValid)
	{ 
      success = postOffice->Send(outPktHdr, outMailHdr, ack_local);
      if(ack_local[0]==SERVERPNG)
		  interrupt->Halt();
      if ( !success ) {
			printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();  // we will halt if the client is dead. The client can't be dead!
      }
	}
}
// -------------------------------------------------------------------------------------------------------------------------
//	This function handles the job of creating locks. It receives the name of the lock in variable name. 
//	It searches whether this lock already exists using the name. If not then it creates a new lock and sends the index
//	back to the client.
//	If the lock already exists, then the client receives the identifier of the existing lock
// -------------------------------------------------------------------------------------------------------------------------

void CreateLock_Netcall(char *name, int fromMachine, int fromBox)
{
	printf("CreateLock Netcall. Name = %s, origniating request from Client%d\n",name, fromMachine);
	//Code for CreateLock
	//First check whether the lock has been already created!
	int i=0;
	int lockID;
	char ack[MaxMailSize];
	bool found = FALSE;
	for(i=0;i<MAX;i++)
	{
		if(!strcmp(NetLocks[i].name,name))
		{
			found = TRUE;
			break;
		}
	}
	if(found)
	{
		lockID = i;
		printf("Lock Already exists. Passing on that value of lockID to the client. LockID = %d\n",lockID);
		NetLocks[lockID].exists = TRUE;
	}
	else
	{
		lockID = currentSLockID++;
		NetLocks[lockID].exists = TRUE; //Lock Created!
		strcpy(NetLocks[lockID].name, name);
	    printf("Lock creation successful. LockID = %d created\n",lockID);
	}
	timeCounterClass tempC;
	formPacket(ack, SC_CreateLock,lockID, 0, "Acknowledgement");
	SendAck(fromMachine,fromBox, ack);
}
// -------------------------------------------------------------------------------------------------------------------------
//	This function checks whether the lock exists or not. If the lock exists, it sees whether the lock is busy or not.
//	If the lock is not busy, then it makes this client the owner of the lock and then sends an acknowledgement back 
//	to the client saying that it has acquired the lock (by setting parameter1 to 1 which indicates a success).
//	If the lock is already busy, then it frames a new reply and appends it to the lock's wait queue defined by replyQueue.
// -------------------------------------------------------------------------------------------------------------------------

void AcquireLock_Netcall(int lockID, int fromMachine, int fromBox)
{
	//Code for AcquireLock
	char ack[MaxMailSize];
	printf("Acquire Netcall from Machine %d. lockID = %d.\n",fromMachine, lockID);
	if(NetLocks[lockID].exists)
	{
	  formPacket(ack, SC_AcquireLock,1, 0,"Acknowledgement"); //Keep the ack handy, but don't use it immediately.
	  appendNumber(ack, messageIndex);
		if(!NetLocks[lockID].isBusy)
		{
			NetLocks[lockID].isBusy = TRUE;
			NetLocks[lockID].ownerMachine = makeOwnerID(fromMachine, fromBox);	
			SendAck(fromMachine, fromBox, ack);
		}
		else
		{
			//Add it to the reply message queue;
		//	if(!NetLocks[lockID].replyQueue->isPresent(messageIndex, makeOwnerID(fromMachine, fromBox)))  // Do not add duplicate messages
		//	{
				replyType *reply = new replyType(fromMachine,fromBox,ack);
				NetLocks[lockID].replyQueue->Append((void *)reply);
				//Not sending the acknowledgement now since the Acquire call has not been successul. 
				// The requesting machins is actually waiting now.
		//	}
		}		
	}
	else
	  {
	    formPacket(ack, SC_AcquireLock,0, 0,"Lock doesn't exist");
	    printf("Cannot Acquire a lock that has not yet been created!\n");
	    SendAck(fromMachine, fromBox,ack);
	  }
}

// -------------------------------------------------------------------------------------------------------------------------
//	This does a couple of checkings - whether the lock exists or not, whether the lock is busy or not and whether
//	the client trying to release the lock is the owner of the lock. If something goes wrong it sends back a 0 to the client
//	with suitable error message. If everything is correct, then it sees whether there are some more clients waiting for
//	the lock (by checking the replyQueue). If there are none, then it just frees up the lock by making the isBusy field to 
//	be false and removing the owner information. If there is someone waiting, the lock's owner information is updated to
//	the waiter's id and then an acknowledgement is sent to the client which was waiting for the lock to be acquired 
//	saying that the lock has been acquired. 
// -------------------------------------------------------------------------------------------------------------------------

void ReleaseLock_Netcall(int lockID, int fromMachine, int fromBox)
{
	printf("Release netcall from Machine %d. lockID = %d.\n",fromMachine, lockID);
	char ack[MaxMailSize];

	if(!NetLocks[lockID].exists)
	{
		printf("The lock doesn't exist! So can't be released (first create the lock)\n");
		formPacket(ack, SC_ReleaseLock,0, 0,"Lock Release Failure"); 
		SendAck(fromMachine, fromBox, ack);
	}
	else if(!NetLocks[lockID].isBusy)
	{
		printf("The lock seems to be already free!\n"); // May be suitably remove this.
		formPacket(ack, SC_ReleaseLock,0, 0,"Free lock"); 
		SendAck(fromMachine, fromBox, ack);
	}
	else if(NetLocks[lockID].ownerMachine!=makeOwnerID(fromMachine,fromBox))
	{
		printf("The machine trying to release the lock is not the owner of the lock : ownerMachine = %d and the machine releasing is %d\n",NetLocks[lockID].ownerMachine,makeOwnerID(fromMachine,fromBox));
		formPacket(ack, SC_ReleaseLock,0, 0,"Not owner"); 
		SendAck(fromMachine, fromBox, ack);
	}
	else
	{
		formPacket(ack, SC_ReleaseLock,1, 0,"Lock Released"); 
		SendAck(fromMachine, fromBox, ack);
		//Also send ack to the next guy - so find out the next reply message and send it
		replyType *reply = (replyType *)NetLocks[lockID].replyQueue->Remove();
		if(reply!=NULL)
		{
			//Update the lock with the new owner's information
			NetLocks[lockID].ownerMachine = makeOwnerID(reply->machineID,reply->inMailHdr_from);
			//The lock is still busy and it still exists.
			messageIndex = getNumber(reply->ack);
			SendAck(reply->inPktHdr_from, reply->inMailHdr_from, reply->ack);
		}
		else
		{
			NetLocks[lockID].isBusy = FALSE;
			NetLocks[lockID].ownerMachine = -1; //No owners exist!
			//	SendAck(inPktHdr.from, inMailHdr.from, ack);
		}
	}		

}
// -------------------------------------------------------------------------------------------------------------------------
//	Again, this does a couple of checkings - whether the lock is exists and whether it is busy. Then it deletes the lock
//	by setting the exists bit to FALSE and then sending appropriate acknowledgement to the client.
// -------------------------------------------------------------------------------------------------------------------------

void DeleteLock_Netcall(int lockID, int fromMachine, int fromBox)
{
	printf("Delete lock netcall from Machine %d. lockID = %d.\n",fromMachine, lockID);
	char ack[MaxMailSize];

	if(!NetLocks[lockID].exists)
	{
		printf("The lock doesn't exist! So can't be deleted (first create the lock)\n");
		formPacket(ack, SC_DeleteLock,0, 0,"Lock non-existent"); 
	}
	else if(NetLocks[lockID].isBusy)
	{
		printf("The lock is busy. Can't be deleted\n");
		formPacket(ack, SC_DeleteLock,0, 0,"Lock in use"); 
	}
	else
	{
		NetLocks[lockID].exists = FALSE;
		formPacket(ack, SC_DeleteLock,1, 0,"Success");
	}
	SendAck(fromMachine, fromBox, ack);
}

// -------------------------------------------------------------------------------------------------------------------------
//	This is similar to the CreateLock_Netcall. It checks whether the condition already exists or not depending on which
//	it either creates a new condition or sends the existing conditions identifier to the client. 
// -------------------------------------------------------------------------------------------------------------------------

void CreateCondition_Netcall(char *name, int fromMachine, int fromBox)
{
	printf("CreateCondition Netcall. Name = %s, origniating request from Client%d\n",name,fromMachine);
	//Code for CreateCondition
	//First check whether the condition has been already created!
	int i=0;
	int condID;
	char ack[MaxMailSize];
	bool found = FALSE;
	for(i=0;i<MAX;i++)
	{
		if(!strcmp(NetConds[i].name,name))
		{
			found = TRUE;
			break;
		}
	}
	if(found)
	{
		condID = i;
		printf("Condition Already exists. Passing on that value of condID to the client. CondID = %d\n",condID);
		NetConds[condID].exists = TRUE; 
	}
	else
	{
		condID = currentSCondID++;
		NetConds[condID].exists = TRUE; //Condition Created!
		strcpy(NetConds[condID].name, name);
	    printf("Condition creation successful. CondID %d created\n",condID);
	}
	formPacket(ack, SC_CreateCondition,condID, 0,"Acknowledgement");
	SendAck(fromMachine, fromBox, ack);
}

// -------------------------------------------------------------------------------------------------------------------------
//	The function begins with a series of error checkings - whether the lock and the condition exists and whether the
//	client has already acquired the lock. If we find that there is no lock associated with the condition, we make
//	that association and then frame a new reply message and append it to the queue. If there is already some association
//	between the lock and the condition then we see whether the incoming lock and condition match that assoication. If not
//	then an error message is sent back to the client. If correct, then we append this reply to the condition's wait queue.
//	No acknowledgement is sent to the client, but lock is released from the client. So the next member that was waiting 
//	for the lock receives an ack that it got the lock. 
// -------------------------------------------------------------------------------------------------------------------------	

void WaitCV_Netcall(int condID, int lockID, int fromMachine, int fromBox)
{
	printf("Wait netcall from Machine %d. condID = %d, lockID = %d\n",fromMachine, condID, lockID);
	char ack[MaxMailSize];
	if((!NetLocks[lockID].exists)||(!NetConds[condID].exists))
	{
		printf("Either the lockID or the condID doesn't exist\n");
		formPacket(ack, SC_WaitCV, 0, 0, "LockID/CondID non-existent");
		SendAck(fromMachine, fromBox, ack);
	}
//	else if(!((NetConds[condID].replyQueue->isPresent(messageIndex, makeOwnerID(fromMachine, fromBox)))||(NetLocks[lockID].replyQueue->isPresent(messageIndex, makeOwnerID(fromMachine, fromBox)))))  // Do not add duplicate messages
//	{	
		else if(NetLocks[lockID].ownerMachine!=makeOwnerID(fromMachine,fromBox))
		{
			printf("Waiting on a lock that a machine has not acquired!\n");
			formPacket(ack, SC_WaitCV, 0, 0, "Wait lock unacquired");
			SendAck(fromMachine, fromBox, ack);
		}
		else 
		  {
			if(NetConds[condID].lockID==-1)
			{
				NetConds[condID].lockID = lockID;
				printf("Lock has been associated with the condition\n");
			}
			if(NetConds[condID].lockID!=lockID)
			{
				printf("Condition has to be waited on a different lockID\n");
				formPacket(ack, SC_WaitCV, 0, 0, "acqd lock doesn't match cond lock");
				SendAck(fromMachine, fromBox, ack);
			}
			else
			{
				
					formPacket(ack, SC_WaitCV, 1, 0, "Wait Finished");
					appendNumber(ack,messageIndex);
					replyType *reply = new replyType(fromMachine, fromBox, ack);
					NetConds[condID].replyQueue->Append((void *)reply);
					NetConds[condID].queueCount++;
				
				// Release the Lock without sending any Acknowledgement to the current machine which is waiting.
				// But of course we have to send a ack to the next waiting member for the lock.
				replyType *reply_release = (replyType *)NetLocks[lockID].replyQueue->Remove();
				formPacket(ack, SC_ReleaseLock,1, 0,"Lock Released"); 
				if(reply_release!=NULL)
				{
					//Update the lock with the new owner's information
					NetLocks[lockID].ownerMachine = makeOwnerID(reply_release->machineID,reply_release->inMailHdr_from);
					//The lock is still busy and it still exists.
					messageIndex = getNumber(reply_release->ack);
					SendAck(reply_release->inPktHdr_from, reply_release->inMailHdr_from, reply_release->ack);
				}
				else
				{
					NetLocks[lockID].isBusy = FALSE;
					NetLocks[lockID].ownerMachine = -1; //No owners exist!
				}
			}
		}
//	}
}
// -------------------------------------------------------------------------------------------------------------------------
//	The SignalCV_Netcall checks whether both the lock and the condition exist or not, whether the lock and condition
//	match each other (ie this is the lock that somebody uses to wait on the condition) and whether the client calling
//	signal has already acquired the lock. If everything goes correct, then we remove a reply from the list and then add
//	it to the wait queue of the lock. So when this client calls a ReleaseLock netcall, the ReleaseLock_Netcall
//	automatically makes the waiter the owner of the lock (when its turn comes). Further, if nobody was waiting but a
//	signal was called upon, we de-associate the lock and the condition variables.
// -------------------------------------------------------------------------------------------------------------------------

void SignalCV_Netcall(int condID, int lockID, int fromMachine, int fromBox)
{
	printf("Signal netcall from Machine %d. lockID = %d, condID = %d\n",fromMachine, lockID, condID);
	char ack[MaxMailSize], ack_signal[MaxMailSize];
	if((!NetLocks[lockID].exists)||(!NetConds[condID].exists))
	{
		printf("Either the lockID or the condID doesn't exist\n");
		formPacket(ack, SC_SignalCV, 0, 0, "LockID/CondID non-existent");
		SendAck(fromMachine, fromBox, ack);
	}
	else if(NetLocks[lockID].ownerMachine!=makeOwnerID(fromMachine,fromBox))
	{
		printf("Signaling using a lock that a machine has not acquired!\n");
		formPacket(ack, SC_SignalCV, 0, 0, "Lock not acquired");
		SendAck(fromMachine, fromBox, ack);
	}
	else
	{
		if(NetConds[condID].lockID==-1)
		{
			printf("Nobody is waiting!!\n");
			formPacket(ack, SC_SignalCV, 1, 0, "Nobody to signal");
			SendAck(fromMachine, fromBox, ack);
		}		
		else if(NetConds[condID].lockID!=lockID)
		{
			printf("The condition and the lock do not match!\n");
			formPacket(ack, SC_SignalCV, 0, 0, "acqd lock doesn't match cond lock");
			SendAck(fromMachine, fromBox, ack);
		}
		else
		{
			replyType *reply = (replyType *) NetConds[condID].replyQueue->Remove();
			if(reply!=NULL)
			{
				// Append this to the wait queue for acquiring the lock.
				NetLocks[lockID].replyQueue->Append((void *)reply);
				NetConds[condID].queueCount--;
			}
			if((NetConds[condID].queueCount == 0))//||(NetConds[condID].queueCount == 1))
			{
				//There don't seem to be any waiters. So clear the condition parameters
				NetConds[condID].lockID = -1;
			}
			formPacket(ack_signal, SC_SignalCV,1,0,"Signaling done");
			SendAck(fromMachine, fromBox, ack_signal);
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------
//	The function does the same error checking as the SignalCV_Netcall. Then it appends all the members to the wait queue
//	of the lock so that when somebody calls release the respective clients get their chances to acquire the lock.
// -------------------------------------------------------------------------------------------------------------------------

void BroadcastCV_Netcall(int condID, int lockID, int fromMachine, int fromBox)
{
	printf("Broadcast netcall from Machine %d. lockID = %d, condID = %d.\n",fromMachine, lockID, condID);
	char ack[MaxMailSize];
	if((!NetLocks[lockID].exists)||(!NetConds[condID].exists))
	{
		printf("Either the lockID or the condID or both don't exist [Broadcast Netcall]\n");
		formPacket(ack, SC_BroadcastCV, 0, 0, "lockID/condID non-existent");
	}
	else if(NetLocks[lockID].ownerMachine!=makeOwnerID(fromMachine,fromBox))
	{
		printf("Broadcasting using a lock that a machine has not acquired!\n");
		formPacket(ack, SC_BroadcastCV, 0, 0, "lock not acquired");
	}
	else
	{
		if(NetConds[condID].lockID==-1)
		{
			printf("Nobody is waiting!!\n");
			formPacket(ack, SC_BroadcastCV, 1, 0, "Nobody to broadcast");
			SendAck(fromMachine, fromBox, ack);
		}		
		else if(NetConds[condID].lockID!=lockID)
		{
			printf("The condition and the lock do not match!\n");
			formPacket(ack, SC_BroadcastCV, 0, 0, "acqd lock doesn't match cond lock");
			SendAck(fromMachine, fromBox, ack);
		}
		else
		{
			replyType *reply;
			reply = (replyType *) NetConds[condID].replyQueue->Remove();
			while(reply !=NULL)
			{					
				// Append this to the wait queue for acquiring the lock.
				NetLocks[lockID].replyQueue->Append((void *)reply);
				NetConds[condID].queueCount--;
				reply = (replyType *) NetConds[condID].replyQueue->Remove();
			}

			//There are no more waiters. 
			NetConds[condID].lockID = -1;
			formPacket(ack, SC_BroadcastCV,1,0,"Broadcast done");
			SendAck(fromMachine, fromBox, ack);
		}
		
	}
	
}

void CheckCondWaitQueue_Netcall(int condID, int fromMachine, int fromBox)
{
	int rv;
	printf("CheckCondWaitQueue netcall from Machine %d. condID = %d.\n",fromMachine, condID);
	char ack[MaxMailSize];
	if((!NetConds[condID].exists))
	{
		printf("Either the lockID or the condID or both don't exist [Broadcast Netcall]\n");
		formPacket(ack, SC_CheckCondWaitQueue, -1, 0, "lockID/condID non-existent");
	}
	else
	{
		if(NetConds[condID].queueCount==0)
			rv = 0;
		else
			rv = 1;

		formPacket(ack, SC_CheckCondWaitQueue,rv,0,"CheckCond");
	}
	SendAck(fromMachine, fromBox, ack);
}


// -------------------------------------------------------------------------------------------------------------------------
//  This function checks whether there is somebody waiting on condition. If not then set the exists bit to FALSE;
// -------------------------------------------------------------------------------------------------------------------------

void DeleteCondition_Netcall(int condID,int fromMachine,int fromBox)
{
	printf("Delete lock netcall from Machine %d. lockID = %d.\n",fromMachine, condID);
	char ack[MaxMailSize];

	if(!NetConds[condID].exists)
	{
		printf("The lock doesn't exist! So can't be deleted (first create the lock)\n");
		formPacket(ack, SC_DeleteCondition,0, 0,"Condition non-existent");
	}
	else if(NetConds[condID].lockID!=-1)
	{
		printf("Somebody is waiting on the condition. Can't be deleted\n");
		formPacket(ack, SC_DeleteCondition,0, 0,"Condition in use");
	}
	else
	{
		NetConds[condID].exists = FALSE;
		formPacket(ack, SC_DeleteCondition,1, 0,"Success");
	}
	SendAck(fromMachine, fromBox, ack);
}

/*
// -------------------------------------------------------------------------------------------------------------------------
	This function is used to create an integer array that is shared between different nachos clients.It takes in the name of the shared
	integer and the length of the shared integer array. It then creates the shared integer and associates a new integerID to
	it if the shared integer already doesn't exist. If it already exists then it associates the existing integerID to it and replies
	back to the stub by creating a packet with this integerID.
// -------------------------------------------------------------------------------------------------------------------------
*/
void CreateSharedInt_Netcall(char *name, int length, int fromMachine, int fromBox)
{
	printf("CreateSharedInt Netcall. Name = %s, origniating request from Client%d\n",name, fromMachine);
	int i = 0;
	bool found = FALSE;
	int integerID;
	char ack[MaxMailSize];
	for(i=0;i<MAX;i++)
	{
		if(!strcmp(sharedInts[i].name, name))
		{
			found = TRUE;
			break;
		}
	}
	if(found)
	{
		integerID = i;
		printf("The shared integer already exists. Passing on that value of integerID to the client. integerID = %d\n",integerID);
	}
	else
	{
		integerID = currentIntegerID++;
		strcpy(sharedInts[integerID].name, name);
		sharedInts[integerID].length = length;
		sharedInts[integerID].value = new int[length];
		// Explicitly initialize to zero
		for(int j=0;j<length;j++)
		{
			if(strcmp(name, "phoneStatus"))
				sharedInts[integerID].value[j] = 0;
			else
				sharedInts[integerID].value[j] = 1;
		}
		printf("Shared Integer creation successful. integerID = %d created\n",integerID);
	}
	sharedInts[integerID].exists = TRUE;
	formPacket(ack, SC_CreateSharedInt,integerID, 0,"Acknowledgement");
	SendAck(fromMachine,fromBox, ack);
}

/*
// -------------------------------------------------------------------------------------------------------------------------
	This function is used to access the shared integer arrays that were created using the CreateSharedInt_Netcall netcall. If a client
	needs to check the current value of a particular shared integer then it can invoke this netcall on that integer. The position
	indicates the location of the shared variable (denoted by integerID) whose value has to be obtained.
// -------------------------------------------------------------------------------------------------------------------------
*/
void GetSharedInt_Netcall(int integerID, int position, int fromMachine, int fromBox)
{
	printf("GetSharedInt Netcall. integerID = %d, position = %d, origniating request from Client%d and value = %d\n",integerID, position, fromMachine,sharedInts[integerID].value[position]);
	char ack[MaxMailSize];
	/* Do some preliminary error checking */
	if((integerID > MAX)||(integerID<0))
	{
		printf("Error in integerID\n");
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else if(!sharedInts[integerID].exists)
	{
		printf("sharedInt doesn't exist for the given integerID = %d\n",integerID);
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else if((position<0)||(position>sharedInts[integerID].length-1))
	{
		printf("Error in position = %d\n",position);
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else
	{
		formPacket(ack, SC_GetSharedInt, sharedInts[integerID].value[position], 0, "Acknowledgement");
	}
	SendAck(fromMachine, fromBox, ack);
}

/*
// -------------------------------------------------------------------------------------------------------------------------
	This function is used to set/modify the value of a shared a variable that was created using the CreateSharedInt_Netcall netcall. 
	The position of the array in the shared variable that has to be modified is given by position.
// -------------------------------------------------------------------------------------------------------------------------
*/

void SetSharedInt_Netcall(int integerID, int position, int value, int fromMachine, int fromBox)
{
	char ack[MaxMailSize];
	printf("SetSharedInt Netcall. integerID = %d, position = %d, value = %d, origniating request from Client%d\n",integerID, position, value, fromMachine);
	if((integerID > MAX)||(integerID<0))
	{
		printf("Error in integerID\n");
		formPacket(ack, SC_SetSharedInt, -1, 0, "Acknowledgement");
	}
	else if(!sharedInts[integerID].exists)
	{
		printf("sharedInt doesn't exist for the given integerID = %d\n",integerID);
		formPacket(ack, SC_SetSharedInt, -1, 0, "Acknowledgement");
	}
	else if((position<0)||(position>sharedInts[integerID].length-1))
	{
		printf("Error in position = %d\n",position);
		formPacket(ack, SC_SetSharedInt, -1, 0, "Acknowledgement");
	}
	else
	{
		sharedInts[integerID].value[position] = value;
		formPacket(ack, SC_SetSharedInt, 1, 0, "Acknowledgement");	
	}
	SendAck(fromMachine, fromBox, ack);
}
// -------------------------------------------------------------------------------------------------------------------------
// 			This function searches for a 1 in the array and sends the first location that it found.
//			If no results are found then the length of the array will be sent.
// -------------------------------------------------------------------------------------------------------------------------

void GetOneIndex_Netcall(int integerID, int fromMachine, int fromBox)
{
	printf("GetOneIndex Netcall. integerID = %d originating request from Client%d\n",integerID, fromMachine);
	char ack[MaxMailSize];
	/* Do some preliminary error checking */
	if((integerID > MAX)||(integerID<0))
	{
		printf("Error in integerID\n");
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else if(!sharedInts[integerID].exists)
	{
		printf("sharedInt doesn't exist for the given integerID = %d\n",integerID);
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else
	{
		int position;
		for(position = 0;position<sharedInts[integerID].length;position++)
		{
			if(sharedInts[integerID].value[position]==1)
				break;
		}
		formPacket(ack, SC_GetOneIndex, position, 0, "Acknowledgement");  // search would have failed if the result is equal to the length
	}
	SendAck(fromMachine, fromBox, ack);
}

// -------------------------------------------------------------------------------------------------------------------------
//		This function gets a index of the first occurence of 0 in the array specified by integerID.
//		If no results are found then the legnth of the array will be sent.
// -------------------------------------------------------------------------------------------------------------------------

void GetZeroIndex_Netcall(int integerID, int fromMachine, int fromBox)
{
	printf("GetZeroIndex Netcall. integerID = %d originating request from Client%d\n",integerID, fromMachine);
	char ack[MaxMailSize];
	/* Do some preliminary error checking */
	if((integerID > MAX)||(integerID<0))
	{
		printf("Error in integerID\n");
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else if(!sharedInts[integerID].exists)
	{
		printf("sharedInt doesn't exist for the given integerID = %d\n",integerID);
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else
	{
		int position;
		for(position = 0;position<sharedInts[integerID].length;position++)
		{
			if(sharedInts[integerID].value[position]==0)
				break;
		}
		formPacket(ack, SC_GetZeroIndex, position, 0, "Acknowledgement");  // search would have failed if the result is equal to the length
	}
	SendAck(fromMachine, fromBox, ack);
}

/*
// -------------------------------------------------------------------------------------------------------------------------
			This is a generic netcall used to do a search in the array specified by the integerID.
			The shared variable is denoted by integerID. 
			The function then scans through the array to find the first occurence of equalityValue.
			Additionally we can also specify a position in the array which has to be skipped. 
			If no results are found then the length of the array will be sent.
			So it is possible to use this function as a replacement to GetOneIndex and GetZeroIndex system calls where the
			equalityValue will be 1 or 0 respectively and the skipIndex can be a value higher than the length of the array.
// -------------------------------------------------------------------------------------------------------------------------
*/
void ArraySearch_Netcall(int integerID, int skipIndex, int equalityValue, int fromMachine, int fromBox)
{
	printf("SearchArray Netcall. integerID = %d originating request from Client%d\n",integerID, fromMachine);
	char ack[MaxMailSize];
	/* Do some preliminary error checking */
	if((integerID > MAX)||(integerID<0))
	{
		printf("Error in integerID\n");
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else if(!sharedInts[integerID].exists)
	{
		printf("sharedInt doesn't exist for the given integerID = %d\n",integerID);
		formPacket(ack, SC_GetSharedInt, -1, 0, "Acknowledgement");
	}
	else
	{
		int position;
		for(position = 0;position<sharedInts[integerID].length;position++)
		{
			if(position!=skipIndex)													// skip this index alone
			{
				if(sharedInts[integerID].value[position]==equalityValue)			//our required checking
					break;
			}
		}
		formPacket(ack, SC_ArraySearch, position, 0, "Acknowledgement");  // search would have failed if the result is equal to the length
	}
	SendAck(fromMachine, fromBox, ack);
}

// Just a test netcall 
// useful in debugging and stuff.

void TestMe_Netcall(int someParameter, int fromMachine, int fromBox)
{
	char ack[MaxMailSize];
	printf("TestMe netcall. some parameter = %d \n", someParameter);
	formPacket(ack, SC_TestMe, someParameter - 1, 0, "Acknowledgement");
	SendAck(fromMachine, fromBox, ack);
}


// -------------------------------------------------------------------------------------------------------------------------
//  Server Heart Beat function.
// -------------------------------------------------------------------------------------------------------------------------
timeCounterClass presentTime, pastTime, diffTime,nSeconds;
char hbt[MaxMailSize];
PacketHeader outPktHdr2, inPktHdr2;
MailHeader outMailHdr2, inMailHdr2;
int j;
bool success2;
void SendHeartBeat(int n)
{
	nSeconds.t1 = 0;
	nSeconds.t2 = n*1000000;		// this is thus our version of n seconds
	Delay(3);
	while(1)
	{   
		presentTime = getTimeStamp();
		diffTime.t1 = presentTime.t1 - pastTime.t1;
		diffTime.t2 = presentTime.t2 - pastTime.t2;			// how much time has elapsed?
		if(isTimeGreater(diffTime,nSeconds)==1)				// if the time elapsed is more than n seconds then..
		{			
			formPacket(hbt, SERVERHBT, 0, myMachineID,1,0,0,0,&presentTime,"HBT");
			aliveLock->Acquire();							// start sending the heart beat !
			for(int machines=0;machines<NServers;machines++)
			{
				if(alive[machines])
				{
					printf("SENDING HEARTBEAT to %d\n", machines);
					outPktHdr2.to = machines;
					outMailHdr2.to = 0;
					outMailHdr2.from = 1;
					outMailHdr2.length = MaxMailSize;
					success2 = postOffice->Send(outPktHdr2, outMailHdr2, hbt);

					if ( !success2 ) {
						printf("%d is dead. Noting it down\n",machines);
											
							alive[machines] = 0;
						
					}else
						printf("%d is alive\n", machines);
				}
			}
			aliveLock->Release();
			pastTime = presentTime;							// start from scratch for sensing time.
		}
		currentThread->Yield();								// yield this thread to give way to other threads too. Very important
	}
}

// return k+1 modulo number of servers
int inc(int k)
{
	return ((k+1)%NServers);
}
int sendPing(int toServer)
{
	PacketHeader outPktHdr4;
	MailHeader outMailHdr4;
	outPktHdr4.to = toServer;
	outMailHdr4.to = 0;
	outMailHdr4.length = MaxMailSize;
	bool success4;
	char pingMessage[MaxMailSize];
	pingMessage[0]=SERVERPNG;
	strcpy(pingMessage+19,"PINGING");
	success4 = postOffice->Send(outPktHdr4, outMailHdr4, pingMessage);
	if(success4)
		return 1;
	else
		return 0;
}

//-------------------------------------------------------------------------------------------------------------------------
//	This is the server function which takes nothing and returns nothing. It continuously loops using a while true loop.
//	The server listens to incoming messages at MailBox 0. Once it receives a packet, it then calls parsePacket function
//	which spilts up the message into components that can be handled by the server. It identifies the machine that has
//	requested the netcall.
//  It identifies whether the incoming message is from a client, or is a forwarded message or a server acknowledgement or
//  whether its a heardbeat and does the processing accordingly.
// -------------------------------------------------------------------------------------------------------------------------

void StartServer()
{
	int i;
	int mType;
	int mid, mbox;
	timeCounterClass time, t_1;
    waitingQueueMember *wMember, *wMember1, *pMember;
	List *oldMessagesAW = new List();
	
	for(i=0;i<MAXSERVERS;i++)
		alive[i] = 1;

	Thread *heartThread = new Thread("Heart Beat Thread");
	heartThread->Fork(SendHeartBeat,HBT_PERIOD);
	timeCounterClass nTime, dTime;
	while(1)
	{	
		printf("Going to receive\n");
		postOffice->Receive(0,&inPktHdr, &inMailHdr, buffer);
	//	Some request received
		if(buffer[0]==SERVERPNG)
			continue;
	
		int f = inPktHdr.from;
		parsePacket(buffer, &mType, &syscallType, &mid, &mbox, &parameter1, &parameter2, &parameter3, &time, msg);	
		printf("REQUEST from %d. mtype = %d | syscall = %d | mid = %d | mbox = %d |p1 = %d | p2 = %d |p3 = %d |t1 = %d |t2 = %d |msg = %s\n\n",f,mType, syscallType, mid, mbox, parameter1, parameter2, parameter3, time.t1, time.t2, msg);
		bool A = (syscallType==SC_AcquireLock);
		bool W = (syscallType==SC_WaitCV);
		if(mType == CLIENTREQ)
		{
			// We just obtained a client message, we will have to conditionally forward it to other servers. 
			// Forward the message to all the other servers.
			int localmessageindex = getNumber(buffer);
			printf("Local message index = %d\n",localmessageindex);
			
			// Check whether its already in the output queue.
			t_1.t1 = localmessageindex;
			t_1.t2 = f;
			wMember1 = (waitingQueueMember *)repliedMessageQueue->SearchEqual(t_1);

			if(wMember1!=NULL)
			{
				outPktHdr.to = mid;
				outMailHdr.to = mbox;
				outMailHdr.length = MaxMailSize;
				postOffice->Send(outPktHdr, outMailHdr, wMember1->packet);
				printf("Just sent a message from already processed list\n");
			}

			else
			{
			
			if(!(A||W))
			{
					buffer[0] = SERVERREQ;
					time = getTimeStamp();
					formPacket(buffer, SERVERREQ, syscallType, mid, mbox, parameter1, parameter2, parameter3, &time, msg);
					appendNumber(buffer, localmessageindex);
					outMailHdr.to = 0;
					outMailHdr.from = 1;
					outMailHdr.length = MaxMailSize;
					aliveLock->Acquire();
					for (int machines = 0; machines < NServers ; machines++ )
					{
						if(alive[machines])
						{
							outPktHdr.to = machines;			
							success = postOffice->Send(outPktHdr, outMailHdr, buffer);
							if ( !success ) {					
								alive[machines] = 0;					
							}
						}
					}
					aliveLock->Release();
			}
			
			else
			{
				// so we have either a acquire netcall or a wait netcall
				// check whether this is a duplicate message. The server might not have replied the client for a long time
				// if the netcall is of type acquire or wait. So the clientMonitor might think that the message has been lost and so 
				// resend the message. So basically we have to ignore the duplicate messages.
				// This could have been checked above itself. But we postpone it to here and have executed the same snippet of code again
				// because of the fact that the function isPresent might be a bit heavy on the server. the list keeps growing to a maximum
				// size of 500 and so cycling through each member will be time consuming.
				
				bool P = (oldMessagesAW->isPresent(localmessageindex, makeOwnerID(mid, mbox)));
				if(!P)
				{ 
					buffer[0] = SERVERREQ;
					time = getTimeStamp();
					formPacket(buffer, SERVERREQ, syscallType, mid, mbox, parameter1, parameter2, parameter3, &time, msg);
					appendNumber(buffer, localmessageindex);
					outMailHdr.to = 0;
					outMailHdr.from = 1;
					outMailHdr.length = MaxMailSize;
					// Now Forward to alive servers. 
					aliveLock->Acquire();
					for (int machines = 0; machines < NServers ; machines++ )
					{
						if(alive[machines])
						{
							outPktHdr.to = machines;			
							success = postOffice->Send(outPktHdr, outMailHdr, buffer);
							if ( !success ) {					
								alive[machines] = 0;	
								printf("[Forwarding]Not Sent to %d because its dead!!\n",machines);
							}
							else
							{
								printf("[Forwarding]Sent to %d\n", machines);
							}

						}
						else
						{
							printf("[Forwarding] Not Sent to %d because we already know its dead\n", machines);
						}
					}
					aliveLock->Release();
				}
				else
					printf("Message in list already!! So not forwarding it\n");
				
			}
			}
				
		}
		else
		{
			// The message is a server forwarded message.
			if(mType == SERVERREQ)
			{
				bool sendAckValid_temp;
				alive[f] = 1;
				if(myMachineID == f)
				{
					sendAckValid_temp = TRUE;
				}
				else
				{
					sendAckValid_temp = FALSE;
				}

				// Update the timestamp table!
				updateTimeStampTable(f, time);
				
			
				//Send only to other machines, not to myself! 
				if(f!=myMachineID)
				{
					timeCounterClass time2 = getTimeStamp();
					char data_local[MaxMailSize];
					MailHeader outMailHdr3;
					PacketHeader outPktHdr3;
					formPacket(data_local, SERVERACK, 0,myMachineID, 0,0,0,0,&time2,"ACK");
					outPktHdr3.to = f;
					outMailHdr3.to = 0;
					outMailHdr3.from = 1;
					outMailHdr3.length = MaxMailSize;
					success = postOffice->Send(outPktHdr3, outMailHdr3, data_local);

					if ( !success ) {
						aliveLock->Acquire();
						alive[f] = 0;
						int k;
						for ( k = inc(f);  ; k = inc(k) )
						{
							if(alive[k]==1)
								break;
						}
						aliveLock->Release();
						if(myMachineID == k)  // this also needs to be changed as to whether f-1 is dead or not. 
						{
							sendAckValid_temp = TRUE;
						}
						else
						{
							sendAckValid_temp = FALSE;
						}
					}
					// Assuming that I have sent the ACK to myself, I update the timestamp table with the timestamp i generated for myself.
					updateTimeStampTable(myMachineID, time2);
				}
				
				
				wMember = new waitingQueueMember(buffer, time, f, sendAckValid_temp);
				//Append it to the queue
				waitingQueue->Append((void *)wMember);
				if(A||W)
				{
					oldMessagesAW->Append((void *)wMember);
				}

			}
			else if(mType==SERVERACK)
			{
				updateTimeStampTable(f, time);
			}
			else if(mType == SERVERHBT)
			{
				printf("RECEIVED HEARTBEAT FROM %d\n",f);
				updateTimeStampTable(f, time);
			}
			else
			{
				printf("ERROR! Unknown type of call!!\n");
				interrupt->Halt();
			}
		
			int isAlive;
			int minWho = findMinTime();
			printf("Min Timestamp is ");displayTime(timeStampTable[minWho]);printf("\n");
	
		    pMember = (waitingQueueMember *)waitingQueue->Search(timeStampTable[minWho]);

			while(pMember!=NULL)
			{
				aliveLock->Acquire();
				int d = pMember->whichServer;
			
				//isAlive = alive[d]; 
				if(d!=myMachineID)
					isAlive = sendPing(d);
				else
					isAlive = 1;

				if(isAlive==0)
					alive[d]=0;
				// Lets now find out whether I have to send the acknowledgement to the client or not.
				// So if I am the guy who forwarded the client message (to myself) then I can send
				// OR
				// If I am the next living member of the dead guy who had forwarded this message then send ACK.
				if(isAlive)
				{
					sendAckValid = pMember->sendAckValid;
				}
				else
				{
						int k;

						for ( k = inc(d);  ; k = inc(k) )
						{
							if(alive[k])
								break;
						}

						if(myMachineID == k)  // this also needs to be changed as to whether f-1 is dead or not. 
						{
							sendAckValid = TRUE;
						}
						else
						{
							sendAckValid = FALSE;
						}
				}
				aliveLock->Release();
					parsePacket(pMember->packet, &mType, &syscallType, &mid, &mbox, &parameter1, &parameter2, &parameter3, &time, msg);
					printf("Going to Execute :  mtype = %d | syscall = %d | mid = %d | mbox = %d |p1 = %d | p2 = %d |p3 = %d |t1 = %d |t2 = %d |msg = %s\n\n",mType, syscallType, mid, mbox, parameter1, parameter2, parameter3, time.t1, time.t2, msg);			
					int from = mid;
					int fromBox = mbox;
					messageIndex = getNumber(pMember->packet);
					switch(syscallType)
					{
					case SC_CreateLock:
						CreateLock_Netcall(msg, from, fromBox);
						break;

					case SC_AcquireLock:
						AcquireLock_Netcall(parameter1, from, fromBox);
						break;

					case SC_ReleaseLock:
						ReleaseLock_Netcall(parameter1, from, fromBox);
						break;

					case SC_DeleteLock:
						DeleteLock_Netcall(parameter1, from, fromBox);
						break;

					case SC_CreateCondition:
						CreateCondition_Netcall(msg, from, fromBox);
						break;

					case SC_WaitCV:
						WaitCV_Netcall(parameter1, parameter2,from, fromBox);
						break;

					case SC_SignalCV:
						SignalCV_Netcall(parameter1, parameter2, from, fromBox);
						break;
				
					case SC_BroadcastCV:
						BroadcastCV_Netcall(parameter1, parameter2, from, fromBox);
						break;

					case SC_CheckCondWaitQueue:
						CheckCondWaitQueue_Netcall(parameter1, from, fromBox);
						break;
						
					case SC_DeleteCondition:
						DeleteCondition_Netcall(parameter1, from, fromBox);
						break;
					
					case SC_CreateSharedInt:
						CreateSharedInt_Netcall(msg, parameter1, from, fromBox);
						break;

					case SC_GetSharedInt:
						GetSharedInt_Netcall(parameter1, parameter2, from, fromBox);
						break;

					case SC_SetSharedInt:
						SetSharedInt_Netcall(parameter1, parameter2, parameter3, from, fromBox);
						break;

					case SC_Halt:
						interrupt->Halt();
						break;

					case SC_GetOneIndex:
						GetOneIndex_Netcall(parameter1, from, fromBox);
						break;

					case SC_GetZeroIndex:
						GetZeroIndex_Netcall(parameter1, from, fromBox);
						break;

					case SC_ArraySearch:
						ArraySearch_Netcall(parameter1, parameter2, parameter3, from, fromBox);
						break;
					
					case SC_TestMe:
						TestMe_Netcall(parameter1, from, fromBox);
						break;

					default:
						printf("Unknown Netcall\n");
					}
				
				pMember = (waitingQueueMember *)waitingQueue->Search(timeStampTable[minWho]);			
			}	
		}
	}
} 
