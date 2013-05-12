#include "math.h"
#include "list.h"
#include "utils.h"

#define round(X) (((X)>= 0)?(int)((X)+0.5):(int)((X)-0.5))

void InitRandom(long l_seed) //initializes the random number generator with the seed
{
	if (l_seed == 0L) 
	{
		time_t localtime=(time_t)0;
		time(&localtime);
		srand48((long)localtime);
	} 
	else 
	{
		srand48(l_seed);
	}
}

double ExponentialInterval(double dval,double rate) //funtion to calculate the interarrival/service time when exponential distribution is specified
{
	double temp = (-(1/rate)*log(dval));
	return (temp*1000);
}

int GetInterval(int exponential, double rate, int tflag, int from, unsigned long run, char *tsfile) //returns the interarrival/service time
{
	FILE *fp;
	int thisnum,filearr,fileser;
	unsigned long loop;
	if (tflag == 0)
	{
		if (exponential) 
		{
			double dval=(double)drand48();
			if (ExponentialInterval(dval, rate)<2) //round the value to be returned to 1, if it is very small
				return 1;
			else if (ExponentialInterval(dval, rate)>10000) //round the value to be returned to 10000, if its too huge
				return 10000;
			else return round(ExponentialInterval(dval, rate));
		}
		else
		{
			double millisecond=((double)1000)/rate; //return the interarrival/service time for deterministic process
			return round(millisecond);
		}
	}
	else if (tflag == 1) //return the interarrival/service time if a trace specification file is provided
	{
		if ((fp = fopen(tsfile,"r")) == NULL)
		{
			cout<<"\n Couldn't open the file "<<tsfile<<" for reading \n\n";
			exit(1);
		}
		fscanf(fp,"%d",&thisnum);
		for (loop=0;loop<run+1;loop++)
		{			
			fscanf(fp,"%d%d",&filearr,&fileser);			
		}
		fclose(fp);
		if (from ==0)
		{					
			if (filearr<1)
				filearr = 1;
			else if (filearr>10000)
				return 10000;
			else 
				return filearr;
		}
		else if (from == 1)
		{			
			if (fileser<1)
				fileser = 1;
			else if (fileser>10000)
				return 10000;
			else 
				return fileser;
		}
	}
	return 0;
}

timeval subtracttime(timeval tm1,timeval tm2) // function that subtracts and provides the time difference between two timeval variables
{	
	timeval k;
	if (tm2.tv_usec>=tm1.tv_usec)
	{		
		k.tv_sec = tm2.tv_sec - tm1.tv_sec;
		k.tv_usec = tm2.tv_usec - tm1.tv_usec;
	}
	else
	{
		tm2.tv_sec--;
		tm2.tv_usec+=1000000;		
		k.tv_sec = tm2.tv_sec - tm1.tv_sec;
		k.tv_usec = tm2.tv_usec - tm1.tv_usec;
	}	
	return k;
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

float getfloat(char pqr[100],int len) //input a string and return the floating point number in the string
{
	int z,divisor;
	char *div;
	char *m;
	char *e;
	int len1,len2,i,j;
	float value1,value2,value;  
	len1=0;
	len2=0;
	i=0;
	j=0;
	divisor=1;
	value1 = 0;
	value2 = 0;
	value = 0;
	z=0;
	for (i=0;i<len;i++)
	{
		if (pqr[i]!='.')
			len1++;
		else 
			break;
	}
	for (i=len1+1;i<len;i++)
		len2++;	
	m=(char *)malloc(len1);
	e=(char *)malloc(len2);
	for (i=0;i<len1;i++)
		m[i]=pqr[i];
	m[len1]='\0';
	for (i=len1+1;i<len;i++)
	{				
		e[z]=pqr[i];
		z++;		
		e[len2]='\0';
	}	
	for(i = 0;i<len1;i++)
	{
		j = (int)m[i]-48;
		if((j>=0)&&(j<=9))
		{
			value1 = (value1 * 10)+j;
		}
		else
		break;
    }	
	for(i = 0;i<len2;i++)
	{
		j = (int)e[i]-48;
		if((j>=0)&&(j<=9))
		{
			value2 = (value2 * 10)+j;
		}
		else
		break;
    }	
	div = (char *)malloc(len2+1);
	div[0]='1';
	for (i=0;i<len2;i++)
		div[i+1]='0';
	divisor =  getnum(div,strlen(div));
	value2/=divisor;
	value=value1+value2;	
	return value;
}

int chkarg(char pqr[100],int len) //checks if the command line argument is correct
{
	int retflag,i,len1,len2,z;
	char *m;
	char *e;
	i=0;
	len1=0;
	len2=0;
	retflag = 0;
	z=0;
	if (isnum(pqr,len)==1)
		return 1;
	else
	{
		for (i=0;i<len;i++)
		{
			if (pqr[i]!='.')
				continue;
			else
			{
				retflag = 1;
				break;
			}
		}
		if (retflag == 0)
			return 0;
		for (i=0;i<len;i++)
		{
			if (pqr[i]!='.')
				len1++;
			else 
				break;
		}
		for (i=len1+1;i<len;i++)
			len2++;	
		m=(char *)malloc(len1);
		e=(char *)malloc(len2);
		for (i=0;i<len1;i++)
			m[i]=pqr[i];
		m[len1]='\0';
		for (i=len1+1;i<len;i++)
		{				
			e[z]=pqr[i];
			z++;		
			e[len2]='\0';
		}
		if (isnum(m,len1)==0)
			return 0;
		else if (isnum(e,len2)==0)
			return 0;
		else
			return 1;
	}
}


void usage() //function to print the correct usage of command line arguments
{
	cout<<"\n Incorrect use of command line arguments ";
	cout<<"\n Usage: mm2 [-lambda lambda] [-mu mu] [-s] [-seed seedval] [-size sz] [-n num] [-d {exp|det}] [-t tsfile] "<<endl;
	exit(0);
}
