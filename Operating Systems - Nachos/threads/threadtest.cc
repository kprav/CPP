

//--------------------------------------------------------------------------/
//                             PROJECT 1                                    /
//                             ~~~~~~~~~                                    /
//     Maheswaran Sathiamoorthy (USC ID:3092088131) - msathiam@usc.edu      /
//     Praveen Kannan           (USC ID:1462194454) - kannanp@usc.edu       /
//     Saravanan Rangaraju      (USC ID:5768510703) - srangara@usc.edu      /
//--------------------------------------------------------------------------/                                

#include "system.h"
#include "string.h"
#include "copyright.h"
#include "synch.h"

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
  int num;
  
  for (num = 0; num < 5; num++) {
    printf("*** thread %d looped %d times\n", which, num);
    currentThread->Yield();
  }
}



//-----------------------------------------------------------------------/
//                           Part 1 Begins                               /
//-----------------------------------------------------------------------/

#ifdef CHANGED

// --------------------------------------------------
// Test Suite
// --------------------------------------------------


// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the 
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");		  // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
  t1_l1.Acquire();
  t1_s1.V();  // Allow t1_t2 to try to Acquire Lock
  
  printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
	  t1_l1.getName());
  t1_s3.P();
  printf ("%s: working in CS\n",currentThread->getName());
  for (int i = 0; i < 1000000; i++) ;
  printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	  t1_l1.getName());
  t1_l1.Release();
  t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {
  
  t1_s1.P();	// Wait until t1 has the lock
  t1_s2.V();  // Let t3 try to acquire the lock
  
  printf("%s: trying to acquire lock %s\n",currentThread->getName(),
	 t1_l1.getName());
  t1_l1.Acquire();
  
  printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
	  t1_l1.getName());
  for (int i = 0; i < 10; i++)
    ;
  printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	  t1_l1.getName());
  t1_l1.Release();
  t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {
  
  t1_s2.P();	// Wait until t2 is ready to try to acquire the lock
  
  t1_s3.V();	// Let t1 do it's stuff
  for ( int i = 0; i < 3; i++ ) {
    printf("%s: Trying to release Lock %s\n",currentThread->getName(),
	   t1_l1.getName());
    t1_l1.Release();
  }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");		// For mutual exclusion
Condition t2_c1("t2_c1");	// The condition variable to test
Semaphore t2_s1("t2_s1",0);	// To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
// done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
  t2_l1.Acquire();
  printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	 t2_l1.getName(), t2_c1.getName());
  t2_c1.Signal(&t2_l1);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
	 t2_l1.getName());
  t2_l1.Release();
  t2_s1.V();	// release t2_t2
  t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
  t2_s1.P();	// Wait for t2_t1 to be done with the lock
  t2_l1.Acquire();
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	 t2_l1.getName(), t2_c1.getName());
  t2_c1.Wait(&t2_l1);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
	 t2_l1.getName());
  t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");		// For mutual exclusion
Condition t3_c1("t3_c1");	// The condition variable to test
Semaphore t3_s1("t3_s1",0);	// To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
  t3_l1.Acquire();
  t3_s1.V();		// Let the signaller know we're ready to wait
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	 t3_l1.getName(), t3_c1.getName());
  t3_c1.Wait(&t3_l1);
  printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
  t3_l1.Release();
  t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {
  
  // Don't signal until someone's waiting
  
  for ( int i = 0; i < 5 ; i++ ) 
    t3_s1.P();
  t3_l1.Acquire();
  printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	 t3_l1.getName(), t3_c1.getName());
  t3_c1.Signal(&t3_l1);
  printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
  t3_l1.Release();
  t3_done.V();
}

// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");		// For mutual exclusion
Condition t4_c1("t4_c1");	// The condition variable to test
Semaphore t4_s1("t4_s1",0);	// To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
  t4_l1.Acquire();
  t4_s1.V();		// Let the signaller know we're ready to wait
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	 t4_l1.getName(), t4_c1.getName());
  t4_c1.Wait(&t4_l1);
  printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
  t4_l1.Release();
  t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {
  
  // Don't broadcast until someone's waiting
  
  for ( int i = 0; i < 5 ; i++ ) 
    t4_s1.P();
  t4_l1.Acquire();
  printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
	 t4_l1.getName(), t4_c1.getName());
  t4_c1.Broadcast(&t4_l1);
  printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
  t4_l1.Release();
  t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");		// For mutual exclusion
