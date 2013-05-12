#include "syscall.h"

int getnum(char *buf,int len)
{
	int i,j, value;
	value = 0;
	for(i = 0;i<len;i++)
	{
		j = (int)buf[i]-48;
		if((j>=0)&&(j<=9))
		{
			value = value*10+j;
		}
		else
			break;
	}	
	return value;
	/*Exit(0);*/
}

int main()
{
	int i,k,num;
	char buf[10];
	Write("Test Suite 3\n",100,1);
	Write("============\n",100,1);
	Write("\nInput the number of Senate Offices : \n",100,1);
	Read(buf,10,0);
	num=getnum(buf,10);	
	Write("Execing Senate program as many as ",100,1);WriteNum(num);Write(" times\n\n",10,1);
	for(i=0;i<num;i++) 
	{		
		Exec("../test/senate",14);
	}
	Exit(0);
}
