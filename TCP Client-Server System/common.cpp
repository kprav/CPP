#include "common.h"

void cleanup(void *arg) //cleanup handler to shutdown and close sockets
{	
	int *desc;
	desc = (int *)malloc(sizeof(int));
	*desc = *((int *)arg);	
	shutdown(*desc,SHUT_RDWR);	
	close(*desc);
}

int isnum(char abc[100],int len) //input a string and check if it contains only an integer
{
	int flag = 0;
	int i,j;
	for (i=0;i<len;i++)
	{
		for (j=48;j<=57;j++)
		{
			if (j!=(int)abc[i])
				continue;
			if (j==(int)abc[i])
			{
				flag = 1;
				break;
			}
		}
		if (flag == 0)
			return 0;
		else
			flag = 0;
	}
	return 1;
}

int getnum(char pqr[100],int len) //input a string and return the integer in the string
{
	int i,j;
	int value;  
	value = 0;
	for(i = 0;i<len;i++)
	{
		j = (int)pqr[i]-48;
		if((j>=0)&&(j<=9))
		{
			value = (value * 10)+j;
		}
		else
		break;
    }	
	return value;
}

void *get_in_addr(struct sockaddr *sa) //This function is to calculate the ip address
{
	if (sa->sa_family == AF_INET)
    {
		return &(((struct sockaddr_in*)sa)->sin_addr);
    }
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
