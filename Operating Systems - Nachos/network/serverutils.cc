#include "serverutils.h"
#include <string.h>

extern "C" { int bcopy(char *, char *, int); };

// ---------------------------------------------------------
// This function splits the integer into two digits and stores 
// their character equivalent inside str[0] and str[1]
// ---------------------------------------------------------
long getTime() {
      struct timeval tv;
      struct timezone tz;
      gettimeofday(&tv, &tz);
      return (tv.tv_sec%100)*1000000+tv.tv_usec;
}

// -----------------------------------------------------------------------------------------------------------------------------------
//	This function appends the number at the last three characters of the data. data should be of length MaxMailSize.
//	For example, if n is 235, data[MaxMailSize-3] will have 2, data[MaxMailSize-2] will have 3 and data[MaxMailSize-1] will have 5
//	all as characters (not integers)
// -----------------------------------------------------------------------------------------------------------------------------------

void appendNumber(char *data, int counter)
{
	data[MaxMailSize-1] = counter%10;
	data[MaxMailSize-2] = (counter/10)%10;
	data[MaxMailSize-3] = counter/100;
}

// -----------------------------------------------------------------------------------------------------------------------------------
//	This function does the opposite of the above function. It generates an integer out of the number that was appended. For example,
//	the above data array will give an integer output of 235 when called using the getNumber function.
// -----------------------------------------------------------------------------------------------------------------------------------
int getNumber(char *data)
{
	return ((data[MaxMailSize-3])*100+(data[MaxMailSize-2])*10+(data[MaxMailSize-1]));
}

// -----------------------------------------------------------------------------------------------------------------------------------
//  This function finds the difference in time between time1 and time2 and returns it.
// -----------------------------------------------------------------------------------------------------------------------------------
timeCounterClass findDiffTime(timeCounterClass time1, timeCounterClass time2)
{
	timeCounterClass dTime;
	dTime.t1 = time1.t1 - time2.t1;
	dTime.t2 = time1.t2 - time2.t2;
	if (dTime.t2 < 0)
	{
		if(dTime.t1>0)
		{
			dTime.t1--;
			if(dTime.t1 < 0)
				dTime.t1 = 0;
			dTime.t2 = 100*1000000 + dTime.t2;
		}
	}
	return dTime;
}


static timeCounterClass pastTime;
timeCounterClass startTime;
timeCounterClass getTimeStamp()
{ 
	timeCounterClass presentTime(getTime());
	if(pastTime.t2 >= presentTime.t2)
	{
		presentTime.t1 = pastTime.t1 + 1;
	}
	else
	{
		presentTime.t1 = pastTime.t1;
	}
	pastTime = presentTime;	 
	//printf(" present time = (%d,%d), starttime = (%d, %d), pastime = (%d, %d)\n",presentTime.t1, presentTime.t2,startTime.t1, startTime.t2, pastTime.t1, pastTime.t2);
	//presentTime = findDiffTime(presentTime, startTime);
	return presentTime;
}
	
timeCounterClass *timeStamp = new timeCounterClass();
void putData(char *str, int i, int applyAlgo)
{
	if(applyAlgo)
	{
		int n = i;
		int j;
		for(j=0;j<4;j++)
		{
			str[3-j] = n&255;
			n = n>>8;
		}
	}
	else
	{
		str[0] = (i/10)+46;
		str[1] = (i%10)+46;
	}
}

// -----------------------------------------------------------------------------------------------------------------
// This function forms the packet in the character array packet by taking in syscallType, paramete1, parameter2 and
// msg. Thus this function calls putData thrice with syscallType strored at packet, parameter1 stored at packet+2,
// parameter2 stored at packet + 4 and the message copied in the remaining array starting from packet + 6
// -----------------------------------------------------------------------------------------------------------------

void formPacket(char *packet, int mType, int syscallType, int machineID, int mailBoxID, int parameter1, int parameter2, int parameter3, timeCounterClass *time,  char *msg)
{
	//printf("In formPacket. mtype = %d | syscall = %d | mid = %d | mbox = %d |p1 = %d | p2 = %d |p3 = %d |t1 = %d |t2 = %d |msg = %s\n",mType, syscallType, machineID, mailBoxID, parameter1, parameter2, parameter3, time->t1, time->t2, msg);
	int parameter;
	if(parameter1/100 > 0) printf("ERROR : parameter must be two digit only\n");
	if(parameter2/100 > 0) printf("ERROR : parameter must be two digit only\n");
	if(parameter3/100 > 0) printf("ERROR : parameter must be two digit only\n");
	parameter = parameter1*10000 + parameter2*100+parameter3; 
	
	packet[0] = mType;
	putData(packet + 1, syscallType,0);
	putData(packet + 3, machineID, 0);
	putData(packet + 5, mailBoxID, 0);
	putData(packet + 7, parameter, 1);
	putData(packet + 11, time->t1, 1);
	putData(packet + 15, time->t2, 1);

//	bcopy(msg, packet + 19, strlen(msg)+1); //bcopy(from, to, length)
	strcpy(packet + 19, msg);
	
}

