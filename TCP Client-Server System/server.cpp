#include <fstream.h>
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
#include <sys/stat.h>
#include <pthread.h>
#include <iostream.h>
#include <signal.h>
#include <openssl/md5.h>
#include "common.h"

#define TRUE 1
#define FALSE 0
#define BUFLIM 512
#define ADR_REQ 65040
#define FSZ_REQ 65056
#define GET_REQ 65072
#define ADR_RPLY 65041
#define FSZ_RPLY 65057
#define GET_RPLY 65073
#define ADR_FAIL 65042
#define FSZ_FAIL 65058
#define GET_FAIL 65074
#define ALL_FAIL 64766

int threadID[100];
int sockfd; 
int tme,dlay,port;
char mval[1];
pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER; //create a lock
int threadcount = 0;

void sigpipehandler(int signum) //handler to handle the SIGPIPE signal
{
	cout<<"\n Error Occurred ";
	cout<<"\n The socket was closed at the client side ";
	cout<<"\n Current server thread has Exited "<<endl;
	pthread_exit(NULL);
}

void *processclient(void *arg) //thread that handles client requests
{		
	struct sigaction action;
	acceptbundle ab;
	int sendsize,bytesreceived;
	int val,p;
	off_t tsendsize;
	struct stat size;
	int connfd,i,msg_buf_sz,rmsg_buf_sz;
	hostent *he;
	char *n;
	unsigned char *md;
	unsigned long len;
	int z;
	ifstream fp;	
	off_t tsize,ssize;
	char *s;
	char *ss;
	struct sockaddr_in a;
	ReqMsg msg,tmsg;
	ReqMsg rmsg,rtmsg;
	unsigned char *printing;
	char *msg_buf;
	char *msg_buf1;
	char *rmsg_buf;	
	int x;
	MD5_CTX c;
	bytesreceived = 0;
	memset(&a, 0, sizeof(struct sockaddr_in));
	memset(&msg,0,sizeof(msg));
	memset(&tmsg,0,sizeof(tmsg));
	msg_buf_sz = 10;	
	msg_buf = (char *)malloc(msg_buf_sz);
	printing = (unsigned char *)malloc(10);
	memset(msg_buf,0,msg_buf_sz);
	ab = *((acceptbundle *)arg);	
	connfd = *(ab.fd);	//the child thread socket for handling the client request
	action.sa_handler = (void (*)(int))sigpipehandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGPIPE,&action,NULL) == -1) //set up the handler to catch SIGPIPE signal
		cout<<"\n Error in Signal SIGPIPE Handling "<<endl;
	pthread_cleanup_push(cleanup,(void *)&connfd);
	pthread_testcancel(); //set up a thread cancellation point
	pthread_cleanup_pop(0);
	
	for (i=0;i<sizeof(unsigned short)+2*sizeof(unsigned int);i++) //read the header of the client request
	{
		pthread_cleanup_push(cleanup,(void *)&connfd);
		pthread_testcancel();
		pthread_cleanup_pop(0);
		recv(connfd,&msg_buf[i],1,0);
		bytesreceived++;
	}
	memcpy(&tmsg.MsgType,&msg_buf[0],sizeof(unsigned short));
	memcpy(&tmsg.Offset,&msg_buf[sizeof(unsigned short)],sizeof(unsigned int));
	memcpy(&tmsg.DataLen,&msg_buf[sizeof(unsigned short)+sizeof(unsigned int)],sizeof(unsigned int));
	msg.MsgType = ntohs((unsigned short)tmsg.MsgType);
	msg.Offset = ntohl((unsigned int)tmsg.Offset);
	msg.DataLen = ntohl((unsigned int)tmsg.DataLen);
	x = msg.DataLen;
	msg.Data = (char *)malloc(msg.DataLen);
	if (msg.DataLen>512)
	{
		msg_buf_sz=512;	
	}
	else
		msg_buf_sz = msg.DataLen;	
	msg_buf1 = (char *)malloc(msg_buf_sz);	
	memset(msg_buf1,0,msg_buf_sz);
	pthread_cleanup_push(cleanup,(void *)&connfd);
	pthread_testcancel(); //set up a thread cancellation point
	pthread_cleanup_pop(0);
	do
	{
		for (i=0;i<msg_buf_sz;i++) //read the data from the client
		{
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);	
			recv(connfd,&msg_buf1[i],1,0);
			bytesreceived++;
		}
		x=x-512;
		strcat(msg.Data,msg_buf1);
	}while(x>0);

	if (strcmp(mval,"y")==0) //send(print) the details from the client to stdout
	{
		memcpy(&printing[0],&msg.MsgType,sizeof(unsigned short));
		memcpy(&printing[sizeof(unsigned short)],&msg.Offset,sizeof(unsigned int));
		memcpy(&printing[sizeof(unsigned short)+sizeof(unsigned int)],&msg.DataLen,sizeof(unsigned int));
		cout<<"\n Received "<<bytesreceived<<" bytes from "<<ab.ip;
		//cout<<"\n Received "<<sizeof(unsigned short)+2*sizeof(unsigned int)+msg.DataLen<<" bytes from "<<ab.ip;
		printf("\n MsgType: 0x");
		for (p=0;p<sizeof(unsigned short);p++)
		{
			printf("%02x",printing[p]);
		}
		printf("\n Offset: 0x");
		for (p=sizeof(short);p<sizeof(short)+sizeof(int);p++)
			printf("%02x",printing[p]);
		printf("\n DataLength: 0x");
		for (p=sizeof(short)+sizeof(int);p<sizeof(short)+2*sizeof(int);p++)
			printf("%02x",printing[p]);	
		printf("\n");
	}
	
	memset(&rmsg,0,sizeof(rmsg));
	memset(&rtmsg,0,sizeof(rtmsg));
	if (msg.MsgType == ADR_REQ)	//handles the adr request from the client
	{				
		if (gethostbyname(msg.Data)==NULL) //the name of the host sent by the client was not resolved - ADR_FAIL
		{						
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			rmsg.MsgType = ADR_FAIL;
			rmsg.Offset = 0;
			rmsg.DataLen = 0;
			rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
			rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
			rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);
			rmsg_buf_sz = 10;	
			rmsg_buf = (char *)malloc(rmsg_buf_sz);
			memset(rmsg_buf,0,rmsg_buf_sz);
			memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
			memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
			memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));		
			for (i=0;i<dlay;i++) //sleep for dlay seconds before sending the reply back to the client
				sleep(1);			
			for (i=0;i<rmsg_buf_sz;i++) //send the reply to the client
			{	
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if ((send(connfd,&rmsg_buf[i],1,0))==-1)
				{	
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");
				}
			}
		}
		else
		{
			//the name of the host sent by the client was resolved succesfully - ADR_RPLY
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			he = gethostbyname(msg.Data);
			a.sin_addr = *((struct in_addr *)he->h_addr);
			s = (char *)malloc(strlen(inet_ntoa(a.sin_addr)));
			strcpy(s,inet_ntoa(a.sin_addr));	//returns the IP address required
			rmsg.MsgType = ADR_RPLY;
			rmsg.Offset = 0;
			rmsg.DataLen = strlen(s);
			rmsg.Data=(char *)malloc(rmsg.DataLen);
			rtmsg.Data=(char *)malloc(rtmsg.DataLen);
			rmsg.Data=s;
			rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
			rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
			rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);
			rtmsg.Data = rmsg.Data;
			rmsg_buf_sz = 10+strlen(rtmsg.Data)+1;	
			rmsg_buf = (char *)malloc(rmsg_buf_sz);
			memset(rmsg_buf,0,rmsg_buf_sz);
			memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
			memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
			memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));
			strcpy(&rmsg_buf[sizeof(unsigned short)+2*sizeof(unsigned int)],rtmsg.Data);
			for (i=0;i<dlay;i++) //sleep for dlay seconds before sending the reply to the client
				sleep(1);	
			for (i=0;i<rmsg_buf_sz;i++) //send the reply to the client
			{	
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if ((send(connfd,&rmsg_buf[i],1,0))==-1)
				{
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");
				}
			}						
		}
	}
	
	else if (msg.MsgType == FSZ_REQ) //handles the fsz request from the client
	{					
		if (stat(msg.Data,&size)==0) //successfully obtained the size of the file in the client request - FSZ_RPLY
		{				
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			rmsg.MsgType = FSZ_RPLY;
			rmsg.Offset = 0;
			tsendsize = size.st_size; //obtain the size of the file						
			sendsize = (int)tsendsize;			
			s = (char *)malloc(sizeof(int));			
			sprintf(s,"%d",sendsize);				
			rmsg.DataLen = strlen(s);			
			rmsg.Data=(char *)malloc(sizeof(int));
			rtmsg.Data=(char *)malloc(sizeof(int));		
			rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
			rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
			rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);			
			rmsg_buf_sz = 10+rmsg.DataLen+1;	
			rmsg_buf = (char *)malloc(rmsg_buf_sz);
			memset(rmsg_buf,0,rmsg_buf_sz);
			memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
			memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
			memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));			
			for (i=0;i<dlay;i++) //wait for dlay seconds before sending reply to the client
				sleep(1);	
			for (i=0;i<10;i++) //send the header to the client
			{	
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if ((send(connfd,&rmsg_buf[i],1,0))==-1)
				{
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");
				}
			}
			for (i=0;i<strlen(s);i++) //send the file size to the client
			{
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if (send(connfd,&s[i],1,0)==-1)
				{
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");			
				}
			}
		}
		else
		{
			//attempt to obtain the size of the file failed - FSZ_FAIL
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			rmsg.MsgType = FSZ_FAIL;
			rmsg.Offset = 0;
			rmsg.DataLen = 0;
			rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
			rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
			rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);
			rmsg_buf_sz = 10;	
			rmsg_buf = (char *)malloc(rmsg_buf_sz);
			memset(rmsg_buf,0,rmsg_buf_sz);
			memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
			memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
			memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));		
			for (i=0;i<dlay;i++) //wait for dlay seconds before sending the reply to the client
				sleep(1);	
			for (i=0;i<rmsg_buf_sz;i++) //send the reply to the client
			{	
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if ((send(connfd,&rmsg_buf[i],1,0))==-1)
				{
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");
				}
			}
		}
	}

	else if (msg.MsgType == GET_REQ) //handles the get request from the client
	{						
		if (stat(msg.Data,&size)==0) //the get request is successful
		{			
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			rmsg.MsgType = GET_RPLY;
			rmsg.Offset = msg.Offset;
			tsize = size.st_size;
			rmsg.DataLen = tsize-msg.Offset;
			rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
			rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
			rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);
			rmsg_buf_sz = 10;	
			rmsg_buf = (char *)malloc(rmsg_buf_sz);
			memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
			memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
			memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));
			for (i=0;i<dlay;i++) //wait for dlay seconds before sending the reply to the client
				sleep(1);	
			for (i=0;i<sizeof(unsigned short)+2*sizeof(unsigned int);i++) //send the header to the client
			{
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if ((send(connfd,&rmsg_buf[i],1,0))==-1)
				{
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");
				}
			}
			MD5_Init(&c); //initialize md5
			fp.open(msg.Data,ios::binary);
			tsize=tsize-msg.Offset;
			if (tsize<=0)			
				tsize=0;			
			if (tsize<512)
				val = tsize;
			else
				val = 512;
			len = (unsigned long)val;
			if (val==0)			
				fp.seekg(ios::end);
			else
				fp.seekg(msg.Offset);
			while (!fp.eof()) //start reading from the file
			{
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				rtmsg.Data = (char *)malloc(val);
				fp.read(rtmsg.Data,val);
				MD5_Update(&c,rtmsg.Data,len); //update md5
				for (i=0;i<val;i++) //read max 512 bytes from the file and send it
				{
					pthread_cleanup_push(cleanup,(void *)&connfd);
					pthread_testcancel();
					pthread_cleanup_pop(0);
					if (send(connfd,&rtmsg.Data[i],1,0)==-1)
					{
						shutdown(connfd,SHUT_RDWR);
						close(connfd);
						perror("send");
					}
				}	
				tsize = tsize-val;
				if (tsize==0)
					break;
				if (tsize<512)
					val = tsize;
				else
					val = 512;
				len = (unsigned long)val;
			}
			md = (unsigned char *)malloc(16);
			MD5_Final(md,&c); //finalize md5	
			fp.close();
		}
		else
		{
			// the get request failed
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			rmsg.MsgType = GET_FAIL;
			rmsg.Offset = 0;
			rmsg.DataLen = 0;
			rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
			rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
			rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);
			rmsg_buf_sz = 10;	
			rmsg_buf = (char *)malloc(rmsg_buf_sz);
			memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
			memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
			memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));
			for (i=0;i<dlay;i++) //wait for dlay seconds before sending reply to the client
				sleep(1);	
			for (i=0;i<sizeof(unsigned short)+2*sizeof(unsigned int);i++) //send the reply to the client
			{
				pthread_cleanup_push(cleanup,(void *)&connfd);
				pthread_testcancel();
				pthread_cleanup_pop(0);
				if ((send(connfd,&rmsg_buf[i],1,0))==-1)
				{
					shutdown(connfd,SHUT_RDWR);
					close(connfd);
					perror("send");
				}
			}			
		}
	}

	else
	{
		//the request was unknown to the server...send ALL_FAIL
		pthread_cleanup_push(cleanup,(void *)&connfd);
		pthread_testcancel();
		pthread_cleanup_pop(0);
		rmsg.MsgType = ALL_FAIL;
		rmsg.Offset = 0;
		rmsg.DataLen = 0;
		rtmsg.MsgType = htons((unsigned short)rmsg.MsgType);
		rtmsg.Offset = htonl((unsigned int)rmsg.Offset);
		rtmsg.DataLen = htonl((unsigned int)rmsg.DataLen);
		rmsg_buf_sz = 10;	
		rmsg_buf = (char *)malloc(rmsg_buf_sz);
		memcpy(&rmsg_buf[0],&rtmsg.MsgType,sizeof(unsigned short));
		memcpy(&rmsg_buf[sizeof(unsigned short)],&rtmsg.Offset,sizeof(unsigned int));
		memcpy(&rmsg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&rtmsg.DataLen,sizeof(unsigned int));
		for (i=0;i<dlay;i++) //wait for dlay seconds before sending reply to the client
				sleep(1);
		for (i=0;i<sizeof(unsigned short)+2*sizeof(unsigned int);i++) //send the ALL_FAIL reply to the client
		{
			pthread_cleanup_push(cleanup,(void *)&connfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			if ((send(connfd,&rmsg_buf[i],1,0))==-1)
			{
				shutdown(connfd,SHUT_RDWR);
				close(connfd);
				perror("send");
			}
		}	
	}
	pthread_cleanup_push(cleanup,(void *)&connfd);
	pthread_testcancel();
	pthread_cleanup_pop(0);
	shutdown(connfd,SHUT_RDWR);
	close(connfd);
	pthread_exit(NULL);
}

