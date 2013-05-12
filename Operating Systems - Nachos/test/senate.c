
#include "syscall.h"


#define FREE 1
#define BUSY 0
#define TRUE 1
#define FALSE 0
#define NOOFPHONES 20

int a[3];
int b, c;

char msg[20];
int lockID1,lockID2,lockID3,lockID4,lockID5,lockID6;
int condID1,condID2,condID3,condID4;
int indOpLock[50],waitForOperVerCV[50],waitForCallerCV[50];
int Nv,Ns,Nop; /* Number of visitors, senators and Operators */
int choice,typeOfTest; /*allow the user to pick his/her choice of test case - system test or repeatable test */
int phoneStatus[20];    /* track status of every phone */
int presidentStatus; /* check if president is waiting to talk or if he is currently talking */
int freeOperators;   /* track the number of free operators */
int operatorStatus[5]; /* track the status of every operator */
int displayLock;

/* -----Operator Authentication related values----- */
int repositoryID[50];
int repositoryMoney[50];
int authenticationMechanism[50];
int moneyReserve[50];
int activate[50];
int NumSenator = 0;
int NumOperator = 0;
int NumVisitor = 0;
int num;
char printing[50];

int visitorAcceptCount = 0;  /* Keep count of number of visitors allowed to make a call */

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

void President() /* code block to perform president operation */
{
  int presidentNumberOfCalls=0; /* keep count of total number of calls made by the president */
  int checkCorrectOperatorP;
  int waitingTime;
  int condnToWait;
  int phoneToUse, gotPhone,i,j;
  int operatorToUse;
  int talkingTime;
  while(presidentNumberOfCalls<5)
    {
      condnToWait = FALSE;
      phoneToUse = 0;
      AcquireLock(lockID1);
      presidentStatus = 1;
      /* loop for the president to keep waiting even if a single phone is busy */
      do
	  {
		  for(i=0;i<NOOFPHONES;i++)
		    {
		     if(phoneStatus[i]==BUSY)
				{
			  condnToWait = TRUE;
			  break;
				}
		    }
		if(i==NOOFPHONES)
		   condnToWait=FALSE;
		  if(condnToWait==TRUE)
		  WaitCV(condID1,lockID1);
	}while(condnToWait==TRUE);
      /* all phones are free now */
      phoneStatus[phoneToUse] = BUSY;
      /* president has obtained a phone now */
      ReleaseLock(lockID1);      
      /* Need to get an operator */
      AcquireLock(lockID2);
      /* loop to wait till an operator is available */
      while(freeOperators==0)
	    WaitCV(condID4,lockID2); 
	  /* president has to wait if there are no free operators available */
      /* Some operator is available. Though I don't know who it is yet, let us find out    */    
      for(j=0;j<Nop;j++)
	  {
	  if(operatorStatus[j]==FREE)
	    {
	      operatorToUse = j;
	      break;
	    }
	 }
      /* operator obtained */
      /* check if the operator to whom president goes for authentication is same as the one who permits him/her to make a call. */
      checkCorrectOperatorP = operatorToUse;
      AcquireLock(indOpLock[operatorToUse]);
      activate[operatorToUse]=0;
      operatorStatus[operatorToUse]=BUSY;
      freeOperators--;
	  ReleaseLock(lockID2);
      authenticationMechanism[operatorToUse] = 1; /* 1 for President | 2 for Senators | 3 for Visitors */
	 
      /* If operator is sleeping, wake up */

      SignalCV(waitForCallerCV[operatorToUse],indOpLock[operatorToUse]);
      WaitCV(waitForOperVerCV[operatorToUse],indOpLock[operatorToUse]); 
      ReleaseLock(indOpLock[operatorToUse]);  
      if(activate[operatorToUse]==0) /* President is denied access to phone.But this will never happen as there is only one president and we assume that his/her ID is never faked */
	  {
	  /*  printf("President is denied access to Phone failing authentication!\n"); */
		Write("President is denied access to Phone failing authentication!\n",sizeof("President is denied access to Phone failing authentication!\n"),1);
	  }
      else if(activate[operatorToUse]==1) /* operator succesfully authenticates the identity of the president */
	 {
	  /* Now Talk */
	  talkingTime = RandomFunction(20); /* randomly generate the amount of time the president is talking */
          /* loop for the president to talk on the phone for the randomly generated time period */
	  for (j=1;j<=talkingTime;j++){
	    /*printf("President \t  %d \t\t %d/%d units \t %d \t\t %d \t    ACCEPTED       NOTAPPLICABLE - verified by operator %d \n",phoneToUse+1,j,talkingTime,operatorToUse+1,presidentNumberOfCalls+1,checkCorrectOperatorP+1);*/
		AcquireLock(displayLock);
		Write("President \t ",13,1);
	    num = phoneToUse+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write("\t\t ",5,1);	    
	    itoa(printing,10,j);
	    Write(printing,sizeof(printing),1);
	    Write("/",1,1);
	    num=talkingTime;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" units \t ",10,1);
	    num=operatorToUse+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \t\t ",6,1);
	    num=presidentNumberOfCalls+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \t   ACCEPTED      NOTAPPLICABLE - verified by operator ",56,1);
	    num=checkCorrectOperatorP+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \n",3,1);
		ReleaseLock(displayLock);
	    Yield();
	  }
	  /* president is done talking */	  
	  /* Set the status to be free */
	  AcquireLock(lockID1);
	  phoneStatus[phoneToUse]=FREE;
	  presidentStatus = 0;
	  BroadcastCV(condID2,lockID1); /* wake up all the senators waiting to talk */
	  BroadcastCV(condID3,lockID1); /* wake up all the visitors waiting to talk */
	  ReleaseLock(lockID1);
	}
      /* president goes away  */
      /* president waits for a random amount of time before coming back to make the next call. Remember maximum number of calls allowed is 5 */
      waitingTime = RandomFunction(7);
      for(j=0;j<waitingTime;j++)
	{
	  Yield();
	}
      presidentNumberOfCalls++; /* increment the number of calls made by the president */
    }
  Exit(0);
}