Lock t5_l2("t5_l2");		// Second lock for the bad behavior
Condition t5_c1("t5_c1");	// The condition variable to test
Semaphore t5_s1("t5_s1",0);	// To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
  t5_l1.Acquire();
  t5_s1.V();	// release t5_t2
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	 t5_l1.getName(), t5_c1.getName());
  t5_c1.Wait(&t5_l1);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
	 t5_l1.getName());
  t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
  t5_s1.P();	// Wait for t5_t1 to get into the monitor
  t5_l1.Acquire();
  t5_l2.Acquire();
  printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	 t5_l2.getName(), t5_c1.getName());
  t5_c1.Signal(&t5_l2);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
	 t5_l2.getName());
  t5_l2.Release();
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
	 t5_l1.getName());
  t5_l1.Release();
}

#endif


//---------------------------------------------------------------------/
//                            Part 2 Begins                            /
//---------------------------------------------------------------------/
#define FREE 1
#define BUSY 0
#define TRUE 1
#define FALSE 0
#define NOOFPHONES 20

int Nv,Ns,Nop; //Number of visitors, senators and Operators
int choice,typeOfTest; //allow the user to pick his/her choice of test case - system test or repeatable test

Lock *phoneLock = new Lock("phoneLock");                  //obtain a master lock for all phones
Lock *operatorLock = new Lock("GlobalOperatorLock");      //obtain a master lock for all the operators
Lock *visitorCountLock = new Lock("vistorCountLock");     //obtain a lock to keep track of the number of visitors permitted to make a call
Lock **individualOperatorLock;                            //obtain an individual lock for every operator

Condition *presidentNeedsPhone = new Condition("presidentNeedsPhone"); //condition variable for the condition that president needs phone
Condition *senatorNeedsPhone = new Condition("senatorNeedsPhone");     //condition variable for the condition that senator needs phone
Condition *visitorNeedsPhone = new Condition("visitorNeedsPhone");     //condition variable for the condition that visitor  needs phone
Condition *processCustomer = new Condition("processCustomer");         //condition variable to allow president/senator/visitor to make a call
Condition **waitForOperatorVerification;                               //condition variable to check if president/senator/visitor get operator authentication to make a call
Condition **waitForCaller;                                             //condition variable for operator to wait for a caller(president/senator/visitor)

int *phoneStatus;    //track status of every phone
int presidentStatus; //check if president is waiting to talk or if he is currently talking
int freeOperators;   //track the number of free operators
int *operatorStatus; //track the status of every operator

//-----Operator Authentication related values-----
int *repositoryID;
int *repositoryMoney;
int *authenticationMechanism;
int *moneyReserve;
int *activate;

int visitorAcceptCount = 0;  //Keep count of number of visitors allowed to make a call

void President(int who) //code block to perform president operation
{
  int presidentNumberOfCalls=0; //keep count of total number of calls made by the president
  while(presidentNumberOfCalls<5)
    {
      int condnToWait = FALSE;
      int phoneToUse, gotPhone,i;
      phoneToUse = 0;
      phoneLock->Acquire();
      presidentStatus = 1;
      //loop for the president to keep waiting even if a single phone is busy
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
	    presidentNeedsPhone->Wait(phoneLock);
	}while(condnToWait==TRUE);
      //all phones are free now
      phoneStatus[phoneToUse] = BUSY;
      //president has obtained a phone now
      phoneLock->Release();      
      //Need to get an operator
      operatorLock->Acquire();
      //loop to wait till an operator is available
      while(freeOperators==0)
	processCustomer->Wait(operatorLock); //president has to wait if there are no free operators available
      //Some operator is available. Though I don't know who it is yet, let us find out 
      int operatorToUse;
      for(int j=0;j<Nop;j++)
	{
	  if(operatorStatus[j]==FREE)
	    {
	      operatorToUse = j;
	      break;
	    }
	}
      //operator obtained
      //check if the operator to whom president goes for authentication is same as the one who permits him/her to make a call.
      int checkCorrectOperator = operatorToUse;
      individualOperatorLock[operatorToUse]->Acquire();
      activate[operatorToUse]=0;
      operatorStatus[operatorToUse]=BUSY;
      freeOperators--;
      operatorLock->Release();
      authenticationMechanism[operatorToUse] = 1; //1 for President | 2 for Senators | 3 for Visitors
      //If operator is sleeping, wake up
      waitForCaller[operatorToUse]->Signal(individualOperatorLock[operatorToUse]);
      waitForOperatorVerification[operatorToUse]->Wait(individualOperatorLock[operatorToUse]);
      individualOperatorLock[operatorToUse]->Release();     
      if(activate[operatorToUse]==0)//President is denied access to phone.But this will never happen as there is only one president and we assume that his/her ID is never faked
	{
	  printf("President is denied access to Phone failing authentication!\n");
	}
      else if(activate[operatorToUse]==1) //operator succesfully authenticates the identity of the president
	{
	  //Now Talk
	  int talkingTime = 1+rand()%20; //randomly generate the amount of time the president is talking
          //loop for the president to talk on the phone for the randomly generated time period
	  for (int j=1;j<=talkingTime;j++){
	    printf("President \t  %d \t\t %d/%d units \t %d \t\t %d \t    ACCEPTED       NOTAPPLICABLE - verified by operator %d \n",phoneToUse+1,j,talkingTime,operatorToUse+1,presidentNumberOfCalls+1,checkCorrectOperator+1);
	    currentThread->Yield();
	  }
	  //president is done talking	  
	  //Set the status to be free
	  phoneLock->Acquire();
	  phoneStatus[phoneToUse]=FREE;
	  presidentStatus = 0;
	  senatorNeedsPhone->Broadcast(phoneLock); //wake up all the senators waiting to talk
	  visitorNeedsPhone->Broadcast(phoneLock); //wake up all the visitors waiting to talk
	  phoneLock->Release();
	}
      //president goes away 
      //president waits for a random amount of time before coming back to make the next call. Remember maximum number of calls allowed is 5
      int waitingTime = 1+rand()%7;
      for(int j=0;j<waitingTime;j++)
	{
	  currentThread->Yield();
	}
      presidentNumberOfCalls++; //increment the number of calls made by the president
    }
}