void formPacket(char *packet, int syscallType, int parameter1, int parameter2, char *msg)
{
	int parameter, parameter3;
	parameter3 = 0;
	if(parameter1/100 > 0) printf("ERROR : parameter must be two digit only\n");
	if(parameter2/100 > 0) printf("ERROR : parameter must be two digit only\n");
//	if(parameter3/100 > 0) printf("ERROR : parameter must be two digit only\n");
	parameter = parameter1*10000 + parameter2*100+parameter3; 
//	putData(packet, mType,0);
	putData(packet + 1, syscallType,0);
//	putData(packet + 3, machineID, 0);
//	putData(packet + 5, mailBoxID, 0);
	putData(packet + 7, parameter, 1);
//	putData(packet + 11, time->t1, 1);
//	putData(packet + 15, time->t2, 1);
	bcopy(msg, packet + 19, strlen(msg)+1); //bcopy(from, to, length)
}
// -------------------------------------------------------------------------------------------------------------------
// This function takes a character array pointed by data, and puts the numeric equivalent of the first two characters
// inside the pointer pointed by i. Hence the calling function then just has to use i. 
// -------------------------------------------------------------------------------------------------------------------

void getData(char *data, int *i, int applyAlgo)
{
	if(applyAlgo)
	{
		int n2 = 0;
		int j;
		for(j=0;j<3;j++)
		{
			n2 = n2 | data[j]&(0x000000FF);
			n2 = n2 <<8;
		}
		n2 = n2 | data[j]&(0x000000FF);
		*i = n2;
	}
	else
	{
		*i = (data[0]-46)*10 + (data[1]-46);
	}
}
 
// -------------------------------------------------------------------------------------------------------------------
//	This function gets in packet and returns (via pointers) the syscallType, parameter1, parameter2 and a character
//	message. Hence you have to perform getData thrice with packet, packet + 2 and packet + 4. The remaining content
//	of the packet (pointed by packet + 6) is copied into msg using strcpy function.
// -------------------------------------------------------------------------------------------------------------------

void parsePacket(char *packet, int *mType, int *syscallType, int *machineID, int *mailBoxID, int *parameter1, int *parameter2, int *parameter3, timeCounterClass *time, char *msg)
{
	*mType = packet[0];
	int parameter;
	int t1, t2;
	getData(packet + 1, syscallType, 0);
	getData(packet + 3, machineID, 0);
	getData(packet + 5, mailBoxID, 0);
	getData(packet + 7, &parameter, 1);
	getData(packet + 11, &t1, 1);
	getData(packet + 15, &t2, 1);
	strcpy(msg, packet + 19);
	*parameter3 = (parameter)%100;
	parameter = (parameter)/100;
	*parameter2 = (parameter)%100;
	parameter = (parameter)/100;
	*parameter1 = (parameter)%100;
	time->t1 = t1;
	time->t2 = t2;
}

void parsePacket(char *packet, int *syscallType, int *parameter1, int *parameter2, int *parameter3, char *msg)
{

	int parameter;
//	int *t1, *t2;
	getData(packet + 1, syscallType, 0);
//	getData(packet + 3, machineID, 0);
//	getData(packet + 5, mailBoxID, 0);
	getData(packet + 7, &parameter,1);
//	getData(packet + 11, t1);
//	getData(packet + 15, t2);
	strcpy(msg, packet + 19);
	*parameter3 = (parameter)%100;
	parameter = (parameter)/100;
	*parameter2 = (parameter)%100;
	parameter = (parameter)/100;
	*parameter1 = (parameter)%100;
//	time->t1 = *t1;
//	time->t2 = *t2;
}

// -------------------------------------------------------------------------------------------------------------------
// Constructor of the class NetLocksClass 
// -------------------------------------------------------------------------------------------------------------------
NetLocksClass::NetLocksClass()
{
	isBusy = FALSE;				//Initially, the lock is not busy
	exists = FALSE;				// The lock doesn't exist until CreateLock is called upon
	replyQueue = new List();	// Initialize to an empty queue.
}

// -------------------------------------------------------------------------------------------------------------------
// Constructor of the class NetCondsClass
// -------------------------------------------------------------------------------------------------------------------
NetCondsClass::NetCondsClass()
{
	exists = FALSE;				// The condition doesn't exist till CreateCondition is called.
	replyQueue = new List();	// Initialize to empty queue.
	queueCount = 0;				// How many members in there queue
	lockID = -1;				// the lockID associated with the condition
}


sharedIntsClass::sharedIntsClass()
{
	length = 0;
	exists = FALSE;
	name[0] = '\0';
}


int makeOwnerID(int fromMachine, int fromBox)
{
	return (100*fromMachine+fromBox);
}
/*
int isTimeGreater(timeCounterClass tS1, timeCounterClass tS2)
{
//	printf("In Funtion: %d | %d ||| %d | %d\n", tS1.t1, tS1.t2, tS2.t1, tS2.t2);
	if(tS1.t1>tS2.t1)
		return 1;
	else if(tS1.t1<tS2.t1)
		return 0;
	else if(tS1.t2 > tS2.t2) 
		return 1;
	else if(tS1.t2 == tS2.t2)
		return 2; //Tie!!
	else
		return 0;
} */

char *formreply(char *msg, int i1, int i2)
{
	char *replymsg;
	int len = sizeof(msg);
	replymsg = new char[len+2];
	strcpy(replymsg, msg);
	replymsg[len-1] = i1 + 46;
	replymsg[len] = i2 + 46;
	replymsg[len+1]='\0';
	return replymsg;
}
