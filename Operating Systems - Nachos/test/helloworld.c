#include "syscall.h"

int main()
{
	int i,j;
	char lockName[10];
	int money;
	TestMe(33,0);
	TestMe(55,1);
	TestMe(66,2);
	TestMe(44,0);
	TestMe(77,0);

	TestMe(88,1);
	TestMe(99,1);
	TestMe(11,1);
	TestMe(22,1);
	TestMe(33,1);
	TestMe(40,1);

	i = CreateLock("TestLock",8);
	WriteNum(i);Write("\n",1,1);
    
	for(i=0;i<29;i++)
	  {
	    Concatenate("TestLock",sizeof("testlock"),i,lockName);
	    j = CreateLock(lockName, sizeof(lockName));
		WriteNum(j);Write("\n",1,1);
	  }	
/*
		AcquireLock(i-1);
		ReleaseLock(i-1);

	money = CreateSharedInt("money",5,10);
	SetSharedInt(money,5,1);
	SetSharedInt(money,3,1);
	SetSharedInt(money,8,10);
	i = ArraySearch(money,3,1);
	WriteNum(i);Write("\n",1,1);
	i = ArraySearch(money,99,10);
	WriteNum(i);Write("\n",1,1);
	*/
}

