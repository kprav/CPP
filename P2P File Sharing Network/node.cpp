#include "node.h"

short terminateNode = 0;
short headerLen = 27;
int myPort = 0;
short userInitiatedCmd = 0;
char* keywordFile;
char *myHomeFiles;
char* cacheFile;
char* metaFile;
char* passFile;
char *fileIdFile;
int noOfFiles = 0;
int currCacheSize =0;
int mySrcResult = 1;
int searchTimeOut;
bool allowGetCmd = false;
char *statOp;
vector<struct bitVect>bitVStorage;
vector<struct getFile>fileIdStore;

pthread_cond_t getNextCmd = PTHREAD_COND_INITIALIZER;
pthread_cond_t forLogging = PTHREAD_COND_INITIALIZER;
pthread_cond_t processQWait = PTHREAD_COND_INITIALIZER;
pthread_cond_t joinWake = PTHREAD_COND_INITIALIZER;
pthread_cond_t cmdPromptWait = PTHREAD_COND_INITIALIZER;

pthread_mutex_t sendQLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thrIDLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t processQLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t testPrinter = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timerLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t joinPrcsSig = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sendFlagLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statusRqsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cmdPromptLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t restart = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t helloLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bitVStoreLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cacheLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nodeLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fileIdLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t treeLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fileLock = PTHREAD_MUTEX_INITIALIZER;

struct hostent* bconIP = NULL;

int GLOBALTIMEOUT;
bool timeToLog = false;
bool softRestart = false;

std::vector<pthread_t> threadID;
int noOfBeacons = 0;
short IAMBEACON = 0;
std::vector<char *> beaconList;
std::queue<struct MsgDetails> processQ;
std::queue<struct MsgDetails> logQueue;
std::vector<int> stPorts;

nodeParam node;
sigset_t sigCatch;

std::vector<struct timerSt> cacheUOID;
char myStatUOID[20];
char mySrcUOID[20];
char getFileName[20];
char myGetUOID[20];
char *myHome;
char *statFile;
char *HOME_DIR;
char *LOG_FILE = NULL;

short MAXBUFFERSIZE = 8192;
std::vector<struct connData> sendQ;
int SLEEPER = 300000;
const int CHUNK = 8192;
std::vector<struct peerInfo> existConn;
pthread_mutex_t existConnLock = PTHREAD_MUTEX_INITIALIZER;
std::vector<struct MsgDetails> neighborNodes;
char myJoinUOID[20];
char **myNeighbors;
char* neighborFile;
char* logFile;
char* statRequester;
bool waitingForCheck = false;
int statusTimeOut;
short helloNo = 0;
short exitID;
short gotInt = 0;
short terminateLogger = 0;
bool waitForJoin = true;
multimap<short, short> namLinks;
pthread_t acceptThread;
bool shutDown = false;
pthread_t sigHandleID;
vector<struct cacheStruct>fileCache;
struct treeNode *nameTree = NULL;
struct treeNode *shaTree = NULL;
char** getFileId = NULL;
char** getFileSha = NULL;
char** defGetFileName = NULL;

char myChkUOID[20];
char *nameIndex;
char *shaIndex;
bool statusFiles = false;
int fileCount = 1;

void earlyExit()
{
	exit(0);
}

//Randomize - Initial Seed
void InitRandom(long l_seed)
{
	if (l_seed == 0L) {
		time_t localtime=(time_t)0;

		time(&localtime);
		srand48((long)localtime);
	} else {
		srand48(l_seed);
	}
}


