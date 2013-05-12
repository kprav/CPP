#include <iostream>
#include <fstream>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <queue>
#include <stdio.h>
#include <math.h>
#include "utils.h"
#include "list.h"

using namespace std;


//--------------------------Global Declarations Start Here-------------------------------//
char *tsfile;
char sval[1];
int threadID[100];
float pa = 0.0;
float qa = 0.0;
float pb = 0.0;
float qb = 0.0;
float calcqtme = 0.0;
float avgcusts1 = 0.0;
float avgcusts2 = 0.0;
float avgcustq1 = 0.0;
float totintarrtme = 0.0;
float totservicetme = 0.0;
float totqueuetime = 0.0;
float tottimeinsys = 0.0;
float tottimeinsyssq = 0.0;
float dropprob = 0.0;
int numinqueue = 0;
int sflag = 0;
int tflag = 0;
double lambda = 0.5;
double mu = 0.35;
int seedval = 0;
int sz = 5;
int num = 20;
int expdet = 0;
int threadcount = 0;
int custcount = 0;
int shutdown = 0;
int numdropped = 0;
int numthrown = 0;
int custgen = 0;
int custserv = 0;
int serverexit1 = 0;
int serverexit2 = 0;
int queuesz = 0;
struct sigaction act;
struct timeval tmm,tma,tmb,rettime,mt1,mt2;
List *q1 = new List();
sigset_t newset;
customer **c;
FILE *fp;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //create a lock
pthread_mutex_t printlock = PTHREAD_MUTEX_INITIALIZER; //create a lock
pthread_mutex_t globaltimelock = PTHREAD_MUTEX_INITIALIZER; //create a lock
pthread_mutex_t queuelock = PTHREAD_MUTEX_INITIALIZER; //create a lock
pthread_mutex_t calclock = PTHREAD_MUTEX_INITIALIZER; //create a lock
pthread_cond_t waitforarrival = PTHREAD_COND_INITIALIZER; //create a condition variable
pthread_cond_t queuetimecalc = PTHREAD_COND_INITIALIZER; //create a condition variable
//--------------------------------End of Declarations------------------------------------//


/* ./mm2 [-lambda lambda] [-mu mu] [-s] [-seed seedval] [-size sz] [-n num] [-d {exp|det}] [-t tsfile] */

void *siginthandler(void *arg) //thread that handles the SIGINT interrupt
{
	int *a = new int;
	sigwait((const sigset_t *)&newset,a); //wait for the SIGINT interrupt
	pthread_mutex_lock(&mutex);	
	if (sflag == 1)
		pthread_cancel(threadID[3]); //cancel the arrival thread if number of servers is 1
	else
		pthread_cancel(threadID[4]); //cancel the arrival thread if number of servers is 2
	pthread_cancel(threadID[1]);     //cancel the thread that is used for calculating the average number of customers in Q1
	pthread_mutex_unlock(&mutex);
	shutdown = 1;					//set the shutdown variable for the servers to close once they finish handling the current customer
	pthread_mutex_lock(&queuelock);
	numthrown = numinqueue;			//number of customers in the queue that were thrown out
	numinqueue = 0;	
	pthread_cond_broadcast(&waitforarrival); //wake up the server threads
	pthread_mutex_unlock(&queuelock);
	pthread_exit(NULL);
}

void *calcqtime(void *arg) //thread that is used for calculating the average number of customers in Q1
{
	struct timeval t1,t2,r;
	float k;
	memset(&t1, 0, sizeof(struct timeval));
	memset(&t2, 0, sizeof(struct timeval));
	memset(&r, 0, sizeof(struct timeval));
	while(1)
	{
		pthread_testcancel();
		pthread_mutex_lock(&calclock);
		gettimeofday(&t1, NULL);
		pthread_cond_wait(&queuetimecalc,&calclock);
		gettimeofday(&t2, NULL);		
		r = subtracttime(t1,t2);
		k = (float) r.tv_usec;
		k/=1000000;
		k+=(float) r.tv_sec;
		calcqtme+=(float) (k*queuesz);	//calculates and stores the time for which the number of customers currently in the queue were in the queue	
		pthread_mutex_unlock(&calclock);
	}
	pthread_exit(NULL);
}