void Senator() /* code block to perform senator operation */
{
  /* senator ID is randomly generated during forking and we assume that a senator with ID greater than 1000 has an authentiate ID */
  int senatorNumberOfCalls=0; /* keep count of maximum number of calls made by a senator */
  int operatorToUse;
  int checkCorrectOperatorS;
  int ID,i,j;
  int talkingTime;
  int condnToWait;
  int phoneToUse, gotPhone;
  int thisActivate;
  int callPresident;
  int randomWaitingTime;
  AcquireLock(lockID4);
  ID = NumSenator + 100*((RandomFunction(2)-1)?10:1);  
  NumSenator++;
  ReleaseLock(lockID4);
  while(senatorNumberOfCalls<10)
    {
      AcquireLock(lockID1);
      condnToWait = TRUE;      
      /* loop to check if president is waiting. If yes, then senator has to wait. Otherwise senator can obtain a phone */
      do
	{      
	  if(presidentStatus==1)
	    condnToWait = TRUE;
	  else
	    {
	      for(i=0;i<NOOFPHONES;i++)
		{
		  if(phoneStatus[i]==FREE)
		    {
		      phoneStatus[i]=BUSY;
		      gotPhone = TRUE;
		      phoneToUse = i;
		      condnToWait = FALSE;
		      break;
		    }
		}
	    }
	  if(condnToWait==TRUE)
	    WaitCV(condID2,lockID1); /* senator waits if the president is already waiting to make a call */
	}while(condnToWait==TRUE);
      ReleaseLock(lockID1);
      /* Senator has got a Phone */
      /* Need to get an operator now */
      AcquireLock(lockID2);
      while(freeOperators==0)
	WaitCV(condID4,lockID2); /* senator has to wait if there are no free operators avaialble */
      /* Some operator available. Let us find out who it is */
      for(j=0;j<Nop;j++)
	{
	  if(operatorStatus[j]==FREE)
	    {
	      operatorToUse = j;
	      break;
	    }
	}
      /* operator obtained*/
	  checkCorrectOperatorS = operatorToUse; /* check if the operator to whom the senator ID is submitted is the same as the one that permits/denies the senator to talk */	  
      AcquireLock(indOpLock[operatorToUse]);	  
      operatorStatus[operatorToUse]=BUSY;
      freeOperators--;
      ReleaseLock(lockID2);
      authenticationMechanism[operatorToUse] = 2; /* 1 for President | 2 for Senators | 3 for Visitors */
      repositoryID[operatorToUse] = ID;
      /* If operator is sleeping, wake up */
      SignalCV(waitForCallerCV[operatorToUse],indOpLock[operatorToUse]);
      WaitCV(waitForOperVerCV[operatorToUse],indOpLock[operatorToUse]);      
      thisActivate = activate[operatorToUse];
      ReleaseLock(indOpLock[operatorToUse]);
      if(thisActivate==0) /* senator is denied access to phone beacuse of fake ID */
	{
	  j=0;
	  talkingTime=0;
	 /* printf("Senator%d \t  UNAVAILABLE \t %d/%d units \t %d \t\t %d \t    DENIED \t  
	 ID is less than 1000 - verified by operator %d \n",ID+1,j,talkingTime,operatorToUse+1,senatorNumberOfCalls+1,checkCorrectOperatorS+1); */
	 AcquireLock(displayLock);
	  Write("Senator ",8,1);
	   num = ID+1;
	/*	itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);*/
		WriteNum(num);
		Write(" \t  UNAVAILABLE \t",100,1);	    	    
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
	    Write(" \t\t ",6,1);
	    num=senatorNumberOfCalls+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \t  DENIED \t   ID is less than 1000 - verified by operator ",100,1);
	    num=checkCorrectOperatorS+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \n",3,1);
		ReleaseLock(displayLock);
	}
      else if(thisActivate==1) /* Senator has an authenticate ID. Operator verifies and senator is allowed to make a call */
	{
	  /* Now Talk */
	  talkingTime = RandomFunction(10); /* randomly generate the amount of time the senator will talk on the phone */
	  /* loop for the senator to talk on the phone for the randomly generated time period */
	  for (i=1;i<=talkingTime;i++){
	   /* printf("Senator%d \t  %d \t\t %d/%d units \t %d \t\t %d \t    ACCEPTED \t   ID is greater than 1000 - verified by operator %d 
	   \n",ID+1,phoneToUse+1,i,talkingTime,operatorToUse+1,senatorNumberOfCalls+1,checkCorrectOperatorS+1); */
	   AcquireLock(displayLock);
	   Write("Senator ",8,1);
	   num = ID+1;
		/*itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);*/
		WriteNum(num);
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
	    Write(" \t\t ",6,1);
	    num=senatorNumberOfCalls+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \t   ACCEPTED \t   ID is greater than 1000 - verified by operator ",100,1);
	    num=checkCorrectOperatorS+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \n",3,1);
		ReleaseLock(displayLock);
	    Yield();
	  }
	  
	}
      senatorNumberOfCalls++; /* increment the number of calls made by the senator */
      /* senator is done talking */
      /* Set the phone status to be free */
      AcquireLock(lockID1);
      phoneStatus[phoneToUse]=FREE;
      if(presidentStatus==0) /* president is not waiting to talk */
	{
	  if(CheckCondWaitQueue(condID2)) 
	    SignalCV(condID2,lockID1); /* wake up the next senator waiting to talk */
	  else
	    SignalCV(condID3,lockID1); /* if no senator is waiting then wake up the next visitor waiting to talk */
	}
      else /* president is waiting to talk, so senators and visitors will have to wait */
	{
	  callPresident = TRUE;
	  for(i=0;i<NOOFPHONES;i++)
	    if((i!=phoneToUse)&&(phoneStatus[i]==BUSY)) /* check if even a single phone is busy other than the phone just used by the senator which he/she sets to free */
	      {
		callPresident = FALSE;
		break;
	      }
	  if(callPresident==TRUE) 
	    SignalCV(condID1,lockID1); /* if all phones are free, then no one is talking currently and so, signal the president */
	}
      ReleaseLock(lockID1);          
      /* senator goes away */
      /* senator waits for a random amount of time before coming back to make the next caa. Remember maximum number of calls allowed per senator is 10. */
      randomWaitingTime = RandomFunction(3);
      for(j=0;j<randomWaitingTime;j++)
	Yield();
    }
  Exit(0);
}

