#include "post.h"
#include "list.h"
#include <sys/time.h>
#include <sys/types.h> 

//define the maximum number of locks or conditions that the server can handle
#define MAX 200
#define CLIENTREQ 0
#define SERVERREQ 1
#define SERVERACK 2
#define SERVERHBT 3
#define SERVERPNG 4

// ---------------------------------------------------------------------------------------------------------------------
// This class is used as the lock datatype by the server. It contains fields that specify whether a lock exists or not, 
// whether or not the lock is busy, the name of the lock, owner of the lock etc. For multiple locks, we use an array
// of these objects.
// ---------------------------------------------------------------------------------------------------------------------
class NetLocksClass
{
  public:
	char name[40];
	bool exists;
	bool isBusy;
	int ownerMachine;
	List *replyQueue;
	NetLocksClass();
};
// --------------------------------------------------------------------------------------------------------------------------
//	This class is similar to the NetLocksClass and is used to represent conditions. Again, an array of the objects of this
//	class is used for muliple conditions.
// -----------------------------------------------------------------------------------------------------------------------
class NetCondsClass
{
  public:
	char name[40];
	bool exists;
	bool isBusy;
	int lockID;
	List *replyQueue;
	int queueCount;
	NetCondsClass();
};



class sharedIntsClass
{
  public:
	char name[40];
	int *value;
	int length;
	bool exists;
	sharedIntsClass();
};
/*
class timeCounterClass
{
	public:
		int t1;
		int t2;
	timeCounterClass();
	timeCounterClass(int);
	timeCounterClass(int, int);
};

class waitingQueueMember
{
	public:
		char packet[MaxMailSize];
		timeCounterClass time;
		int whichServer; //To break ties!
		waitingQueueMember(char *, timeCounterClass, int);
};
*/
void appendNumber(char *, int);
int getNumber(char *);
void putData(char *, int, int);

void formPacket(char *packet, int syscallType, int parameter1, int parameter2, char *msg);
void formPacket(char *packet, int mType, int syscallType, int machineID, int mailBoxID, int parameter1, int parameter2, int parameter3, timeCounterClass *time,  char *msg);


void getData(char *data, int *i, int applyAlgo);

void parsePacket(char *packet, int *mType, int *syscallType, int *machineID, int *mailBoxID, int *parameter1, int *parameter2, int *parameter3, timeCounterClass *time,  char *msg);
void parsePacket(char *packet, int *syscallType, int *parameter1, int *parameter2, int *parameter3, char *msg);

int makeOwnerID(int fromMachine, int fromBox);
extern timeCounterClass startTime;
long getTime();
timeCounterClass findDiffTime(timeCounterClass, timeCounterClass);
timeCounterClass getTimeStamp();
//int isTimeGreater(timeCounterClass, timeCounterClass);
char *formreply(char *, int, int);
