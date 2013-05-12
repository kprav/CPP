#include "syscall.h"

void testfunc()
{
	Write("\n [4] In Test Suite 4 Thread \n");
	Exit(0);
}

int main()
{
	int num = 2;
	int i;
	Write("\n\n Test Suite 4\n\n (bracketed terms denote the testsuites)\n\nPlease give enough time for completion\n\n",200,1);
	Write("\n[4] Execing sort twice\n",100,1);
	for (i=0;i<num;i++)
	{
		Exec("../test/sort",12);
	}
	Exit(0);
}
