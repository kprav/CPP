#include "syscall.h"
#define MAX 10
int i;
int locks[MAX];
int conds[MAX];
char lockName[20], condName[20];
int visitorCountLock;
int visitorCount = 0;

void TestFunc()
{
	int id; 

	AcquireLock(visitorCountLock);
	id = visitorCount++;
	ReleaseLock(visitorCountLock);
	
	Write("\nThread #",20 , 1);
	WriteNum(id);Write(" Forked \n",20,1);
	Write("\nYielding..\n",100,1);
	Yield();
	Exit(0);
}

int main()
{
	Write("\n\n Test Suite 2\n\n (bracketed terms denote the testsuites)\n\n",100,1);
	Write("\n[2] Execing testsuite2_1\n",100,1);
	i = Exec("../test/testsuite2_1",20);
	Write("\n[2] Execing testsuite2_2\n",100,1);
	i = Exec("../test/testsuite2_2",20); 
	visitorCountLock = CreateLock("vclock",6);
	for(i=0;i<MAX;i++)
	{
		Fork(TestFunc);
	}
	Exit(0);
}
