#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace std;

//structure that stores all the relevant details of the customer.
//This is pushed and popped from the queue.
struct customer 
{
	int cno;
	timeval qenttmetv;
	timeval qentry;
	timeval prevtime;
	timeval arrtme;	
	float qenttme;
	public:
	customer()
	{
		cno=-1;
		memset(&qenttmetv, 0, sizeof(struct timeval));
		memset(&qentry, 0, sizeof(struct timeval));
		memset(&prevtime, 0, sizeof(struct timeval));
		memset(&arrtme, 0, sizeof(struct timeval));
	}
};

int isnum(char abc[100],int len); //input a string and check if it contains only an integer

int getnum(char pqr[100],int len); //input a string and return the integer in the string

float getfloat(char pqr[100],int len); //input a string and return the floating point number in the string

int chkarg(char pqr[100],int len); //checks if the command line argument is correct

void usage(); //function to print the correct usage of command line arguments

void InitRandom(long l_seed); //Initializes the random number generator with the seed

int GetInterval(int exponential, double rate, int tflag, int from, unsigned long loop, char *tsfile); //returns the interarrival/service time for customers

timeval subtracttime(timeval tm1,timeval tm2); //returns the timedifference between two timeval variables