void Visitor() /* code block to perform visitor operation */
{
  int condnToWait,who,i,j;
  int talkingTime;
  int checkCorrectOperatorV;
  int phoneToUse,gotPhone;
  int thisActivate;
  int operatorToUse;
  int callPresident;
  AcquireLock(lockID5);
  who = NumVisitor;
  NumVisitor++;
  ReleaseLock(lockID5);
  AcquireLock(lockID1);
  /* loop to check if the president or senator is waiting. If any one is waiting, then visitor has to wait before he/she can make a call. Otherwise visitor can go ahead */
  do
    {
      condnToWait = TRUE;      
      if(presidentStatus == 1)
	condnToWait = TRUE;
      /* Check if some senator is already waiting! */
      else if(CheckCondWaitQueue(condID2)==1)
	{
	  /* Bad luck, there seems to be a senator.  */
	  condnToWait = TRUE;
	}
      else
	{
	  for(i=0;i<NOOFPHONES;i++)
	    {
	      if(phoneStatus[i]==FREE)
		{
		  phoneToUse = i;
		  phoneStatus[phoneToUse]=BUSY;
		  condnToWait = FALSE;
		  break;
		}
	    }	  
	}
      if(condnToWait)
	WaitCV(condID3,lockID1); /* visitor waits if there is a president or a senator already waitng to make a call. */
    }while(condnToWait);
  ReleaseLock(lockID1);
  /* Visitor has got a phone */
  /* Need to get an operator now */
  AcquireLock(lockID2);
  while(freeOperators==0)
    WaitCV(condID4,lockID2);
  /* visitor has to wait if there are no free operators available */
  /* Some operator is available. Though I don't know who it is. Let us find out.   */ 
  for(j=0;j<Nop;j++)
    {
      if(operatorStatus[j]==FREE)
	{
	  operatorToUse = j;
	  break;
	}
    }
  /* operator obtained */
  checkCorrectOperatorV = operatorToUse; /* check if the operator to whom the visitor pays money is the same as the one the permits/denies the visitor to make a call */
  AcquireLock(indOpLock[operatorToUse]);
  activate[operatorToUse]=0;
  operatorStatus[operatorToUse]=BUSY;
  freeOperators--;
  ReleaseLock(lockID2);
  authenticationMechanism[operatorToUse] = 3; /* 1 for President | 2 for Senators | 3 for Visitors */
  repositoryMoney[operatorToUse] = ((RandomFunction(100)-1)>80)?0:1; /* randomly generate whether the visitor pays $1 or not */
  /* If operator is sleeping, wake up */
  SignalCV(waitForCallerCV[operatorToUse],indOpLock[operatorToUse]);
  WaitCV(waitForOperVerCV[operatorToUse],indOpLock[operatorToUse]);
  thisActivate=0;
  thisActivate=activate[operatorToUse];  
  ReleaseLock(indOpLock[operatorToUse]);
  if (thisActivate==0) 
    {
	  /* visitor is denied access to phone beacause he/she didn't pay $1. */
      j=0;
      talkingTime=0;
      /* printf("Visitor%d \t  UNAVAILABLE \t %d/%d units \t %d \t   NOTAPPLICABLE    DENIED \t   Money paid is $0 - verified by operator 
	  %d \n",who+1,j,talkingTime,operatorToUse+1,checkCorrectOperatorV+1); */
	   AcquireLock(displayLock);
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
		ReleaseLock(displayLock);
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
	AcquireLock(displayLock);
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
		ReleaseLock(displayLock);
	Yield();
      }
      /* visitor is done talking */
      /* Set the phone status to be free */
    }
  AcquireLock(lockID1);
  phoneStatus[phoneToUse]=FREE;
  if(presidentStatus==0) /* president is not waking to talk */
    {
      if(CheckCondWaitQueue(condID2))
	SignalCV(condID2,lockID1); /* wake up the next senator waiting to talk */
      else
	SignalCV(condID3,lockID1); /* if no senator is waiting, then wake up the next visitor waiting to talk */
    }
  else /* president is waiting to talk, so senators and visitors will have to wait */
    {
      callPresident = TRUE;
      for(i=0;i<NOOFPHONES;i++)
	if((i!=phoneToUse)&&(phoneStatus[i]==BUSY)) /* check if even a single phone is busy other than the phone just used by the visitor which he/she sets to free */
	  {
	    callPresident = FALSE;
	    break;
	  }
      if(callPresident==TRUE)
	SignalCV(condID1,lockID1); /* if all phones are free, then no one is talking currently and so, signal the president */
    }
  /* visitor goes away and does not return. Remember visitors can make a maximum of just one call       */
  ReleaseLock(lockID1);	
  Exit(0);
}