void *server(void *arg) //the server thread that handles the customers from Q1
{
	gettimeofday(&tma, NULL);	
	struct timeval tm6,rets,rets1,rets2,record,local;
	struct timeval tm7,tm8,tm9,tmc,tmd,rettime1,local1;
	int serverID;
	int check = -1;
	serverID = *((int *)arg);
	float k,exit1,exit2,timeinsystem,l,m;
	int servicetime,sleeptime;
	int breakloop = 0;
	m = 0;
	memset(&tm6, 0, sizeof(struct timeval));
	memset(&rets, 0, sizeof(struct timeval));
	memset(&tm7, 0, sizeof(struct timeval));
	memset(&rets1, 0, sizeof(struct timeval));
	memset(&tm8, 0, sizeof(struct timeval));
	memset(&tm9, 0, sizeof(struct timeval));
	memset(&rets2, 0, sizeof(struct timeval));
	memset(&tmc, 0, sizeof(struct timeval));
	memset(&tmd, 0, sizeof(struct timeval));
	memset(&rettime1, 0, sizeof(struct timeval));
	memset(&mt1, 0, sizeof(struct timeval));
	memset(&mt2, 0, sizeof(struct timeval));
	memset(&local, 0, sizeof(struct timeval));
	memset(&local1, 0, sizeof(struct timeval));
	for(;breakloop==0;)
	{		
		customer *rem = new customer(); 
		pthread_mutex_lock(&queuelock);		
		while (numinqueue == 0 && shutdown==0) //wait for a customer to be available in the queue for servicing
		{
			pthread_cond_wait(&waitforarrival,&queuelock);	
		}
		pthread_mutex_unlock(&queuelock);
		gettimeofday(&tmc, NULL);
		if (shutdown == 1 && numinqueue>0) //signal the other server in case the arrival thread has finished but there are still customers left in the queue
		{			
			pthread_cond_signal(&waitforarrival);
		}			
		if (numinqueue>0) //there are customers in the queue
		{
			check = 0;
			pthread_mutex_lock(&queuelock);
			if (q1->ListSize()>0)
				rem = (customer *)q1->Remove(); //remove a customer from the queue
			else
			{
				pthread_mutex_unlock(&queuelock);
				continue;
			}
			custserv++;  //increment the number of customers served by 1
			numinqueue--; //decrement the number of customers in queue by 1		
			pthread_mutex_lock(&calclock);
			queuesz = q1->ListSize()+1;	   //number of customers in the queue before the current customer was removed from the queue		
			pthread_cond_signal(&queuetimecalc); //calculate the amount of time for which the above number of customers were in the queue
			pthread_mutex_unlock(&calclock);
			pthread_mutex_lock(&calclock);
			pthread_mutex_unlock(&calclock);
			pthread_mutex_unlock(&queuelock);
			gettimeofday(&tm6, NULL);			
			rets = subtracttime(rem->qenttmetv,tm6);	//find the amount of time the current customer was in the queue
			k = (float) (rets.tv_usec);
			k/=1000;		
			k+=(float) (rets.tv_sec*1000);
			totqueuetime += k;
			pthread_mutex_lock(&globaltimelock);			
			tmm.tv_sec = rem->qentry.tv_sec+(rets.tv_sec*1000); //update the global clock
			tmm.tv_usec = rem->qentry.tv_usec+rets.tv_usec;	
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}			
			local = tmm;
			memcpy(&local1,&local,sizeof(struct timeval));
			pthread_mutex_unlock(&globaltimelock);
			
			pthread_mutex_lock(&printlock);		//print the time at which the current customer left the queue and the amount of time for which it was in the queue
			printf("\n %08d.%03dms: ",(int)local.tv_sec,(int)local.tv_usec);
			cout<<"c"<<rem->cno+1<<" leaves Q1, time in Q1 = ";
			printf("%03.3fms",k);	
			pthread_mutex_unlock(&printlock);	
			gettimeofday(&tm9, NULL);
			rets2 = subtracttime(tm6,tm9);	//calculate the time at which the current customer is beginning to be serviced					
			pthread_mutex_lock(&globaltimelock);			
			tmm.tv_sec = local.tv_sec+(rets.tv_sec*1000); //update the global clock
			tmm.tv_usec = local.tv_usec+rets.tv_usec;
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}
			record = tmm;
			local = tmm;
			pthread_mutex_unlock(&globaltimelock);
			pthread_mutex_lock(&printlock);  //print the time at which the current customer is beginning to be serviced
			printf("\n %08d.%03dms: ",(int)local.tv_sec,(int)local.tv_usec);
			cout<<"c"<<rem->cno+1<<" begin service at s"<<serverID;
			pthread_mutex_unlock(&printlock);
			if (tflag == 0) //calculate the service time if a trace specification file is not specified
			{
				if (expdet == 0) //exponential process
					servicetime = GetInterval(1,mu,0,1,(unsigned long)rem->cno,tsfile); //obtain service time
				else if (expdet == 1) //deterministic process
					servicetime = GetInterval(0,mu,0,1,(unsigned long)rem->cno,tsfile); //obtain service time
			}
			else if (tflag == 1) //calculate the service time if a trace specification file is specified
			{
				servicetime = GetInterval(1,mu,1,1,(unsigned long)rem->cno,tsfile); //obtain service time
			}			
			sleeptime = servicetime*1000;	
			gettimeofday(&tm7, NULL); //sleep for the amount of service time
			usleep(sleeptime);
			gettimeofday(&tm8,NULL);			
			rets1 = subtracttime(tm7,tm8);	//calculate the actual time for which the server thread slept		
			pthread_mutex_lock(&globaltimelock);						
			tmm.tv_sec = record.tv_sec+(rets1.tv_sec*1000); //update the global clock
			l = (float) rets1.tv_usec;
			l/=1000;
			tmm.tv_usec = record.tv_usec+rets1.tv_usec;
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}
			local = tmm;
			k=(float)rets1.tv_sec*1000+l;
			totservicetme+=k;
			exit1 = (float) tmm.tv_usec;
			exit1/=1000;
			exit1+=tmm.tv_sec;
			pthread_mutex_unlock(&globaltimelock);
			exit2 = (float) rem->arrtme.tv_usec;
			exit2/=1000;
			exit2+=(float)(rem->arrtme.tv_sec*1000);			
			timeinsystem = exit1-exit2;			//calculate the total time for which the current customer was in the system
			//print the time the current customer departs from the system along with the service time and the time in system
			pthread_mutex_lock(&printlock);
			printf("\n %08d.%03dms: ",(int)local.tv_sec,(int)local.tv_usec); 
			cout<<"c"<<rem->cno+1<<" departs from s"<<serverID<<", service time = ";
			printf("%03.3fms, ",k);	
			cout<<"time in system = ";
			printf("%03.3fms ",timeinsystem);
			tottimeinsys+=timeinsystem; //update the total time in system for all customers
			tottimeinsyssq+=pow(((float)timeinsystem/1000),2); //the total time in system for all customers in seconds
			pthread_mutex_unlock(&printlock);
		}
		if (shutdown == 1 && numinqueue<=0) //all the customers have been serviced...time to quit
		{			
			breakloop = 1;			
		}
		gettimeofday(&tmd, NULL);
		rettime1 = subtracttime(tmc,tmd); //find the time for which the server was busy servicing the current customer
		m = (float) rettime1.tv_usec;
		m/=1000;
		m+=(float) (rettime1.tv_sec*1000);
		if (serverID == 1)
			pa+=m;
		else if (serverID == 2)
			pb+=m;
		m=0;
	}
	gettimeofday(&tmb, NULL); //find the total time for which the server was running
	rettime = subtracttime(tma,tmb);
	if (serverID == 1)
	{
		if (check==0)
		{
			qa = (float) rettime.tv_usec;
			qa/=1000;
			qa+=(float) (rettime.tv_sec*1000);
		}
		else
			qa=0.0;
	}
	else if (serverID == 2)
	{
		if (check==0)
		{
			qb = (float) rettime.tv_usec;
			qb/=1000;
			qb+=(float) (rettime.tv_sec*1000);
		}
		else
			qb=0.0;
	}
	if (serverID==1)
		serverexit1 = 1; //set a variable notifying that server 1 has exited
	if (serverID==2)
		serverexit2 = 1; //set a variable notifying that server 2 has exited
	if (sflag == 1) //there is only one server
	{
		if (serverexit1==1) //check if the server has exited
		{
			pthread_mutex_lock(&mutex);
			pthread_cancel(threadID[0]); //cancel the thread for handling SIGINT
			pthread_cancel(threadID[1]); //cancel the thread for calculating the average number of customers in Q1
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&printlock);
			cout<<"\n";
			pthread_mutex_unlock(&printlock);
			gettimeofday(&mt2, NULL);
		}
	}
	else //there are two servers
	{
		if (serverexit1==1&&serverexit2==1) //check if both the servers has exited
		{
			pthread_mutex_lock(&mutex);
			pthread_cancel(threadID[0]); //cancel the thread for handling SIGINT
			pthread_cancel(threadID[1]); //cancel the thread for calculating the average number of customers in Q1
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&printlock);
			cout<<"\n";
			pthread_mutex_unlock(&printlock);
			gettimeofday(&mt2, NULL);
		}
	}	
	pthread_exit(NULL);
}

