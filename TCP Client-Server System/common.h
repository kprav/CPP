#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

struct ReqMsg //the message format structure
{
	uint16_t MsgType;
	uint32_t Offset;
	uint32_t DataLen;
	char *Data;
};

struct acceptbundle //structure to bundle the ip address of the client and its socket descriptor
{
	int *fd;
	char ip[INET6_ADDRSTRLEN];
};

void cleanup(void *arg); //cleanup handler to shutdown and close sockets

int isnum(char abc[100],int len); //input a string and check if it contains only an integer

int getnum(char pqr[100],int len); //input a string and return the integer in the string

void *get_in_addr(struct sockaddr *sa); //This function is to calculate the ip address