void Operator()
{
  /* loop for the operator thread to run continuously */
  int who;
  AcquireLock(lockID6);
  who = NumOperator;
  NumOperator++;
  ReleaseLock(lockID6);
  while(1)
    {    
      AcquireLock(lockID2);
      if(CheckCondWaitQueue(condID4)) /* checks if anyone (president/senator/visitor) is waiting for an operator */
	{	 	 
	  SignalCV(condID4,lockID2); /* signal the waiting person */
	}
      /* I (operator) am free. So make my status as free so that some customer might be able to use me. */
      operatorStatus[who]=FREE;
      freeOperators++;
      /* Acquire lock specific to me */
      AcquireLock(indOpLock[who]);
      ReleaseLock(lockID2);
      /* Initialize some values */
      authenticationMechanism[who]=0;
      while(authenticationMechanism[who]==0) /* wait till some one is waiting for the operator - 1->President, 2->Senator, 3->Visitor */
	WaitCV(waitForCallerCV[who],indOpLock[who]);
      switch(authenticationMechanism[who]) /* process the customer based on whether the authenticationMechanism value is 1 or 2 or 3 */
	{
	case 0: 
	  /* printf("Illegal\n"); */
	  break;
	case 1:            /* president is talking to operator */
	  activate[who]=1; /* allow him/her to talk */
	  break;
	case 2:                /* senator is talking to operator    */        
	  if(repositoryID[who]>=1000)
	    {
	      activate[who]=1; /* allow him/her to talk on verification of ID */
	    }
	  else
	    {
	      activate[who]=0; /* deny access to phone for senator is ID verifiaction fails */
	    }
	  break;
	case 3:                      /* visitor is talking to operator */
	  if(repositoryMoney[who]==1)
	    {	      
              activate[who]=1;       /* allow him/her to talk if the visitor pays $1	  */    
	      moneyReserve[who]++;   /* increment the amount of money collected by the current operator */
	      AcquireLock(lockID3);
	      visitorAcceptCount++;  /* increment the number of visitors permitted to make a call	 */      	      
	      ReleaseLock(lockID3);
	    }
	  else if(repositoryMoney[who]==0)
	    {
	      activate[who]=0;      /* deny the visitor to make a call bacause he/she failed to pay $1 */
	    }         
	  break;
	default:
	  /* printf("Unknown Authentication Type\n");	  */
	}
      SignalCV(waitForOperVerCV[who],indOpLock[who]);      
      ReleaseLock(indOpLock[who]);
    }
  Exit(0);
}


