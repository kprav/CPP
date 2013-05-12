#include "syscall.h"

int main()
{
	int lockID,condID;
	int i;
	Write("Client2\nCreating Lock with name Client1 Lock and Condition with name Client1 Cond\n",100,1);
	lockID = CreateLock("Client1 Lock",12);
	condID = CreateCondition("Client1 Cond",12);
	Write("Delay 3 seconds before acquiring the lock\n",100,1);
	for(i=0;i<3;i++)
	{
		Delay(1);
		WriteNum(i);Write("  ",5,1);
	}
	Write("\nAcquiring lock\n",20,1);
	AcquireLock(lockID);
	for(i=0;i<10;i++)
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
	Write("Releasing Lock\n",100,1);
	ReleaseLock(lockID);
}