/*******************************************************************************
Function to Parse the metafile - Using Copyright (c) 2000 by Nicolas Devillard.
*******************************************************************************/
void parseMetaData(char *iniFile, char* bArr, char *oName, char *shaArr, char *nonce,
					char *kWord, int* size)
{
	dictionary *dict;
	char *tHold;
	char *temp = (char *)malloc(256);
	dict = iniparser_load(iniFile);

	if(dict == NULL)
		return;

	tHold = (char *)malloc(strlen("metadata:Bit-Vector")+1);
	memset(temp, '\0', 256);
	strcpy(tHold, "metadata:Bit-Vector");
	tHold[strlen("metadata:Bit-Vector")] = '\0';
	temp = iniparser_getstr(dict, tHold);
	free(tHold);

	if(bArr != NULL)
		memcpy(bArr, temp, strlen(temp));
	free(temp);

	temp = (char *)malloc(40);
	memset(temp, '\0', 40);
	tHold = (char *)malloc(strlen("metadata:SHA1")+1);
	strcpy(tHold, "metadata:SHA1");
	tHold[strlen("metadata:SHA1")] = '\0';
	temp = iniparser_getstr(dict, tHold);
	free(tHold);

	if(shaArr != NULL)
		memcpy(shaArr, temp, 40);
	free(temp);

	temp = (char *)malloc(256);
	memset(temp, '\0', 256);
	tHold = (char *)malloc(strlen("metadata:FileName")+1);
	strcpy(tHold, "metadata:FileName");
	tHold[strlen("metadata:FileName")] = '\0';
	temp = iniparser_getstr(dict, tHold);
	free(tHold);

	if(oName != NULL)
	{
		memcpy(oName, temp, strlen(temp));
		oName[strlen(temp)] = '\0';
	}
	free(temp);

	temp = (char *)malloc(40);
	memset(temp, '\0', 40);
	tHold = (char *)malloc(strlen("metadata:Nonce")+1);
	strcpy(tHold, "metadata:Nonce");
	tHold[strlen("metadata:Nonce")] = '\0';
	temp = iniparser_getstr(dict, tHold);
	free(tHold);

	if(nonce != NULL)
		memcpy(nonce, temp, 40);
	free(temp);

	temp = (char *)malloc(256);
	memset(temp, '\0', 256);
	tHold = (char *)malloc(strlen("metadata:Keywords")+1);
	strcpy(tHold, "metadata:Keywords");
	tHold[strlen("metadata:Keywords")] = '\0';
	temp = iniparser_getstr(dict, tHold);
	free(tHold);

	if(kWord != NULL)
	{
		memcpy(kWord, temp, strlen(temp));
		kWord[strlen(temp)] = '\0';
	}
	free(temp);

	tHold = (char *)malloc(strlen("metadata:FileSize")+1);
	strcpy(tHold, "metadata:FileSize");
	tHold[strlen("metadata:FileSize")] = '\0';
	if(size != NULL)
		*size = iniparser_getint(dict, tHold, -1);
	free(tHold);

	free(dict);
}
/**************************************************************
Function to parse ini File.
used Ini parser created by
Copyright (c) 2000 by Nicolas Devillard.
**************************************************************/
void parseIni(char *iniFile)
{
	dictionary *dict;
	char *tHold;
	dict = iniparser_load(iniFile);

	if (dict == NULL)
	{
		cout<<"\n IniFile Parsing Failed" <<endl;
		earlyExit();
	}

	tHold = (char *)malloc(strlen("init:Port")+1);
	strcpy(tHold, "init:Port");
	tHold[strlen("init:Port")]='\0';
	node.port = iniparser_getint(dict, tHold,-1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:Location")+1);
	strcpy(tHold, "init:Location");
	tHold[strlen("init:Location")]='\0';
	node.location = iniparser_getdouble(dict, tHold,-1.0);
	free(tHold);

	tHold = (char *)malloc(strlen("init:HomeDir")+1);
	memset(tHold, '\0', strlen("init:HomeDir"));
	strncpy(tHold, "init:HomeDir", strlen("init:HomeDir"));
	tHold[strlen("init:HomeDir")]='\0';
	node.homeDir = (char *)malloc(strlen(iniparser_getstr(dict, tHold))+1);
	node.homeDir = iniparser_getstr(dict, tHold);

	HOME_DIR = (char *)malloc(strlen(iniparser_getstr(dict, tHold))+1);
	strncpy(HOME_DIR, node.homeDir, strlen(iniparser_getstr(dict, tHold)));
	HOME_DIR[strlen(iniparser_getstr(dict, tHold))]='\0';
	free(tHold);

	tHold = (char *)malloc(strlen("init:LogFilename")+1);
	strcpy(tHold, "init:LogFilename");
	tHold[strlen("init:LogFilename")]='\0';
	if (iniparser_getstr(dict, tHold)!=NULL)
	{

		node.logFile = (char *)malloc(strlen(iniparser_getstr(dict, tHold))+1);
		node.logFile = iniparser_getstr(dict, tHold);
		LOG_FILE = (char *)malloc(strlen(iniparser_getstr(dict, tHold))+1);
		strncpy(LOG_FILE, node.logFile, strlen(iniparser_getstr(dict, tHold)));
		LOG_FILE[strlen("strlen(iniparser_getstr(dict, tHold))")] = '\0';
	}
	else
	{
		LOG_FILE = (char *)malloc(strlen("servant.log")+1);
		strncpy(LOG_FILE, "servant.log", strlen("servant.log"));
		LOG_FILE[strlen("servant.log")] = '\0';
	}
	free(tHold);

	tHold = (char *)malloc(strlen("init:TTL")+1);
	strcpy(tHold, "init:TTL");
	tHold[strlen("init:TTL")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.ttl = iniparser_getint(dict,tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:InitNeighbors")+1);
	strcpy(tHold, "init:InitNeighbors");
	tHold[strlen("init:InitNeighbors")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.initNeighbors = iniparser_getint(dict,tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:MinNeighbors")+1);
	strcpy(tHold, "init:MinNeighbors");
	tHold[strlen("init:MinNeighbors")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.minNeighbors = iniparser_getint(dict,tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:MsgLifetime")+1);
	strcpy(tHold, "init:MsgLifetime");
	tHold[strlen("init:MsgLifetime")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.msgLifeTime = iniparser_getint(dict, tHold, -1);

	free(tHold);

	tHold = (char *)malloc(strlen("init:keepAliveTimeout")+1);
	strcpy(tHold, "init:keepAliveTimeout");
	tHold[strlen("init:keepAliveTimeout")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.keepAliveTimeOut = iniparser_getint(dict,tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("beacons:Retry")+1);
	strcpy(tHold, "beacons:Retry");
	tHold[strlen("beacons:Retry")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.retry = iniparser_getint(dict, tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:AutoShutdown")+1);
	strcpy(tHold, "init:AutoShutdown");
	tHold[strlen("init:AutoShutdown")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.autoShutDown = iniparser_getint(dict,tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:NoCheck")+1);
	strcpy(tHold, "init:NoCheck");
	tHold[strlen("init:NoCheck")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.noCheck = iniparser_getint(dict,tHold, -1);
	free(tHold);

	tHold = (char *)malloc(strlen("init:NeighborStoreProb")+1);
	strcpy(tHold, "init:NeighborStoreProb");
	tHold[strlen("init:NeighborStoreProb")]='\0';
	if(iniparser_getdouble(dict, tHold, -1) != -1.0)
		node.neighborStoreProb = iniparser_getdouble(dict, tHold,-1.0);
	free(tHold);


	tHold = (char *)malloc(strlen("init:CacheSize")+1);
	strcpy(tHold, "init:CacheSize");
	tHold[strlen("init:CacheSize")]='\0';
	if(iniparser_getint(dict, tHold, -1) != -1)
		node.cacheSize = iniparser_getint(dict, tHold,-1);
	node.cacheSize = node.cacheSize * 1024;
	free(tHold);

	tHold = (char *)malloc(strlen("init:StoreProb")+1);
	strcpy(tHold, "init:StoreProb");
	tHold[strlen("init:StoreProb")]='\0';
	if(iniparser_getdouble(dict, tHold, -1) != -1.0)
		node.storeProb = iniparser_getdouble(dict, tHold,-1.0);
	free(tHold);

	tHold = (char *)malloc(strlen("init:CacheProb")+1);
	strcpy(tHold, "init:CacheProb");
	tHold[strlen("init:CacheProb")]='\0';
	if(iniparser_getdouble(dict, tHold, -1) != -1.0)
		node.cacheProb = iniparser_getdouble(dict, tHold,-1.0);
	free(tHold);

	GLOBALTIMEOUT = node.autoShutDown;

	char* secname = (char *)malloc(strlen("beacons")+1);
	strcpy(secname, "beacons");
	secname[strlen("beacons")]='\0';
	char *tmp;

	for (int j=0 ; j<dict->size ; j++) {
		if (dict->key[j]==NULL)
			continue ;

		string str(dict->key[j]);
		size_t fPos = str.find_first_of(":");

		if(fPos == string::npos)
		 continue;

		if(!strcmp((str.substr(0, fPos)).c_str(), secname) )
		{
			string str2((str.substr(fPos+1, str.size())).c_str());
			size_t fPos2 = str2.find_first_of(":");

			if(fPos2 == string::npos)
		 		continue;

			tmp = (char *)malloc(str.size() - fPos);
			memcpy(tmp, (str.substr(fPos+1, str.size())).c_str(), str.size() - fPos);
			beaconList.push_back(tmp);
			noOfBeacons++;
		}
	}
}

/**************************************************************
Print SHA Tree or Name Tree to File
***************************************************************/
void printTree(struct treeNode *root, short flag, ofstream &fp)
{
	char chkData[256];
	memset(chkData, '\0', sizeof chkData);

	if(!fp.is_open() || root == NULL)
	{
		return;
	}

	struct treeNode* curr = root;

	if(!strncmp(curr->data.fileName, chkData, sizeof chkData))
	{
	}
	else
	{
		fp.write(reinterpret_cast<char *>(&curr->data), sizeof curr->data);
	}	

	printTree(curr->left, flag, fp);
	printTree(curr->right, flag, fp);
}

/****************************************************************
Returns all the data files that belongs to the current node
****************************************************************/
void returnTree(struct treeNode *root, short* flag, char*** files)
{	
	char *chkData = new char[256];
	memset(chkData, '\0', 256);

	if(root == NULL)
	{
		delete[] chkData;
		return;
	}

	struct treeNode* curr = root;
	
	if(*files == NULL)
	{
		*files = (char **)malloc(sizeof(char *)*(*flag+1));

	}
	else
	{
		char **tempHol = *files;
		*files = (char **)malloc(sizeof(char *)*(*flag+1));
		memcpy(*files, tempHol, sizeof(char *)*(*flag));		
	}


		if(!strncmp(curr->data.fileName, chkData, sizeof curr->data.fileName))
		{
		}
		else
		{
			if( *flag < 1 )
			{
				(*files)[*flag] = (char *)malloc(strlen(curr->data.realName)+1);
				strncpy((*files)[*flag], curr->data.realName, strlen(curr->data.realName));
				((*files)[*flag])[strlen(curr->data.realName)] = '\0';

				*flag = *flag + 1;
			}
			else
			{
				if(!strncmp((*files)[*flag-1], curr->data.realName, strlen((*files)[*flag-1])))
				{
				}
				else
				{
					(*files)[*flag] = (char *)malloc(strlen(curr->data.realName)+1);
					strncpy((*files)[*flag], curr->data.realName, strlen(curr->data.realName));
					((*files)[*flag])[strlen(curr->data.realName)] = '\0';

					*flag = *flag + 1;
				}
			}
		}

	returnTree(curr->left, flag, files);
	returnTree(curr->right, flag, files);
	delete[] chkData;
}

/****************************************************************
Function to insert a node into a binary tree.
Made use of code from:
http://www.cplusplus.happycodings.com/Algorithms/code5.html
****************************************************************/
void insertInTree(struct searchData node)
{
	for(int j=0; node.fileName[j] != '\0' ; j++)
		node.fileName[j] = tolower(node.fileName[j]);

	struct treeNode* newNode = (struct treeNode *)malloc(sizeof(struct treeNode));	
	newNode->data = node;
	newNode->right = NULL;
	newNode->left = NULL;

	if(nameTree == NULL)
	{
		nameTree = newNode;
	}
	else
	{
		treeNode *curr = nameTree;
		treeNode *parent;		

		while(curr != NULL)
		{
			parent = curr;
			if(strncmp(node.fileName, curr->data.fileName, strlen(node.fileName)) > 0)
				curr = curr->right;
			else if(strncmp(node.fileName, curr->data.fileName, strlen(node.fileName)) < 0)
				curr = curr->left;
			else
				break;
		}

		if(strncmp(node.fileName, parent->data.fileName, strlen(node.fileName)) > 0)
			parent->right = newNode;
		else if(strncmp(node.fileName, parent->data.fileName, strlen(node.fileName)) < 0)
			parent->left = newNode;		
	}

	//Insert into SHA tree
	if(shaTree == NULL)
	{
		shaTree = (struct treeNode *)malloc(sizeof(struct treeNode));		
		shaTree->data = node;
		shaTree->right = NULL;
		shaTree->left = NULL;		
	}
	else
	{
		treeNode *curr = shaTree;
		treeNode *parent;

		while(curr != NULL)
		{
			parent = curr;
			if(strcmp(node.sha_val, curr->data.sha_val) > 0)
				curr = curr->right;
			else if(strcmp(node.sha_val, curr->data.sha_val) < 0)
				curr = curr->left;
			else
				break;
		}

		if(strncmp(node.sha_val, parent->data.sha_val, sizeof node.sha_val) > 0)
		{
			parent->right = (struct treeNode *)malloc(sizeof(struct treeNode));
			parent->right->data = node;
			parent->right->right = NULL;
			parent->right->left = NULL;			
		}
		else if(strncmp(node.sha_val, parent->data.sha_val, sizeof node.sha_val) < 0)
		{
			parent->left = (struct treeNode *)malloc(sizeof(struct treeNode));
			parent->left->data = node;
			parent->left->right = NULL;
			parent->left->left = NULL;			
		}		
	}
}

/****************************************************************
Search for a particular node in either SHA Tree or Name Tree
Made use of code from:
http://www.cprogramming.com/tutorial/lesson18.html
****************************************************************/
struct treeNode* searchTree(struct treeNode *bTree, struct searchData node, short type)
{

	if( type == 1)
	{
		if(bTree == NULL)
			return NULL;

		if(!strncmp(node.fileName, bTree->data.fileName, strlen(node.fileName)))
			return bTree;

		if(strncmp(node.fileName, bTree->data.fileName, strlen(node.fileName)) < 0)
			return searchTree(bTree->left, node, 1);

		if(strncmp(node.fileName, bTree->data.fileName, strlen(node.fileName)) > 0)
			return searchTree(bTree->right, node, 1);
	}
	else if( type == 0)
	{
		if(bTree == NULL)
			return NULL;

		if(!memcmp(node.sha_val, bTree->data.sha_val, sizeof node.sha_val))
			return bTree;

		if(memcmp(node.sha_val, bTree->data.sha_val, sizeof node.sha_val) < 0)
			return searchTree(bTree->left, node, 0);

		if(memcmp(node.sha_val, bTree->data.sha_val, sizeof node.sha_val) > 0)
			return searchTree(bTree->right, node, 0);
	}
	else
	{
		if(bTree == NULL)
			return NULL;

		if(!strncmp(node.realName, bTree->data.realName, strlen(node.realName)))
			return bTree;

		if(strncmp(node.realName, bTree->data.realName, strlen(node.realName)) < 0)
			return searchTree(bTree->left, node, 1);

		if(strncmp(node.realName, bTree->data.realName, strlen(node.realName)) > 0)
			return searchTree(bTree->right, node, 1);

	}
	return NULL;
}

/********************************************************
Takes in data character array and returns the hexadecimal 
equivalentstored in an unsigned character array
********************************************************/
unsigned char* convToHex(unsigned char *Op, int len)
{
	int i = 0;
	int j = 0;
	unsigned char* result = (unsigned char *)malloc(2*len+1);

	while(i <= len)
	{
		sprintf((char *)&result[j], "%02x", Op[i]);
		i++;
		j+=2;
	}
	result[2*len]='\0';	
	return result;
}

/******************************************************
Overload the & operator to perform bitwise AND
******************************************************/
bool operator& (nodeParam &n, uint8_t op2[])
{
    bool chk1 = false, chk2 = false;

	for( int i=0; i < 128/2 ; i++)
	{
		n.intArray[i] = n.intArray[i] & op2[i];

		if(n.intArray[i] > 0)
		{
			chk1 = true;
			break;
		}
	}

	for(int i=64; i <  128 ; i++)
	{
		n.intArray[i] = n.intArray[i] & op2[i];

		if(n.intArray[i] > 0)
		{
			chk2 = true;
			break;
		}
	}

	if(chk1 && chk2)
		return true;
	else
		return false;
}

/******************************************************
Overload the | operator to perform bitwise OR
******************************************************/
void operator| (nodeParam &n, uint8_t orArr[])
{
	for(int j=0 ; j< 128; j++)
	{
		n.intArray[j] = n.intArray[j] | orArr[j] ;
	}
}

/******************************************************
Overload the << operator to perform left shift
******************************************************/
void operator<< (nodeParam &n, int shiftVal)
{
	int q,r;

	uint8_t tempArr[128];
	r = shiftVal%8 ;
	q = (int)shiftVal/8;

	int shift = 1;

	for(int j=0 ; j<128 ; j++)
		tempArr[j] = 0;

	if(q == 0)
	{
		shift = shift << r;
		tempArr[127] = shift;
	}
	else if(r > 0 || q == 127)
	{
		shift = shift << r;
		tempArr[128 - (q+1)] = shift;
	}
	else
	{
		shift = 1;
		tempArr[128 - q - 1] = shift;
	}

	node | tempArr;
}

/********************************************************
Returns the decimal equivalent of a hexadecimal character
This code was broadcasted to the class mailing group
by Professor. Cheng
********************************************************/
uint8_t returnDecValue(char buf[])
{
	int hi_nibble, lo_nibble;
     switch (buf[0]) {
     case '0': hi_nibble = 0; break;
     case '1': hi_nibble = 1; break;
     case '2': hi_nibble = 2; break;
     case '3': hi_nibble = 3; break;
     case '4': hi_nibble = 4; break;
     case '5': hi_nibble = 5; break;
     case '6': hi_nibble = 6; break;
     case '7': hi_nibble = 7; break;
     case '8': hi_nibble = 8; break;
     case '9': hi_nibble = 9; break;
     case 'a': hi_nibble = 10; break;
     case 'b': hi_nibble = 11; break;
     case 'c': hi_nibble = 12; break;
     case 'd': hi_nibble = 13; break;
     case 'e': hi_nibble = 14; break;
     case 'f': hi_nibble = 15; break;
     }
     switch (buf[1]) {
     case '0': lo_nibble = 0; break;
     case '1': lo_nibble = 1; break;
     case '2': lo_nibble = 2; break;
     case '3': lo_nibble = 3; break;
     case '4': lo_nibble = 4; break;
     case '5': lo_nibble = 5; break;
     case '6': lo_nibble = 6; break;
     case '7': lo_nibble = 7; break;
     case '8': lo_nibble = 8; break;
     case '9': lo_nibble = 9; break;
     case 'a': lo_nibble = 10; break;
     case 'b': lo_nibble = 11; break;
     case 'c': lo_nibble = 12; break;
     case 'd': lo_nibble = 13; break;
     case 'e': lo_nibble = 14; break;
     case 'f': lo_nibble = 15; break;
     }
     return ((hi_nibble * 16) + lo_nibble);
}

/******************************************************
Geneterates an integer from a hexadecimal number
******************************************************/
uint8_t* generateIntFromHex(char bitVector[])
{
	uint8_t *op = new uint8_t[128];

	for(int i=0; i<128; i++)
	{
		op[i] = returnDecValue(&bitVector[2*i]);
	}

	return op;
}

/******************************************************
Function to generate the bit vector
******************************************************/
void generateBitVector(char bitVector[], char **keywords, int n)
{


	int shiftVal;
	unsigned char *forSha;
	unsigned char *printSha;
	unsigned char *forMd5;
	unsigned char *printMd5;
	SHA_CTX shaPointer;
	forSha = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
	printSha = (unsigned char *)malloc(SHA_DIGEST_LENGTH);

	MD5_CTX md5Pointer;
	forMd5 = (unsigned char *)malloc(MD5_DIGEST_LENGTH);
	printMd5 = (unsigned char *)malloc(MD5_DIGEST_LENGTH);



	unsigned short printing,printing1;



	pthread_mutex_lock(&nodeLock);
	node.intArray = new uint8_t[128];
	for(int j=0; j<128; j++)
		node.intArray[j] = 0;
	pthread_mutex_unlock(&nodeLock);

	for(int k=0; k < n ; k++)
	{		
		SHA1_Init(&shaPointer);
		SHA1_Update(&shaPointer,keywords[k],strlen(keywords[k]));
		SHA1_Final(forSha,&shaPointer);
		shiftVal = (SHA_DIGEST_LENGTH)-9;

		memcpy(&printing,&forSha[SHA_DIGEST_LENGTH-2],2);
		printing = (printing&511)+512;
		printSha = convToHex(forSha,SHA_DIGEST_LENGTH);

		pthread_mutex_lock(&nodeLock);
	    node << printing;
		pthread_mutex_unlock(&nodeLock);

		MD5_Init(&md5Pointer);
		MD5_Update(&md5Pointer,keywords[k],strlen(keywords[k]));
		MD5_Final(forMd5,&md5Pointer);		

		memcpy(&printing1,&forMd5[MD5_DIGEST_LENGTH-2],sizeof(short));
		printing1 = printing1&511;
		printMd5 = convToHex(forMd5,MD5_DIGEST_LENGTH);

		pthread_mutex_lock(&nodeLock);
		node << printing1 ;
		pthread_mutex_unlock(&nodeLock);
	}

	pthread_mutex_lock(&nodeLock);
	for(int j=0; j < 128 ; j++)
	{
		sprintf(&bitVector[2*j], "%02x", node.intArray[j]);
	}
	pthread_mutex_unlock(&nodeLock);	
}

/***************************************************************
Thread to process Command line arguments
****************************************************************/
void *commandProcessor(void *arg)
{
	struct timeval tme;
	char cmdBuffer[1024];
	int maxfd;
	int length = 0;
	fd_set readSet;
	int fileNum = fileno(stdin);
	tme.tv_sec = 0;
	tme.tv_usec = 500000;
	while(!shutDown)
	{
		FD_ZERO(&readSet);
		memset(cmdBuffer,'\0',sizeof(cmdBuffer));
		length = 0;

		while(waitForJoin)
			sleep(1);

		cout<<"servant:"<<node.port<<"> ";
		cout<<flush;


		while(!shutDown)
		{
			FD_SET(fileNum,&readSet);
			maxfd = fileNum;
			select(maxfd+1,&readSet,NULL,NULL,&tme);
			if (gotInt)
			{
				gotInt = 0;
				break;
			}
			if (FD_ISSET(fileNum,&readSet))
			{
				read(fileNum,cmdBuffer,sizeof(cmdBuffer));
				break;
			}
		}

		if (gotInt)//got Ctrl+C give back prompt
		{
			cout << "\r";
			continue;
		}

		if(!(strncmp(cmdBuffer, "status neighbors", strlen("status neighbors"))))
		{
			statusFiles = false;
			pthread_mutex_lock(&statusRqsLock);
			allowGetCmd = false;
			pthread_mutex_unlock(&statusRqsLock);

			pthread_mutex_lock(&statusRqsLock);
			statusTimeOut = node.msgLifeTime;
			pthread_mutex_unlock(&statusRqsLock);

			pthread_mutex_lock(&statLock);
			memset(myStatUOID, '\0', sizeof myStatUOID);
			pthread_mutex_unlock(&statLock);

			namLinks.clear();
			stPorts.clear();

			MsgDetails msg;
			int n;
			char str2[30];
			sscanf(cmdBuffer, "%*s %*s %d %s", &n, str2);

			statFile = (char *)malloc(strlen(str2)+1);
			memset(statFile, '\0', strlen(str2));
			memcpy(statFile, str2, strlen(str2));
			statFile[strlen(str2)] = '\0';
			msg.TTL = (uint8_t)n;
			msg.messageType = STRQ;
			msg.dataLength = 0;
			msg.snof = 1;

			pthread_mutex_lock(&processQLock);
			processQ.push(msg);
			pthread_mutex_unlock(&processQLock);
			pthread_cond_signal(&processQWait);

			pthread_mutex_lock(&cmdPromptLock);
			pthread_cond_wait(&cmdPromptWait, &cmdPromptLock);
			pthread_mutex_unlock(&cmdPromptLock);
		}
		if(!(strncmp(cmdBuffer, "status files", strlen("status files"))))
		{
			pthread_mutex_lock(&statusRqsLock);
			statusTimeOut = node.msgLifeTime;
			pthread_mutex_unlock(&statusRqsLock);

			statusFiles = true;
			pthread_mutex_lock(&statusRqsLock);
			allowGetCmd = false;
			pthread_mutex_unlock(&statusRqsLock);

			pthread_mutex_lock(&statusRqsLock);
			statusTimeOut = node.msgLifeTime;
			pthread_mutex_unlock(&statusRqsLock);

			pthread_mutex_lock(&statLock);
			memset(myStatUOID, '\0', sizeof myStatUOID);
			pthread_mutex_unlock(&statLock);

			MsgDetails msg;
			int n;
			char str2[30];
			sscanf(cmdBuffer, "%*s %*s %d %s", &n, str2);

			statFile = (char *)malloc(strlen(str2)+1);
			memset(statFile, '\0', strlen(str2));
			memcpy(statFile, str2, strlen(str2));
			statFile[strlen(str2)] = '\0';
			msg.TTL = (uint8_t)n;
			msg.messageType = STRQ;
			msg.dataLength = 0;
			msg.snof = 2;

			pthread_mutex_lock(&processQLock);
			processQ.push(msg);
			pthread_mutex_unlock(&processQLock);
			pthread_cond_signal(&processQWait);

			pthread_mutex_lock(&cmdPromptLock);
			pthread_cond_wait(&cmdPromptWait, &cmdPromptLock);
			pthread_mutex_unlock(&cmdPromptLock);
		}
		else if(!strncmp(cmdBuffer, "search", strlen("search")))
		{
			char *fileParam = NULL;
			size_t fPos;
			string str1;
			MsgDetails msg;

			msg.messageType = SHRQ;
			msg.rsrv = 0;
			msg.nodeID = NULL;

			searchTimeOut = node.msgLifeTime;
			if(!strncmp(cmdBuffer, "search keywords", strlen("search keywords")))
			{


				for (int j=0;cmdBuffer[j]!='\0';j++)
				{
					if (cmdBuffer[j]!='"' && cmdBuffer[j]!='=' && cmdBuffer[j]!=' ')
					{
						cmdBuffer[j]=tolower(cmdBuffer[j]);
					}
					if (cmdBuffer[j]=='=')
						cmdBuffer[j] = ' ';
				}

				for (int j=0;cmdBuffer[j]!='\0';j++)
				{
					if (cmdBuffer[j]=='"' || cmdBuffer[j]=='\n')
					{
						for (int k=j;cmdBuffer[k]!='\0';k++)
						{
							cmdBuffer[k]=cmdBuffer[k+1];
						}
					}
				}

				str1.assign(cmdBuffer);
				fPos = str1.find_first_of(" ", strlen("search keywords"));
				fileParam = (char *)malloc((str1.substr(fPos+1,str1.size())).size()+1);
				strcpy(fileParam,(str1.substr(fPos+1,str1.size())).c_str());
				fileParam[(str1.substr(fPos+1,str1.size())).size()]='\0';
				
				msg.data = (char *)malloc(strlen(fileParam)+2);
				memcpy(&msg.data[0], &type3, sizeof(uint8_t));
				memcpy(&msg.data[sizeof(uint8_t)], fileParam, strlen(fileParam));

				msg.dataLength = strlen(fileParam)+sizeof(uint8_t);
				msg.dataLength = htonl(msg.dataLength);

				pthread_mutex_lock(&processQLock);
				processQ.push(msg);
				pthread_mutex_unlock(&processQLock);
				pthread_cond_signal(&processQWait);
			}
			else if(!strncmp(cmdBuffer, "search filename", strlen("search filename")))
			{
				for (int j=0;cmdBuffer[j]!='\0';j++)
				{
					if (cmdBuffer[j]!='"' && cmdBuffer[j]!='=' && cmdBuffer[j]!=' ')
					{
						cmdBuffer[j]=tolower(cmdBuffer[j]);
					}
					if (cmdBuffer[j]=='=')
						cmdBuffer[j] = ' ';
				}

				for (int j=0;cmdBuffer[j]!='\0';j++)
				{
					if (cmdBuffer[j]=='"' || cmdBuffer[j]=='\n')
					{
						for (int k=j;cmdBuffer[k]!='\0';k++)
						{
							cmdBuffer[k]=cmdBuffer[k+1];
						}
					}
				}

				str1.assign(cmdBuffer);
				fPos = str1.find_first_of(" ", strlen("search filename"));
				fileParam = (char *)malloc((str1.substr(fPos+1,str1.size())).size()+1);
				strcpy(fileParam,(str1.substr(fPos+1,str1.size())).c_str());
				fileParam[(str1.substr(fPos+1,str1.size())).size()]='\0';
				
				msg.data = (char *)malloc(strlen(fileParam)+sizeof(uint8_t));
				memcpy(&msg.data[0], &type1, sizeof(uint8_t));
				memcpy(&msg.data[sizeof(uint8_t)], fileParam, strlen(fileParam));

				msg.dataLength = strlen(fileParam)+sizeof(uint8_t);
				msg.dataLength = htonl(msg.dataLength);

				pthread_mutex_lock(&processQLock);
				processQ.push(msg);
				pthread_mutex_unlock(&processQLock);
				pthread_cond_signal(&processQWait);
			}
			else if(!strncmp(cmdBuffer, "search sha1hash", strlen("search sha1hash")))
			{
				for (int j=0;cmdBuffer[j]!='\0';j++)
				{
					if (cmdBuffer[j]!='"' && cmdBuffer[j]!='=' && cmdBuffer[j]!=' ')
					{
						cmdBuffer[j]=tolower(cmdBuffer[j]);
					}
					if (cmdBuffer[j]=='=')
						cmdBuffer[j] = ' ';
				}

				for (int j=0;cmdBuffer[j]!='\0';j++)
				{
					if (cmdBuffer[j]=='"' || cmdBuffer[j]=='\n')
					{
						for (int k=j;cmdBuffer[k]!='\0';k++)
						{
							cmdBuffer[k]=cmdBuffer[k+1];
						}
					}
				}

				str1.assign(cmdBuffer);
				fPos = str1.find_first_of(" ", strlen("search sha1hash"));
				fileParam = (char *)malloc((str1.substr(fPos+1,str1.size())).size()+1);
				strcpy(fileParam,(str1.substr(fPos+1,str1.size())).c_str());
				fileParam[(str1.substr(fPos+1,str1.size())).size()]='\0';
				
				msg.data = (char *)malloc(strlen(fileParam)+1);
				memcpy(&msg.data[0], &type2, sizeof(uint8_t));
				memcpy(&msg.data[sizeof(uint8_t)], fileParam, strlen(fileParam));

				msg.dataLength = strlen(fileParam)+sizeof(uint8_t);
				msg.dataLength = htonl(msg.dataLength);

				pthread_mutex_lock(&processQLock);
				processQ.push(msg);
				pthread_mutex_unlock(&processQLock);
				pthread_cond_signal(&processQWait);
			}

			pthread_mutex_lock(&cmdPromptLock);
			pthread_cond_wait(&cmdPromptWait, &cmdPromptLock);
			pthread_mutex_unlock(&cmdPromptLock);
		}
		else if(!(strncmp(cmdBuffer, "get", strlen("get"))))
		{

			pthread_mutex_lock(&statusRqsLock);
			statusTimeOut = node.msgLifeTime;
			pthread_mutex_unlock(&statusRqsLock);

			pthread_mutex_lock(&statusRqsLock);
			if(!allowGetCmd)
			{
				pthread_mutex_unlock(&statusRqsLock);
				continue;
			}
			pthread_mutex_unlock(&statusRqsLock);

			int choice;
			struct MsgDetails gMsg;
			memset(getFileName, '\0', sizeof getFileName);
			sscanf(cmdBuffer, "%*s %d %s", &choice, getFileName);

			if(choice <= mySrcResult)
			{
				gMsg.messageType = GTRQ;
				gMsg.rsrv = 0;
				gMsg.TTL = (uint8_t)node.ttl;
				char* temp = (char *)malloc(strlen("peer"));
				strcpy(temp, "peer");
				GetUOID(node.nodeInstance, temp, (char *)gMsg.UOID, sizeof gMsg.UOID);

				memcpy(myGetUOID, gMsg.UOID, sizeof myGetUOID);
				free(temp);

				struct timerSt *pTimer = (struct timerSt*)malloc(sizeof(struct timerSt));
				memcpy(pTimer->msgUOID, gMsg.UOID, sizeof gMsg.UOID);
				pTimer->timeout = node.msgLifeTime;
				pTimer->senderNode = NULL;				

				//New Msg, store UOID
				pthread_mutex_lock(&timerLock);
				cacheUOID.push_back(*pTimer);
				pthread_mutex_unlock(&timerLock);

				gMsg.dataLength = 2*sizeof gMsg.UOID;
				gMsg.data = (char *)malloc(gMsg.dataLength);

				gMsg.dataLength = htonl(gMsg.dataLength);
				gMsg.nodeID = NULL;

				memcpy(&gMsg.data[0], getFileId[choice-1], sizeof gMsg.UOID);
				memcpy(&gMsg.data[sizeof gMsg.UOID], getFileSha[choice-1], sizeof gMsg.UOID);
			}
			else
			{				
				continue;
			}

			if(getFileName[0] == '\0')
			{
				strncpy(getFileName, defGetFileName[choice-1], strlen(defGetFileName[choice-1]));
			}

			ifstream ip;
			ip.open(getFileName, ios::in);

			char *ipBuf = new char[20];

			if(ip.is_open())
			{
				cout << "File exists. Overwrite y/n?" ;
				cin >> ipBuf;

				if(ipBuf[0] != 'y' && ipBuf[0] != 'Y')
				{
					delete[] ipBuf;
					continue;
				}
			}


			pthread_mutex_lock(&processQLock);
			processQ.push(gMsg);
			pthread_mutex_unlock(&processQLock);
			pthread_cond_signal(&processQWait);

			pthread_mutex_lock(&cmdPromptLock);
			pthread_cond_wait(&cmdPromptWait, &cmdPromptLock);
			pthread_mutex_unlock(&cmdPromptLock);
		}
		else if (!(strncmp(cmdBuffer, "delete", strlen("delete"))))
		{
			int noFiles= 0;
			bool foundFile = false;
			bool fileExists = false;
			ifstream op;
			char **matchFiles = (char **)malloc(sizeof(char *));
			MsgDetails msg;
			char* temp = (char *)malloc(strlen("peer"));
			strcpy(temp, "peer");
			GetUOID(node.nodeInstance, temp, (char *)msg.UOID, sizeof msg.UOID);
			free(temp);
			msg.messageType = DELT;
			msg.rsrv = 0;
			msg.TTL = node.ttl;
			msg.nodeID = NULL;
			char *tpassFile;
			if(cmdBuffer[strlen(cmdBuffer)-1] == '\n')
				cmdBuffer[strlen(cmdBuffer)-1] = '\0' ;
			char *userFileName = (char *)malloc(256);
			memset(userFileName,'\0',256);
			char *userNonce = (char *)malloc(40);
			char *userSHA = (char *)malloc(40);
			for (int i=0;cmdBuffer[i]!='\0';i++)
			{
				if (cmdBuffer[i]=='=')
					cmdBuffer[i]=' ';
			}
			sscanf(cmdBuffer, "%*s %*s %s %*s %s %*s %s", userFileName, userSHA, userNonce);
			userFileName[strlen(userFileName)]='\0';			
			char *tdataFile;
			char *ttmetaFile;
			struct searchData s;			
			strncpy(s.fileName, userFileName, strlen(userFileName));
			struct treeNode* t;

			pthread_mutex_lock(&treeLock);
			t = searchTree(nameTree, s, 1);


			if(t != NULL)
			{
				struct treeNode* curr = t;
				tpassFile = (char *)malloc(strlen(curr->data.realName)+1);
				tdataFile = (char *)malloc(strlen(curr->data.realName)+1);
				ttmetaFile = (char *)malloc(strlen(curr->data.realName)+1);
				
					char *tmetaFile = (char *)malloc(strlen(curr->data.realName)+1);
					strcpy(tmetaFile, curr->data.realName);

					tmetaFile[strlen(curr->data.realName)-3] = 'e';
					tmetaFile[strlen(curr->data.realName)-4] = 'm';
					tmetaFile[strlen(curr->data.realName)] = '\0';

					matchFiles = (char **)realloc(matchFiles, sizeof(char *)*(noFiles+1));
					matchFiles[noFiles] = (char *)malloc(strlen(tmetaFile)+1);
					strncpy(matchFiles[noFiles], tmetaFile, strlen(tmetaFile));
					(matchFiles[noFiles])[strlen(tmetaFile)] = '\0';

					foundFile = true;
					noFiles++;					
			}
			pthread_mutex_unlock(&treeLock);

			if (foundFile == false)
			{				
				tpassFile = (char *)malloc(strlen("Dummy_LINES_PASSED") + 1);
				strcpy(tpassFile, "Dummy_LINES_Passed");
				tpassFile[strlen("Dummy_LINES_PASSED")] = '\0';
			}
			else if (foundFile == true)
			{
				char fromMetaFName[256];
				memset(fromMetaFName,'\0',256);
				char *fromMetaSHA = (char *)malloc(40);
				char *fromMetaNonce = (char *)malloc(40);
				for (int i=0;i<noFiles;i++)
				{
					parseMetaData(matchFiles[i],NULL,fromMetaFName,(char *)fromMetaSHA,(char *)fromMetaNonce,NULL,NULL);
					if (!strcmp(userFileName,fromMetaFName))
					{						
						if (!strncmp(userSHA,fromMetaSHA, 40))
						{							
							if (!strncmp(userNonce,fromMetaNonce, 40))
							{								
								fileExists = true;
								strcpy(tpassFile,matchFiles[i]);
								tpassFile[strlen(matchFiles[i])]='\0';
								strcpy(tdataFile,matchFiles[i]);
								tdataFile[strlen(matchFiles[i])]='\0';
								strcpy(ttmetaFile,matchFiles[i]);
								tdataFile[strlen(matchFiles[i])]='\0';
								break;
							}
						}
					}
				}
			}
			
				int ok=0;
				char password[20];
				char userInput[256];
				tpassFile[strlen(tpassFile)-4]='p';
				tdataFile[strlen(tdataFile)-4]='d';
				tdataFile[strlen(tdataFile)-3]='a';				
				op.open(tpassFile,ios::in|ios::binary);
				if (!op.is_open())
				{
					cout<<"No one-time password found."<<endl;
					cout<<"Okay to use a random password [yes/no]? ";
					cin>>userInput;
					userInput[strlen(userInput)]='\0';
					if (userInput[0]=='y'||userInput[0]=='Y')
					{
						ok=1;
						char *dummy;
						dummy = (char *)malloc(strlen("peer"));
						strcpy(dummy, "peer");
						GetUOID(node.nodeInstance, dummy, password, sizeof password);
					}
				}
				else if (op.is_open())
				{
					ok=1;
					op.read(password,20);
					userInput[0]='n';
					userInput[1]='\0';

					pthread_mutex_lock(&bitVStoreLock);
					for(unsigned int j=0 ; j < bitVStorage.size() ; j++)
					{
						if(!strncmp(bitVStorage[j].fName, tdataFile,strlen(tdataFile)))
						{
							bitVStorage.erase(bitVStorage.begin() + j);
						}
					}

					pthread_mutex_unlock(&bitVStoreLock);

					pthread_mutex_lock(&treeLock);
					struct treeNode *t = NULL;
					struct searchData s;
					strncpy(s.realName, tdataFile, strlen(tdataFile));

					t = searchTree(nameTree, s, 2);

					if(t != NULL)
					{
						memset(t->data.fileName, '\0', sizeof t->data.fileName);
						memset(t->data.sha_val, '\0', sizeof t->data.sha_val);
						memset(t->data.realName, '\0', sizeof t->data.realName);
					}
					pthread_mutex_unlock(&treeLock);

					remove(tpassFile);
					remove(ttmetaFile);
					remove(tdataFile);					
				}
				op.close();



				if (ok==1)
				{					

					msg.dataLength = 136+strlen(userFileName);
					msg.data = (char *)malloc(136+strlen(userFileName));
					strcpy(msg.data,"FileName=");
					strcat(msg.data,userFileName);
					strcat(msg.data,"\r\n");
					strcat(msg.data,"SHA1=");
					strcat(msg.data,userSHA);
					strcat(msg.data,"\r\n");
					strcat(msg.data,"Nonce=");
					strcat(msg.data,userNonce);
					strcat(msg.data,"\r\n");
					strcat(msg.data,"Password=");
					strcat(msg.data,password);
					msg.data[strlen(msg.data)]='\0';					

					pthread_mutex_lock(&processQLock);
					processQ.push(msg);
					pthread_mutex_unlock(&processQLock);
					pthread_cond_signal(&processQWait);
				}			
		}
		else if(!(strncmp(cmdBuffer, "store", strlen("store"))))
		{
			pthread_mutex_lock(&statusRqsLock);
			allowGetCmd = false;
			pthread_mutex_unlock(&statusRqsLock);

			if(cmdBuffer[strlen(cmdBuffer)-1] == '\n')
				cmdBuffer[strlen(cmdBuffer)-1] = '\0' ;			

			MsgDetails msg;
			char password[20];
			unsigned char *nonce;
			unsigned char *hexNonce;
			char* inputFileNme;
			inputFileNme = (char *)malloc(256);
			memset(inputFileNme, '\0', 256);

			char *fileParam;
			int TTL;
			char *temp;
			temp = (char *)malloc(10);
			memset(temp, '\0', 10);

			int keepTrack = 0;

			for (int j=0;cmdBuffer[j]!='\0';j++)
			{
				if (cmdBuffer[j]==' ')
				{
					keepTrack++;
				}
				if (keepTrack>=2)
				{
					if (cmdBuffer[j]!='"' && cmdBuffer[j]!='=' && cmdBuffer[j]!=' ')
					{
						cmdBuffer[j]=tolower(cmdBuffer[j]);
					}
					if (cmdBuffer[j]=='=')
						cmdBuffer[j] = ' ';
				}
			}

			for (int j=0;cmdBuffer[j]!='\0';j++)
			{
				if (cmdBuffer[j]=='"')
				{
					for (int k=j;cmdBuffer[k]!='\0';k++)
					{
						cmdBuffer[k]=cmdBuffer[k+1];
					}
				}
			}

			char **keywords;
			char *tmp;
			sscanf(cmdBuffer, "%*s %s %s", inputFileNme, temp);
			inputFileNme[strlen(inputFileNme)]='\0';
			msg.TTL = (uint8_t)atoi(temp);
			string str;
			str.assign(temp);
			string str1;
			str1.assign(cmdBuffer);
			size_t fPos;
			fPos = str1.find_first_of(str,strlen("store ")+strlen(inputFileNme));
			fileParam = (char *)malloc((str1.substr(fPos+str.size(),str1.size())).size()+1);
			strcpy(fileParam,(str1.substr(fPos+str.size(),str1.size())).c_str());
			fileParam[(str1.substr(fPos+str.size(),str1.size())).size()+1]='\0';
			sscanf(temp, "%d", &TTL);

			fileParam[strlen(fileParam)]='\0';


			int i;
			i=0;			
			tmp = strtok(fileParam," ");
			keywords=(char **)malloc(sizeof(char *));
			while (tmp!=NULL)
			{

				keywords=(char **)realloc(keywords,(i+1)*sizeof(char *));
				keywords[i] = (char *)malloc(strlen(tmp));
				strcpy(keywords[i],tmp);
				tmp = strtok(NULL," ");				
				i++;
			}

			fstream fOp;
			SHA_CTX shaPointer;
			SHA1_Init(&shaPointer);
			unsigned char* fileHash;

			struct stat fs;
			int fileSize;
			if (stat(inputFileNme,&fs)==0)
				fileSize = fs.st_size;

			int Qt = (int)fileSize/MAXBUFFERSIZE;
			int Rmn = fileSize%MAXBUFFERSIZE;

			fOp.open(inputFileNme, ios::in|ios::binary);
			char *rdBuf = new char[MAXBUFFERSIZE];
			memset(rdBuf, '\0', MAXBUFFERSIZE);


			if(!fOp.is_open())
			{				
				continue;
			}

			for (int k=0;k<Qt;k++)
			{				
				fOp.read(rdBuf,MAXBUFFERSIZE);
				SHA1_Update(&shaPointer, (void *)rdBuf, MAXBUFFERSIZE);
				memset(rdBuf, '\0', MAXBUFFERSIZE);
			}

			if (Rmn!=0)
			{				
				rdBuf = new char[Rmn];
				fOp.read(rdBuf,Rmn);
				SHA1_Update(&shaPointer, (void *)rdBuf, Rmn);
				memset(rdBuf, '\0', Rmn);
			}

			fileHash = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
			SHA1_Final(fileHash, &shaPointer);
			fOp.close();




			char *dummy;
			dummy = (char *)malloc(strlen("peer"));
			strcpy(dummy, "peer");

			GetUOID(node.nodeInstance, dummy, password, sizeof password);
			nonce = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
			SHA1_Init(&shaPointer);
			SHA1_Update(&shaPointer, password, sizeof password);
			SHA1_Final(nonce, &shaPointer);
			hexNonce = convToHex(nonce, SHA_DIGEST_LENGTH);			

			struct bitVect* newBitVect = (struct bitVect*)malloc(sizeof(struct bitVect));
			memset(newBitVect->vArr,'0',256);

			generateBitVector(newBitVect->vArr, keywords, i);
			newBitVect->vArr[256]='\0';


			int fileSz;
			struct stat fileDet;
			if (stat(inputFileNme,&fileDet)==0&&!(S_ISDIR(fileDet.st_mode)))
			{
				fileSz = fileDet.st_size;
			}

			char *metaData = (char *)malloc(strlen(".meta")+4);
			memset(metaData, '\0', strlen(".meta")+4);

			char *dataFileNme = (char *)malloc(strlen(".data")+4);
			memset(dataFileNme, '\0', strlen(".meta")+4);

			char *passFileNme = (char *)malloc(strlen(".peta")+4);
			memset(passFileNme, '\0', strlen(".peta")+4);


				pthread_mutex_lock(&statLock);
				msg.data = (char *)malloc(sizeof(int));
				sprintf(msg.data,"%d",fileCount);				
				sprintf(metaData, "%d", (int)fileCount);
				sprintf(dataFileNme, "%d", (int)fileCount);
				sprintf(passFileNme, "%d", (int)fileCount);
				fileCount++;
				pthread_mutex_unlock(&statLock);


			strcat(metaData, ".meta");
			strcat(dataFileNme, ".data");
			strcat(passFileNme, ".peta");
			
			metaFile = (char *)malloc(strlen(myHomeFiles) + strlen(metaData) +2);
			memset(metaFile, '\0', strlen(myHomeFiles) + strlen(metaData) +2);
			strcpy(metaFile, myHomeFiles);
			strcat(metaFile, "/");
			strcat(metaFile, metaData);

			passFile = (char *)malloc(strlen(myHomeFiles) + strlen(passFileNme) +2);
			memset(passFile, '\0', strlen(myHomeFiles) + strlen(passFileNme) +2);
			strcpy(passFile, myHomeFiles);
			strcat(passFile, "/");
			strcat(passFile, passFileNme);

			ofstream pOp;
			pOp.open(passFile, ios::out|ios::binary);
			if (pOp.is_open())
			{				
				pOp.write(password, sizeof password);
			}
			pOp.close();			

			char *dataFile = (char *)malloc(strlen(myHomeFiles) + strlen(dataFileNme) +2);
			memset(dataFile, '\0', strlen(myHomeFiles) + strlen(dataFileNme) +2);
			strcpy(dataFile, myHomeFiles);
			strcat(dataFile, "/");
			strcat(dataFile, dataFileNme);

			memset(newBitVect->fName, '\0', sizeof newBitVect->fName);
			strcpy(newBitVect->fName, dataFile);

			pthread_mutex_lock(&bitVStoreLock);
			bitVStorage.push_back(*newBitVect);
			struct bitVect tempBitVect;
			for (unsigned int i=0;i<bitVStorage.size()-1;i++)
			{
				for (unsigned int j=0;j<bitVStorage.size()-1-i;j++)
				{
					if (strcmp(bitVStorage[j].vArr,bitVStorage[j+1].vArr)>0)
					{
						tempBitVect = bitVStorage[j];
						bitVStorage[j] = bitVStorage[j+1];
						bitVStorage[j+1] = tempBitVect;
					}
				}
			}
			pthread_mutex_unlock(&bitVStoreLock);


			struct searchData sData;
			strncpy(sData.fileName, inputFileNme, strlen(inputFileNme));
			strncpy(sData.realName, dataFile, strlen(dataFile));
			strncpy(sData.sha_val, (char *)convToHex(fileHash, SHA_DIGEST_LENGTH), sizeof sData.sha_val);

			pthread_mutex_lock(&treeLock);
			insertInTree(sData);
			pthread_mutex_unlock(&treeLock);

			ofstream mOp;
			mOp.open(metaFile, ios::out);

			if(mOp.is_open())
			{
				mOp << "[metadata]" << "\n";
				mOp << "FileName=" << inputFileNme << "\n" ;
				mOp << "FileSize=" << fileSz << "\n" ;
				mOp << "SHA1=" << convToHex(fileHash, SHA_DIGEST_LENGTH) << "\n" ;
				mOp << "Nonce=" << convToHex(nonce, SHA_DIGEST_LENGTH) << "\n" ;
				mOp << "Keywords=" ;

				for(int k =0; k < i ; k++)
				{					
					mOp.write(keywords[k],strlen(keywords[k]));
					mOp<<" ";
				}

				mOp << "\n";
				mOp << "Bit-Vector=" ;
				mOp.write(newBitVect->vArr, strlen(newBitVect->vArr));
			}

			mOp.close();

			if(mOp.fail())
			{
				remove(metaFile);				
				continue;
			}

			FILE *ip,*op;
			ip = fopen(inputFileNme,"rb");
			op = fopen(dataFile,"wb");

			bool returnError = false;

			for (int k=0;k<Qt;k++)
			{
				char *buffer = new char[MAXBUFFERSIZE+1];
				memset(buffer, '\0', MAXBUFFERSIZE+1);
				fread(buffer,sizeof(char),MAXBUFFERSIZE,ip);
				buffer[strlen(buffer)]='\0';
				if (fwrite(buffer,sizeof(char),MAXBUFFERSIZE,op)==0)
				{
					returnError = true;
					break;
				}
				fflush(op);
				delete[] buffer;
			}

			if (Rmn!=0&&returnError==false)
			{
				char *buffer = new char[Rmn+1];
				memset(buffer, '\0', Rmn+1);
				fread(buffer,sizeof(char),Rmn,ip);
				buffer[strlen(buffer)]='\0';
				if ((fwrite(buffer,sizeof(char),Rmn,op))==0)
				{
					returnError = true;
				}
				fflush(op);
				delete[] buffer;
			}
			fclose(ip);
			fclose(op);

			msg.messageType = STOR;

			if (returnError == true)
			{
				remove(dataFile);
				remove(metaFile);
				remove(passFile);
			}
			if (returnError == false)
			{
				pthread_mutex_lock(&processQLock);
				processQ.push(msg);
				pthread_mutex_unlock(&processQLock);
				pthread_cond_signal(&processQWait);
			}
			
			delete[] rdBuf;
			free(newBitVect);
			free(fileHash);
			free(fileParam);
			free(tmp);			
			for(int j = 0; j < i ; j++)
			{
			   free(keywords[j]);
			}
			
			free(inputFileNme);
			free(temp);
			str.erase();
			str1.erase();

		}
		else if(!(strncmp(cmdBuffer, "check", strlen("check"))))
		{
			pthread_mutex_lock(&statusRqsLock);
			allowGetCmd = false;
			pthread_mutex_unlock(&statusRqsLock);

			MsgDetails msg;
			msg.messageType = CKRQ;
			msg.nodeID = (char *)malloc(sizeof(char));
			memset(msg.nodeID, '\0', sizeof(char));

			pthread_mutex_lock(&processQLock);
			processQ.push(msg);
			pthread_mutex_unlock(&processQLock);
			pthread_cond_signal(&processQWait);
		}
		else if(!(strncmp(cmdBuffer, "shutdown", strlen("shutdown"))))
		{
			ofstream fp;
			char chkBit[256];
			memset(chkBit, 0, sizeof chkBit);

			pthread_mutex_lock(&bitVStoreLock);
			if(bitVStorage.size() > 0)
				fp.open(keywordFile,ios::out);

			for (unsigned int i=0; i < bitVStorage.size();i++)
			{				
				struct bitVect writeOut;				
				writeOut = bitVStorage[i];

				if(!strncmp(bitVStorage[i].vArr, chkBit, sizeof chkBit))
				{
				}
				else
				{
					fp.write(reinterpret_cast<char *>(&writeOut), sizeof writeOut);
				}				
			}
			pthread_mutex_unlock(&bitVStoreLock);
			fp.close();

			pthread_mutex_lock(&cacheLock);
			if(fileCache.size() > 0)
				fp.open(cacheFile, ios::out);

			for(unsigned int i=0; i < fileCache.size() ; i++)
			{
				fp.write(reinterpret_cast<char *>(&fileCache[i]), sizeof(fileCache[i]));
			}
			pthread_mutex_unlock(&cacheLock);
			fp.close();

			pthread_mutex_lock(&fileIdLock);
			if(fileIdStore.size() > 0)
				fp.open(fileIdFile, ios::out);

			for(unsigned int i=0; i < fileIdStore.size() ; i++)
			{
				fp.write(reinterpret_cast<char *>(&fileIdStore[i]), sizeof(fileIdStore[i]));
			}
			pthread_mutex_unlock(&fileIdLock);
			fp.close();

			if(nameTree != NULL)
			{
				fp.open(nameIndex, ios::out);
				pthread_mutex_lock(&treeLock);
				printTree(nameTree, 0, fp);
				pthread_mutex_unlock(&treeLock);

				fp.close();
			}

			if(shaTree != NULL)
			{
				fp.open(shaIndex, ios::out);
				pthread_mutex_lock(&treeLock);
				printTree(shaTree, 1, fp);
				pthread_mutex_unlock(&treeLock);
				fp.close();
			}

			pthread_mutex_lock(&restart);
			terminateNode = true;
			exitID = 1;
			pthread_mutex_unlock(&restart);
			sendNotify("all",1);
			usleep(800000);
			shutDown = true;

			pthread_cancel(sigHandleID);
			pthread_cancel(acceptThread);
			pthread_cond_signal(&processQWait);
		}
	}
	pthread_exit(NULL);
}

/**************************************************************************
Function to create Reader thread and sender thread for join messages
**************************************************************************/
void joinRsp(struct MsgDetails reqMsg)
{
	pthread_t thrid;
	struct connData *parmD = (struct connData*)malloc(sizeof(struct connData));

	short reqPort;
	char strreqPort[7];
	memset(strreqPort, '\0', sizeof strreqPort);

	parmD->sockDesc = reqMsg.pSock;
	parmD->peerNode = (char *)malloc(ntohl(reqMsg.dataLength)-sizeof(short)-sizeof(unsigned int)+1+sizeof strreqPort);
	memcpy(parmD->peerNode, &reqMsg.data[sizeof(unsigned int)+sizeof(short)], ntohl(reqMsg.dataLength)-sizeof(short)-sizeof(unsigned int));

	memcpy(&reqPort, &reqMsg.data[sizeof(unsigned int)], sizeof(short));
	reqPort = ntohs(reqPort);
	sprintf(strreqPort, "%u", reqPort);

	strcat(parmD->peerNode, ":");
	strcat(parmD->peerNode, strreqPort);
	parmD->gotHello = 0;
	pthread_mutex_lock(&thrIDLock);
	pthread_create(&thrid, NULL, permConnReader, parmD);
	threadID.push_back(thrid);
	pthread_mutex_unlock(&thrIDLock);
}

/********************************************************
Dummy function to return name info of host
********************************************************/
void returName(char *peerHost, int mySock)
{
	struct sockaddr_in peerAd;
	size_t peerLen = sizeof peerAd;
	getpeername(mySock, (struct sockaddr*)&peerAd, &peerLen);
	getnameinfo((struct sockaddr*)&peerAd, peerLen, peerHost, sizeof peerHost, NULL, 0, 0);
}

/*********************************************************************
Thread to handle new connections. spawned by accept
*********************************************************************/
void *handleConn(void *arg)
{

	int* sockID = (int *)arg;

	unsigned char recvHdr[headerLen];
	int bytesToRcv, bytesRmn;
	MsgDetails msg;
	struct connData *parm = (struct connData *)malloc(sizeof(struct connData));
	pthread_t tid;
	bool sameMsg = false;

		bytesToRcv= (int)headerLen/CHUNK;
		bytesRmn = headerLen%CHUNK;
		int i = 0;
		int temp;
		int bytesRcvd = 0;
		int k = 0;

		while( k < bytesToRcv && (temp = recv(*sockID, &recvHdr[bytesRcvd], CHUNK, 0)) != 0)
		{
			if( temp != -1)
			{
				bytesRcvd += temp;
				k++;
			}
		}

		if(bytesRmn > 0)
		{
			temp = recv(*sockID, &recvHdr[bytesRcvd], bytesRmn, 0);

			if(temp > 0)
			  bytesRcvd += temp;
		}

		while(bytesRcvd < headerLen && (temp = recv(*sockID, &recvHdr[bytesRcvd], sizeof(char), 0)) != 0)
		{
			if (temp > 0)
			  bytesRcvd += temp;
		 }

		if(temp == 0)
		{
		    struct timeval pt;
			gettimeofday(&pt, NULL);
			pthread_exit(NULL);
		}

		//Received Header
		memcpy(&msg.messageType, &recvHdr[0], sizeof(uint8_t));
		memcpy(&msg.UOID, &recvHdr[sizeof(uint8_t)], sizeof msg.UOID);
		memcpy(&msg.TTL, &recvHdr[sizeof(uint8_t) + sizeof msg.UOID], sizeof(uint8_t));
		memcpy(&msg.rsrv, &recvHdr[2*sizeof(uint8_t) + sizeof msg.UOID], sizeof(uint8_t));
		memcpy(&msg.dataLength, &recvHdr[3*sizeof(uint8_t) + sizeof msg.UOID], sizeof(int));

		msg.data = (char *)malloc(ntohl(msg.dataLength));
		char *recvBuf = (char *)malloc(ntohl(msg.dataLength));
		memset(recvBuf, '\0', ntohl(msg.dataLength));
		bytesToRcv = (int)((ntohl(msg.dataLength))/CHUNK);
		bytesRmn   = (ntohl(msg.dataLength))%CHUNK;
		i=0;
		k =0;
		bytesRcvd = 0;

		while(k < bytesToRcv && (temp = recv(*sockID, &recvBuf[bytesRcvd], CHUNK, 0)) != 0)
		{
			if(temp != -1)
			{
				k++;
				bytesRcvd += temp;
			}
		}

		if(bytesRmn > 0)
		{
			temp = recv(*sockID, &recvBuf[bytesRcvd], bytesRmn, 0);

			if(temp > 0)
			  bytesRcvd += temp;
		 }

		 while(bytesRcvd < ntohl(msg.dataLength) && (temp = recv(*sockID, &recvBuf[bytesRcvd], sizeof(char), 0)) != 0)
		 {
			 if(temp > 0)
			  bytesRcvd += temp;
		  }

	 if(temp == 0)
		{
				struct timeval pt;
				gettimeofday(&pt, NULL);
			    pthread_exit(NULL);
		}

		memcpy(msg.data, recvBuf, ntohl(msg.dataLength));
		parm->sockDesc = *sockID;

		msg.pSock = *sockID;

		if(msg.messageType == HLLO)
		{
			short peerPort;
			char portNumber[8];
			char *tempData = (char *)malloc(ntohl(msg.dataLength));

			memset(tempData, '\0', ntohl(msg.dataLength));
			memcpy(tempData,msg.data,ntohl(msg.dataLength));

			memcpy(&peerPort, &tempData[0], sizeof(short));
			sprintf(portNumber,"%d",peerPort);
			parm->peerNode = (char *)malloc(ntohl(msg.dataLength)+1);

			memcpy(parm->peerNode, &tempData[sizeof(short)], ntohl(msg.dataLength)-sizeof(short));

			strcat(parm->peerNode,":");
			strcat(parm->peerNode,portNumber);

			msg.nodeID = (char *)malloc(strlen(parm->peerNode));
			strcpy(msg.nodeID,parm->peerNode);
			msg.kind = 1;
			pthread_mutex_lock(&logLock);
			logQueue.push(msg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);

			if(!connExists(parm->peerNode))
			{
				parm->gotHello = 1;
				pthread_mutex_lock(&thrIDLock);
				pthread_create(&tid, NULL, permConnReader, parm);
				threadID.push_back(tid);
				pthread_mutex_unlock(&thrIDLock);

				struct peerInfo *newEntry = (struct peerInfo*)malloc(sizeof(struct peerInfo));
				newEntry->nodeInfo = (char *)malloc(strlen(parm->peerNode)+1);
				strcpy(newEntry->nodeInfo, parm->peerNode);
				newEntry->nodeInfo[strlen(parm->peerNode)]='\0';
				newEntry->peerSock = parm->sockDesc;
				newEntry->myConn = 0;
				newEntry->forKeepAlive = parm;
				newEntry->forKeepAlive->keepAlive = node.keepAliveTimeOut-3;

				newEntry->forKeepAlive->peerContact = node.keepAliveTimeOut;


				pthread_mutex_lock(&existConnLock);
				existConn.push_back(*newEntry);
				pthread_mutex_unlock(&existConnLock);

				struct MsgDetails rhMsg;
				rhMsg.messageType = HLLO;
				sendHello(rhMsg, newEntry->nodeInfo);

				pthread_mutex_lock(&helloLock);
				helloNo++;
				pthread_mutex_unlock(&helloLock);

				  pthread_mutex_lock(&testPrinter);
				  waitForJoin = false;
				  pthread_mutex_unlock(&testPrinter);

			}
			else
			{
				sleep(1);
				shutdown(*sockID, SHUT_RDWR);
				close(*sockID);
			}
		}
		else
		{


			pthread_mutex_lock(&timerLock);
			for(unsigned int j=0; j < cacheUOID.size(); j++)
			{
				if(!memcmp(msg.UOID, cacheUOID[j].msgUOID, sizeof msg.UOID))
				{
					pthread_mutex_unlock(&timerLock);
					sameMsg = true;
					break;
				}
			}
			pthread_mutex_unlock(&timerLock);

			if(sameMsg)
			{
				free(parm);
				pthread_exit(NULL);
			}

			if(msg.messageType == JNRQ)
			{
				short reqPort;
				struct timerSt *pTimer = (struct timerSt*)malloc(sizeof(struct timerSt));
				char strreqPort[7];
				memset(strreqPort, '\0', sizeof strreqPort);

				memcpy(pTimer->msgUOID, msg.UOID, sizeof msg.UOID);
				pTimer->timeout = node.msgLifeTime;

				memcpy(&reqPort, &msg.data[sizeof(unsigned int)], sizeof(short));
				reqPort = ntohs(reqPort);


				sprintf(strreqPort, "%u", reqPort);

				pTimer->senderNode = (char *)malloc(ntohl(msg.dataLength)-sizeof(short)-sizeof(unsigned int)+1+strlen(strreqPort));
				memset(pTimer->senderNode, '\0', ntohl(msg.dataLength)-sizeof(short)-sizeof(unsigned int));
				memcpy(pTimer->senderNode, &msg.data[sizeof(unsigned int)+sizeof(short)], ntohl(msg.dataLength)-sizeof(short)-sizeof(unsigned int));

				strcat(pTimer->senderNode, ":");
				strcat(pTimer->senderNode, strreqPort);

				pthread_mutex_lock(&timerLock);
				cacheUOID.push_back(*pTimer);
				pthread_mutex_unlock(&timerLock);

				msg.nodeID = (char *)malloc(strlen(pTimer->senderNode));
				strcpy(msg.nodeID,pTimer->senderNode);
				msg.kind = 1;
				pthread_mutex_lock(&logLock);
				logQueue.push(msg);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);

				free(pTimer);

				if(IAMBEACON)
					joinRsp(msg);
			}


			pthread_mutex_lock(&processQLock);
			processQ.push(msg);
			pthread_mutex_unlock(&processQLock);
			pthread_cond_signal(&processQWait);

			free(parm);
		}

    parm = NULL;
	pthread_exit(NULL);
}

/*********************************************************************
Thread to process join responses. waits for msglifetime seconds to timeout
**********************************************************************/
void* joinRspProcessor(void *arg)
{
	struct connData* cancelID = (struct connData*)arg;
	unsigned int temp = 0;
	unsigned int* min;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	struct timespec ts;

	ts.tv_sec = tv.tv_sec + node.msgLifeTime;
	ts.tv_nsec = tv.tv_usec*1000;

	fstream fOp;

	int nSleep = 0;
	while( nSleep < node.msgLifeTime - 10 && !terminateNode)
	{
		sleep(1);
		nSleep++;
	}

	pthread_mutex_lock(&joinPrcsSig);
	if(neighborNodes.size() < (unsigned int)node.initNeighbors || terminateNode)
	{
		shutdown(cancelID->sockDesc,SHUT_RDWR);
		close(cancelID->sockDesc);
		pthread_mutex_unlock(&joinPrcsSig);

		pthread_mutex_lock(&restart);
		GLOBALTIMEOUT = 0;
		pthread_mutex_unlock(&restart);
		pthread_exit(NULL);
	}

	shutdown(cancelID->sockDesc,SHUT_RDWR);
	close(cancelID->sockDesc);

	min = (unsigned int*)malloc(sizeof(unsigned int)*neighborNodes.size());
	for(unsigned int j=0 ; j < neighborNodes.size() ; j++)
	{


		memcpy(&temp, &neighborNodes[j].data[sizeof myJoinUOID], sizeof(unsigned int));

		min[j] = ntohl(temp);
	}
	pthread_mutex_unlock(&joinPrcsSig);

	//Sort responses in ascending Order
	unsigned int swapt;

	for (unsigned int k=0; k<neighborNodes.size()-1; k++)
	{
		for(unsigned int l=0; l < neighborNodes.size()-1-k ; l++)
		{
			if (min[l]>min[l+1])
			{
				swapt = min[l];
				min[l] = min[l+1];
				min[l+1] = swapt;
			}
		}
	}

	char *dummy;
	short nPort;
	char cPort[7];

	fOp.open(neighborFile, ios::out);

	if(!fOp.is_open())
	{		
		pthread_exit(NULL);
	}

	unsigned int t,tt;
	int y=0;
	unsigned int z=0;
	int flag = 0;


	//Choose neigbors from sorted data structure and populate global structure
	for(int k=0; k<node.initNeighbors&&z<neighborNodes.size(); k++)
	{
		flag = 0;
		memcpy(&tt, &neighborNodes[z].data[sizeof myJoinUOID], sizeof(unsigned int));
		t = ntohl(tt);

		for (y=0;y<node.initNeighbors;y++)
		{
			if (t==min[y])
			{
				flag = 1;
				break;
			}
		}
		if (flag == 1)
		{
			dummy = (char *)malloc(ntohl(neighborNodes[z].dataLength)-sizeof neighborNodes[z].UOID-sizeof(unsigned int)-sizeof(short)+2+sizeof cPort);
			memset(dummy, '\0', ntohl(neighborNodes[z].dataLength)-sizeof neighborNodes[z].UOID-sizeof(unsigned int)-sizeof(short)+2+sizeof cPort);

			memcpy(dummy, &neighborNodes[z].data[sizeof neighborNodes[z].UOID + sizeof(unsigned int) + sizeof(short)], ntohl(neighborNodes[z].dataLength)-sizeof neighborNodes[z].UOID-sizeof(unsigned int)-sizeof(short));

			strcat(dummy, ":");
			memcpy(&nPort, &neighborNodes[z].data[sizeof neighborNodes[z].UOID+sizeof(unsigned int)], sizeof(short));
			nPort = ntohs(nPort);
			sprintf(cPort, "%u", nPort);
			strcat(dummy, cPort);
			strcat(dummy, "\0");

			myNeighbors[k] = (char *)malloc(strlen(dummy));

			strcpy(myNeighbors[k], dummy);
			z++;
			free(dummy);
		}
		else
		{
			k--;
			z++;
		}
	}

	for(int k=0; k<node.initNeighbors; k++)
	{
		fOp.write(myNeighbors[k], strlen(myNeighbors[k]));
		fOp.write("\n", 1);
	}

	fOp.close();

	struct MsgDetails hMsg;
	hMsg.messageType = HLLO;
	hMsg.dataLength = 0;

	pthread_mutex_lock(&processQLock);
	processQ.push(hMsg);
	pthread_mutex_unlock(&processQLock);
	pthread_cond_signal(&processQWait);
	pthread_exit(NULL);
}

/***************************************************************************
Function to initiate node joining to network
***************************************************************************/
void joinPeers()
{
    struct MsgDetails *msg;
  	unsigned int byteLocation;
    connData cdB;
    char* connPeer;

	msg = (struct MsgDetails*)malloc(sizeof(struct MsgDetails));
	msg->messageType = JNRQ;

   char *tempH;
   tempH = (char *)malloc(strlen("peer"));
   strcpy(tempH, "peer");
   GetUOID(node.nodeInstance, tempH, (char *)msg->UOID, sizeof msg->UOID);

   memcpy(myJoinUOID, msg->UOID, sizeof msg->UOID);

   short welKPort = htons((short)node.port);
   byteLocation = (unsigned int)node.location;
   byteLocation = htonl(byteLocation);

   msg->TTL = (uint8_t)node.ttl;
   msg->rsrv = 0;
   msg->dataLength = (sizeof(unsigned int)+sizeof(short)+strlen(node.hostNme));

   cdB.dataLen = (3*sizeof(uint8_t)+sizeof msg->UOID+sizeof(int)+msg->dataLength);
   cdB.sendBuf = (unsigned char*)malloc(cdB.dataLen);

   msg->dataLength = htonl(msg->dataLength);

   //Header
   memcpy(&cdB.sendBuf[0], &msg->messageType, sizeof(uint8_t));
   memcpy(&cdB.sendBuf[sizeof(uint8_t)], msg->UOID, sizeof msg->UOID);
   memcpy(&cdB.sendBuf[sizeof(uint8_t)+sizeof msg->UOID], &msg->TTL, sizeof(uint8_t));
   memcpy(&cdB.sendBuf[2*sizeof(uint8_t)+sizeof msg->UOID], &msg->rsrv, sizeof(uint8_t));
   memcpy(&cdB.sendBuf[3*sizeof(uint8_t)+sizeof msg->UOID], &msg->dataLength, sizeof(int));
   //Data
   msg->data = (char *)malloc(sizeof(unsigned int)+sizeof(short)+strlen(node.hostNme));
   memcpy(&msg->data[0], &byteLocation, sizeof(unsigned int));
   memcpy(&msg->data[sizeof(unsigned int)], &welKPort, sizeof(short));
   memcpy(&msg->data[sizeof(unsigned int)+sizeof(short)], node.hostNme, strlen(node.hostNme));
   memcpy(&cdB.sendBuf[3*sizeof(uint8_t)+sizeof msg->UOID+sizeof(int)], &byteLocation, sizeof(unsigned int));
   memcpy(&cdB.sendBuf[3*sizeof(uint8_t)+sizeof msg->UOID+sizeof(int)+sizeof(unsigned int)], &welKPort, sizeof(short));
   memcpy(&cdB.sendBuf[3*sizeof(uint8_t)+sizeof msg->UOID+sizeof(int)+sizeof(unsigned int)+sizeof(short)], node.hostNme, strlen(node.hostNme));


   if(beaconList.size() > 0)
   {
	   int j = rand()%beaconList.size() ;
	   connPeer = (char *)malloc(strlen(beaconList[j]));
	   memcpy(connPeer, beaconList[j], strlen(beaconList[j]));
   }
   else
   {
	   pthread_exit(NULL);
   }

   	string str(connPeer);
   	size_t fPos = str.find_first_of(":");

   	if(fPos == string::npos)
   	{
   		pthread_mutex_lock(&testPrinter);
   		pthread_mutex_unlock(&testPrinter);
   		pthread_exit(NULL);
   	}


		struct sockaddr_in peerAddr;

		char* peerName = (char *)malloc((str.substr(0, fPos)).size());
		strcpy(peerName, (str.substr(0, fPos)).c_str());

   if(bconIP == NULL)
	{
	    bconIP = (struct hostent*)malloc(sizeof(struct hostent));
		pthread_mutex_lock(&testPrinter);
		bconIP = gethostbyname(peerName);
		pthread_mutex_unlock(&testPrinter);
	}

	short peerPort = (short)(atoi((str.substr(fPos+1, str.size())).c_str()));

   	peerAddr.sin_family = PF_INET;
   	peerAddr.sin_port = htons(peerPort);
   	memcpy(&peerAddr.sin_addr, bconIP->h_addr, bconIP->h_length);
	memset(&peerAddr.sin_zero, '\0', sizeof peerAddr.sin_zero);


  int mySock = socket(PF_INET, SOCK_STREAM, 0);
  unsigned int nom = 0;
  int j = 0;
  while(1)
  {
	if(connect(mySock, (struct sockaddr*)&peerAddr, sizeof peerAddr) == -1)
	{
		close(mySock);
		mySock = socket(PF_INET, SOCK_STREAM, 0);
		for(unsigned int k=0; k < beaconList.size() ; k++)
		{
			if(!strncmp(connPeer, beaconList[k], strlen(connPeer)))
			{
				beaconList.erase(beaconList.begin()+k);//Delete unavailable beacon
			}
		}

		if(nom > beaconList.size())//no more beacons
		{
				pthread_mutex_lock(&restart);
				GLOBALTIMEOUT = 0;
				pthread_mutex_unlock(&restart);
				pthread_exit(NULL);
	    }
		nom++;
		if (beaconList.size()>0)
		{
			j = ((int)(rand()+(drand48()*10)))%beaconList.size() ;
		}
		else
			continue;

	   connPeer = (char *)malloc(strlen(beaconList[j])); //Choose next available beacon
	   memcpy(connPeer, beaconList[j], strlen(beaconList[j]));

		str.assign(connPeer);
		size_t fPos = str.find_first_of(":");

		peerName = (char *)malloc((str.substr(0, fPos)).size());
		strcpy(peerName, (str.substr(0, fPos)).c_str());

		pthread_mutex_lock(&testPrinter);
		bconIP = gethostbyname(peerName);
		pthread_mutex_unlock(&testPrinter);

	   	peerPort = (short)(atoi((str.substr(fPos+1, str.size())).c_str()));

		peerAddr.sin_family = PF_INET;
		peerAddr.sin_port = htons(peerPort);
		memcpy(&peerAddr.sin_addr, bconIP->h_addr, bconIP->h_length);
		j++;
	}
	else
		break;
  }

  	msg->nodeID = (char *)malloc(strlen(peerName)+1+ strlen((str.substr(fPos+1, str.size())).c_str()));
  	strncpy(msg->nodeID, peerName, strlen(peerName));
  	strcat(msg->nodeID, ":");
  	strcat(msg->nodeID, (str.substr(fPos+1, str.size())).c_str()) ;

	msg->kind = 3;
	pthread_mutex_lock(&logLock);
	logQueue.push(*msg);
	pthread_mutex_unlock(&logLock);
	pthread_cond_signal(&forLogging);

	int temp, k =0 ;
	int bytesSent = 0 ;
	int bytesToSend = 0;
	int bytesRmn ;

		bytesToSend = (int)cdB.dataLen/CHUNK;
		bytesRmn = cdB.dataLen%CHUNK;

		while( k < bytesToSend && ((temp=send(mySock, &cdB.sendBuf[bytesSent], CHUNK, 0)) != -1))
		{
			k++;

			if(temp > 0)
			 bytesSent += temp;
		}

		if(bytesRmn > 0)
		{
			temp = send(mySock, &cdB.sendBuf[bytesSent], bytesRmn, 0);

			if(temp > 0)
			  bytesSent += temp;
		}

		while(bytesSent < cdB.dataLen && (temp = send(mySock, &cdB.sendBuf[bytesSent], sizeof(char), 0)) != -1)
		{
			if(temp > 0)
			 bytesSent += temp;
		}


	pthread_t tid;
	struct connData *lisRply = (struct connData*)malloc(sizeof(struct connData));
	lisRply->peerNode = (char *)malloc(strlen(connPeer));
	strcpy(lisRply->peerNode, connPeer);
	lisRply->sockDesc = mySock;
	lisRply->gotHello = 0;

	pthread_mutex_lock(&thrIDLock);
	pthread_create(&tid, NULL, permConnReader, lisRply);
	threadID.push_back(tid);

	pthread_create(&tid, NULL, joinRspProcessor, lisRply);
	threadID.push_back(tid);
	pthread_mutex_unlock(&thrIDLock);
}

/*************************************************
Function - logic to set TTL for messages
**************************************************/
uint8_t setTTL(uint8_t msgttl)
{
	if(msgttl <= node.ttl)
		return --msgttl;
	else
		return node.ttl;
}

/**************************************************
Cleanup handler - close listening socket
**************************************************/
void closeListen(void *arg)
{
	int* p = (int *)arg;
	shutdown(*p, SHUT_RDWR);
	close(*p);
}

/*****************************************************
Thread to accept new connections
*****************************************************/
void *strtServer(void *arg)
{
	int sockID;
	int clientSock;
	int *temp;
	pthread_t thrid;
	struct sockaddr_in serverAddr, clientAddr;
	short MAXCON = 30;
	size_t clLen = sizeof(struct sockaddr_in);
	int reuse;
	char myPort[10];
	MsgDetails msg;

	sockID = socket(PF_INET, SOCK_STREAM, 0);

	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(node.port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));


	char* hostHost = (char *)malloc(sizeof(char) * 30);
	gethostname(hostHost, 30);

	if(setsockopt(sockID, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
	{
		cout << "Setsockopt failed" << endl;
	}

	if( bind(sockID, (struct sockaddr*)&serverAddr, sizeof serverAddr) == -1)
	{
		cout << "Server Binding Failed " << errno << endl;
		earlyExit();
	}

	if( listen(sockID, MAXCON) == -1 )
	{
		cout << "Listen on port failed" << endl;
		earlyExit();
	}

      pthread_mutex_lock(&thrIDLock);
      pthread_create(&thrid, NULL, processData, NULL);
      threadID.push_back(thrid);
      pthread_mutex_unlock(&thrIDLock);

	char* hostName = (char *)malloc(sizeof(char) * 30);
	gethostname(hostName, 30);
	node.hostNme = (char *)malloc(strlen(hostName));
	strcpy(node.hostNme, hostName);

	sprintf(myPort, "%d", node.port);

	node.nodeInstance = (char *)malloc(strlen(hostName));
	strcat(node.nodeInstance, "_");
	strcat(node.nodeInstance, myPort);
	strcat(node.nodeInstance, "_");

	char nowTime[15];
	time_t currTime = time(NULL);
	sprintf(nowTime, "%d", (int)currTime);
	strcat(node.nodeInstance, nowTime);

	strcat(hostName, ":");
	strcat(hostName, myPort);

	for (int j=0; (unsigned int)j < beaconList.size() ; j++)
	{
		if(!strcmp(hostName, beaconList[j]))
		{
			beaconList.erase(beaconList.begin() + j );
			IAMBEACON = 1;
			j = noOfBeacons;
		}
	}

  if(!IAMBEACON)
  {
		fstream nFile;
		nFile.open(neighborFile, ios::in);
		struct MsgDetails hMsg;
		hMsg.messageType = HLLO;

		if(!nFile.is_open())
			joinPeers();
		else
		{
			char nNode[75];
			int k =0;

			while(!nFile.eof() && k < node.minNeighbors)
			{
				memset(nNode, '\0', sizeof nNode);
				nFile.getline(nNode, sizeof nNode);
				myNeighbors[k] = (char *)malloc(strlen(nNode));
				strcpy(myNeighbors[k++], nNode);
			}

			if(k < node.minNeighbors)
			{
				if(remove(neighborFile) != 0)
				{
				}

				pthread_mutex_lock(&restart);
				GLOBALTIMEOUT = 0;
				pthread_mutex_unlock(&restart);

				pthread_exit(NULL);
			}

			pthread_mutex_lock(&processQLock);
			processQ.push(hMsg);
			pthread_mutex_unlock(&processQLock);
			pthread_cond_signal(&processQWait);
		}
  }
  else
  {

	  pthread_mutex_lock(&testPrinter);
	  waitForJoin = false;
	  pthread_mutex_unlock(&testPrinter);

	  msg.messageType = HLLO;
	  pthread_mutex_lock(&processQLock);
	  processQ.push(msg);
	  pthread_mutex_unlock(&processQLock);
	  pthread_cond_signal(&processQWait);
  }
//hello spawn
	pthread_cleanup_push(closeListen,&sockID);
	while(!terminateNode)
	{
		if( (clientSock = accept(sockID, (struct sockaddr*)&clientAddr, (socklen_t *)&clLen)) == -1)
		{
				continue;
		}

		temp = (int *)malloc(sizeof(int));
		*temp = clientSock;

		pthread_mutex_lock(&thrIDLock);
		pthread_create(&thrid, NULL, handleConn, temp);
		threadID.push_back(thrid);
		pthread_mutex_unlock(&thrIDLock);

		temp = NULL;
	}

	pthread_cleanup_pop(1);
	pthread_exit(NULL);
}

/***********************************************************************
Signal Handler: Handles Ctrl+C, SIGPIPE
***********************************************************************/
void *sigHandler(void *arg)
{
	int sigRecv;

	while(!terminateNode)
	{
		sigwait(&sigCatch, &sigRecv);

		if(sigRecv == SIGPIPE)
		{
			sigRecv = 0;
			continue;
		}
		else if(sigRecv == SIGINT)
		{
			gotInt = 1;
			sigRecv = 0;
			pthread_mutex_lock(&statLock);
			memset(myStatUOID, '\0', sizeof myStatUOID);
			pthread_mutex_unlock(&statLock);

			memset(mySrcUOID, '\0', sizeof mySrcUOID);
			memset(myGetUOID, '\0', sizeof myGetUOID);
			pthread_cond_signal(&cmdPromptWait);
		}
	 }
  pthread_exit(NULL);
}

/************************************************************************
Timer Thread. Deletes cached UOIDs and set GLOBALTIMEOUT
*************************************************************************/
void* timeKeeper(void *arg)
{
	struct MsgDetails kMsg;
	kMsg.messageType = KPAV;
	struct connData *kData = (struct connData*)malloc(sizeof(struct connData));

	std::vector <struct peerInfo> copyExistConn;

	while(!terminateNode)
	{
		usleep(1400000);

		pthread_mutex_lock(&statusRqsLock);
		if(statusTimeOut > -1)
		{
			statusTimeOut -= 2;
		}

		if(searchTimeOut > -1)
		{
			searchTimeOut -= 2;
		}

		if(statusTimeOut <= 0 && searchTimeOut <= 0)
			pthread_cond_signal(&cmdPromptWait);



		pthread_mutex_unlock(&statusRqsLock);

		pthread_mutex_lock(&restart);
		GLOBALTIMEOUT -= 2;
		pthread_mutex_unlock(&restart);

		if(GLOBALTIMEOUT <= 0)
		{
			ofstream fp;

			pthread_mutex_lock(&bitVStoreLock);
			if(bitVStorage.size() > 0)
				fp.open(keywordFile,ios::out);
			for (unsigned int i=0;i<bitVStorage.size();i++)
			{
				fp.write(reinterpret_cast<char *>(&bitVStorage[i]), sizeof(bitVStorage[i]));
			}
			pthread_mutex_unlock(&bitVStoreLock);
			fp.close();

			pthread_mutex_lock(&cacheLock);
			if(fileCache.size() > 0)
				fp.open(cacheFile, ios::out);

			for(unsigned int i=0; i < fileCache.size() ; i++)
			{
				fp.write(reinterpret_cast<char *>(&fileCache[i]), sizeof(fileCache[i]));
			}
			pthread_mutex_unlock(&cacheLock);
			fp.close();


			if(nameTree != NULL)
			{
				fp.open(nameIndex, ios::out);
				pthread_mutex_lock(&treeLock);
				printTree(nameTree, 0, fp);
				pthread_mutex_unlock(&treeLock);

				fp.close();
			}

			if(shaTree != NULL)
			{
				fp.open(shaIndex, ios::out);
				pthread_mutex_lock(&treeLock);
				printTree(shaTree, 1, fp);
				pthread_mutex_unlock(&treeLock);
				fp.close();
			}

			pthread_mutex_lock(&restart);
			shutDown = true;
			terminateNode = true;
			exitID = 0;
			pthread_mutex_unlock(&restart);

			sendNotify("all", 0);
			usleep(800000);

			pthread_cancel(sigHandleID);
			pthread_cancel(acceptThread);
			pthread_cond_signal(&processQWait);

			pthread_cond_signal(&cmdPromptWait);

			pthread_mutex_lock(&existConnLock);
			for(unsigned int j=0; j<existConn.size() ; j++)
			{
				shutdown(existConn[j].peerSock, SHUT_RDWR);
				close(existConn[j].peerSock);
			}
			pthread_mutex_unlock(&existConnLock);

			terminateLogger = 1;
			pthread_cond_signal(&forLogging);
			pthread_mutex_lock(&thrIDLock);
			for(unsigned int j=0; j < threadID.size(); j++)
			{
					pthread_cancel(threadID[j]);
			}
			pthread_mutex_unlock(&thrIDLock);
			break;
		}

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0; j<cacheUOID.size() ; j++)
		{
			cacheUOID[j].timeout = cacheUOID[j].timeout - 2;

			if(cacheUOID[j].timeout <= 0)
				cacheUOID.erase(cacheUOID.begin() + j);
		}
		pthread_mutex_unlock(&timerLock);

		pthread_mutex_lock(&existConnLock);
		copyExistConn = existConn;


		for(unsigned int i=0; i<copyExistConn.size(); i++)
		{
			copyExistConn[i].forKeepAlive->keepAlive = copyExistConn[i].forKeepAlive->keepAlive - 2;
			copyExistConn[i].forKeepAlive->peerContact = copyExistConn[i].forKeepAlive->peerContact - 2;
			if(copyExistConn[i].forKeepAlive->keepAlive <= 2)
			{
				kMsg.nodeID = (char *)malloc(strlen(copyExistConn[i].nodeInfo));
				strncpy(kMsg.nodeID, copyExistConn[i].nodeInfo, strlen(copyExistConn[i].nodeInfo));

				kData = sendKeepAlive(kMsg);
				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*kData);
				pthread_mutex_unlock(&sendQLock);

				copyExistConn[i].forKeepAlive->keepAlive = node.keepAliveTimeOut-3;
			}

		}
		pthread_mutex_unlock(&existConnLock);

	}

	pthread_exit(NULL);
}

/***************************************************************
Main thread. Spawns threads and waits for all threads to finish
****************************************************************/
int main(int argc, char *argv[])
{
	char *iniFile;
	char *tempBuf;
	short RESET = 0;
	pthread_t tid;

	if(argc <= 1)
	{
		cout << "Insufficient Arguments" << endl;
		earlyExit();
	}
	else if(argc == 2)
	{
		tempBuf = (char *)malloc(256) ;
		strncpy(tempBuf, argv[argc-1], strlen(argv[argc-1]));
	}
	else if( argc == 3)
	{
		for (int i=1; i<argc; i++)
		{
			if(!strcmp(argv[i], "-reset"))
			{
				RESET = 1;
			}
			else
			{
				tempBuf = (char *)malloc(strlen(argv[i])) ;
				memset(tempBuf, '\0', strlen(tempBuf));
				strncpy(tempBuf, argv[i], strlen(argv[i]));
			}

		}
	}
	else
	{
		cout << "Too Many Arguments" << endl;
		earlyExit();
	}

	string fChk(tempBuf);
	size_t dotOp = fChk.find_last_of(".");
	if( (fChk.substr(dotOp+1, fChk.size())) != "ini")
	{
		cout << "Invalid File" << endl;
		earlyExit();
	}

	sigemptyset(&sigCatch);
	sigaddset(&sigCatch, SIGPIPE);
	sigaddset(&sigCatch, SIGINT);
	pthread_sigmask(SIG_BLOCK, &sigCatch, NULL);

	pthread_create(&tid, NULL, sigHandler, NULL);
	sigHandleID = tid;
	threadID.push_back(tid);

	iniFile = (char *)malloc(strlen(tempBuf));
	memset(iniFile, '\0', sizeof(iniFile));
	strcpy(iniFile, tempBuf);
	parseIni(iniFile);

	char pPort[7];
	sprintf(pPort, "%d", node.port);

	myHome = (char *)malloc(strlen(pPort)+strlen(HOME_DIR));
	strncpy(myHome, HOME_DIR, strlen(HOME_DIR));
	myHome[strlen(HOME_DIR)]='\0';

	DIR* chkHome = opendir(myHome);
	if(chkHome == NULL)
	{
		cout << "Home dir does not exist" << endl;
		exit(0);
	}

	myHomeFiles = (char *)malloc(strlen(myHome)+1+strlen("files"));
	strcpy(myHomeFiles,myHome);
	strcat(myHomeFiles,"/");
	strcat(myHomeFiles,"files");

	if(RESET)
	{
		DIR* op = opendir(myHome);
		struct dirent *ent;

		char fileD[256];

		if(op != NULL)
		{
			while((ent = readdir(op)) != NULL)
			{
				memset(fileD, '\0', sizeof fileD);
				strncpy(fileD, myHome, strlen(myHome));
				strcat(fileD, "/");
				strcat(fileD, ent->d_name);								
				remove(fileD);
			}
		}

		op = opendir(myHomeFiles);
		if(op != NULL)
		{
			while((ent = readdir(op)) != NULL)
			{
				
				memset(fileD, '\0', sizeof fileD);
				strncpy(fileD, myHomeFiles, strlen(myHomeFiles));
				strcat(fileD, "/");
				strcat(fileD, ent->d_name);
				fileD[strlen(myHomeFiles)+strlen(ent->d_name)+1] = '\0';				
				remove(fileD);
			}
		}

		if(remove(neighborFile) != 0)
			cout << "Unable to delete neighbor list" << endl;

		if(remove(logFile) != 0)
			cout << "Unable to remove log file" << endl;
	}

	chkHome = opendir(myHomeFiles);
	if(chkHome == NULL)
	{
		mkdir(myHomeFiles, S_IRWXU);
	}

	keywordFile = (char *)malloc(strlen(myHome)+1+strlen(KWRD_FILE));
	strcpy(keywordFile,myHome);
	strcat(keywordFile,"/");
	strcat(keywordFile,KWRD_FILE);

	InitRandom(0);	

	ifstream kOp;
	kOp.open(keywordFile, ios::in);
	if(kOp.is_open())
	{
		while(!kOp.eof() && kOp.peek() != EOF)
		{
			struct bitVect t;
			kOp.read(reinterpret_cast<char *>(&t), sizeof(t));
			bitVStorage.push_back(t);
			struct bitVect tempBitVect;
			for (unsigned int i=0;i<bitVStorage.size()-1;i++)
			{
				for (unsigned int j=0;j<bitVStorage.size()-1-i;j++)
				{
					if (strcmp(bitVStorage[j].vArr,bitVStorage[j+1].vArr)>0)
					{
						tempBitVect = bitVStorage[j];
						bitVStorage[j] = bitVStorage[j+1];
						bitVStorage[j+1] = tempBitVect;
					}
				}
			}
			fileCount++;			
		}
	}
	kOp.close();	

	neighborFile = (char *)malloc(strlen(myHome) + 1 + strlen(NEIGHBOR_FILE));
	strcpy(neighborFile, myHome);
	strcat(neighborFile, "/");
	strcat(neighborFile, NEIGHBOR_FILE);

	logFile = (char *)malloc(strlen(myHome) + 1 + strlen(LOG_FILE));
	strcpy(logFile, myHome);
	strcat(logFile, "/");
	strcat(logFile, LOG_FILE);

	cacheFile = (char *)malloc(strlen(myHome)+1+strlen(CACHE_FILE));
	strcpy(cacheFile, myHome);
	strcat(cacheFile, "/");
	strcat(cacheFile, CACHE_FILE);

	kOp.open(cacheFile, ios::in);
	if(kOp.is_open())
	{
		while(!kOp.eof() && kOp.peek() != EOF)
		{
			struct cacheStruct c;
			kOp.read(reinterpret_cast<char *>(&c), sizeof(c));
			fileCache.push_back(c);

			struct cacheStruct tempFileCache;
			for (unsigned int i=0;i<fileCache.size()-1;i++)
			{
				for (unsigned int j=0;j<fileCache.size()-1-i;j++)
				{
					if (strcmp(fileCache[j].fileName,fileCache[j+1].fileName)>0)
					{
						tempFileCache = fileCache[j];
						fileCache[j] = fileCache[j+1];
						fileCache[j+1] = tempFileCache;
					}
				}
			}
			currCacheSize += c.fileSz;
		}
	}
	kOp.close();

	nameIndex = (char *)malloc(strlen(myHome)+1+strlen(NAME_INDEX));
	strcpy(nameIndex, myHome);
	strcat(nameIndex, "/");
	strcat(nameIndex, NAME_INDEX);

	shaIndex = (char *)malloc(strlen(myHome)+1+strlen(SHA_INDEX));
	strcpy(shaIndex, myHome);
	strcat(shaIndex, "/");
	strcat(shaIndex, SHA_INDEX);

	kOp.open(nameIndex, ios::in);
	if(kOp.is_open())
	{
		while(!kOp.eof() && kOp.peek() != EOF)
		{
			struct searchData s;
			kOp.read(reinterpret_cast<char *>(&s), sizeof(s));
			insertInTree(s);
		}
	}
	kOp.close();

	fileIdFile = (char *)malloc(strlen(myHome)+1+strlen(FILE_ID));
	strcpy(fileIdFile, myHome);
	strcat(fileIdFile, "/");
	strcat(fileIdFile, FILE_ID);

	kOp.open(fileIdFile, ios::in);
	if(kOp.is_open())
	{
		while(!kOp.eof() && kOp.peek() != EOF)
		{
			struct getFile g;
			kOp.read(reinterpret_cast<char *>(&g), sizeof(g));
			fileIdStore.push_back(g);
		}
	}
	kOp.close();

	if (node.initNeighbors > node.minNeighbors)
	{
		myNeighbors = (char **)malloc(sizeof(char *)*node.initNeighbors);
	}
	else
	{
		myNeighbors = (char **)malloc(sizeof(char *)*node.minNeighbors);
	}



	  pthread_create(&tid, NULL, strtServer, NULL);
	  threadID.push_back(tid);
	  acceptThread = tid;
	  pthread_create(&tid, NULL, infoLogger, NULL);

	  pthread_create(&tid, NULL, commandProcessor, NULL);
	  threadID.push_back(tid);

	  pthread_create(&tid, NULL, timeKeeper, NULL);
	  threadID.push_back(tid);

	while(!shutDown)
	{
		if(softRestart)
		{
			remove(neighborFile);
			neighborNodes.clear();
			pthread_mutex_lock(&existConnLock);
			for(unsigned int j=0; j<existConn.size() ; j++)
			{
				shutdown(existConn[j].peerSock, SHUT_RDWR);
				close(existConn[j].peerSock);
			}
			existConn.clear();
			pthread_mutex_unlock(&existConnLock);
			free(myNeighbors);

			if (node.initNeighbors > node.minNeighbors)
			{
				myNeighbors = (char **)malloc(sizeof(char *)*node.initNeighbors);
			}
			else
			{
				myNeighbors = (char **)malloc(sizeof(char *)*node.minNeighbors);
			}

			pthread_mutex_lock(&helloLock);
			helloNo = 0;
			pthread_mutex_unlock(&helloLock);

			joinPeers();
			softRestart = false;

			pthread_mutex_lock(&statLock);
			waitingForCheck = false;
			pthread_mutex_unlock(&statLock);
		}
		sleep(2);
	}

  terminateLogger = 1;
  pthread_cond_signal(&forLogging);

  	for(unsigned int i = 0; i < threadID.size() ; i++)
	{
		pthread_join(threadID[i], NULL);
	}

	while(processQ.size()>0)
	{
		processQ.pop();
	}
	while(logQueue.size()>0)
	{
		logQueue.pop();
	}
	sendQ.clear();
	existConn.clear();
	cacheUOID.clear();
	neighborNodes.clear();
	stPorts.clear();
	beaconList.clear();
	return 0;
}