void *acceptfunction(void *arg) //thread that accepts client connection and spawns child thread to handle the client request
{		
	pthread_cleanup_push(cleanup,(void *)&sockfd);
	pthread_testcancel();
	pthread_cleanup_pop(0);
	int newfd, n;
	struct sockaddr_in clientAddr;
	int addrlen = sizeof clientAddr;	
	pthread_t tid;
	acceptbundle ab;	
	memset(&clientAddr, 0, sizeof(struct sockaddr_in));
	while(1) //loop continuously to accept child connection
	{				
		if ((newfd = accept(sockfd,(struct sockaddr *)&clientAddr,(socklen_t *)&addrlen))==-1)
		{							
			pthread_cleanup_push(cleanup,(void *)&sockfd);
			pthread_testcancel();
			pthread_cleanup_pop(0);
			perror("accept");
			exit(-1);			
		}
		inet_ntop(clientAddr.sin_family,get_in_addr((struct sockaddr *)&clientAddr),ab.ip, sizeof ab.ip); //resolve the ip address of the server
		ab.fd = (int *)malloc(sizeof(int));		
		*ab.fd = newfd;
		pthread_create(&tid,NULL,processclient,(void *)&ab); //spawn child thread to handle the client request
		pthread_mutex_lock(&LOCK);
		threadID[threadcount]=tid;
		threadcount++;					
		pthread_mutex_unlock(&LOCK);	
		pthread_cleanup_push(cleanup,(void *)&sockfd);
		pthread_testcancel();
		pthread_cleanup_pop(0);
    }
	shutdown(sockfd,SHUT_RDWR);
	close(sockfd);
	pthread_exit(NULL);
}