void Summary() /* print the number of visitors, money collected by each operator and total money */
{
  int notTheEnd = FALSE;
  int i,totalMoney=0;
  int j,k;
  do
    {
      notTheEnd = FALSE;
      for(k=0;k<(Ns+Nv+1)*100;k++) /* yield the summary thread until all the other threads have finished executing */
	Yield();
      if( !CheckCondWaitQueue(condID1) || !CheckCondWaitQueue(condID2) || !CheckCondWaitQueue(condID3) )
	{
	  AcquireLock(lockID1);
	  for(i=0;i<NOOFPHONES;i++)
	    {
	      if((phoneStatus[i]==BUSY))
		{
		  notTheEnd = TRUE;
		  break;
		}
	    }
	  ReleaseLock(lockID1);
	 /*  WriteMe("\n\nSummary\n");
	  WriteMe("~~~~~~~\n"); */
	  if(!notTheEnd)
	    {	      	      
	      if ((typeOfTest!=9)&&(typeOfTest!=10)&&(typeOfTest!=15)&&(typeOfTest!=16))
		{
		WriteMe("\n\nSummary\n");
		WriteMe("\n----------\n");		 
		   WriteMe("Total number of Visitors : ");WriteNum(Nv);  Write("\n",1,1);
		  WriteMe("Number of Visitors Accepted : ");WriteNum(visitorAcceptCount);WriteMe("\n");
		  WriteMe("Number of Visitors Denied : ");WriteNum(Nv - visitorAcceptCount);WriteMe("\n"); 
		  for(j=0;j<Nop;j++)
		    {
		       WriteMe("Money collected by Operator ");WriteNum(j+1);WriteMe(" is ");WriteNum(moneyReserve[j]); WriteMe("\n");
		      totalMoney += moneyReserve[j];
		    }
		  WriteMe("Total money collected by all operators is ");WriteNum(totalMoney);WriteMe("\n");
		  WriteMe("It can be seen that number of visitors accepted is equal to the total money collected by all the operators.\n\n"); 
		}
              if ((typeOfTest!=13)&&(typeOfTest!=14)&&(typeOfTest!=16)&&(typeOfTest!=17))
		{
		  Write("The president talks continuously with no interruption from any senator thread or visitor thread till the end of the \nmaximum time units per call. This is a clear indication that when the president talks, no other person is talking.\n",900,1); 
		}
	       Write("\nThe number in the operator column of each thread matches the operator number by whom it was verified (under the \nremarks column) which is again a clear indication that every senator or visitor or president talk exactly to one operator before \nmaking a call. In other words, the senator is verified by the operator to whom he/she submits his/her ID and the \nvisitor is verified by the operator to whom he/she paid money.\n\n\n",1000,1); 
	      break;	 
	    }
	}
      else
	Yield();
    }while (typeOfTest!=1);
	/* Delete all locks and condition variables */

	DeleteLock(lockID1);
	DeleteLock(lockID2);
	DeleteLock(lockID3);
	DeleteLock(lockID4);
	DeleteLock(lockID5);
	DeleteLock(lockID6);
	DeleteLock(displayLock);
	DeleteCondition(condID1);
	DeleteCondition(condID2);
	DeleteCondition(condID3);
	DeleteCondition(condID4); 
/*	delete[] operatorStatus, repositoryMoney, repositoryID, activate, printing, msg; */


  Exit(0);
}