void allocate(unsigned long track) //funtion to dynamically allocate the number of customer structs
{
	unsigned long i;
	c = new customer*[track];
	for (i=0;i<track;i++)
		c[i] = new customer();
}

void *arrival(void *arg) //the arrival thread that handles the customer arrivals and pushes them into Q1/drops them if queue is full
{
	gettimeofday(&mt1, NULL);
	pthread_testcancel();	
	int j,sleeptme;
	unsigned long i;
	float k,l;
	float m;
	unsigned long track = num;
	if (num<=1000000) 
	{
		c = new customer*[num];
		for (i=0;i<(unsigned long)num;i++)
			c[i] = new customer();
	}
	struct timeval tm1,tm2,tm3,tm4,tm5,ret;
	struct timeval diff,rets,tm6,tm7,local,local1;	
	memset(&tm1, 0, sizeof(struct timeval));
	memset(&tm2, 0, sizeof(struct timeval));
	memset(&tm3, 0, sizeof(struct timeval));
	memset(&tm4, 0, sizeof(struct timeval));
	memset(&tm5, 0, sizeof(struct timeval));
	memset(&ret, 0, sizeof(struct timeval));
	memset(&diff, 0, sizeof(struct timeval));
	memset(&rets, 0, sizeof(struct timeval));
	memset(&tm6, 0, sizeof(struct timeval));
	memset(&tm7, 0, sizeof(struct timeval));	
	memset(&local, 0, sizeof(struct timeval));
	memset(&local1, 0, sizeof(struct timeval));	
	pthread_testcancel();
	for (i=0;i<track;i++)
	{
		if (num>1000000)
		{
			if (i==0 || i%1000000==0)
			{
				if (track!=0)
				{
					if (track>1000000)
					{
						allocate(1000000);						
						track-=1000000;
					}
					else
					{
						allocate(track);
						track = 0;
					}
				}
				else
					break;				
			}
		}
		gettimeofday(&tm1, NULL);		
		if (tflag==0) //the trace specification file is not specified
		{
			if (expdet == 0) //exponential process
				j = GetInterval(1,lambda,0,0,(unsigned long)custcount,tsfile); //obtain interarrival time
			else if (expdet == 1) //deterministic process
				j = GetInterval(0,lambda,0,0,(unsigned long)custcount,tsfile); //obtain interarrival time
		}
		else if (tflag == 1) //the trace specification file is specified
			j = GetInterval(1,lambda,1,0,(unsigned long)custcount,tsfile); //obtain interarrival time
		c[i]->cno = custcount;					
		gettimeofday(&tm2, NULL);		
		ret = subtracttime(tm1,tm2); //calculate the bookkeeping time
		sleeptme = (j*1000)-((int)((ret.tv_sec * 1000000)+ret.tv_usec)); //subtract the bookkeeping time from the interarrival time
		if (sleeptme<0)
			sleeptme = 0;	
		
		gettimeofday(&tm6, NULL);		
		usleep(sleeptme); //sleep for the amount of sleeptime
		gettimeofday(&tm7, NULL);	
		
		rets = subtracttime(tm6,tm7); //calculate the actual time for which the arrival thread slept
		l = (float) rets.tv_usec;
		l+=(float) ret.tv_usec;
		l/=1000;
		k=(float) (rets.tv_sec*1000)+l+(ret.tv_sec*1000);
		totintarrtme+=k; //keep track of the total interarrival time for all the customers
		pthread_testcancel();
		if (custcount == 0)		
		{			
			c[i]->arrtme.tv_sec = rets.tv_sec + ret.tv_sec; //store the arrival time for the first customer in its structure
			c[i]->arrtme.tv_usec = rets.tv_usec + ret.tv_usec;	
			if (c[i]->arrtme.tv_usec>=1000000)
			{				
				c[i]->arrtme.tv_sec+=c[i]->arrtme.tv_usec/1000000;
				c[i]->arrtme.tv_usec = c[i]->arrtme.tv_usec%1000000;				
			}
		}
		else
		{			
			c[i]->arrtme.tv_sec = c[i-1]->arrtme.tv_sec+rets.tv_sec+ret.tv_sec; //store the arrival time for the current customer(not first) in its structure
			c[i]->arrtme.tv_usec = c[i-1]->arrtme.tv_usec+rets.tv_usec+ret.tv_usec;
			if (c[i]->arrtme.tv_usec>=1000000)
			{				
				c[i]->arrtme.tv_sec+=c[i]->arrtme.tv_usec/1000000;
				c[i]->arrtme.tv_usec = c[i]->arrtme.tv_usec%1000000;				
			}
		}
		pthread_mutex_lock(&globaltimelock);
		if (i!=(unsigned long)num-1)
			c[i+1]->prevtime = tmm;
		if (i==0)
		{			
			tmm.tv_sec += (rets.tv_sec*1000)+(ret.tv_sec*1000); //update the global clock if the current customer is the first customer
			tmm.tv_usec += rets.tv_usec+ret.tv_usec;
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}
		}
		else
		{			
			//update the global clock if the current customer is not the first customer
			tmm.tv_sec = (c[i-1]->arrtme.tv_sec*1000)+(rets.tv_sec*1000)+(ret.tv_sec*1000);
			tmm.tv_usec = c[i-1]->arrtme.tv_usec+rets.tv_usec+ret.tv_usec;			
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}
		}
		local = tmm;
		memcpy(&local1,&local,sizeof(struct timeval));
		pthread_mutex_unlock(&globaltimelock);		
		//print the time at which the current customer arrived along with the interarrival time
		pthread_mutex_lock(&printlock);		
		printf("\n %08d.%03dms: ",(int)local.tv_sec,(int)local.tv_usec);
		cout<<"c"<<custcount+1<<" arrives, interarrival time = ";
		printf("%03.3fms",k);
		pthread_mutex_unlock(&printlock);		
		custgen++; //increment the number of customers generated by 1
		pthread_testcancel();
		gettimeofday(&tm4, NULL); //find the time at which the current customer is pushed into Q1/dropped		
		if (numinqueue < sz) //there is room in Q1 for the current customer
		{				
			pthread_mutex_lock(&queuelock);
			c[i]->qenttmetv = tm4; //store the Q1 entry time for the current customer as timeval structure
			q1->Append((void *)c[i]);
			numinqueue++;			//increment the number of customers in the queue
			pthread_mutex_lock(&calclock);
			queuesz = q1->ListSize()-1;	//find the number of customers in the queue before the current customer was added
			pthread_cond_signal(&queuetimecalc); //calculate the amount of time for which the above number of customers were in the Q1
			pthread_mutex_unlock(&calclock); 
			pthread_mutex_lock(&calclock);
			pthread_mutex_unlock(&calclock);			
			pthread_testcancel();
			gettimeofday(&tm5, NULL);			
			c[i]->qenttmetv = tm4;					
			ret = subtracttime(tm4,tm5); //calculate the time taken to push the customers into Q1	
			m = (float)ret.tv_usec;
			m/=1000;			
			c[i]->qenttme = (float) (ret.tv_sec*1000)+m; //update the Q1 entry time for current customer in milliseconds
			pthread_cond_signal(&waitforarrival); //signal a server
			pthread_mutex_unlock(&queuelock);
			pthread_mutex_lock(&globaltimelock);
			tmm.tv_sec = local.tv_sec+(ret.tv_sec*1000); //update the global clock
			tmm.tv_usec = local.tv_usec+ret.tv_usec;
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}
			local = tmm;
			c[i]->qentry = tmm;
			pthread_mutex_unlock(&globaltimelock);	
			//print the time at which the current customer was pushed into Q1
			pthread_mutex_lock(&printlock);	
			printf("\n %08d.%03dms: ",(int)local.tv_sec,(int)local.tv_usec);
			cout<<"c"<<custcount+1<<" enters Q1";
			pthread_mutex_unlock(&printlock);	
		}
		else //Q1 is full, current customer has to be dropped
		{
			gettimeofday(&tm5, NULL);			
			numdropped++;
			pthread_testcancel();
			ret = subtracttime(tm4,tm5); //calculate the time of dropping
			pthread_mutex_lock(&globaltimelock); //update the global clock
			tmm.tv_sec = local.tv_sec+(ret.tv_sec*1000);
			tmm.tv_usec = local.tv_usec+ret.tv_usec;
			if (tmm.tv_usec>=1000)
			{
				tmm.tv_sec+=(tmm.tv_usec/1000);
				tmm.tv_usec = tmm.tv_usec%1000;				
			}
			local = tmm;
			pthread_mutex_unlock(&globaltimelock);
			
			pthread_mutex_lock(&printlock);	//print the time at which the current customer was dropped
			printf("\n %08d.%03dms: ",(int)local.tv_sec,(int)local.tv_usec);
			cout<<"c"<<custcount+1<<" dropped";
			pthread_mutex_unlock(&printlock);			
		}		
		custcount++; //increment the customer count that serves at customer id
		if (i==(unsigned long)num-1) //all the customers have arrived...time for arrival thread to exit
		{
			shutdown = 1; //set the global variable indicating the servers can shutdown after serving the remaining customers
			pthread_mutex_lock(&queuelock);
			pthread_cond_signal(&waitforarrival); //signal the server
			pthread_mutex_unlock(&queuelock);
		}
		pthread_testcancel();
	}		
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) //the main function that parses the command line arguments and spawns the required threads
{	
	int lflag,mflag,sdflag,szflag,nflag,dflag,count;
	int i;
	int serverID[2];
	float avgarrtme,avgsvctme,rr;
	struct timeval r;
	pthread_t tid;
	void *status;	
	lflag=mflag=sdflag=szflag=nflag=dflag=count=0;
	float var = 0.0;
	float stddev = 0.0;
	float avgtimeinsys = 0.0;
	float tottimeinsysinsec = 0.0;
	float avgtimeinsysinsec = 0.0;	
	strcpy(sval,"n");
	tmm.tv_sec = 0;
	tmm.tv_usec = 0;
	serverID[0] = 1;
	serverID[1] = 2;
	memset(&tmm, 0, sizeof(struct timeval));
	memset(&tma, 0, sizeof(struct timeval));
	memset(&tmb, 0, sizeof(struct timeval));
	memset(&tmb, 0, sizeof(struct timeval));
	memset(&r, 0, sizeof(struct timeval));

	sigemptyset(&newset);
	sigaddset(&newset,SIGINT);
	pthread_sigmask(SIG_BLOCK,&newset,NULL);	
	pthread_create(&tid,NULL,siginthandler,NULL); //create the thread for handling SIGINT
	pthread_mutex_lock(&mutex);
	threadID[threadcount]=tid;
	threadcount++;					
	pthread_mutex_unlock(&mutex);
	pthread_create(&tid,NULL,calcqtime,NULL); //create the thread for calculating the average number of customers in Q1
	pthread_mutex_lock(&mutex);
	threadID[threadcount]=tid;
	threadcount++;					
	pthread_mutex_unlock(&mutex);
	
	//----code for checking the correctness of command line arguments and parse them-------------//
	if (strcmp(argv[argc-1],"-lambda")==0||strcmp(argv[argc-1],"-mu")==0||strcmp(argv[argc-1],"-seed")==0||strcmp(argv[argc-1],"-sargc-1ze")==0||strcmp(argv[argc-1],"-n")==0||strcmp(argv[argc-1],"-d")==0||strcmp(argv[argc-1],"-t")==0)
	{
		usage();		
	}
	for (i=1;i<argc;i++)
	{
		if ((chkarg(argv[i],strlen(argv[i]))==1)||(isnum(argv[i],strlen(argv[i]))))
		{		
			if (i!=argc-1)
			{
				if (chkarg(argv[i+1],strlen(argv[i+1]))==1||isnum(argv[i+1],strlen(argv[i+1])))
				{
					usage();									
				}
			}
		}
	}
	for (i=1;i<argc;i++)
	{
		if (strcmp(argv[i],"-lambda")!=0&&strcmp(argv[i],"-mu")!=0&&strcmp(argv[i],"-s")!=0&&strcmp(argv[i],"-seed")!=0&&strcmp(argv[i],"-size")!=0&&strcmp(argv[i],"-n")!=0&&strcmp(argv[i],"-d")!=0&&strcmp(argv[i],"-t")!=0)
		{
			if (strcmp(argv[i],"exp")!=0&&strcmp(argv[i],"det")!=0&&strcmp(argv[i-1],"-t")!=0)
			{
				if ((chkarg(argv[i],strlen(argv[i]))==0)&&(!isnum(argv[i],strlen(argv[i]))))
				{
					usage();										
				}
			}
		}
	}
	for (i=1;i<argc;i++)
	{		
		if (count==8)
			break;
		if ((strcmp(argv[i],"-lambda")==0)&&(lflag==0))
		{
			if (chkarg(argv[i+1],strlen(argv[i+1]))==0)
			{
				usage();
			}
			lambda = getfloat(argv[i+1],strlen(argv[i+1]));			
			i=0;
			lflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-mu")==0)&&(mflag==0))
		{
			if (chkarg(argv[i+1],strlen(argv[i+1]))==0)
			{				
				usage();
			}
			mu = getfloat(argv[i+1],strlen(argv[i+1]));
			i=0;
			mflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-s")==0)&&(sflag==0))
		{
			if (count<7&&i!=argc-1)
			{
				if (chkarg(argv[i+1],strlen(argv[i+1]))==1)
				{
					usage();
				}
				if (isnum(argv[i+1],strlen(argv[i+1])))
				{
					usage();
				}
			}
			strcpy(sval,"y");
			i=0;
			sflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-seed")==0)&&(sdflag==0))
		{
			if (!isnum(argv[i+1],strlen(argv[i+1])))
			{
				usage();
			}
			seedval = getnum(argv[i+1],strlen(argv[i+1]));
			i=0;
			sdflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-size")==0)&&(szflag==0))
		{
			if (!isnum(argv[i+1],strlen(argv[i+1])))
			{
				usage();
			}
			sz = getnum(argv[i+1],strlen(argv[i+1]));
			i=0;
			szflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-n")==0)&&(nflag==0))
		{
			if (!isnum(argv[i+1],strlen(argv[i+1])))
				usage();
			num = getnum(argv[i+1],strlen(argv[i+1]));
			i=0;
			nflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-d")==0)&&(dflag==0))
		{
			if (strcmp(argv[i+1],"exp")==0&&strcmp(argv[i+1],"det")==0)
			{
				usage();
			}
			if (strcmp(argv[i+1],"exp")==0) 
				expdet = 0;
			else if (strcmp(argv[i+1],"det")==0)
				expdet = 1;
			i=0;
			dflag=1;
			count++;
			continue;
		}
		if ((strcmp(argv[i],"-t")==0)&&(tflag==0))
		{
			tsfile = (char *)malloc(strlen(argv[i+1]));
			strcpy(tsfile,argv[i+1]);
			i=0;
			tflag=1;
			count++;
			continue;
		}
	}	

	if (tflag==1)
	{
		if ((fp = fopen(tsfile,"r")) == NULL)
		{
			cout<<"\n Couldn't open the file"<<tsfile<<" for reading \n\n";
			exit(1);
		}
		fscanf(fp,"%d",&num);
		fclose(fp);
	}

	InitRandom(seedval);

	cout<<"\n Parameters: "; //print the command line parameters
	if (tflag == 0)
	{
		cout<<"\n\t Lambda       = "<<lambda;
		cout<<"\n\t mu           = "<<mu;
		if (sflag == 1)
			cout<<"\n\t system       = M/M/1 ";
		else
			cout<<"\n\t system       = M/M/2 ";
		cout<<"\n\t seed         = "<<seedval;
		cout<<"\n\t size         = "<<sz;
		cout<<"\n\t number       = "<<num;
		cout<<"\n\t distribution = ";
		if (expdet == 0)
			cout<<"exp";
		else
			cout<<"det";
	}
	if (tflag == 1) //trace specification file is specified
	{
		if (sflag == 1)
			cout<<"\n\t system = M/M/1";
		else
			cout<<"\n\t system = M/M/2";
		cout<<"\n\t size   = "<<sz;
		cout<<"\n\t number = "<<num;
		cout<<"\n\t tsfile = "<<tsfile<<endl;
	}
	else
		cout<<endl;
	cout<<"\n 00000000.000ms: emulation begins";
	cout<<flush;
	
	if (sflag==1) //number of servers is 1
	{
		pthread_create(&tid,NULL,server,&serverID[0]); //create the server thread
		pthread_mutex_lock(&mutex);
		threadID[threadcount]=tid;
		threadcount++;					
		pthread_mutex_unlock(&mutex);
	}
	else //number of servers is 2
	{
		pthread_create(&tid,NULL,server,&serverID[0]); //create the first server thread
		pthread_mutex_lock(&mutex);
		threadID[threadcount]=tid;
		threadcount++;							
		pthread_create(&tid,NULL,server,&serverID[1]); //create the second server thread
		threadID[threadcount]=tid;
		threadcount++;							
		pthread_mutex_unlock(&mutex);
	}
	pthread_create(&tid,NULL,arrival,NULL);  //create the arrival thread
	pthread_mutex_lock(&mutex);
	threadID[threadcount]=tid;
	threadcount++;					
	pthread_mutex_unlock(&mutex);
	
	for (int jn=0;jn<threadcount;jn++)	//calls a join on all the threads that are alive
		pthread_join(threadID[jn],&status);
	
	dropprob = (float) numdropped/(custgen-numthrown); //drop probability
	avgarrtme = totintarrtme/custgen; //average arrival time in milliseconds
	avgsvctme = totservicetme/custserv;	//average service time in milliseconds	
	avgtimeinsys = (float) (tottimeinsys/custserv); //average time in system in millisecondss
	tottimeinsysinsec = (float) (tottimeinsys/1000); //total time in system in seconds
	avgtimeinsysinsec = (float) (tottimeinsysinsec/custserv); //average time in system in seconds
	r = subtracttime(mt1,mt2); //total time for which the system was on
	rr = (float) r.tv_usec;
	rr/=1000000;
	rr+=(float) r.tv_sec;
	avgcustq1 = (float) (calcqtme/rr); //average number of customers in Q1		
	var = (float) (fabs((tottimeinsyssq/custserv)-pow(avgtimeinsysinsec,2))); //variance
	stddev = (float) (sqrt(var)); //standard deviation for the time spent in the system
	avgarrtme/=1000; //average arrival time in seconds
	avgsvctme/=1000; //average service time in seconds
	avgtimeinsys/=1000; //average time in system in seconds

	//--------------------------Print the statistics-------------------------//
	cout<<"\n Statistics: \n";
	if (custgen != 0)
	{
		cout<<"\n\t average interarrival time = ";
		printf("%0.6fsec",avgarrtme);
	}
	else
		cout<<"\n\t average interarrival time = 0.000000sec";
	if (custserv != 0)
	{
		cout<<"\n\t average service time = ";
		printf("%0.6fsec",avgsvctme);
	}
	else
		cout<<"\n\t average service time = 0.000000sec";
	cout<<"\n";
	if (rr!=0)
	{
		cout<<"\n\t average number of customers in q1 = ";
		printf("%0.6f",avgcustq1);
	}
	else
		cout<<"\n\t average number of customers in q1 = 0.000000";
	if (sflag == 1)
	{	
		if (qa!=0.0)
		{
			avgcusts1 = pa/qa;
			cout<<"\n\t average number of customers in s1 = ";
			printf("%0.6f",avgcusts1);
		}
		else
		{
			cout<<"\n\t average number of customers in s1 = 0.000000";
		}
	}
	else
	{
		if (qa!=0.0)
		{
			avgcusts1 = pa/qa;
			cout<<"\n\t average number of customers in s1 = ";
			printf("%0.6f",avgcusts1);
		}
		else
		{
			cout<<"\n\t average number of customers in s1 = 0.000000";
		}
		if (qb!=0)
		{
			avgcusts2 = pb/qb;
			cout<<"\n\t average number of customers in s2 = ";
			printf("%0.6f",avgcusts2);
		}
		else
		{
			cout<<"\n\t average number of customers in s2 = 0.000000";
		}
	}
	cout<<"\n";
	if (custserv != 0&&var>0)
	{		
		cout<<"\n\t average time spent in system = ";
		printf("\%0.6fsec",avgtimeinsys);
		cout<<"\n\t standard deviation for time spent in system = ";
		printf("\%0.6f",stddev);
		cout<<"\n";
	}
	else
	{
		cout<<"\n\t average time spent in system = 0.000000sec";
		cout<<"\n\t standard deviation for time spent in system = 0.000000sec";
	}
	if (custgen !=0 )
	{
		cout<<"\n\t customer drop probability = ";
		printf("%0.6f",dropprob);
	}
	else
		cout<<"\n\t customer drop probability = 0.000000";
	cout<<"\n\n";
	return 0;
}
