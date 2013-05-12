#include "syscall.h"

int main()
{
	int lockID, condID, lockID2, condID2;
	int i;
	Write("Client5\nCreating Lock with name Client5 Lock and Condition with name Client5 Cond\n",100,1);
	lockID = CreateLock("Client1 Lock",12);
	condID = CreateCondition("Client1 Cond",12);
	lockID2 = CreateLock("Client5 Lock",12);
	condID2 = CreateCondition("Client5 Cond",12);
	for(i=0;i<6;i++)
	{
	  WriteNum(i);Write("\n",1,1);Delay(1);
	}
	Write("Deleting Client5 Lock and Client5 Cond\n",100,1);
	DeleteLock(lockID2);
	DeleteCondition(condID2);
	Write("Going to try to delete Client1 Lock\n",200,1);
	DeleteLock(0);
	Write("Going to wait for 30 seconds\n",100,1);
	for(i=0;i<30;i++)
	{
	  WriteNum(i);Write("\n",1,1);Delay(1);
	}
	AcquireLock(lockID);
	Write("Going to broadcast all those who are waiting on Client1 Cond with Client1 Lock\n",100,1);
	BroadcastCV(condID,lockID);
	Write("Going to release the lock\n",50,1);
	ReleaseLock(lockID);

	Write("Trying to delete non-existent lock 33 and non-existent condition 12\n",200,1);
	DeleteLock(33);
	DeleteCondition(12);
	Write("Wait for 20 seconds before deleting Client1 Lock and Client1 Condition\n",100,1);
	Delay(20);
	DeleteLock(lockID);
}