void *terminate(void *arg) //thread that terminates the server on timeout
{	
	int i;	
	for (i=0;i<tme;i++)		
		sleep(1);
	
	pthread_mutex_lock(&LOCK);
	for (i=0;i<threadcount;i++)
	{		
		pthread_testcancel();	
		if (threadID[i]!=pthread_self())
		{
			pthread_cancel(threadID[i]);
		}
	}
	pthread_mutex_unlock(&LOCK);	
	pthread_exit(NULL);
}

void siginthandler(int signum) //handler to handle the SIGINT signal
{
	int i;	
	pthread_mutex_lock(&LOCK);
	for (i=0;i<threadcount;i++)
		pthread_cancel(threadID[i]);
	pthread_mutex_unlock(&LOCK);
}

int main(int argc, char *argv[]) //main function
{
	struct sigaction action;
	struct linger ling;
	int i,j,tflag,dflag,mflag,len,prtpos;
	int jn;
	int count = 0;
	pthread_t tid;
	int *fd;
	void *status;
	int options;
	int msg_buf_sz;		
	char buffer[100];
	char p[INET6_ADDRSTRLEN];
	struct sockaddr_in serverAddr;
	dlay = 0;
	tme = 60;
	options = 1;

	action.sa_handler = (void (*)(int))siginthandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGINT,&action,NULL) == -1) //set up the SIGINT handler
		cout<<"\n Error in Signal SIGINT Handling "<<endl;	

	strcpy(mval,"n");	
	tflag=0;
	mflag=0;
	dflag=0;	
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
		
	/* The Following checks if the command line input is correct */
	if (argc<=1)
	{
		printf("\n Insufficient Command Line Arguments");
		cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
		exit(0);
	}

		if (argc>7)
	{
		printf("\n Excessive Command Line Arguments");
		cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
		exit(0);
	}

	else if (argc == 2)
	{		
		if (!isnum(argv[argc-1],strlen(argv[argc-1])))
		{			
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
		
	}

	else if (argc == 3)
	{
		if (strcmp(argv[1],"-m")!=0)
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
		if (!isnum(argv[2],strlen(argv[2])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
	}

	else if (argc == 4)
	{
		if (strcmp(argv[1],"-t")!=0&&strcmp(argv[1],"-d")!=0)
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
		if (strcmp(argv[1],"-t")==0||strcmp(argv[1],"-d")==0)
		{
			if (!isnum(argv[2],strlen(argv[2])))
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}
		}
		if (!isnum(argv[3],strlen(argv[3])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
	}

	else if (argc == 5)
	{
		if (strcmp(argv[1],"-m")==0)
		{
			if (strcmp(argv[2],"-t")!=0&&strcmp(argv[2],"-d")!=0)
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);	
			}
			if (!isnum(argv[3],strlen(argv[3])))
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}			
		}
		if (strcmp(argv[3],"-m")==0)
		{
			if (strcmp(argv[1],"-t")!=0&&strcmp(argv[1],"-d")!=0)
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);	
			}
			if (!isnum(argv[2],strlen(argv[2])))
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}			
		}
		if (!isnum(argv[4],strlen(argv[4])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
	}

	else if (argc == 6)
	{
		if (strcmp(argv[1],"-t")!=0&&strcmp(argv[1],"-d")!=0)
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
		if (strcmp(argv[3],"-t")!=0&&strcmp(argv[3],"-d")!=0)
		{			
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}			
		if (!isnum(argv[2],strlen(argv[2])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
		if (!isnum(argv[4],strlen(argv[4])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
		if (!isnum(argv[5],strlen(argv[5])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}

	}

	else if (argc == 7 )
	{
		if (strcmp(argv[1],"-m")==0)
		{
			if (strcmp(argv[2],"-t")!=0&&strcmp(argv[2],"-d")!=0&&strcmp(argv[4],"-t")!=0&&strcmp(argv[4],"-d")!=0)
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);	
			}
			if (!isnum(argv[3],strlen(argv[3])))
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}			
			if (!isnum(argv[5],strlen(argv[5])))
			{	
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}
		}
		if (strcmp(argv[3],"-m")==0)
		{
			if (strcmp(argv[1],"-t")!=0&&strcmp(argv[1],"-d")!=0&&strcmp(argv[4],"-t")!=0&&strcmp(argv[4],"-d")!=0)
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);	
			}
			if (!isnum(argv[2],strlen(argv[2])))
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}			
			if (!isnum(argv[5],strlen(argv[5])))
			{	
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}
		}
		if (strcmp(argv[5],"-m")==0)
		{
			if (strcmp(argv[1],"-t")!=0&&strcmp(argv[1],"-d")!=0&&strcmp(argv[3],"-t")!=0&&strcmp(argv[3],"-d")!=0)
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);	
			}
			if (!isnum(argv[2],strlen(argv[2])))
			{
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}			
			if (!isnum(argv[4],strlen(argv[4])))
			{	
				cout<<"\n Error!!! Incorrect command line arguments ";
				cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
				exit(0);
			}
		}
		if (!isnum(argv[6],strlen(argv[6])))
		{
			cout<<"\n Error!!! Incorrect command line arguments ";
			cout<<"\n Usage: ./server [-t seconds] [-d delay] [-m] port "<<endl;
			exit(0);
		}
	}

		/* The following code resolves the command line arguments */
		for (i=1;i<argc;i++)
		{		
			if (count==3)
				break;
			if ((strcmp(argv[i],"-t")==0)&&(tflag==0))
			{
				tme = getnum(argv[i+1],strlen(argv[i+1]));
				i=0;
				tflag=1;
				count++;
				continue;
			}
			if ((strcmp(argv[i],"-d")==0)&&(dflag==0))
			{
				dlay = getnum(argv[i+1],strlen(argv[i+1]));
				i=0;
				dflag=1;
				count++;
				continue;
			}
			if ((strcmp(argv[i],"-m")==0)&&(mflag==0))
			{
				strcpy(mval,"y");
				i=0;
				mflag=1;
				count++;
				continue;
			}
		}	
		port = getnum(argv[argc-1],strlen(argv[argc-1]));
	
	if (port<1024) //port must be above 1023
	{
		cout<<"\n Bad Port "<<endl;
		exit(0);
	}
		
	pthread_create(&tid,NULL,terminate,NULL); //spawn the terminate thread that exits server after timeout
	pthread_mutex_lock(&LOCK);
	threadID[threadcount]=tid;
	threadcount++;	
	pthread_mutex_unlock(&LOCK);		
	
	if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) //create the socket
	{
		perror("socket");
	    exit(-1);
    }
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(void *)&options,sizeof(int)); //set the socketoptions

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((unsigned short)port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(sockfd,(struct sockaddr *)&serverAddr,(socklen_t)sizeof(serverAddr))==-1) //bind to the port
    {
		perror("bind");
		exit(-1);
    }	

	if (listen(sockfd,50)==-1) //listen for connections
    {
		perror("listen");
	    exit(-1);
    }

	pthread_mutex_lock(&LOCK);
	pthread_create(&tid,NULL,acceptfunction,NULL); //spawn the accept thread that accepts client connections
	threadID[threadcount]=tid;
	threadcount++;
	pthread_mutex_unlock(&LOCK);

	for (jn=0;jn<threadcount;jn++)	//calls a join on all the threads that are alive
		pthread_join(threadID[jn],&status);
	return 0;
}
