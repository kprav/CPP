#include "syscall.h"

int main()
{
	int lockID, condID,lockID2, condID2;
	int i;
	Write("Client4\nCreating Lock with name Client4 Lock and Condition with name Client4 Cond\n",100,1);
	lockID2 = CreateLock("Client4 Lock",12);
	condID2 = CreateCondition("Client4 Cond",12);
	Write("Creating Lock with name Client1 Lock and Condition with name Client1 Cond\n",100,1);
	Write("Delay 5 seconds before acquiring the lock\n",100,1);
	for(i=0;i<5;i++)
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
	Write("Waiting on Client1 Cond with Client1 Lock\n",100,1);
	WaitCV(condID,lockID);
	Write("Woken Up! Going to execute again\n",100,1);
	for(i=0;i<3;i++)
	{
	  WriteNum(i);Write("\n",1,1);Delay(1);
	}
	Write("Releasing Lock\n",50,1);
	ReleaseLock(lockID);
}

