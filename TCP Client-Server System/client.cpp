#include <iostream.h>
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
#include <openssl/md5.h>
#include "common.h"

#define MAXDATASIZE 100
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

int main(int argc, char *argv[]) //client main function
{
	struct hostent *he;
	unsigned char *md;
	unsigned long ln;
	int bytesreceived;
	char *strng;
	char *msg_buf;
	char *sz;
	int size,tsize;
	int val;
	char *s;
	char *t1;
	char *t2;
	char *t3;
	MD5_CTX c;
	char *hname;
	char p[INET6_ADDRSTRLEN];
	char prt[10];
	char *temp;
	ReqMsg msg;
	int i,j,reqflag,oflag,mflag,n,msg_buf_sz,flag,chkflag;
	int count = 0;
	int offset,port,req,tmp,len,len1,len2;
	char mval[2];
	mval[0]='n';
	mval[1]='\0';
	chkflag = 0;
	flag = 0;
	reqflag=0;
	mflag=0;
	oflag=0;
	offset=0;
	len=0;
	len1=0;
	len2=0;
	bytesreceived = 0;
	struct sockaddr_in serverAddr;
	int sockfd, addrlen = sizeof(serverAddr);
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	memset(&msg,0,sizeof(ReqMsg));

	/* the following checks if the command line arguments are correct */
	if (argc<4)
	{
		printf("\n\t Insufficient Command Line Arguments");
		cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
		exit(0);
	}
	if (argc>7)
	{
		printf("\n\t Excessive Command Line Arguments");
		cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
		exit(0);
	}
	
	if (strcmp(argv[1],"adr")!=0&&strcmp(argv[1],"get")!=0&&strcmp(argv[1],"fsz")!=0)
	{
		cout<<"\n\t Error!!! Incorrect command line arguments ";
		cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
		exit(0);
	}
	
	for (i=2;i<4;i++)
	{
		if (strcmp(argv[i],"-o")==0&&!isnum(argv[i+1],strlen(argv[i+1])))
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
	}

	t1 = (char *)malloc(strlen(argv[argc-2]));;
	strcpy(t1,argv[argc-2]);
	for (i=0;i<strlen(t1);i++)
	{
		if (t1[i]==':')
		{
			flag = 1;
			break;
		}
	}

	if (flag == 0)
	{
		cout<<"\n\t Error!!! Incorrect command line arguments ";
		cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
		exit(0);
	}
	
	else if (flag == 1)
	{		
		for (i=0;i<strlen(t1);i++)
		{
			if (t1[i]==':')
				break;
			else
				len1++;
		}
		len2=strlen(t1)-len1-1;	
		t2 = (char *)malloc(len1);
		t3 = (char *)malloc(len2);
		for (i=0,j=0;i<strlen(t1);i++)
		{			
			if (t1[i]==':')
				continue;
			if (t1[i-j-1]==':')
			{
				t3[j]=t1[i];
				j++;
				continue;
			}
			if (t1[i]!=':')
				t2[i]=t1[i];			
		}
		if (!isnum(t3,strlen(t3)))
		{
			cout<<"\n\t Error!!! Incorrect command line arguments - port incorrect ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
		if (gethostbyname(t2)==NULL)
		{
			cout<<"\n\t Error!!! Incorrect command line arguments - hostname incorrect ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
	}

	if (argc == 5)
	{
		if (strcmp(argv[2],"-m")!=0)
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
	}

	if (argc == 6)
	{
		if (strcmp(argv[2],"-o")!=0)
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
		if (!isnum(argv[3],strlen(argv[3])))
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
	}
	
	if (argc == 7)
	{
		if (strcmp(argv[2],"-m")!=0&&strcmp(argv[4],"-m")!=0)
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
			exit(0);
		}
		if (strcmp(argv[2],"-m")==0)
		{
			if (strcmp(argv[3],"-o")!=0)
			{
				cout<<"\n\t Error!!! Incorrect command line arguments ";
				cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
				exit(0);
			}
		}
		if (strcmp(argv[4],"-m")==0)
		{
			if (strcmp(argv[2],"-o")!=0)
			{
				cout<<"\n\t Error!!! Incorrect command line arguments ";
				cout<<"\n\t Usage: ./client {adr|fzs|get} [-o offset] [-m] hostname:port string"<<endl;
				exit(0);
			}
		}
	}

		/* the following resolves the command line arguments*/
		for (i=1;i<argc;i++)
		{		
			if (count>=5)
				break;
			if ((strcmp(argv[i],"adr")==0)&&(reqflag==0))
			{
				req = ADR_REQ;
				i=0;
				reqflag=1;
				count++;
				continue;
			}
			if ((strcmp(argv[i],"fsz")==0)&&(reqflag==0))
			{
				req = FSZ_REQ;
				i=0;
				reqflag=1;
				count++;
				continue;
			}
			if ((strcmp(argv[i],"get")==0)&&(reqflag==0))
			{
				req = GET_REQ;
				i=0;
				reqflag=1;
				count++;
				continue;
			}
			if ((strcmp(argv[i],"-o")==0)&&(oflag==0))
			{
				offset = getnum(argv[i+1],strlen(argv[i+1]));
				i=0;
				oflag=1;
				count++;
				continue;
			}
			if ((strcmp(argv[i],"-m")==0)&&(mflag==0))
			{
				mval[0]='y';
				mval[1]='\0';
				i=0;
				mflag=1;
				count++;
				continue;
			}
			temp = (char *)malloc(strlen(argv[argc-2]));;
			strcpy(temp,argv[argc-2]);
			strng = (char *)malloc(strlen(argv[argc-1]));
			strcpy(strng,argv[argc-1]);
		}	
		
		for (i=0;i<strlen(temp);i++)
		{
			if (temp[i]==':')
				break;
			else
				len++;
		}
		hname = (char *)malloc(len);
		for (i=0,j=0;i<strlen(temp);i++)
		{			
			if (temp[i]==':')
				continue;
			if (temp[i-j-1]==':')
			{
				prt[j]=temp[i];
				j++;
				continue;
			}
			if (temp[i]!=':')
				hname[i]=temp[i];			
		}
		prt[strlen(prt)]='\0';
		port = getnum(prt,strlen(prt));
	
	if (port<1024) //port must be above 1023
	{
		cout<<"\n\t Bad Port "<<endl;
		exit(0);
	}

	if (req == ADR_REQ)
	{	
		if (oflag == 1)
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client adr [-m] hostname:port url"<<endl;
			exit(-1);
		}
	}

	if (req == FSZ_REQ)
	{
		if (oflag == 1)
		{
			cout<<"\n\t Error!!! Incorrect command line arguments ";
			cout<<"\n\t Usage: ./client fsz [-m] hostname:port filename"<<endl;
			exit(-1);
		}
	}

	if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) //create the socket
    {
		perror("socket");
		exit(-1);
    }
	serverAddr.sin_family =AF_INET;
	serverAddr.sin_port = htons((unsigned short)port);
	he = gethostbyname(hname);			
	serverAddr.sin_addr = *((struct in_addr *)he->h_addr);
	if (connect(sockfd,(struct sockaddr*)&serverAddr,(socklen_t)sizeof(serverAddr))==-1) //connect to the client
    {
		perror("connect");
		exit(-1);
    }
	inet_ntop(serverAddr.sin_family,get_in_addr((struct sockaddr *)&serverAddr),p, sizeof p); //resolve the ip address of the server

	/* Populate the message structure based on the request type */
	switch(req)
	{
		case ADR_REQ:	msg.MsgType=htons((unsigned short)0xfe10);
						msg.Offset=htonl((unsigned int)offset);
						msg.Data=strng;
						tmp=strlen(msg.Data);
						msg.DataLen=htonl((unsigned int)tmp);						
						break;					

		case FSZ_REQ:	msg.MsgType=htons((unsigned short)0xfe20);
						msg.Offset=htonl((unsigned int)offset);
						msg.Data=strng;
						tmp=strlen(msg.Data);
						msg.DataLen=htonl((unsigned int)tmp);						
						break;

		case GET_REQ:	msg.MsgType=htons((unsigned short)0xfe30);
						msg.Offset=htonl((unsigned int)offset);
						msg.Data=strng;
						tmp=strlen(msg.Data);
						msg.DataLen=htonl((unsigned int)tmp);						
						break;

		default:		msg.MsgType=htons((unsigned short)0x0000);
						msg.Offset=htonl((unsigned int)offset);
						msg.Data=strng;
						tmp=strlen(msg.Data);
						msg.DataLen=htonl((unsigned int)tmp);						
						break;
	}
	
	msg_buf_sz = 10+strlen(msg.Data)+1;	
	msg_buf = (char *)malloc(msg_buf_sz);
	memset(msg_buf,0,msg_buf_sz);
	memcpy(&msg_buf[0],&msg.MsgType,sizeof(unsigned short));
	memcpy(&msg_buf[sizeof(unsigned short)],&msg.Offset,sizeof(unsigned int));
	memcpy(&msg_buf[sizeof(unsigned short)+sizeof(unsigned int)],&msg.DataLen,sizeof(unsigned int));
	strcpy(&msg_buf[sizeof(unsigned short)+2*sizeof(unsigned int)],msg.Data);
	for (i=0;i<sizeof(unsigned short)+2*sizeof(unsigned int);i++) //send the header to the server
	{
		if ((send(sockfd,&msg_buf[i],1,0))==-1)
			perror("send");
	}
	
	for (i=(sizeof(unsigned short)+2*sizeof(unsigned int));i<msg_buf_sz;i++) //send the data to the server
	{
		if (send(sockfd,&msg_buf[i],1,0)==-1)
			perror("send");
	}

	ReqMsg rmsg,rtmsg;
	char *rmsg_buf;
	unsigned char *printing;
	int rmsg_buf_sz;
	char *buff;
	int pr;
	memset(&rmsg,0,sizeof(rmsg));
	memset(&rtmsg,0,sizeof(rtmsg));
	printing = (unsigned char *)malloc(10);

	s=(char *)malloc(10);
	for (i=0;i<10;i++) //read the header of the reply from the server
	{
		if (recv(sockfd,&s[i],1,0)==0)
		{
			cout<<"\n Something Went Wrong "<<endl;
			exit(0);
		}
		bytesreceived++;
	}

	
	memcpy(&rtmsg.MsgType,&s[0],sizeof(unsigned short));
	memcpy(&rtmsg.Offset,&s[sizeof(unsigned short)],sizeof(unsigned int));
	memcpy(&rtmsg.DataLen,&s[sizeof(unsigned short)+sizeof(unsigned int)],sizeof(unsigned int));
	rmsg.MsgType = ntohs((unsigned short)rtmsg.MsgType);
	rmsg.Offset = ntohl((unsigned int)rtmsg.Offset);
	rmsg.DataLen = ntohl((unsigned int)rtmsg.DataLen);	
	buff = (char *)malloc(rmsg.DataLen);

	if (rmsg.MsgType == ADR_RPLY) //read the ip address sent by the server
	{
		for (i=0;i<rmsg.DataLen;i++)
		{
			if (recv(sockfd,&buff[i],1,0)==0)
			{
				cout<<"\n Something Went Wrong "<<endl;
				exit(0);
			}
			bytesreceived++;
		}
		buff[rmsg.DataLen]='\0';		
		rmsg.Data=buff;
	}	
	
	else if (rmsg.MsgType == FSZ_RPLY) //read the file size sent by the server
	{
		
		s=(char *)malloc(rmsg.DataLen);
		for (i=0;i<rmsg.DataLen;i++)
		{
			if (recv(sockfd,&s[i],1,0)==0)
			{
				cout<<"\n Something Went Wrong "<<endl;
				exit(0);
			}
			bytesreceived++;
		}		
		s[rmsg.DataLen]='\0';					
	}	

	else if (rmsg.MsgType == GET_RPLY) //read the file bytes sent by the server and calculate the md5
	{
		size = rmsg.DataLen;
		MD5_Init(&c); //initialize md5
		//set up the buffer size of maximum 512
		if (size<512)
		{
			rmsg_buf=(char *)malloc(size);
			val = size;
		}
		else
		{
			rmsg_buf=(char *)malloc(512);
			val = 512;
		}
		ln = 1;
		for (i=0;i<val;i++) //read val bytes from the server - max 512
		{
			t2++;
			if (t2==t1)
				break;
			if (recv(sockfd,&rmsg_buf[i],1,0)==0)
			{
				cout<<"\n Something Went Wrong "<<endl;
				exit(0);
			}
			bytesreceived++;
			MD5_Update(&c,&rmsg_buf[i],ln); //uodate md5
			if (i==(val-1))
			{
				i=-1;				
				size = size-val;
				if (size>512)
					val = 512;
				else
					val = size;
				rmsg_buf=(char *)malloc(val);
			}			
		}
		md = (unsigned char *)malloc(16);
		MD5_Final(md,&c); //finalize md5
	}	

	if (strcmp(mval,"y")==0) //print all the relevant information to stdout
	{
		memcpy(&printing[0],&rmsg.MsgType,sizeof(unsigned short));
		memcpy(&printing[sizeof(short)],&rmsg.Offset,sizeof(unsigned int));
		memcpy(&printing[sizeof(short)+sizeof(int)],&rmsg.DataLen,sizeof(unsigned int));				
		cout<<"\n\t Received "<<bytesreceived<<" bytes from "<<p;			
		printf("\n\t MsgType: 0x");		
		for (pr=0;pr<sizeof(unsigned short);pr++)
		{
			printf("%02x",printing[pr]);
		}
		printf("\n\t Offset: 0x");
		for (pr=sizeof(unsigned short);pr<sizeof(unsigned short)+sizeof(unsigned int);pr++)
			printf("%02x",printing[pr]);
		printf("\n\t DataLength: 0x");
		for (pr=sizeof(unsigned short)+sizeof(unsigned int);pr<sizeof(unsigned short)+2*sizeof(unsigned int);pr++)
			printf("%02x",printing[pr]);	
		if (rmsg.MsgType == ADR_RPLY)
		{
			cout<<"\n\t ADDR = "<<rmsg.Data;
		}
		else if (rmsg.MsgType == ADR_FAIL)
		{
			cout<<"\n\t ADDR request for '"<<msg.Data<<"' failed.";
		}
		else if (rmsg.MsgType == FSZ_RPLY)
		{
			cout<<"\n\t FILESIZE = "<<s;
		}
		else if (rmsg.MsgType == FSZ_FAIL)
		{
			cout<<"\n\t FILESIZE request for '"<<msg.Data<<"' failed.";
		}
		else if (rmsg.MsgType == GET_RPLY)
		{
			cout<<"\n\t FILESIZE = "<<rmsg.DataLen<<", MD5 = ";
			if (rmsg.DataLen!=0)
			{
				for (i=0;i<16;i++)
					printf("%02x",md[i]);			
			}
			else if (rmsg.DataLen==0)
			{
				for (i=0;i<16;i++)
					printf("%02x",0);
			}
		}
		else if (rmsg.MsgType == GET_FAIL)
		{
			cout<<"\n\t GET request for '"<<msg.Data<<"' failed.";
		}
		else
			cout<<"\n\t ERROR!!! Request not compatible with server specifications";
		printf("\n");
	}
	close(sockfd);
	return 0;
}
