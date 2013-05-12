#include "syscall.h"
void TestFunc()
{
	Write("Hello in test func inside testsuite2_3\n",100,1);
	Exit(0);
}
int main()
{
	Write("In TestSuite2_3\n",20,1);
	Fork(TestFunc);
	Exit(0);
}

