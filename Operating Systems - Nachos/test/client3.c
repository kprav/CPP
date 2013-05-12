#include "syscall.h"

int main()
{
	int lockID, condID,lockID2, condID2;
	int i;
	Write("Client3\nCreating Lock with name Client3 Lock and Condition with name Client3 Cond\n",100,1);
	lockID2 = CreateLock("Client3 Lock",12);
	condID2 = CreateCondition("Client3 Cond",12);
	Write("Creating Lock with name Client1 Lock and Condition with name Client1 Cond\n",100,1);
	lockID = CreateLock("Client1 Lock",12);
	condID = CreateCondition("Client1 Cond",12);
	Write("Delay 15 seconds before acquiring the lock\n",100,1);
	for(i=0;i<15;i++)
	{
		Delay(1);
		WriteNum(i);Write("  ",5,1);
	}
	Write("\nAcquiring lock\n",30,1);
	AcquireLock(lockID);
	for(i=0;i<5;i++)
	{
	  WriteNum(i);Write("\n",1,1);Delay(1);
	}
	Write("Signal once to wake up those who are waiting on Client1 Cond with Client1 Lock",200,1);
	SignalCV(condID,lockID);
	Write("Going to release the lock\n",50,1);
	ReleaseLock(lockID);
}