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

int NumSenator = 0;

void Senator() /* code block to perform senator operation */
{
  /* senator ID is randomly generated during forking and we assume that a senator with ID greater than 1000 has an authentiate ID */
  int senatorNumberOfCalls=0; /* keep count of maximum number of calls made by a senator */
  int operatorToUse;
  int checkCorrectOperatorS;
  int ID,i,j, num;
  int talkingTime;
  int condnToWait;
  int phoneToUse, gotPhone;
  int thisActivate;
  int callPresident;
  int randomWaitingTime;

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
  int repositoryID;

  presidentStatus = CreateSharedInt("presidentStatus",15,1);
  phoneStatus = CreateSharedInt("phoneStatus",11,NOOFPHONES);
  freeOperators = CreateSharedInt("freeOperators",13,1);
  operatorStatus = CreateSharedInt("operatorStatus",14,Nop);
  activate = CreateSharedInt("activate",sizeof("activate"),Nop);
  authMechanism = CreateSharedInt("authMechanism",sizeof("authMechanism"),Nop);
  repositoryID = CreateSharedInt("repositoryID",sizeof("repositoryID"),Nop);


  lockID1 = CreateLock("phoneLock",10);                  /* obtain a master lock for all phones */
  lockID2 = CreateLock("GlobalOpLock",sizeof("globaloplock"));     /* obtain a master lock for all the operators */
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
  
  
  
  AcquireLock(lockID4);
  ID = NumSenator + ((RandomFunction(10)>2)?10:0); 
  NumSenator++;
  ReleaseLock(lockID4);
  while(senatorNumberOfCalls<3)
    {
      AcquireLock(lockID1);
      condnToWait = TRUE;      
      /* loop to check if president is waiting. If yes, then senator has to wait. Otherwise senator can obtain a phone */
		do
		{      
			if(GetSharedInt(presidentStatus,0)==1)
				condnToWait = TRUE;
			else
			{
				/*
				for(i=0;i<NOOFPHONES;i++)
				{
					if(GetSharedInt(phoneStatus,i)==FREE)
					{
						SetSharedInt(phoneStatus,i,BUSY);
						gotPhone = TRUE;
						phoneToUse = i;
						condnToWait = FALSE;
						break;
					}
				} */
				phoneToUse = GetOneIndex(phoneStatus);
				if(phoneToUse!=NOOFPHONES)
					condnToWait = FALSE;
			}
			if(condnToWait==TRUE)
				WaitCV(condID2,lockID1); /* senator waits if the president is already waiting to make a call */
		}while(condnToWait==TRUE);
		ReleaseLock(lockID1);
      /* Senator has got a Phone */
      /* Need to get an operator now */
      AcquireLock(lockID2);
      while(GetSharedInt(freeOperators,0)==0)
			WaitCV(condID4,lockID2); /* senator has to wait if there are no free operators avaialble */
      /* Some operator available. Let us find out who it is */
	  /*
      for(j=0;j<Nop;j++)
	{
	  if(GetSharedInt(operatorStatus,j)==FREE)
	    {
	      operatorToUse = j;
	      break;
	    }
	} */
	operatorToUse = GetOneIndex(operatorStatus);
      /* operator obtained*/
      checkCorrectOperatorS = operatorToUse; /* check if the operator to whom the senator ID is submitted is the same as the one that permits/denies the senator to talk */	  
      AcquireLock(indOpLock[operatorToUse]);	  
      SetSharedInt(operatorStatus, operatorToUse, BUSY);
      SetSharedInt(freeOperators, 0, GetSharedInt(freeOperators, 0) - 1);
	  SetSharedInt(activate,operatorToUse,2);
      ReleaseLock(lockID2);
      SetSharedInt(authMechanism, operatorToUse, 2); /* 1 for President | 2 for Senators | 3 for Visitors */
      SetSharedInt(repositoryID, operatorToUse, ID);
      /* If operator is sleeping, wake up */
	  
      SignalCV(waitForCallerCV[operatorToUse],indOpLock[operatorToUse]);
	  while(GetSharedInt(activate,operatorToUse)==2)
	      WaitCV(waitForOperVerCV[operatorToUse],indOpLock[operatorToUse]);      

      thisActivate = GetSharedInt(activate, operatorToUse);
      ReleaseLock(indOpLock[operatorToUse]);
      if(thisActivate==0) /* senator is denied access to phone beacuse of fake ID */
	{
	  j=0;
	  talkingTime=0;
	  /* printf("Senator%d \t  UNAVAILABLE \t %d/%d units \t %d \t\t %d \t    DENIED \t  
	     ID is less than 1000 - verified by operator %d \n",ID+1,j,talkingTime,operatorToUse+1,senatorNumberOfCalls+1,checkCorrectOperatorS+1); */
	  /* AcquireLock(displayLock);*/
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
	  Write(" \t  DENIED \t   ID is less than 10 - verified by operator ",100,1);
	  num=checkCorrectOperatorS+1;
	  itoa(printing,10,num);
	  Write(printing,sizeof(printing),1);
	  Write(" \n",3,1);
	  /*ReleaseLock(displayLock);*/
	}
      else if(thisActivate==1) /* Senator has an authenticate ID. Operator verifies and senator is allowed to make a call */
	{
	  /* Now Talk */
	  talkingTime = RandomFunction(10); /* randomly generate the amount of time the senator will talk on the phone */
	  /* loop for the senator to talk on the phone for the randomly generated time period */
	  for (i=1;i<=talkingTime;i++){
	    /* printf("Senator%d \t  %d \t\t %d/%d units \t %d \t\t %d \t    ACCEPTED \t   ID is greater than 1000 - verified by operator %d 
	       \n",ID+1,phoneToUse+1,i,talkingTime,operatorToUse+1,senatorNumberOfCalls+1,checkCorrectOperatorS+1); */
	    /*AcquireLock(displayLock);*/
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
	    Write(" \t   ACCEPTED \t   ID is greater than 10 - verified by operator ",100,1);
	    num=checkCorrectOperatorS+1;
	    itoa(printing,10,num);
	    Write(printing,sizeof(printing),1);
	    Write(" \n",3,1);
	    /*	ReleaseLock(displayLock);*/
	 /*   Yield(); */
	  }
	  
	}
      senatorNumberOfCalls++; /* increment the number of calls made by the senator */
      /* senator is done talking */
      /* Set the phone status to be free */
      AcquireLock(lockID1);
      SetSharedInt(phoneStatus,phoneToUse,FREE);
      if(GetSharedInt(presidentStatus,0)==0) /* president is not waiting to talk */
	{
	  if(CheckCondWaitQueue(condID2)) 
	    SignalCV(condID2,lockID1); /* wake up the next senator waiting to talk */
	  else if(CheckCondWaitQueue(condID3))
	    SignalCV(condID3,lockID1); /* if no senator is waiting then wake up the next visitor waiting to talk */
	}
      else /* president is waiting to talk, so senators and visitors will have to wait */
	{
	  callPresident = TRUE;
	  /*
	  for(i=0;i<NOOFPHONES;i++)
	    if((i!=phoneToUse)&&(GetSharedInt(phoneStatus,i)==BUSY)) / check if even a single phone is busy other than the phone just used by the senator which he/she sets to free 
	      {
		callPresident = FALSE;
		break;
	      }
		  */
		i = ArraySearch(phoneStatus, phoneToUse, BUSY);
		if(i!=NOOFPHONES)
			callPresident = FALSE;

	  if(callPresident==TRUE) 
	    SignalCV(condID1,lockID1); /* if all phones are free, then no one is talking currently and so, signal the president */
	}
      ReleaseLock(lockID1);     
	  if(thisActivate == 0)
		  Exit(0);
      /* senator goes away */
      /* senator waits for a random amount of time before coming back to make the next caa. Remember maximum number of calls allowed per senator is 10. */
      randomWaitingTime = RandomFunction(3);
      for(j=0;j<randomWaitingTime;j++)
	Yield();
    }
  WriteMe("Senator ");WriteNum(ID+1);WriteMe(" leaving\n");
  Exit(0);
}


int main()
{
	int i;
	Write("Senator Client Program\nForking ",100,1); WriteNum(Ns); Write(" Senators\n",100,1);
	for(i = 0;i<Ns;i++)
		Fork(Senator);	
	Exit(0);
}

