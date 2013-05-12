#include "syscall.h"
#define MAX 10

int i;
int locks[MAX];
int conds[MAX];
char lockName[20], condName[20];
int visitorCountLock;
int visitorCount = 0;

void Visitor()
{
	int id; 

	AcquireLock(visitorCountLock);
	id = visitorCount++;
	ReleaseLock(visitorCountLock);
	
	Write("Visitor ", sizeof("Visitor "), 1);
	WriteNum(id);

	
/*	if(id==1)
	{
		lock = lock1;
		cond = cond1;
	}
	else if(id==2)
	{
		cond = cond2;
		lock = lock2;
	}*/
    /*	WriteNum(id);*/
	
	AcquireLock(locks[id]);
	WaitCV(conds[id],locks[id]);
	ReleaseLock(locks[id]);
	

	Write("Woken : ",8,1);WriteNum(id);
	Write("\n",1,1);
	Exit(0);
}

int main()
{


	visitorCountLock = CreateLock("vclock",6);

	for(i=0;i<MAX;i++)
	{
		Concatenate("LockID",sizeof("lockID"),i,lockName);
		locks[i] = CreateLock(lockName,sizeof(lockName));
		Concatenate("CondID",sizeof("condID"),i,condName);
		conds[i] = CreateCondition(condName,sizeof(condName));
	} 
	for(i=0;i<MAX;i++)
	{
		Fork(Visitor);
	}


	for(i=0;i<MAX;i++)
	{
		AcquireLock(locks[i]);
		SignalCV(conds[i],locks[i]);
		ReleaseLock(locks[i]);
	}

	Exit(0);
}