int
main()
{
  int lockIDD1, lockIDD2, lockIDD3, pid,i,j,k,z;  
  char *buf;
  int ID;
  char lockName[30];
  char condName1[30];
  char condName2[30];
  lockID1 = CreateLock("phoneLock",10);                  /* obtain a master lock for all phones */
  lockID2 = CreateLock("GlobalOperatorLock",19);     /* obtain a master lock for all the operators */
  lockID3 = CreateLock("visitorCountLock",17);     /* obtain a lock to keep track of the number of visitors permitted to make a call */
  lockID4 = CreateLock("NumSenators",12);
  lockID5 = CreateLock("NumVisitors",12);
  lockID6 = CreateLock("NumOperators",13);
  displayLock = CreateLock("DispLock",7);
  /* Lock **individualOperatorLock; */                            /* obtain an individual lock for every operator */
  condID1 = CreateCondition("presidentNeedsPhone",20); /* condition variable for the condition that president needs phone */
  condID2 = CreateCondition("senatorNeedsPhone",18);     /* condition variable for the condition that senator needs phone */
  condID3 = CreateCondition("visitorNeedsPhone",18);     /* condition variable for the condition that visitor  needs phone */
  condID4 = CreateCondition("processCustomer",16);         /* condition variable to allow president/senator/visitor to make a call */
  /* Condition **waitForOperatorVerification; */  /* condition variable to check if president/senator/visitor get operator authentication to make a call
  /* Condition **waitForCaller;  */               /* condition variable for operator to wait for a caller(president/senator/visitor) */


  
  WriteMe("\n\n\n ~~~~~~~~~~~~~Choose Type of Test~~~~~~~~~~~~~ "); /* Choose system test/reapeatable test?  */
  WriteMe("\n 1. System Tests/Stress Tests ");
  WriteMe("\n 2. Repeatable Tests/Controlled Tests ");
  WriteMe("\n\n Enter your choice : " ); 
  /* scanf("%d",&choice); */
  /* choice = int.Parse(Console.ReadLine()); */
 
  Read(buf,10,0);
  choice=getnum(buf,10);

   WriteMe("\n\n Enter the number of Operators : "); /* input the number of operators */  
  /* scanf("%d",&Nop); */  
  Read(buf,10,0);
  Nop=getnum(buf,10);

   WriteMe("\n Enter the number of Senators  : "); /* inpuit the number of senators */
  Read(buf,10,0);
  Ns=getnum(buf,10);

   WriteMe("\n Enter the number of Visitors  : "); /* input the number of visitors */
  /* scanf("%d",&Nv); */
  Read(buf,10,0);
  Nv=getnum(buf,10);
  presidentStatus = 0;
  /* -------initialize all the declared variables---------- */

 /* waitForOperVerCV = (int *)MemAllocate(Nop);
  waitForCallerCV = (int *)MemAllocate(Nop);*/
  for (i=0;i<Nop;i++)
    {
     Concatenate("OperatorLock",sizeof("OperatorLock"),i,lockName);
     Concatenate("waitForOperatorVerification",sizeof("waitForOperatorVerification"),i,condName1);
     Concatenate("waitForCaller",sizeof("waitForCaller"),i,condName2); 
     indOpLock[i] = CreateLock(lockName,sizeof(lockName));
     waitForOperVerCV[i] = CreateCondition(condName1,sizeof(condName1));
     waitForCallerCV[i] = CreateCondition(condName2,sizeof(condName2));
    }      
 /* activate = (int *)MemAllocate(Nop);*/
  /* Thread *p = new Thread("President Thread"); */ /* create the president thread */
  /* Thread *s[Ns]; */ /* declare total number of senator threads */
  /* Thread *v[Nv]; */ /* declare total number of visitor threads */
  /* Thread *op[Nop];*/ /* declare total number of operator threads */
 /* phoneStatus = (int *)MemAllocate(NOOFPHONES);
  operatorStatus = (int *)MemAllocate(Nop);*/
  freeOperators = 0;
/*  repositoryID = (int *)MemAllocate(Nop);
  repositoryMoney = (int *)MemAllocate(Nop);
  authenticationMechanism = (int *)MemAllocate(Nop);
  moneyReserve = (int *)MemAllocate(Nop); */
  for(i=0;i<NOOFPHONES;i++) 
    phoneStatus[i]=FREE; /* initially set the status of all the phones to be free */
  for (k=0;k<Nop;k++){
    activate[k]=0;
    moneyReserve[k] = 0;
  }
  for (z=0;z<Nv;z++)
    repositoryMoney[z]=0;  

  switch(choice)
    {
    case 1: /* run the system test   */ 
     WriteMe("\n\n Speaker \t Phone # \t    Time      Operator #    Repetition       Status                     Remarks ");
      WriteMe("\n ~~~~~~~ \t ~~~~~~~ \t    ~~~~      ~~~~~~~~~~    ~~~~~~~~~~       ~~~~~~                     ~~~~~~~ \n\n");           		  
      for(j=0;j<Nop;j++) /* create the total number of operator threads */
	{
	  /* op[j] =  new Thread("Operator"); */
	  Fork(Operator); /* fork the operators */
	}
      Fork(President); /* fork the president */
      for(i=0;i<Ns;i++) /* create the total number of senator threads */
	{
	  /* s[i] = new Thread("Senator"); */
	  /* ID = i + 100*(rand()%2?10:1); //randomly generate the ID for senators */
	  Fork(Senator);      /* fork the senators */
	  
	}
      for(i=0;i<Nv;i++) /* create total number of visitor threads */
	{
	  /* v[i]=new Thread("Visitor"); */
	  Fork(Visitor); /* fork the visitors */
	}	
  
      break;
      
    case 2: /* run the repeatable tests */
      /* different possible test cases are available as shown below and the user is allowed to pick his/her choice */
       WriteMe("\n\n\n ----------------Choose the type of Repeatable Test--------------------- ");
      WriteMe("\n 1. All phones are busy initially ");
      WriteMe("\n 2. All Operators are busy initially");
      WriteMe("\n 3. President comes first, then Senators, then Visitors ");
      WriteMe("\n 4. President comes first, then Visitors, then Senators ");
      WriteMe("\n 5. Senators come first, then President, then Visitors ");
      WriteMe("\n 6. Senators come first, then Visitors, then President ");
      WriteMe("\n 7. Visitors come first, then President, then Senators ");
      WriteMe("\n 8. Visitors come first, then Senators, then President ");     
      WriteMe("\n 9. President comes first, then Senators, NO Visitors ");
      WriteMe("\n 10. Senators come first, then President, NO Visitors ");
      WriteMe("\n 11. President comes first, then Visitors, NO Senators ");
      WriteMe("\n 12. Visitors come first, then President, NO Senators ");
      WriteMe("\n 13. Senators come first, then Visitors, NO President ");
      WriteMe("\n 14. Visitors come first, then Senators, NO President "); 
      WriteMe("\n 15. President only ");
      WriteMe("\n 16. Senators only ");
      WriteMe("\n 17. Visitors only ");
      WriteMe("\n\n Enter your choice : "); 
      /*scanf("%d",&typeOfTest);*/
	  Read(buf,10,0);
	  typeOfTest=getnum(buf,10);
       WriteMe("\n\n Speaker \t Phone # \t    Time      Operator #    Repetition       Status                     Remarks ");
      WriteMe("\n ~~~~~~~ \t ~~~~~~~ \t    ~~~~      ~~~~~~~~~~    ~~~~~~~~~~       ~~~~~~                     ~~~~~~~ \n\n"); 
      switch(typeOfTest)
	{
	  /* in all the test cases, the opearators are forked before all the other threads */
	case 1: /* all the phones are busy initially even before the threads are forked */
	  for(i=0;i<NOOFPHONES;i++)
	    phoneStatus[i]=BUSY;
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President); 
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }	            
	  break;
	case 2: /* all the operators are busy initially even before the threads are forked */
	  for(i=0;i<Nop;i++)
	    operatorStatus[i]=BUSY;  	
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President); 
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  break;
	case 3: /* This is an ideal test. The president is forked first, then the senators, then the visitors */
	  /*  for(int i=0;i<NOOFPHONES;i++) */
	  /*  phoneStatus[i]=BUSY; */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President); 
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  break;
	case 4: /* The president is forked first, then the visitors and then the senators */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President); 
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  break;
	case 5: /* The senators are forked first, then the president and then the visitors */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  Fork(President); 
	  for(i=0;i<Nv;i++)
	    {
	      /*  v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  break;
	case 6: /* The senators are forked first, then the visitors and then the president */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  Fork(President); 		
	  break;
	case 7: /* the visitors are forked first, then the president and then the senators */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  Fork(President); 
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  break;
	case 8: /* the visitors are forked first, then the senators and then the president */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  Fork(President); 
	  break;
	case 9: /* the president is forked first, then the senators. Visitors are NOT forked */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President); 
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  break;
	case 10: /* the senators are forked first, then the presidet. Visitors are NOT forked */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  Fork(President); 
	  break;
	case 11: /* the president is forked first, then the visitors. Senators are NOT forked */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President); 
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  break;
	case 12: /* the visitors are forked first, then the president. Senators are NOT forked */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  Fork(President); 
	  break;
	case 13: /* the senators are forked forst, then the visitors. President is NOT forked */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  break;
	case 14: /* the visitors are forked first, then the senators. President is NOT forked */
	  for(j=0;j<Nop;j++)
	    {
	      /*  op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      Fork(Senator);
	    }
	  break;
	case 15: /* ONLY the President is forked */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  Fork(President);
	  break;
	case 16: /* ONLY the Senators are forked  */
	  for(j=0;j<Nop;j++)
	    {
	      /* op[j] =  new Thread("Operator"); */
	      Fork(Operator);
	    }
	  for(i=0;i<Ns;i++)
	    {
	      /* s[i] = new Thread("Senator"); */
	      /* ID =  i+(100*(rand()%2?10:1)); */
	      Fork(Senator);
	    }
	  break;
	case 17: /* ONLY the visitors are forked */
	  for(j=0;j<Nop;j++)
		{
		  /* op[j] =  new Thread("Operator"); */
		  Fork(Operator);
		}
	  for(i=0;i<Nv;i++)
	    {
	      /* v[i]=new Thread("Visitor"); */
	      Fork(Visitor);
	    }
	  break;
	default:
	   WriteMe("\n SORRY!!! WRONG CHOICE ");
	  break;                              
	} 
      break;
    default:
       WriteMe("\n SORRY!!! WRONG CHOICE "); 
      break;  
    }
  /*  Thread *lastone = new Thread("last guy"); //create the summary thread */
 /* Fork(Summary);  /* fork the summary thread     */
  Exit(0);
}
