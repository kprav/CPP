#include "syscall.h"
int testLock;
int i;
int testCount = 0;

void TestFunc()
{
	int id; 
	AcquireLock(testLock);
	id = testCount++;
	ReleaseLock(testLock);	
	Write("\n[2_1]Thread #",20 , 1);
	WriteNum(id);Write(" Forked \n",20,1); 
	Exit(0);
}

int main()
{

	Write("\n[2_1]In Testsuite2_1\n",100,1);
	testLock = CreateLock("tlock",5);
	Write("\n[2_1]testLock = ",15,1);WriteNum(testLock);Write("\n",1,1);
	for(i=0;i<4;i++)
	{
		Fork(TestFunc);
	}
	Write("\n[2_1]testLock = ",15,1);WriteNum(testLock);Write("\n",1,1);
/*	DeleteLock(testLock); */
	Exit(0);
}
