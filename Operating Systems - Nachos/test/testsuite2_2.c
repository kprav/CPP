#include "syscall.h"

void test()
{
	Write("\n[2_2] Hello!!! Inside test thread forked in testsuite2_2 \n",100,1);
	Exit(0);
}

int main()
{
	int i;
	int num = 2;
	Write("\n[2_2] In Testsuite2_2\nExecing 2 matmults!!\n",100,1);		
	for(i=0;i<num;i++) 
	{		
		Exec("../test/matmult",15);
		Write("\n[2_2]Hello!!! in main function \n",100,1);
	}

	for (i=0;i<5;i++)
	{
		Fork(test);
	}
	Exit(0);
}
