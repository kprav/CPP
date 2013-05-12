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

int NumOperator = 0;
void Operator()
{
  /* loop for the operator thread to run continuously */
  int who, i;
  
  char lockName[30];
  char condName1[30];
  char condName2[30];
  int presidentStatus;
  int phoneStatus;
  char printing[50];
  int lockID1, lockID2, lockID3, lockID4, lockID5, lockID6, condID1, condID2, condID3, condID4;
  int indOpLock[20], waitForOperVerCV[20], waitForCallerCV[20];
  int activate, authMechanism, freeOperators, operatorStatus;
  int repositoryID, repositoryMoney;
  int moneyReserve, visitorAcceptCount;


  presidentStatus = CreateSharedInt("presidentStatus",15,1);
  phoneStatus = CreateSharedInt("phoneStatus",11,NOOFPHONES);
  freeOperators = CreateSharedInt("freeOperators",13,1);
  operatorStatus = CreateSharedInt("operatorStatus",14,Nop);
  activate = CreateSharedInt("activate",sizeof("activate"),Nop);
  authMechanism = CreateSharedInt("authMechanism",sizeof("authMechanism"),Nop);
  repositoryID = CreateSharedInt("repositoryID",sizeof("repositoryID"),Nop);
  repositoryMoney = CreateSharedInt("repositoryMoney",sizeof("repositoryMoney"),Nop);
  moneyReserve = CreateSharedInt("moneyReserve",sizeof("moneyReserve"),Nop);
  visitorAcceptCount = CreateSharedInt("visitorAcceptCount",sizeof("visitorAcceptCount"),Nop);

  lockID1 = CreateLock("phoneLock",10);                  /* obtain a master lock for all phones */
  lockID2 = CreateLock("GlobalOpLock",19);     /* obtain a master lock for all the operators */
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
      SetSharedInt(operatorStatus,who,FREE);
      SetSharedInt(freeOperators, 0, GetSharedInt(freeOperators, 0) + 1); /*freeOperators++; */
      /* Acquire lock specific to me */
      AcquireLock(indOpLock[who]);
      ReleaseLock(lockID2);
      /* Initialize some values */
      SetSharedInt(authMechanism,who,0);
      
      while(GetSharedInt(authMechanism,who)==0) /* wait till some one is waiting for the operator - 1->President, 2->Senator, 3->Visitor */
		WaitCV(waitForCallerCV[who],indOpLock[who]);
      
    switch(GetSharedInt(authMechanism,who)) /* process the customer based on whether the authenticationMechanism value is 1 or 2 or 3 */
	{
	case 0: 
	  /* printf("Illegal\n"); */
	  break;
	case 1:            /* president is talking to operator */
	  SetSharedInt(activate,who,1); /* allow him/her to talk */
	  break;
	case 2:                /* senator is talking to operator    */        
	  if(GetSharedInt(repositoryID,who)>=10)
	    {
	      SetSharedInt(activate,who,1); /* allow him/her to talk on verification of ID */
	    }
	  else
	    {
	      SetSharedInt(activate,who,0); /* deny access to phone for senator is ID verifiaction fails */
	    }
	  break;
	case 3:                      /* visitor is talking to operator */
	  if(GetSharedInt(repositoryMoney,who)==1)
	    {	      
	      SetSharedInt(activate,who,1);       /* allow him/her to talk if the visitor pays $1	  */    
	      SetSharedInt(moneyReserve,who, GetSharedInt(moneyReserve, who)+1);   /* increment the amount of money collected by the current operator */
	      AcquireLock(lockID3);
	      SetSharedInt(visitorAcceptCount, who, GetSharedInt(visitorAcceptCount, who)+1);  /* increment the number of visitors permitted to make a call	 */      	      
	      ReleaseLock(lockID3);
	    }
	  else if(GetSharedInt(repositoryMoney,who)==0)
	    {
	      SetSharedInt(activate,who,0);      /* deny the visitor to make a call bacause he/she failed to pay $1 */
		  
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

int main()
{
	int i, phoneStatus;
	Write("Operator Client Program\nForking",100,1); WriteNum(Nop); Write("Operators\n",100,1);
	/*phoneStatus = CreateSharedInt("phoneStatus",11,NOOFPHONES);
	for(i=0;i<NOOFPHONES;i++)
		SetSharedInt(phoneStatus,i,1);*/
	for(i = 0;i<Nop;i++)
		Fork(Operator);	
	Exit(0);
}

