#include "syscall.h"
#include "senate.h"

void WriteMe(char *buf)
{
	Write(buf,200,1);
}

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

int myexp ( int count )
{
	int i, val=1;
	for (i=0; i<count; i++ ) 
	{
		val = val * 10;
	}
	return val;
	Exit(0);
}

void itoa( char arr[], int size, int val ) 
{
	int i, max, dig, subval, loc;
	for (i=0; i<size; i++ )
	{
		arr[i] = '\0';
	}
	for ( i=1; i<=2; i++ ) 
	{
		if (( val / myexp(i) ) == 0 ) 
		{
			max = i-1;
			break;
		}
	}
	subval = 0;
	loc = 0;
	for ( i=max; i>=0; i-- )
	{
		dig = 48 + ((val-subval) / myexp(i));
		subval = (dig-48) * myexp(max);
		arr[loc] = dig;
		loc++;
	}
	return;
	Exit(0);
}

int NumVisitor = 4;

void Visitor() /* code block to perform visitor operation */
{
  int condnToWait,who,i,j, num;
  int talkingTime;
  int checkCorrectOperatorV;
  int phoneToUse,gotPhone;
  int thisActivate;
  int operatorToUse;
  int callPresident;

/* Some common parameters */


  char lockName[30];
  char condName1[30];
  char condName2[30];
  int presidentStatus;
  int phoneStatus;
  char printing[50];
  int lockID1, lockID2, lockID3, lockID4, lockID5, lockID6, condID1, condID2, condID3, condID4;
  int indOpLock[20], waitForOperVerCV[20], waitForCallerCV[20];
  int activate, authMechanism, freeOperators, operatorStatus;
  int repositoryMoney;
 

  presidentStatus = CreateSharedInt("presidentStatus",15,1);
  phoneStatus = CreateSharedInt("phoneStatus",11,NOOFPHONES);
  freeOperators = CreateSharedInt("freeOperators",13,1);
  operatorStatus = CreateSharedInt("operatorStatus",14,Nop);
  activate = CreateSharedInt("activate",sizeof("activate"),Nop);
  authMechanism = CreateSharedInt("authMechanism",sizeof("authMechanism"),Nop);
  repositoryMoney = CreateSharedInt("repositoryMoney",sizeof("repositoryMoney"),Nop);
 
  

  lockID1 = CreateLock("phoneLock",10);                  /* obtain a master lock for all phones */
  lockID2 = CreateLock("GlobalOpLock",12);     /* obtain a master lock for all the operators */
  lockID3 = CreateLock("visitorCountLock",17);     /* obtain a lock to keep track of the number of visitors permitted to make a call */
  lockID4 = CreateLock("NumSenators",12);
  lockID5 = CreateLock("NumVisitors",12);
  lockID6 = CreateLock("NumOperators",13);
  /*  displayLock = CreateLock("DispLock",7); */
  /* Lock **individualOperatorLock; */                            /* obtain an individual lock for every operator */
  condID1 = CreateCondition("presiNeedsPhone",16); /* condition variable for the condition that president needs phone */
  condID2 = CreateCondition("senatorNeedsPhone",18);     /* condition variable for the condition that senator needs phone */
  condID3 = CreateCondition("visitorNeedsPhone",18);     /* condition variable for the condition that visitor  needs phone */
  condID4 = CreateCondition("processCustomer",16);         /* condition variable to allow president/senator/visitor to make a call */
  for (i=0;i<Nop;i++)
    {
      Concatenate("OperatorLock",sizeof("OperatorLock"),i,lockName);
      Concatenate("waitForOpVer",sizeof("waitForOpVer"),i,condName1);
      Concatenate("waitForCaller",sizeof("waitForCaller"),i,condName2); 
      indOpLock[i] = CreateLock(lockName,sizeof(lockName));
      waitForOperVerCV[i] = CreateCondition(condName1,sizeof(condName1));
      waitForCallerCV[i] = CreateCondition(condName2,sizeof(condName2));
    } 
  
  
  
  /* End of common parameters */

  AcquireLock(lockID5);
  who = NumVisitor;
  NumVisitor++;
  ReleaseLock(lockID5);
  AcquireLock(lockID1);
  /* loop to check if the president or senator is waiting. If any one is waiting, then visitor has to wait before he/she can make a call. Otherwise visitor can go ahead */
  do
    {
      condnToWait = TRUE;      
      if(GetSharedInt(presidentStatus,0) == 1)
		condnToWait = TRUE;
      /* Check if some senator is already waiting! */
      else if(CheckCondWaitQueue(condID2)==1)
		{
		  /* Bad luck, there seems to be a senator.  */
		  condnToWait = TRUE;
		}
      else
		{
		  /*
	  for(i=0;i<NOOFPHONES;i++)
	    {
	      if(GetSharedInt(phoneStatus,i)==FREE)
		   {
			  phoneToUse = i;
			  SetSharedInt(phoneStatus,i,BUSY);
			  condnToWait = FALSE;
			  break;
		   }
	    }	*/
		phoneToUse = GetOneIndex(phoneStatus);
		if(phoneToUse!=NOOFPHONES)
			condnToWait = FALSE;
	}
      if(condnToWait)
	WaitCV(condID3,lockID1); /* visitor waits if there is a president or a senator already waitng to make a call. */
    }while(condnToWait);
  ReleaseLock(lockID1);
  /* Visitor has got a phone */
  /* Need to get an operator now */
  AcquireLock(lockID2);
  while(GetSharedInt(freeOperators,0)==0)
    WaitCV(condID4,lockID2);
  /* visitor has to wait if there are no free operators available */
  /* Some operator is available. Though I don't know who it is. Let us find out.   */ 
/*  for(j=0;j<Nop;j++)
    {
      if(GetSharedInt(operatorStatus,j)==FREE)
	{
	  operatorToUse = j;
	  break;
	}
    } */
	operatorToUse = GetOneIndex(operatorStatus);
  /* operator obtained */
  checkCorrectOperatorV = operatorToUse; /* check if the operator to whom the visitor pays money is the same as the one the permits/denies the visitor to make a call */
  AcquireLock(indOpLock[operatorToUse]);
  SetSharedInt(activate, operatorToUse, 2);
  SetSharedInt(operatorStatus, operatorToUse, BUSY);
  SetSharedInt(freeOperators, 0, GetSharedInt(freeOperators, 0) - 1);
  ReleaseLock(lockID2);
  SetSharedInt(authMechanism, operatorToUse, 3);  /* 1 for President | 2 for Senators | 3 for Visitors */
  SetSharedInt(repositoryMoney, operatorToUse, ((RandomFunction(100)-1)>80)?0:1); /* randomly generate whether the visitor pays $1 or not */
  /* If operator is sleeping, wake up */

  SignalCV(waitForCallerCV[operatorToUse],indOpLock[operatorToUse]);
  SignalCV(waitForCallerCV[operatorToUse],indOpLock[operatorToUse]);
  while(GetSharedInt(activate,operatorToUse)==2)
	  WaitCV(waitForOperVerCV[operatorToUse],indOpLock[operatorToUse]);
  thisActivate=0;
  thisActivate=GetSharedInt(activate, operatorToUse);  
  ReleaseLock(indOpLock[operatorToUse]);
  if (thisActivate==0) 
    {
	  /* visitor is denied access to phone beacause he/she didn't pay $1. */
      j=0;
      talkingTime=0;
      /* printf("Visitor%d \t  UNAVAILABLE \t %d/%d units \t %d \t   NOTAPPLICABLE    DENIED \t   Money paid is $0 - verified by operator 
	  %d \n",who+1,j,talkingTime,operatorToUse+1,checkCorrectOperatorV+1); */
	  /*	   AcquireLock(displayLock); */
	  Write("Visitor ",8,1);
	   num = who+1;
		itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
		Write(" \t UNAVAILABLE \t",100,1);	   
		num = j;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write("/",1,1);
	    num=talkingTime;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" units \t ",10,1);
	    num=operatorToUse+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \t",6,1);	
	    Write(" NOTAPPLICABLE    DENIED \t   Money paid is $0 - verified by operator ",100,1);
	    num=checkCorrectOperatorV+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \n",3,1);
		/*ReleaseLock(displayLock);*/
	Yield();

      /* printf("Access to Phone for visitor %d Denied by Operator %d!\n",who+1,operatorToUse+1); */
    }
  else if (thisActivate==1) /* visitor has paid $1. Operator verifies and visitor is allowed to make a call */
    {
      /* Now Talk */
      talkingTime = RandomFunction(5); /* randomly generate the amount of time the visitor will talk on the phone */
      /* loop for the visitor to talk on the phone for the randomly generated time period */
      for (i=1;i<=talkingTime;i++){
	/* printf("Visitor%d \t  %d \t\t %d/%d units \t %d \t   NOTAPPLICABLE    ACCEPTED \t   
	Money paid is $1 - verified by operator %d \n",who+1,phoneToUse+1,i,talkingTime,operatorToUse+1,checkCorrectOperatorV+1); */
	/*AcquireLock(displayLock);*/
	Write("Visitor ",8,1);
	   num = who+1;
		itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
		Write(" \t",2,1);
	    num = phoneToUse+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write("\t\t ",5,1);
		num = i;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write("/",1,1);
	    num=talkingTime;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" units \t ",10,1);
	    num=operatorToUse+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \t",6,1);	
	    Write(" NOTAPPLICABLE    ACCEPTED \t   Money paid is $1 - verified by operator ",100,1);
	    num=checkCorrectOperatorV+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \n",3,1);
		/*ReleaseLock(displayLock);*/
	Yield();
      }
      /* visitor is done talking */
      /* Set the phone status to be free */
    }
  AcquireLock(lockID1);
  SetSharedInt(phoneStatus,phoneToUse,FREE);
  if(GetSharedInt(presidentStatus,0)==0) /* president is not waking to talk */
    {
      if(CheckCondWaitQueue(condID2))
		SignalCV(condID2,lockID1); /* wake up the next senator waiting to talk */
      else
		SignalCV(condID3,lockID1); /* if no senator is waiting, then wake up the next visitor waiting to talk */
    }
  else /* president is waiting to talk, so senators and visitors will have to wait */
    {
      callPresident = TRUE;
      	  /*
      for(i=0;i<NOOFPHONES;i++)
		if((i!=phoneToUse)&&(GetSharedInt(phoneStatus,i)==BUSY)) // check if even a single phone is busy other than the phone just used by the visitor which he/she sets to free 
	  {
	    callPresident = FALSE;
	    break;
	  }*/
		i = ArraySearch(phoneStatus, phoneToUse, BUSY);
		if(i!=NOOFPHONES)
			callPresident = FALSE;
      if(callPresident==TRUE)
	SignalCV(condID1,lockID1); /* if all phones are free, then no one is talking currently and so, signal the president */
    }
  /* visitor goes away and does not return. Remember visitors can make a maximum of just one call       */
  ReleaseLock(lockID1);	
  WriteMe("Visitor ");WriteNum(who + 1);WriteMe("Leaving\n");
  Exit(0);
}


int main()
{
	int i;
	Write("Visitor Client Program\nForking ",100,1); WriteNum(5); Write(" Visitors\n",100,1);
	for(i = 0;i<5;i++)
		Fork(Visitor);	
	Exit(0);
}
