#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <pthread.h>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <iterator>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <dirent.h>
#include <queue>
#include <map>
#include <fstream>
#include <errno.h>
#include <ctype.h>
#include "iniparser.h"
#include "strlib.h"
#include "dictionary.h"
using namespace std;

#define min(A,B) (((A)>(B)) ? (B) : (A))
#define NEIGHBOR_FILE "init_neighbor_list"
#define KWRD_FILE "kwrd_index"
#define CACHE_FILE "cache"
#define NAME_INDEX "name_index"
#define SHA_INDEX "sha_index"
#define FILE_ID	"file_id"

//All messages are stored in this struct
struct MsgDetails
{
	MsgDetails()
	{
		messageType = 0;
		memset(UOID, '\0', sizeof UOID);
		TTL = 0;
		rsrv = 0;
		dataLength = 0;
		data = NULL;
		pSock = 0;
		kind = 0;
		snof = -1;
		nodeID = NULL;
	 };

	uint8_t messageType;
	unsigned char UOID[20];
	uint8_t TTL;
	uint8_t rsrv;
	int dataLength;
	char *data;
	int pSock;
	short kind;
	char *nodeID;
	int snof;
};

//Data for Sender and Receiver (dumb threads)
struct connData
{
	connData()
	{
		sendBuf = NULL;
		peerNode = NULL;
		dataLen = 0;
		sockDesc = 0;
		sendFlag = NULL;
		keepAlive = 0;
		peerContact = 0;
		gotHello = 0;
		snof = -1;
	}

	unsigned char *sendBuf;
	char *peerNode;
	int dataLen;
	int sockDesc;
	int *sendFlag;
	int keepAlive;
	int peerContact;
	short gotHello;
	int snof;
};

//Store connection about connections
struct peerInfo
{
	peerInfo()
	{
		nodeInfo = NULL;
		peerSock = 0;
		myConn = 0;
		forKeepAlive = NULL;
	}
	char *nodeInfo;
	int peerSock;
	int myConn;
	struct connData *forKeepAlive;
};

//Message cache struct
struct timerSt
{
	timerSt()
	{
		memset(msgUOID, '\0', sizeof msgUOID);
		timeout = 0;
		senderNode = NULL;
	}
	char msgUOID[20];
	short timeout;
	char* senderNode;
};

struct bitVect
{
	bitVect()
	{
		memset(vArr, '\0', sizeof vArr);
		memset(fName, '\0', sizeof fName);
	}
	char vArr[257];
	char fName[256];
};

struct cacheStruct
{
	cacheStruct()
	{
		fileSz = 0;
		memset(fileName, '\0', sizeof fileName);
	}
	int fileSz;
	char fileName[256];
};

struct searchData
{
	searchData()
	{
		memset(fileName, '\0', sizeof fileName);
		memset(sha_val, '\0', sizeof sha_val);
		memset(realName, '\0', sizeof realName);
	}
	char fileName[256];
	char sha_val[40];
	char realName[256];
};

struct treeNode
{
	struct treeNode *left;
	struct treeNode *right;
	struct searchData data;
};

struct getFile
{
	getFile()
	{
		memset(fileName, '\0', 256);
		memset(fileID, '\0', 20);
	}

	char fileName[256];
	char fileID[20];
};

//Global parmater class
class nodeParam
{
	public:
	int port;
	double location;
	char *homeDir;
	char *logFile;
	int autoShutDown;
	int ttl;
	int noCheck;
	int msgLifeTime;
	int getMsgLifeTime;
	int initNeighbors;
	int joinTimeOut;
	int keepAliveTimeOut;
	int minNeighbors;
	float cacheProb;
	float storeProb;
	float neighborStoreProb;
	int cacheSize;
	int permSize;
	int retry;
	char *nodeInstance;
	char *hostNme;
	uint8_t *intArray;

	nodeParam()
	{
		autoShutDown = 900;
		ttl = 30;
		msgLifeTime = 30;
		getMsgLifeTime = 300;
		initNeighbors = 3;
		keepAliveTimeOut = 60;
		minNeighbors = 2;
		noCheck = 0;
		cacheProb = 0.1;
		storeProb = 0.1;
		neighborStoreProb = 0.2;
		cacheSize = 500;
		retry = 15;
	}
};