void Senator(int ID) //code block to perform senator operation
{
  //senator ID is randomly generated during forking and we assume that a senator with ID greater than 1000 has an authentiate ID
  int senatorNumberOfCalls=0; //keep count of maximum number of calls made by a senator
  while(senatorNumberOfCalls<10)
    {
      phoneLock->Acquire();
      int condnToWait = TRUE;
      int phoneToUse, gotPhone;
      //loop to check if president is waiting. If yes, then senator has to wait. Otherwise senator can obtain a phone
      do
	{      
	  if(presidentStatus==1)
	    condnToWait = TRUE;
	  else
	    {
	      for(int i=0;i<NOOFPHONES;i++)
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
	    senatorNeedsPhone->Wait(phoneLock); //senator waits if the president is already waiting to make a call
	}while(condnToWait==TRUE);
      phoneLock->Release();
      //Senator has got a Phone
      //Need to get an operator now
      operatorLock->Acquire();
      while(freeOperators==0)
	processCustomer->Wait(operatorLock); //senator has to wait if there are no free operators avaialble
      //Some operator available. Let us find out who it is
      int operatorToUse;
      for(int j=0;j<Nop;j++)
	{
	  if(operatorStatus[j]==FREE)
	    {
	      operatorToUse = j;
	      break;
	    }
	}
      //operator obtained
      int checkCorrectOperator = operatorToUse; //check if the operator to whom the senator ID is submitted is the same as the one that permits/denies the senator to talk      
      individualOperatorLock[operatorToUse]->Acquire();
      operatorStatus[operatorToUse]=BUSY;
      freeOperators--;
      operatorLock->Release();
      authenticationMechanism[operatorToUse] = 2; //1 for President | 2 for Senators | 3 for Visitors
      repositoryID[operatorToUse] = ID;
      //If operator is sleeping, wake up
      waitForCaller[operatorToUse]->Signal(individualOperatorLock[operatorToUse]);
      waitForOperatorVerification[operatorToUse]->Wait(individualOperatorLock[operatorToUse]);
      int thisActivate;
      thisActivate = activate[operatorToUse];
      individualOperatorLock[operatorToUse]->Release();
      if(thisActivate==0) //senator is denied access to phone beacuse of fake ID
	{
	  int j=0;
	  int talkingtime=0;
	  printf("Senator%d \t  UNAVAILABLE \t %d/%d units \t %d \t\t %d \t    DENIED \t   ID is less than 1000 - verified by operator %d \n",ID+1,j,talkingtime,operatorToUse+1,senatorNumberOfCalls+1,checkCorrectOperator+1);
	}
      else if(thisActivate==1) //Senator has an authenticate ID. Operator verifies and senator is allowed to make a call
	{
	  //Now Talk
	  int talkingTime = 1+rand()%10; //randomly generate the amount of time the senator will talk on the phone
	  //loop for the senator to talk on the phone for the randomly generated time period
	  for (int i=1;i<=talkingTime;i++){
	    printf("Senator%d \t  %d \t\t %d/%d units \t %d \t\t %d \t    ACCEPTED \t   ID is greater than 1000 - verified by operator %d \n",ID+1,phoneToUse+1,i,talkingTime,operatorToUse+1,senatorNumberOfCalls+1,checkCorrectOperator+1);
	    currentThread->Yield();
	  }
	  
	}
      senatorNumberOfCalls++; //increment the number of calls made by the senator
      //senator is done talking
      //Set the phone status to be free
      phoneLock->Acquire();
      phoneStatus[phoneToUse]=FREE;
      if(presidentStatus==0) //president is not waiting to talk
	{
	  if(senatorNeedsPhone->isSomebodyWaiting()) 
	    senatorNeedsPhone->Signal(phoneLock); //wake up the next senator waiting to talk
	  else
	    visitorNeedsPhone->Signal(phoneLock); //if no senator is waiting then wake up the next visitor waiting to talk
	}
      else //president is waiting to talk, so senators and visitors will have to wait
	{
	  int callPresident = TRUE;
	  for(int i=0;i<NOOFPHONES;i++)
	    if((i!=phoneToUse)&&(phoneStatus[i]==BUSY)) //check if even a single phone is busy other than the phone just used by the senator which he/she sets to free
	      {
		callPresident = FALSE;
		break;
	      }
	  if(callPresident==TRUE) 
	    presidentNeedsPhone->Signal(phoneLock); //if all phones are free, then no one is talking currently and so, signal the president
	}
      phoneLock->Release();          
      //senator goes away
      //senator waits for a random amount of time before coming back to make the next caa. Remember maximum number of calls allowed per senator is 10.
      int randomWaitingTime = 1+rand()%3;
      for(int j=0;j<randomWaitingTime;j++)
	currentThread->Yield();
    }
}

void Visitor(int who) //code block to perform visitor operation
{
  int condnToWait;
  int phoneToUse,gotPhone;
  phoneLock->Acquire();
  //loop to check if the president or senator is waiting. If any one is waiting, then visitor has to wait before he/she can make a call. Otherwise visitor can go ahead
  do
    {
      condnToWait = TRUE;      
      if(presidentStatus == 1)
	condnToWait = TRUE;
      //Check if some senator is already waiting!
      else if(senatorNeedsPhone->isSomebodyWaiting()==1)
	{
	  // Bad luck, there seems to be a senator. 
	  condnToWait = TRUE;
	}
      else
	{
	  for(int i=0;i<NOOFPHONES;i++)
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
	visitorNeedsPhone->Wait(phoneLock); //visitor waits if there is a president or a senator already waitng to make a call.
    }while(condnToWait);
  phoneLock->Release();
  //Visitor has got a phone
  //Need to get an operator now
  operatorLock->Acquire();
  while(freeOperators==0)
    processCustomer->Wait(operatorLock);//visitor has to wait if there are no free operators available
  //Some operator is available. Though I don't know who it is. Let us find out. 
  int operatorToUse;
  for(int j=0;j<Nop;j++)
    {
      if(operatorStatus[j]==FREE)
	{
	  operatorToUse = j;
	  break;
	}
    }
  //operator obtained
  int checkCorrectOperator = operatorToUse; //check if the operator to whom the visitor pays money is the same as the one the permits/denies the visitor to make a call
  individualOperatorLock[operatorToUse]->Acquire();
  activate[operatorToUse]=0;
  operatorStatus[operatorToUse]=BUSY;
  freeOperators--;
  operatorLock->Release();
  authenticationMechanism[operatorToUse] = 3; //1 for President | 2 for Senators | 3 for Visitors
  repositoryMoney[operatorToUse] = ((rand()%100)>80)?0:1; //randomly generate whether the visitor pays $1 or not
  //If operator is sleeping, wake up
  waitForCaller[operatorToUse]->Signal(individualOperatorLock[operatorToUse]);
  waitForOperatorVerification[operatorToUse]->Wait(individualOperatorLock[operatorToUse]);
  int thisActivate=0;
  thisActivate=activate[operatorToUse];  
  individualOperatorLock[operatorToUse]->Release();
  if (thisActivate==0) //visitor is denied access to phone beacause he/she didn't pay $1.
    {
      int j=0;
      int talkingtime=0;
      printf("Visitor%d \t  UNAVAILABLE \t %d/%d units \t %d \t   NOTAPPLICABLE    DENIED \t   Money paid is $0 - verified by operator %d \n",who+1,j,talkingtime,operatorToUse+1,checkCorrectOperator+1);
      //printf("Access to Phone for visitor %d Denied by Operator %d!\n",who+1,operatorToUse+1);
    }
  else if (thisActivate==1) //visitor has paid $1. Operator verifies and visitor is allowed to make a call
    {
      //Now Talk
      int talkingTime = 1+rand()%5; //randomly generate the amount of time the visitor will talk on the phone
      //loop for the visitor to talk on the phone for the randomly generated time period
      for (int i=1;i<=talkingTime;i++){
	printf("Visitor%d \t  %d \t\t %d/%d units \t %d \t   NOTAPPLICABLE    ACCEPTED \t   Money paid is $1 - verified by operator %d \n",who+1,phoneToUse+1,i,talkingTime,operatorToUse+1,checkCorrectOperator+1);
	currentThread->Yield();
      }
      //visitor is done talking
      //Set the phone status to be free
    }
  phoneLock->Acquire();
  phoneStatus[phoneToUse]=FREE;
  if(presidentStatus==0) //president is not waking to talk
    {
      if(senatorNeedsPhone->isSomebodyWaiting())
	senatorNeedsPhone->Signal(phoneLock); //wake up the next senator waiting to talk
      else
	visitorNeedsPhone->Signal(phoneLock); //if no senator is waiting, then wake up the next visitor waiting to talk
    }
  else //president is waiting to talk, so senators and visitors will have to wait
    {
      int callPresident = TRUE;
      for(int i=0;i<NOOFPHONES;i++)
	if((i!=phoneToUse)&&(phoneStatus[i]==BUSY)) //check if even a single phone is busy other than the phone just used by the visitor which he/she sets to free
	  {
	    callPresident = FALSE;
	    break;
	  }
      if(callPresident==TRUE)
	presidentNeedsPhone->Signal(phoneLock); //if all phones are free, then no one is talking currently and so, signal the president
    }
  //visitor goes away and does not return. Remember visitors can make a maximum of just one call      
  phoneLock->Release();	
}

void Operator(int who)
{
  //loop for the operator thread to run continuously
  while(1)
    {    
      operatorLock->Acquire();
      if(processCustomer->isSomebodyWaiting()) //checks if anyone (president/senator/visitor) is waiting for an operator
	{
	  processCustomer->Signal(operatorLock); //signal the waiting person
	}
      // I (operator) am free. So make my status as free so that some customer might be able to use me.
      operatorStatus[who]=FREE;
      freeOperators++;
      //Acquire lock specific to me
      individualOperatorLock[who]->Acquire();
      operatorLock->Release();
      //Initialize some values
      authenticationMechanism[who]=0;
      while(authenticationMechanism[who]==0) //wait till some one is waiting for the operator - 1->President, 2->Senator, 3->Visitor
	waitForCaller[who]->Wait(individualOperatorLock[who]);
      switch(authenticationMechanism[who]) //process the customer based on whether the authenticationMechanism value is 1 or 2 or 3
	{
	case 0: 
	  printf("Illegal\n");
	  break;
	case 1:            //president is talking to operator
	  activate[who]=1; //allow him/her to talk
	  break;
	case 2:                //senator is talking to operator           
	  if(repositoryID[who]>=1000)
	    {
	      activate[who]=1; //allow him/her to talk on verification of ID
	    }
	  else
	    {
	      activate[who]=0; //deny access to phone for senator is ID verifiaction fails
	    }
	  break;
	case 3:                      //visitor is talking to operator
	  if(repositoryMoney[who]==1)
	    {	      
              activate[who]=1;       //allow him/her to talk if the visitor pays $1	     
	      moneyReserve[who]++;   //increment the amount of money collected by the current operator
	      visitorCountLock->Acquire();
	      visitorAcceptCount++;  //increment the number of visitors permitted to make a call	      	      
	      visitorCountLock->Release();
	    }
	  else if(repositoryMoney[who]==0)
	    {
	      activate[who]=0;      //deny the visitor to make a call bacause he/she failed to pay $1
	    }         
	  break;
	default:
	  printf("Unknown Authentication Type\n");	  
	}
      waitForOperatorVerification[who]->Signal(individualOperatorLock[who]);      
      individualOperatorLock[who]->Release();
    }
}


void Summary(int who) //print the number of visitors, money collected by each operator and total money
{
  int notTheEnd = FALSE;
  int i, totalMoney=0;
  do
    {
      notTheEnd = FALSE;
      for(int k=0;k<(Ns+Nv+1)*100;k++) //yield the summary thread until all the other threads have finished executing
	currentThread->Yield();
      if(!(presidentNeedsPhone->isSomebodyWaiting())||(!senatorNeedsPhone->isSomebodyWaiting())||(!visitorNeedsPhone->isSomebodyWaiting()))
	{
	  phoneLock->Acquire();
	  for(i=0;i<NOOFPHONES;i++)
	    {
	      if((phoneStatus[i]==BUSY))
		{
		  notTheEnd = TRUE;
		  break;
		}
	    }
	  phoneLock->Release();
	  printf("\n\nSummary\n");
	  printf("~~~~~~~\n");
	  if(!notTheEnd)
	    {	      	      
	      if ((typeOfTest!=9)&&(typeOfTest!=10)&&(typeOfTest!=15)&&(typeOfTest!=16))
		{		 
		  printf("Total number of Visitors : %d\n",Nv);
		  printf("Number of Visitors Accepted : %d\n",visitorAcceptCount);
		  printf("Number of Visitors Denied : %d\n",Nv - visitorAcceptCount);
		  for(int j=0;j<Nop;j++)
		    {
		      printf("Money collected by Operator %d is %d\n",j+1,moneyReserve[j]);
		      totalMoney += moneyReserve[j];
		    }
		  printf("Total money collected by all operators is %d\n",totalMoney);
		  printf("It can be seen that number of visitors accepted is equal to the total money collected by all the operators.\n\n");
		}
              if ((typeOfTest!=13)&&(typeOfTest!=14)&&(typeOfTest!=16)&&(typeOfTest!=17))
		{
		  printf("The president talks continuously with no interruption from any senator thread or visitor thread till the end of the \nmaximum time units per call. This is a clear indication that when the president talks, no other person is talking.\n");
		}
	      printf("\nThe number in the operator column of each thread matches the operator number by whom it was verified (under the \nremarks column) which is again a clear indication that every senator or visitor or president talk exactly to one operator before \nmaking a call. In other words, the senator is verified by the operator to whom he/she submits his/her ID and the \nvisitor is verified by the operator to whom he/she paid money.\n\n\n");
	      break;	 
	    }
	}
      else
	currentThread->Yield();
    }while (typeOfTest!=1);
}


void ThreadTest(char *val)
{
  if (!(strcmp(val,"-t")&&strcmp(val,"-T"))) //TEST SUITE FOR PART 1 OF THE PROJECT
    {    
      // --------------------------------------------------
      //     This is the main thread of the test suite.  It runs the
      //     following tests:
      //
      //       1.  Show that a thread trying to release a lock it does not
      //       hold does not work
      //
      //       2.  Show that Signals are not stored -- a Signal with no
      //       thread waiting is ignored
      //
      //       3.  Show that Signal only wakes 1 thread
      //
      //	      4.  Show that Broadcast wakes all waiting threads
      //
      //       5.  Show that Signalling a thread waiting under one lock
      //       while holding another is a Fatal error
      //
      //     Fatal errors terminate the thread in question.
      // --------------------------------------------------    
      

      // Test 1
      Thread *t = new Thread("forked thread");
      char *name;
      int i;
      printf("Starting Test 1\n");
      
      t = new Thread("t1_t1");
      t->Fork((VoidFunctionPtr)t1_t1,0);
      
      t = new Thread("t1_t2");
      t->Fork((VoidFunctionPtr)t1_t2,0);
      
      t = new Thread("t1_t3");
      t->Fork((VoidFunctionPtr)t1_t3,0);
      
      // Wait for Test 1 to complete
      for (  i = 0; i < 2; i++ )
	t1_done.P();
      
      // Test 2
      
      printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
      printf("completes\n");
      
      t = new Thread("t2_t1");
      t->Fork((VoidFunctionPtr)t2_t1,0);
      
      t = new Thread("t2_t2");
      t->Fork((VoidFunctionPtr)t2_t2,0);
      
      // Wait for Test 2 to complete
      t2_done.P();
      
      // Test 3
      
      printf("Starting Test 3\n");
      
      for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t3_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t3_waiter,0);
      }
      t = new Thread("t3_signaller");
      t->Fork((VoidFunctionPtr)t3_signaller,0);
      
      // Wait for Test 3 to complete
      for (  i = 0; i < 2; i++ )
	t3_done.P();
      
      // Test 4
      
      printf("Starting Test 4\n");
      
      for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t4_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t4_waiter,0);
      }
      t = new Thread("t4_signaller");
      t->Fork((VoidFunctionPtr)t4_signaller,0);
      
      // Wait for Test 4 to complete
      for (  i = 0; i < 6; i++ )
	t4_done.P();
      
      // Test 5
      
      printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
      printf("completes\n");
      
      t = new Thread("t5_t1");
      t->Fork((VoidFunctionPtr)t5_t1,0);
      
      t = new Thread("t5_t2");
      t->Fork((VoidFunctionPtr)t5_t2,0);
    }
  

  else if (!(strcmp(val,"-p2")&&strcmp(val,"-P2"))) //SYSTEM TEST & REPEATABLE TESTS FOR PART 2 OF PROJECT
    {   
      printf("\n\n\n ~~~~~~~~~~~~~Choose Type of Test~~~~~~~~~~~~~ "); //Choose system test/reapeatable test?
      printf("\n 1. System Tests/Stress Tests ");
      printf("\n 2. Repeatable Tests/Controlled Tests ");
      printf("\n\n Enter your choice : " );
      scanf("%d",&choice);
      printf("\n\n Enter the number of Operators : "); //input the number of operators
      scanf("%d",&Nop);
      printf("\n Enter the number of Senators  : "); //inpuit the number of senators
      scanf("%d",&Ns);
      printf("\n Enter the number of Visitors  : "); //input the number of visitors
      scanf("%d",&Nv);
      presidentStatus = 0;
      //-------initialize all the declared variables----------
      individualOperatorLock = new Lock *[Nop];
      waitForOperatorVerification = new Condition *[Nop];
      waitForCaller = new Condition *[Nop];
      activate = new int[Nop];
      for (int j=0;j<Nop;j++)
	{
	  individualOperatorLock[j]=new Lock("individualOperatorLock");
	  waitForCaller[j] = new Condition("waitForCallerCV");
	  waitForOperatorVerification[j] = new Condition("waitForOperatorAnswerCV");
	}
      Thread *p = new Thread("President Thread"); //create the president thread
      Thread *s[Ns]; //declare total number of senator threads
      Thread *v[Nv]; //declare total number of visitor threads
      Thread *op[Nop]; //declare total number of operator threads
      phoneStatus = new int[NOOFPHONES];
      operatorStatus = new int[Nop];
      freeOperators = 0;
      repositoryID = new int[Nop];
      repositoryMoney = new int[Nop];
      authenticationMechanism = new int[Nop];
      moneyReserve = new int[Nop];
      int ID;
      for(int i=0;i<NOOFPHONES;i++) 
	phoneStatus[i]=FREE; //initially set the status of all the phones to be free
      for (int k=0;k<Nop;k++){
	activate[k]=0;
	moneyReserve[k] = 0;
      }
      for (int z=0;z<Nv;z++)
	repositoryMoney[z]=0;
      switch(choice)
	{
	case 1: //run the system test              		  
	  for(int j=0;j<Nop;j++) //create the total number of operator threads
	    {
	      op[j] =  new Thread("Operator");
	      op[j]->Fork(Operator,j); //fork the operators
	    }
	  p->Fork(President,1); //fork the president
	  for(int i=0;i<Ns;i++) //create the total number of senator threads
	    {
	      s[i] = new Thread("Senator");
	      ID = i + 100*(rand()%2?10:1); //randomly generate the ID for senators
	      s[i]->Fork(Senator, ID);      //fork the senators
	      
	    }
	  for(int i=0;i<Nv;i++) //create total number of visitor threads
	    {
	      v[i]=new Thread("Visitor");
	      v[i]->Fork(Visitor,i); //fork the visitors
	    }	
	  printf("\n\n Speaker \t Phone # \t    Time      Operator #    Repetition       Status                     Remarks ");
	  printf("\n ~~~~~~~ \t ~~~~~~~ \t    ~~~~      ~~~~~~~~~~    ~~~~~~~~~~       ~~~~~~                     ~~~~~~~ \n\n");
	  break;
	  
	case 2: //run the repeatable tests
	  //different possible test cases are available as shown below and the user is allowed to pick his/her choice
	  printf("\n\n\n ----------------Choose the type of Repeatable Test--------------------- ");
	  printf("\n 1. All phones are busy initially ");
	  printf("\n 2. All Operators are busy initially");
	  printf("\n 3. President comes first, then Senators, then Visitors ");
	  printf("\n 4. President comes first, then Visitors, then Senators ");
	  printf("\n 5. Senators come first, then President, then Visitors ");
	  printf("\n 6. Senators come first, then Visitors, then President ");
	  printf("\n 7. Visitors come first, then President, then Senators ");
	  printf("\n 8. Visitors come first, then Senators, then President ");     
	  printf("\n 9. President comes first, then Senators, NO Visitors ");
	  printf("\n 10. Senators come first, then President, NO Visitors ");
	  printf("\n 11. President comes first, then Visitors, NO Senators ");
	  printf("\n 12. Visitors come first, then President, NO Senators ");
	  printf("\n 13. Senators come first, then Visitors, NO President ");
	  printf("\n 14. Visitors come first, then Senators, NO President "); 
	  printf("\n 15. President only ");
	  printf("\n 16. Senators only ");
	  printf("\n 17. Visitors only ");
	  printf("\n\n Enter your choice : ");
	  scanf("%d",&typeOfTest);
	  printf("\n\n Speaker \t Phone # \t    Time      Operator #    Repetition       Status                     Remarks ");
	  printf("\n ~~~~~~~ \t ~~~~~~~ \t    ~~~~      ~~~~~~~~~~    ~~~~~~~~~~       ~~~~~~                     ~~~~~~~ \n\n");
	  switch(typeOfTest)
	    {
	      //in all the test cases, the opearators are forked before all the other threads
	    case 1: //all the phones are busy initially even before the threads are forked
	      for(int i=0;i<NOOFPHONES;i++)
		phoneStatus[i]=BUSY;
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}	            
	      break;
	    case 2: //all the operators are busy initially even before the threads are forked
	      for(int i=0;i<Nop;i++)
		operatorStatus[i]=BUSY;  	
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      break;
	    case 3: //This is an ideal test. The president is forked first, then the senators, then the visitors
	      // for(int i=0;i<NOOFPHONES;i++)
	      // phoneStatus[i]=BUSY;
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      break;
	    case 4: //The president is forked first, then the visitors and then the senators
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      break;
	    case 5: //The senators are forked first, then the president and then the visitors
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      break;
	    case 6: //The senators are forked first, then the visitors and then the president
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      p->Fork(President,1); 		
	      break;
	    case 7: //the visitors are forked first, then the president and then the senators
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      break;
	    case 8: //the visitors are forked first, then the senators and then the president
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      p->Fork(President,1); 
	      break;
	    case 9: //the president is forked first, then the senators. Visitors are NOT forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      break;
	    case 10: //the senators are forked first, then the presidet. Visitors are NOT forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      p->Fork(President,1); 
	      break;
	    case 11: //the president is forked first, then the visitors. Senators are NOT forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      break;
	    case 12: //the visitors are forked first, then the president. Senators are NOT forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      p->Fork(President,1); 
	      break;
	    case 13: //the senators are forked forst, then the visitors. President is NOT forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      break;
	    case 14: //the visitors are forked first, then the senators. President is NOT forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  s[i]->Fork(Senator, i+(100*(rand()%2?10:1)));
		}
	      break;
	    case 15: //ONLY the President is forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      p->Fork(President,1); 
	      break;
	    case 16: //ONLY the Senators are forked 
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Ns;i++)
		{
		  s[i] = new Thread("Senator");
		  ID =  i+(100*(rand()%2?10:1));
		  s[i]->Fork(Senator,ID);
		}
	      break;
	    case 17: //ONLY the visitors are forked
	      for(int j=0;j<Nop;j++)
		{
		  op[j] =  new Thread("Operator");
		  op[j]->Fork(Operator,j);
		}
	      for(int i=0;i<Nv;i++)
		{
		  v[i]=new Thread("Visitor");
		  v[i]->Fork(Visitor,i);
		}
	      break;
	    default:
	      printf("\n SORRY!!! WRONG CHOICE ");
	      break;                              
	    } 
	  break;
	default:
	  printf("\n SORRY!!! WRONG CHOICE ");
	  break;  
	}
      Thread *lastone = new Thread("last guy"); //create the summary thread
      lastone->Fork(Summary,1); //fork the summary thread
    }
  
  else if (!strcmp(val,"null"))
    {   
      //----------------------------------------------------------------------
      // 	Set up a ping-pong between two threads, by forking a thread 
      //	to call SimpleThread, and then calling SimpleThread ourselves.
      //----------------------------------------------------------------------
      DEBUG('t', "Entering SimpleTest");
      Thread *t = new Thread("forked thread");
      t->Fork(SimpleThread, 1);
      SimpleThread(0);
      // Thread *t;
      char *name;
      int i;    
    }
}

