#include "syscall.h"
#define MAX 5

int i;
int locks[MAX];
int conds[MAX];
char lockName[20], condName[20];
int visitorCountLock;
int visitorCount = 0;
int wake = 0;
int mainLock;
int mainCond;
int wakeup[]={0,0,0,0,0};
void TestFunc()
{
	int id; 

	AcquireLock(visitorCountLock);
	id = visitorCount++;
	ReleaseLock(visitorCountLock);
	
	Write("Thread #",20 , 1);
	WriteNum(id);Write(" Forked \n",20,1);
	if(id == MAX-1)
	{
		AcquireLock(mainLock);
		wake = 1;
		SignalCV(mainCond, mainLock);
		ReleaseLock(mainLock);
	}
/*	Write("Thread %d going to Acquire its lock\n",id); */
	AcquireLock(locks[id]);
/*	Write("Thread %d going to wait on its lock\n",id); */
	while(wakeup[id]==0)
		WaitCV(conds[id],locks[id]);
	ReleaseLock(locks[id]);

	Write("Woken : ",8,1);WriteNum(id);
	Write("\n",1,1);

	Exit(0);
}

int main()
{
	Write("\n\nTest Suite 2\n\n (bracketed terms denote the testsuites)\n\n",150,1);
	visitorCountLock = CreateLock("vclock",6);
	mainLock = CreateLock("mainLock",8);
	mainCond = CreateCondition("mainCond",8);
	for(i=0;i<MAX;i++)
	{
		wakeup[i] = 0;
		Concatenate("LockID",sizeof("lockID"),i,lockName);
		locks[i] = CreateLock(lockName,sizeof(lockName));
		Concatenate("CondID",sizeof("condID"),i,condName);
		conds[i] = CreateCondition(condName,sizeof(condName));
	} 
	for(i=0;i<MAX;i++)
	{
		Fork(TestFunc);
	}
	AcquireLock(mainLock);
	while(wake == 0)
		WaitCV(mainCond, mainLock);
	ReleaseLock(mainLock);
		
	for(i=0;i<MAX;i++)
	{
		AcquireLock(locks[i]);
		wakeup[i]=1;
		SignalCV(conds[i],locks[i]);
		ReleaseLock(locks[i]);
	}
	
	Write("[1]Trying to acquire locks with id -1 and 33\n",50,1);
	AcquireLock(-1);
	AcquireLock(33);
	/*Try to wait on some condition that doesn't exist*/
	WaitCV(-1,1);
	WaitCV(34,1);
	SignalCV(55,-1);
	Exit(0);
}