extern const uint8_t JNRQ;
extern const uint8_t JNRS;
extern const uint8_t HLLO;
extern const uint8_t KPAV;
extern const uint8_t NTFY;
extern const uint8_t CKRQ;
extern const uint8_t CKRS;
extern const uint8_t SHRQ;
extern const uint8_t SHRS;
extern const uint8_t GTRQ;
extern const uint8_t GTRS;
extern const uint8_t STOR;
extern const uint8_t DELT;
extern const uint8_t STRQ;
extern const uint8_t STRS;
extern const uint8_t type1;
extern const uint8_t type2;
extern const uint8_t type3;

extern char *logFile;
extern nodeParam node;
extern short IAMBEACON;
extern short terminateNode;
extern std::vector<char *> beaconList;
extern pthread_cond_t getNextCmd;
extern short userInitiatedCmd;
extern pthread_mutex_t thrIDLock;
extern std::vector<pthread_t> threadID;
extern pthread_mutex_t processQLock;
extern pthread_mutex_t logLock;
extern pthread_mutex_t statLock;
extern pthread_mutex_t statusRqsLock;
extern pthread_mutex_t cmdPromptLock;
extern pthread_mutex_t restart;
extern pthread_mutex_t helloLock;
extern pthread_mutex_t bitVStoreLock;
extern pthread_mutex_t cacheLock;
extern pthread_mutex_t nodeLock;
extern pthread_mutex_t fileIdLock;
extern pthread_mutex_t treeLock;
extern pthread_mutex_t fileLock;

extern pthread_cond_t cmdPromptWait;
extern pthread_cond_t processQWait;
extern pthread_cond_t forLogging;
extern std::queue<struct MsgDetails> processQ;
extern std::queue<struct MsgDetails> logQueue;

extern std::vector<struct connData> sendQ;
extern short headerLen;
extern pthread_mutex_t sendQLock ;
extern const int CHUNK;
extern pthread_mutex_t testPrinter;
extern int SLEEPER;
extern std::vector<struct peerInfo> existConn;
extern pthread_mutex_t existConnLock;
extern pthread_mutex_t timerLock;
extern short MAXBUFFERSIZE;
extern char myJoinUOID[20];
extern std::vector<struct timerSt> cacheUOID;
extern std::vector<struct MsgDetails> neighborNodes;
extern pthread_cond_t joinWake;
extern pthread_mutex_t joinPrcsSig;
extern pthread_mutex_t sendFlagLock;
extern char** myNeighbors;
extern bool timeToLog;
extern char myStatUOID[20];
extern char *statFile;
extern std::vector<int> stPorts;
extern char *myHome;
extern multimap<short, short> namLinks;
extern bool waitingForCheck;
extern int statusTimeOut;
extern char *HOME_DIR;
extern char *LOG_FILE;
extern short helloNo;
extern short exitID;
extern short terminateLogger;
extern bool softRestart;
extern bool shutDown;
extern int GLOBALTIMEOUT;
extern char myChkUOID[20];
extern char mySrcUOID[20];
extern char myGetUOID[20];
extern bool waitForJoin;
extern vector<struct bitVect>bitVStorage;
extern vector<struct cacheStruct>fileCache;
extern vector<struct getFile>fileIdStore;
extern char* keywordFile;
extern int noOfFiles;
extern char *myHomeFiles;
extern char* cacheFile;
extern int currCacheSize;
extern struct treeNode* shaTree ;
extern struct treeNode* nameTree ;
extern int mySrcResult;
extern bool allowGetCmd;
extern char getFileName[20];
extern char** getFileId;
extern char** getFileSha;
extern char** defGetFileName;
extern bool statusFiles;
extern char *statOp;
extern int fileCount;

extern void* processData(void *arg);
extern void* permConnReader(void *arg);
extern void* permConnSender(void *arg);
extern void* infoLogger(void *arg);

extern short connExists(char *);
extern char* GetUOID(char *, char *, char *, int);
extern void joinRsp(struct MsgDetails);
extern void sendHello(struct MsgDetails, char*);
extern void sendNotify(string, uint8_t);
extern uint8_t setTTL(uint8_t);
extern struct connData *sendKeepAlive(struct MsgDetails);
extern void parseMetaData(char *, char *, char*, char*, char*,
							char*, int*);
extern void insertInTree(struct searchData);
extern struct treeNode* searchTree(struct treeNode *, struct searchData, short);
void returnTree(struct treeNode *, short*, char ***);
extern void generateBitVector(char [], char **, int);
extern bool operator& (nodeParam &, uint8_t[]);
extern uint8_t* generateIntFromHex(char []);
extern uint8_t returnDecValue(char []);
extern unsigned char* convToHex(unsigned char *, int);

#endif
