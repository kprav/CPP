#include "node.h"

//Declaring Constant Values for message headers
const uint8_t JNRQ = (uint8_t)0xFC;
const uint8_t JNRS = (uint8_t)0xFB;
const uint8_t HLLO = (uint8_t)0xFA;
const uint8_t KPAV = (uint8_t)0xF8;
const uint8_t NTFY = (uint8_t)0xF7;
const uint8_t CKRQ = (uint8_t)0xF6;
const uint8_t CKRS = (uint8_t)0xF5;
const uint8_t SHRQ = (uint8_t)0xEC;
const uint8_t SHRS = (uint8_t)0xEB;
const uint8_t GTRQ = (uint8_t)0xDC;
const uint8_t GTRS = (uint8_t)0xDB;
const uint8_t STOR = (uint8_t)0xCC;
const uint8_t DELT = (uint8_t)0xBC;
const uint8_t STRQ = (uint8_t)0xAC;
const uint8_t STRS = (uint8_t)0xAB;

const uint8_t stNReq = (uint8_t)0x01;
const uint8_t stFReq = (uint8_t)0x02;

const uint8_t type1 = (uint8_t)0x01;
const uint8_t type2 = (uint8_t)0x02;
const uint8_t type3 = (uint8_t)0x03;



/****************************************************************
Function CheckNSend:
Parameters : (char *)hostname:port
Called by permConnSender to see if there are messages to be sent.
*****************************************************************/
struct connData checkNSend(char *cmp)
{
	struct connData msg;
	msg.sockDesc = -20;
	pthread_mutex_lock(&sendQLock);
	for(unsigned int j = 0; j < sendQ.size() ; j++)
	{
			msg = sendQ[j];
			if(!strncmp(msg.peerNode, cmp, strlen(msg.peerNode)))
			{
				sendQ.erase(sendQ.begin() + j);
				j = sendQ.size() + 1;
			}
			else
			{
				msg.sockDesc = -20;
			}
	 }
	 pthread_mutex_unlock(&sendQLock);

	 return msg;
 }

/**********************************************************************
Tie breaker function. Used to check if node sending HELLO is
already connected.
**********************************************************************/
short connExists(char *peerInfo)
{
	string str(peerInfo);
	int peer1Port;
	short presnt = 0;

	size_t fPos = str.find_first_of(":");


	sscanf((str.substr(fPos+1, str.size())).c_str(), "%d", &peer1Port);

	pthread_mutex_lock(&existConnLock);
	for(unsigned int i=0; i < existConn.size() ; i++)
	{
		if(!strncmp(existConn[i].nodeInfo, peerInfo, strlen(peerInfo)))
		{
			if(peer1Port < node.port)
			{
				if(!existConn[i].myConn)
				{
					sleep(1);
					shutdown(existConn[i].peerSock, SHUT_RDWR);
					close(existConn[i].peerSock);
					presnt = 0;

					existConn.erase(existConn.begin() + i);
				}
				else
				{
					presnt = 1;
				}

				i = existConn.size() +1 ;
			}
			else if(peer1Port > node.port)
			{
				if(existConn[i].myConn)
				{
					sleep(1);
					shutdown(existConn[i].peerSock, SHUT_RDWR);
					close(existConn[i].peerSock);
					presnt = 0;

					existConn.erase(existConn.begin() + i);
				}
				else
				{
					presnt = 1;
				}

				i = existConn.size() +1 ;
			}
			else
			{
				if(strncmp(peerInfo, existConn[i].nodeInfo, strlen(peerInfo)) > 0)
				{
					presnt = 1;
					i = existConn.size()+1 ;
				}
				else
				{
					sleep(1);
					shutdown(existConn[i].peerSock, SHUT_RDWR);
					close(existConn[i].peerSock);
					i = existConn.size()+1 ;

					existConn.erase(existConn.begin() + i);
				}
			}
		}
	}
	pthread_mutex_unlock(&existConnLock);
	return presnt;
}

/************************************************************************
Cleanup function. Called when permConnSender exits. Closes socket
and frees up memory.
************************************************************************/
void senderCleanup(void *arg)
{
	struct connData *freeStr = (struct connData *)arg;

	shutdown(freeStr->sockDesc,SHUT_RDWR);
	close(freeStr->sockDesc);

	pthread_mutex_lock(&existConnLock);
	for(unsigned int j=0 ; j<existConn.size() ; j++)
	{
		if(!strncmp(existConn[j].nodeInfo, freeStr->peerNode, strlen(existConn[j].nodeInfo)))
		{
			existConn.erase(existConn.begin()+j);
			break;
		}
	}
	pthread_mutex_unlock(&existConnLock);

	  free(freeStr->peerNode);
  	  free(freeStr);

}

/**********************************************************************
Generic Sender thread. spawned for all connections to external nodes.
Checks sendQ for messages to send by calling checkNSend.
sleeps for 300ms if no messages to send.
**********************************************************************/
void * permConnSender(void *arg)
{

	struct connData toSend;
	struct connData *sendSock = (struct connData *)arg;
	short SEND = 0;
	int temp, k =0 ;
	int bytesSent = 0 ;
	int bytesToSend = 0;
	int bytesRmn;

pthread_cleanup_push(senderCleanup, arg);
while(1)
 {
	pthread_mutex_lock(&existConnLock);
	if (sendSock->peerContact< -5)
	{
	}
    pthread_mutex_unlock(&existConnLock);

	pthread_testcancel();
	toSend = checkNSend(sendSock->peerNode);
	bytesSent = 0;

	if(toSend.sockDesc == -20)//-20 Dummy value implies no message to send
	{
		SEND = 0;
	}
	else
	{
		pthread_mutex_lock(&existConnLock);
		sendSock->keepAlive = node.keepAliveTimeOut-3;
		pthread_mutex_unlock(&existConnLock);

		bytesToSend = (int)toSend.dataLen/CHUNK;
		bytesRmn = toSend.dataLen%CHUNK;

			while( k < bytesToSend && ((temp=send(sendSock->sockDesc, &toSend.sendBuf[bytesSent], CHUNK, 0)) != -1))
			{
				k++;

				if(temp > 0)
				 bytesSent += temp;
			}

				if(temp == -1)
				  perror("send");

			if(bytesRmn > 0)
			{
				temp = send(sendSock->sockDesc, &toSend.sendBuf[bytesSent], bytesRmn, 0);

				if(temp > 0)
				  bytesSent += temp;
			}

			while(bytesSent < toSend.dataLen && (temp = send(sendSock->sockDesc, &toSend.sendBuf[bytesSent], sizeof(char), 0)) != -1)
			{
				if(temp > 0)
				 bytesSent += temp;
			}

			if ((uint8_t)toSend.sendBuf[0]==NTFY)//IF NTFY sent, no more to send, exit
			{
				break;
			}
	}

	if(!SEND)
	{
		usleep(SLEEPER);
	}

  }

  pthread_cleanup_pop(1);

  	shutdown(sendSock->sockDesc,SHUT_RDWR);
	close(sendSock->sockDesc);

    pthread_exit(NULL);
}

/***************************************************************
Generic receiver thread. spawned for each unique connection.
spawns permConnSender.
***************************************************************/
void *permConnReader(void *arg)
{
	connData *sendData  = (struct connData*)arg;
	pthread_t thrid;
	bool returnError = false;

	unsigned char recvHdr[headerLen];
	int bytesToRcv, bytesRmn;
	int bytesRcvd;
	bool sameMsg = false;

	pthread_mutex_lock(&thrIDLock);
	pthread_create(&thrid, NULL, permConnSender, sendData);
	threadID.push_back(thrid);
	pthread_mutex_unlock(&thrIDLock);

	while(!terminateNode)
	{
		MsgDetails msg;
		bytesToRcv= (int)headerLen/CHUNK;
		bytesRmn = headerLen%CHUNK;

		int i = 0;
		int temp;
		bytesRcvd = 0;
		sameMsg = false;
		int k = 0;

		while( k < bytesToRcv && (temp = recv(sendData->sockDesc, &recvHdr[bytesRcvd], CHUNK, 0)) != 0 && !terminateNode)
		{
			if( temp != -1)
			{
				bytesRcvd += temp;
				k++;
			}
		}

		if(bytesRmn > 0)
		{
			temp = recv(sendData->sockDesc, &recvHdr[bytesRcvd], bytesRmn, 0);

			if(temp > 0)
			 bytesRcvd += temp;
		 }

		 while(bytesRcvd < headerLen && (temp = recv(sendData->sockDesc, &recvHdr[bytesRcvd], sizeof(char), 0)) != 0 && !terminateNode)
		 {
			 if(temp > 0)
			  bytesRcvd += temp;
		 }

		if(sendData->gotHello && terminateNode)
		{
			sendNotify(sendData->peerNode ,exitID);
			sleep(3);
			pthread_cancel(thrid);
			pthread_exit(NULL);
		}

		if(temp == 0 )
		{
			struct timeval pt;
			gettimeofday(&pt, NULL);

			if(!terminateNode && sendData->gotHello && !IAMBEACON)//Sending Check Message
			{
				struct MsgDetails cMsg;
				cMsg.messageType = CKRQ;
				cMsg.nodeID = (char *)malloc(sizeof(char));
				memset(cMsg.nodeID, '\0', sizeof(char));

				pthread_mutex_lock(&processQLock);
				processQ.push(cMsg);
				pthread_mutex_unlock(&processQLock);
				pthread_cond_signal(&processQWait);
			}

			pthread_cancel(thrid);
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&existConnLock);
		sendData->keepAlive = node.keepAliveTimeOut-3;
		pthread_mutex_unlock(&existConnLock);

		//Received Header - Parsing
		memcpy(&msg.messageType, &recvHdr[0], sizeof(uint8_t));
		memcpy(&msg.UOID, &recvHdr[sizeof(uint8_t)], sizeof msg.UOID);
		memcpy(&msg.TTL, &recvHdr[sizeof(uint8_t) + sizeof msg.UOID], sizeof(uint8_t));
		memcpy(&msg.rsrv, &recvHdr[2*sizeof(uint8_t) + sizeof msg.UOID], sizeof(uint8_t));
		memcpy(&msg.dataLength, &recvHdr[3*sizeof(uint8_t) + sizeof msg.UOID], sizeof(int));

		// Status Response Processing ***********************************************
		if (msg.messageType == STRS && ntohl(msg.dataLength)>MAXBUFFERSIZE)
		{
			bytesRcvd = 0;
			short prt;
			short info;
			char *hn;
			int metaDataLen;
			msg.data = (char *)malloc(24);

			while(bytesRcvd < 24 && (temp = recv(sendData->sockDesc, msg.data, 24, 0)) != 0)
			{
				bytesRcvd += temp;
			}
			bytesRcvd = 0;			


			char rspUOID[20];
			memcpy(rspUOID, &msg.data[0], sizeof rspUOID);

			msg.nodeID = (char *)malloc(strlen(sendData->peerNode));
			strcpy(msg.nodeID,sendData->peerNode);
			msg.kind = 1;
			pthread_mutex_lock(&logLock);
			logQueue.push(msg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);

			if(!memcmp(rspUOID, myStatUOID, sizeof rspUOID))
			{
				memcpy(&info,&msg.data[20],sizeof(short));
				msg.data = (char *)realloc(msg.data,24+info-2);
				

				while(bytesRcvd < info-2 && (temp = recv(sendData->sockDesc, &msg.data[24], info-2, 0)) != 0)
				{
					bytesRcvd += temp;
				}
				bytesRcvd = 0;

				hn = new char[info-1];
				memcpy(&prt,&msg.data[22],sizeof(short));
				memcpy(&hn,&msg.data[24],info-2);
				fstream fp;
				ifstream ip;				

				metaDataLen = ntohl(msg.dataLength)-(24+info-2);
				char *dummyFile;
				dummyFile = (char *)malloc(strlen(myHome)+1+strlen("$Dummy_File"));
				strcpy(dummyFile, myHome);
				strcat(dummyFile, "/");
				strcat(dummyFile, "$Dummy_File");
				fp.open(dummyFile,ios::out);

				int recordLen;
				int count = 0;
				int received = 0;
				while (received < metaDataLen)
				{
					char *lenOfRecord = (char *)malloc(sizeof(int));
					

					while((unsigned int)bytesRcvd < sizeof(int) && (temp = recv(sendData->sockDesc, lenOfRecord, sizeof(int), 0)) != 0)
					{
						bytesRcvd += temp;
					}
					bytesRcvd = 0;

					memcpy(&recordLen,lenOfRecord,sizeof(int));
					received += sizeof(int);
					free(lenOfRecord);
					if (recordLen == 0)
					{
						recordLen = ntohl(msg.dataLength)-20-2-info-received;
					}
					char *writeToFile = new char[recordLen];

					while(bytesRcvd < recordLen && (temp = recv(sendData->sockDesc, writeToFile, recordLen, 0)) != 0)
					{
						bytesRcvd += temp;
					}
					bytesRcvd = 0;
					
					fp.write(writeToFile, recordLen);
					fp.flush();

					received += recordLen;
					count ++;
					delete[] writeToFile;
				}
				fp.close();
				ip.open(dummyFile,ios::in);
				fp.open(statOp, ios::in);
				if(!fp.is_open())
				{
					fp.close();
				}
				else
				{
					fp.close();
					remove(statOp);
				}
				fp.open(statOp,ios::out);
				fp.write(hn,strlen(hn));
				fp<<":"<<prt<<" has "<<count<<" files"<<endl;
				while(!ip.eof())
				{
					char *readFromFile = new char[MAXBUFFERSIZE];
					ip.read(readFromFile, MAXBUFFERSIZE);
					fp.write(readFromFile, strlen(readFromFile));
					fp.flush();
					delete[] readFromFile;
				}
				ip.close();
				fp.close();
				remove(dummyFile);
			}
			else
			{
				struct connData *fwdMsg = (struct connData *)malloc(sizeof (struct connData));
				msg.TTL = setTTL(msg.TTL);				

				fwdMsg->dataLen = (3*sizeof(uint8_t)+sizeof msg.UOID+sizeof(int)+ntohl(msg.dataLength));

				fwdMsg->sendBuf = (unsigned char *)malloc(3*sizeof(uint8_t)+sizeof msg.UOID+sizeof(int));
				memcpy(&fwdMsg->sendBuf[0], &msg.messageType, sizeof(uint8_t));
				memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)], msg.UOID, sizeof msg.UOID);
				memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)+sizeof msg.UOID], &msg.TTL, sizeof(uint8_t));
				memcpy(&fwdMsg->sendBuf[2*sizeof(uint8_t)+sizeof msg.UOID], &msg.rsrv, sizeof(uint8_t));
				memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof msg.UOID], &msg.dataLength, sizeof(int));

				pthread_mutex_lock(&timerLock);
				for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
				{
					if(!memcmp(rspUOID, cacheUOID[j].msgUOID, sizeof rspUOID))
					{
						fwdMsg->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
						strcpy(fwdMsg->peerNode, cacheUOID[j].senderNode);
						pthread_mutex_lock(&sendQLock);
						if(msg.TTL > 0)
							sendQ.push_back(*fwdMsg);
						pthread_mutex_unlock(&sendQLock);

						msg.kind = 2;
						msg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
						memcpy(msg.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
						pthread_mutex_lock(&logLock);
						logQueue.push(msg);
						pthread_mutex_unlock(&logLock);
						pthread_cond_signal(&forLogging);
						break;
					}
				}
				pthread_mutex_unlock(&timerLock);


				int toRecv = ntohl(msg.dataLength)-24;
				int received = 0;
				int keepTrack = toRecv;

				while (received<toRecv)
				{
					if (received == 0)
					{
						msg.data = (char *)realloc(msg.data,MAXBUFFERSIZE);
						recv(sendData->sockDesc, &msg.data[24], MAXBUFFERSIZE-24, 0);
						received += MAXBUFFERSIZE-24;
						keepTrack -= (MAXBUFFERSIZE-24);
					}

					else
					{
						if (keepTrack>=MAXBUFFERSIZE)
						{
							msg.data = (char *)malloc(MAXBUFFERSIZE);
							recv(sendData->sockDesc, msg.data, MAXBUFFERSIZE, 0);
							received += MAXBUFFERSIZE;
							keepTrack -= (MAXBUFFERSIZE);
						}
						else
						{
							msg.data = (char *)malloc(keepTrack);
							recv(sendData->sockDesc, msg.data, keepTrack, 0);
							received += keepTrack;
							keepTrack -= (keepTrack);
						}

					}



					fwdMsg->sendBuf = (unsigned char*)malloc(strlen(msg.data));

					memcpy(fwdMsg->sendBuf, msg.data, strlen(msg.data));

					pthread_mutex_lock(&timerLock);
					for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
					{
						if(!memcmp(rspUOID, cacheUOID[j].msgUOID, sizeof rspUOID))
						{
							fwdMsg->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
							strcpy(fwdMsg->peerNode, cacheUOID[j].senderNode);
							pthread_mutex_lock(&sendQLock);
							if(msg.TTL > 0)
								sendQ.push_back(*fwdMsg);
							pthread_mutex_unlock(&sendQLock);

							break;
						}
					}
					pthread_mutex_unlock(&timerLock);
				}
				free(fwdMsg);
			}
		}

		//Store or Get Processing
		else if(msg.messageType == STOR || msg.messageType == GTRS)
		{
			returnError = false;			
			k=0;
			bytesRcvd = 0;

			char* newBuffer;

			if(ntohl(msg.dataLength) > MAXBUFFERSIZE)
				newBuffer = (char *)malloc(MAXBUFFERSIZE);
			else
				newBuffer = (char *)malloc(ntohl(msg.dataLength));

			char *metaData = (char *)malloc(strlen(".meta")+4);
			memset(metaData, '\0', strlen(".meta")+4);

			char *dataFileNme = (char *)malloc(strlen(".data")+4);
			memset(dataFileNme, '\0', strlen(".meta")+4);
			
				pthread_mutex_lock(&statLock);
				msg.data = (char *)malloc(sizeof(int));
				sprintf(msg.data,"%d",fileCount);
				sprintf(metaData, "%d", fileCount);
				sprintf(dataFileNme, "%d", fileCount);
				fileCount++;
				pthread_mutex_unlock(&statLock);

			strcat(metaData, ".meta");
			strcat(dataFileNme, ".data");


			char* metaFile = (char *)malloc(strlen(myHomeFiles) + strlen(metaData) +2);
			memset(metaFile, '\0', strlen(myHomeFiles) + strlen(metaData) +2);
			strcpy(metaFile, myHomeFiles);
			strcat(metaFile, "/");
			strcat(metaFile, metaData);

			char *dataFile = (char *)malloc(strlen(myHomeFiles) + strlen(dataFileNme) +2);
			memset(dataFile, '\0', strlen(myHomeFiles) + strlen(dataFileNme) +2);
			strcpy(dataFile, myHomeFiles);
			strcat(dataFile, "/");
			strcat(dataFile, dataFileNme);

			ofstream op;
			ofstream op1;
			int metaFileSz = 0;
			int writeOut = 0;
			int writeOut1 = 0;
			op.open(metaFile, ios::out);
			op1.open(dataFile, ios::out|ios::binary);

			temp = 0;
			char gUOID[20];

			if(msg.messageType == GTRS)
			{				
				while(bytesRcvd < 20 && (temp = recv(sendData->sockDesc, &newBuffer[0], sizeof msg.UOID, 0)) != 0)
				{
					bytesRcvd += temp;
				}				
				
				memcpy(gUOID, &newBuffer[0], sizeof msg.UOID);
				

				msg.dataLength = htonl(ntohl(msg.dataLength) - sizeof gUOID);
				bytesRcvd = 0;
			}


			bytesToRcv = (int)(ntohl(msg.dataLength))/CHUNK;
			bytesRmn   = (ntohl(msg.dataLength))%CHUNK;
			
			while(k < bytesToRcv && (temp = recv(sendData->sockDesc, &newBuffer[0], CHUNK, 0)) != 0 && !terminateNode)
			{
				if( temp != -1)
				{
					bytesRcvd += temp;
					k++;

					if(bytesRcvd > 4 && metaFileSz == 0)
					{
						memcpy(&metaFileSz, &newBuffer[0], sizeof(int));
						metaFileSz = ntohl(metaFileSz);
					}

					if((unsigned int)bytesRcvd >= (metaFileSz+sizeof(int)))
					{
					    if(writeOut < metaFileSz)
					    {
							op.write(&newBuffer[sizeof(int)+writeOut], metaFileSz);
							if (op.fail())
							{
								returnError = true;
								break;
							}
							op.flush();
							writeOut += metaFileSz;

							if((bytesRcvd - (metaFileSz+sizeof(int))) > 0)
							{
								op1.write(&newBuffer[sizeof(int)+metaFileSz], bytesRcvd - metaFileSz -sizeof(int));
								if (op1.fail())
								{
									returnError = true;
									break;
								}
								op1.flush();
								writeOut1 += bytesRcvd - metaFileSz -sizeof(int);
							}
						}
						else
						{
							if((unsigned int)writeOut1 < (ntohl(msg.dataLength)-sizeof(int)-metaFileSz))
							{
								op1.write(&newBuffer[0], temp);
								if (op1.fail())
								{
									returnError = true;
									break;
								}
								op1.flush();
								writeOut1 += temp;
							}
						}
					}
				}				
			}

			if(bytesRmn > 0 && returnError == false)
			{
				temp = recv(sendData->sockDesc, &newBuffer[0], bytesRmn, 0);

				if(temp > 0)
				{
					bytesRcvd += temp;

					if(bytesRcvd > 4 && metaFileSz == 0)
					{
						memcpy(&metaFileSz, &newBuffer[0], sizeof(int));
						metaFileSz = ntohl(metaFileSz);
					}

					if((unsigned int)bytesRcvd >= (metaFileSz+sizeof(int)))
					{
					    if(writeOut < metaFileSz)
					    {
							op.write(&newBuffer[sizeof(int)+writeOut], metaFileSz);
							if (op.fail())
							{
								returnError = true;								
							}
							op.flush();
							writeOut += metaFileSz;

							if((bytesRcvd - (metaFileSz+sizeof(int))) > 0)
							{
								op1.write(&newBuffer[sizeof(int)+metaFileSz], bytesRcvd - metaFileSz -sizeof(int));
								if (op1.fail())
								{
									returnError = true;									
								}
								op1.flush();
								writeOut1 += bytesRcvd - metaFileSz -sizeof(int);
							}
						}
						else if (returnError==false)
						{
							if((unsigned int)writeOut1 < (ntohl(msg.dataLength)-sizeof(int)-metaFileSz))
							{
								op1.write(&newBuffer[0], temp);
								if (op1.fail())
								{
									returnError = true;									
								}
								op1.flush();
								writeOut1 += temp;
							}
						}
					}
				}				
			}

			while(bytesRcvd < ntohl(msg.dataLength) && (temp = recv(sendData->sockDesc, &newBuffer[0], sizeof(char), 0) !=0 ) && !terminateNode && returnError == false)
			{
				if(temp > 0)
				{
				  bytesRcvd += temp;

					if(bytesRcvd > 4 && metaFileSz == 0)
					{
						memcpy(&metaFileSz, &newBuffer[0], sizeof(int));
						metaFileSz = ntohl(metaFileSz);
					}

					if((unsigned int)bytesRcvd > (metaFileSz+sizeof(int)))
					{
					    if(writeOut < metaFileSz)
					    {
							op.write(&newBuffer[sizeof(int)+writeOut], metaFileSz);
							if (op.fail())
							{
								returnError = true;
								break;
							}
							op.flush();
							writeOut += metaFileSz;

							if((bytesRcvd - (metaFileSz+sizeof(int))) > 0)
							{
								op1.write(&newBuffer[sizeof(int)+metaFileSz], bytesRcvd - metaFileSz -sizeof(int));
								if (op1.fail())
								{
									returnError = true;
									break;
								}
								op1.flush();
								writeOut1 += bytesRcvd - metaFileSz -sizeof(int);
							}
						}
						else
						{
							if((unsigned int)writeOut1 < (ntohl(msg.dataLength)-sizeof(int)-metaFileSz))
							{
								op1.write(&newBuffer[0], temp);
								if (op1.fail())
								{
									returnError = true;
									break;
								}
								writeOut1 += temp;
								op1.flush();
							}
						}

					}
			  	}				
			}

			op.close();
			op1.close();

			if (returnError == true || op.fail())
			{
				remove(metaFile);
				remove(dataFile);
			}

			if(sendData->gotHello && terminateNode)//Duplicate logic from above to check for node exit and send check
			{
				sendNotify(sendData->peerNode ,exitID);
				sleep(3);
				pthread_cancel(thrid);
				pthread_exit(NULL);
			}

			 if(temp == 0)
			 {
				struct timeval pt;
				gettimeofday(&pt, NULL);

				if(!terminateNode && sendData->gotHello && !IAMBEACON)
				{
					struct MsgDetails cMsg;
					cMsg.messageType = CKRQ;
					cMsg.nodeID = (char *)malloc(sizeof(char));
					memset(cMsg.nodeID, '\0', sizeof(char));

					pthread_mutex_lock(&processQLock);
					processQ.push(cMsg);
					pthread_mutex_unlock(&processQLock);
					pthread_cond_signal(&processQWait);
				}

				pthread_cancel(thrid);
				pthread_exit(NULL);
			  }


				if(msg.messageType == GTRS)
				{
					free(msg.data);
					msg.data = (char *)malloc(sizeof msg.UOID+strlen(dataFile)+1);
					memcpy(msg.data, gUOID, sizeof msg.UOID);
					memcpy(&msg.data[sizeof msg.UOID], dataFile, strlen(dataFile));
					msg.data[sizeof msg.UOID+strlen(dataFile)] = '\0';

					msg.nodeID = (char *)malloc(strlen(sendData->peerNode));
					strcpy(msg.nodeID,sendData->peerNode);
					msg.kind = 1;
					pthread_mutex_lock(&logLock);
					logQueue.push(msg);
					pthread_mutex_unlock(&logLock);
					pthread_cond_signal(&forLogging);
				}
		}
		
		else
		{
			msg.data = (char *)malloc(ntohl(msg.dataLength));
			unsigned char *recvBuf = (unsigned char *)malloc(ntohl(msg.dataLength));
			bytesToRcv = (int)ntohl(msg.dataLength)/CHUNK;
			bytesRmn   = ntohl(msg.dataLength)%CHUNK;
			i=0;
			k =0;
			bytesRcvd = 0;

			while(k < bytesToRcv && (temp = recv(sendData->sockDesc, &recvBuf[bytesRcvd], CHUNK, 0)) != 0 && !terminateNode)
			{
				if( temp != -1)
				{
					bytesRcvd += temp;
					k++;
				}
			}

			if(bytesRmn > 0)
			{
				temp = recv(sendData->sockDesc, &recvBuf[bytesRcvd], bytesRmn, 0);

				if(temp > 0)
				  bytesRcvd += temp;
			}

			while(bytesRcvd < ntohl(msg.dataLength) && (temp = recv(sendData->sockDesc, &recvBuf[bytesRcvd], sizeof(char), 0) !=0 ) && !terminateNode)
			{
				if(temp > 0)
				  bytesRcvd += temp;
			 }

			if(sendData->gotHello && terminateNode)//Duplicate logic from above to check for node exit and send check
			{
				sendNotify(sendData->peerNode ,exitID);
				sleep(3);
				pthread_cancel(thrid);
				pthread_exit(NULL);
			}

			 if(temp == 0)
			 {
				struct timeval pt;
				gettimeofday(&pt, NULL);

				if(!terminateNode && sendData->gotHello && !IAMBEACON)
				{
					struct MsgDetails cMsg;
					cMsg.messageType = CKRQ;
					cMsg.nodeID = (char *)malloc(sizeof(char));
					memset(cMsg.nodeID, '\0', sizeof(char));

					pthread_mutex_lock(&processQLock);
					processQ.push(cMsg);
					pthread_mutex_unlock(&processQLock);
					pthread_cond_signal(&processQWait);
				}

				pthread_cancel(thrid);
				pthread_exit(NULL);
			  }

			memcpy(msg.data, recvBuf, ntohl(msg.dataLength));
			free(recvBuf);
		}

		if(msg.messageType == STRQ)
		{
			if(!memcmp(msg.data, &stNReq, sizeof(uint8_t)))
			{
				msg.snof = 1;
			}
			else if (!memcmp(msg.data, &stFReq, sizeof(uint8_t)))
			{
				msg.snof = 2;
			}
		}

		pthread_mutex_lock(&existConnLock);
		sendData->keepAlive = node.keepAliveTimeOut-3;
		pthread_mutex_unlock(&existConnLock);



	//Check if received message is duplicate
	pthread_mutex_lock(&timerLock);
	for(unsigned int j=0; j < cacheUOID.size(); j++)
	{
		if(!memcmp(msg.UOID, cacheUOID[j].msgUOID, sizeof msg.UOID))
		{			
			sameMsg = true;
			break;
		}
	}
	pthread_mutex_unlock(&timerLock);

	//Pushing for logging
	if (msg.messageType != STRS && msg.messageType != GTRS)
	{
		msg.nodeID = (char *)malloc(strlen(sendData->peerNode));
		strcpy(msg.nodeID,sendData->peerNode);
		msg.kind = 1;
		pthread_mutex_lock(&logLock);
		logQueue.push(msg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);
	}
	else if(msg.messageType != GTRS && ntohl(msg.dataLength) <= MAXBUFFERSIZE)
	{
		msg.nodeID = (char *)malloc(strlen(sendData->peerNode));
		strcpy(msg.nodeID,sendData->peerNode);
		msg.kind = 1;
		pthread_mutex_lock(&logLock);
		logQueue.push(msg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);
	}


	pthread_mutex_lock(&existConnLock);
    sendData->peerContact = node.keepAliveTimeOut;//update timer for keep alive
    pthread_mutex_unlock(&existConnLock);

	if(sameMsg) //drop duplicate
		continue;

	struct timerSt *pTimer = (struct timerSt*)malloc(sizeof(struct timerSt));
	memcpy(pTimer->msgUOID, msg.UOID, sizeof msg.UOID);
	pTimer->timeout = node.msgLifeTime;
	pTimer->senderNode = (char *)malloc(strlen(sendData->peerNode));
	strcpy(pTimer->senderNode, sendData->peerNode);

	//New Msg, store UOID
	pthread_mutex_lock(&timerLock);
	cacheUOID.push_back(*pTimer);
	pthread_mutex_unlock(&timerLock);

	free(pTimer);

	  if(msg.messageType == NTFY)
	  {
			pthread_mutex_lock(&existConnLock);
			for(unsigned int j=0 ; j<existConn.size() ; j++)
			{
				if(!strncmp(existConn[j].nodeInfo, sendData->peerNode, strlen(existConn[j].nodeInfo)))
				{
					existConn.erase(existConn.begin()+j);
				}
			}
			pthread_mutex_unlock(&existConnLock);


			if(!terminateNode && sendData->gotHello && !IAMBEACON)
			{
				struct MsgDetails cMsg;
				cMsg.messageType = CKRQ;
				cMsg.nodeID = (char *)malloc(sizeof(char));
				memset(cMsg.nodeID, '\0', sizeof(char));

				pthread_mutex_lock(&processQLock);
				processQ.push(cMsg);
				pthread_mutex_unlock(&processQLock);
				pthread_cond_signal(&processQWait);
			}

		  shutdown(sendData->sockDesc,SHUT_RDWR);
		  close(sendData->sockDesc);
		  pthread_cancel(thrid);
		  break;
	  }
	  else if(msg.messageType == KPAV)
	  {		  
		  //No special processing
	  }
	  else if(msg.messageType != HLLO)//Push for downstream processing
	  {
		msg.pSock = sendData->sockDesc;
		pthread_mutex_lock(&processQLock);
		processQ.push(msg);
		pthread_mutex_unlock(&processQLock);
		pthread_cond_signal(&processQWait);
	  }
	  else
	  {
		  pthread_mutex_lock(&testPrinter);
		  waitForJoin = false;
		  pthread_mutex_unlock(&testPrinter);
		  sendData->gotHello = 1;//flag for sending check - received HLLO, CTS for check
	  }	  

   }
	pthread_exit(NULL);
}

/*****************************************************************
Thread: Spawned by sendHello Function. Tries to connect to the node
passed as arg.
Exits or sets soft restart flag after unsuccessful attempts.
*****************************************************************/
void* createConn(void *arg)
{
	pthread_t thrid;
	struct hostent* peerIP;
	struct sockaddr_in peerAddr;

	connData *sendData  = (struct connData*)arg;

	string str(sendData->peerNode);
	size_t fPos = str.find_first_of(":");

	if(fPos == string::npos)//Wrong format
	{
		pthread_exit(NULL);
	}

	short peerPort = (short)(atoi((str.substr(fPos+1, str.size())).c_str()));
	char *peerName = (char *)malloc((str.substr(0, fPos)).size());
	strcpy(peerName, (str.substr(0, fPos)).c_str());

	pthread_mutex_lock(&testPrinter);
	peerIP = gethostbyname(peerName);
	pthread_mutex_unlock(&testPrinter);


	peerAddr.sin_family = PF_INET;
	peerAddr.sin_port = htons(peerPort);
	memcpy(&peerAddr.sin_addr, peerIP->h_addr, peerIP->h_length);
	memset(&peerAddr.sin_zero, '\0', sizeof peerAddr.sin_zero);

	int temp;

	   int m;
	   int n =0;
	   while(!terminateNode)
	   {
		    m=0;
			int peerSock = socket(PF_INET, SOCK_STREAM, 0);

			if((temp = connect(peerSock, (struct sockaddr*)&peerAddr, (socklen_t)sizeof peerAddr)) == -1)
			{
				shutdown(peerSock,SHUT_RDWR);
				close(peerSock);

				if(IAMBEACON)
				{
					while(m < node.retry)
					{
						sleep(1);
						m++;

						pthread_mutex_lock(&existConnLock);
						for(unsigned int j=0; j< existConn.size(); j++)
						{
							if(!strncmp(sendData->peerNode, existConn[j].nodeInfo, strlen(sendData->peerNode)) && !existConn[j].myConn)
							{
								free(sendData);
								free(peerName);
								pthread_mutex_unlock(&existConnLock);
								pthread_exit(NULL);
							}
						}
						pthread_mutex_unlock(&existConnLock);
					}
				}
				else
				{
					sleep(2);
					n++;

					if(n > 4)
					{
						pthread_mutex_lock(&helloLock);
						if(helloNo < node.minNeighbors)
						{
							pthread_mutex_lock(&restart);
							softRestart = true;
							pthread_mutex_unlock(&restart);
						}
						pthread_mutex_unlock(&helloLock);
						pthread_exit(NULL);
					}
					continue;

				}
			}
			else
			{
				sendData->sockDesc = peerSock;
				int temp, k =0 ;
				int bytesSent = 0 ;
				int bytesToSend = 0;
				int bytesRmn    ;

				//SENDING AND LOGGING HELLO
				short welKPort = htons(node.port);
				struct MsgDetails hMsg;
				memcpy(&hMsg.messageType, &sendData->sendBuf[0], sizeof(uint8_t));
				memcpy(&hMsg.UOID, &sendData->sendBuf[sizeof(uint8_t)], sizeof hMsg.UOID);
				memcpy(&hMsg.TTL, &sendData->sendBuf[sizeof(uint8_t) + sizeof hMsg.UOID], sizeof(uint8_t));
				memcpy(&hMsg.rsrv, &sendData->sendBuf[2*sizeof(uint8_t) + sizeof hMsg.UOID], sizeof(uint8_t));
				memcpy(&hMsg.dataLength, &sendData->sendBuf[3*sizeof(uint8_t) + sizeof hMsg.UOID], sizeof(int));

				hMsg.data = (char *)malloc(sizeof(short)+strlen(node.hostNme));
				memcpy(&hMsg.data[0], &welKPort, sizeof(short));
				memcpy(&hMsg.data[sizeof(short)], node.hostNme, strlen(node.hostNme));



				bytesToSend = (int)sendData->dataLen/CHUNK;
				bytesRmn = sendData->dataLen%CHUNK;

				while( k < bytesToSend && ((temp=send(sendData->sockDesc, &sendData->sendBuf[bytesSent], CHUNK, 0)) != -1))
				{
					k++;

					if(temp > 0)
					 bytesSent += temp;
				}

				if(bytesRmn > 0)
				{
					temp = send(sendData->sockDesc, &sendData->sendBuf[bytesSent], bytesRmn, 0);

					if(temp > 0)
					  bytesSent += temp;
				}

				while(bytesSent < sendData->dataLen && (temp = send(sendData->sockDesc, &sendData->sendBuf[bytesSent], sizeof(char), 0)) != -1)
				{
					if(temp > 0)
					 bytesSent += temp;
				}

				if(!connExists(sendData->peerNode))//check tie break
				{
					sendData->gotHello = 0;
					pthread_mutex_lock(&thrIDLock);
					pthread_create(&thrid, NULL, permConnReader, sendData);
					threadID.push_back(thrid);
					pthread_mutex_unlock(&thrIDLock);

					struct peerInfo* newEntry = (struct peerInfo*)malloc(sizeof(struct peerInfo));
					newEntry->nodeInfo = (char *)malloc(strlen(sendData->peerNode)+1);
					strcpy(newEntry->nodeInfo, sendData->peerNode);
					newEntry->nodeInfo[strlen(sendData->peerNode)]='\0';
					newEntry->peerSock = sendData->sockDesc;
					newEntry->myConn = 1;
					newEntry->forKeepAlive = sendData;
					newEntry->forKeepAlive->keepAlive = node.keepAliveTimeOut-3;

					newEntry->forKeepAlive->peerContact = node.keepAliveTimeOut;

					pthread_mutex_lock(&existConnLock);
					existConn.push_back(*newEntry);
					pthread_mutex_unlock(&existConnLock);

					hMsg.nodeID = (char *)malloc(strlen(sendData->peerNode));
					strncpy(hMsg.nodeID, sendData->peerNode, strlen(sendData->peerNode));

					hMsg.kind = 3;
					pthread_mutex_lock(&logLock);
					logQueue.push(hMsg);
					pthread_mutex_unlock(&logLock);
					pthread_cond_signal(&forLogging);

					pthread_mutex_lock(&helloLock);
					helloNo++;
					pthread_mutex_unlock(&helloLock);

  				    pthread_mutex_lock(&testPrinter);
				    waitForJoin = false;
				    pthread_mutex_unlock(&testPrinter);

				}
				else//Connection to node
				{
					sleep(1);
					shutdown(peerSock, SHUT_RDWR);
					close(peerSock);
				}

				break;
			}
		}
	pthread_exit(NULL);
 }

/************************************************************
Pre-Defined function for generating message UOID.
Copyright:Bill Cheng
************************************************************/
char *GetUOID(char *node_inst_id, char *obj_type,
					char *uoid_buf, int uoid_buf_sz)
{
	static unsigned long seq_no=(unsigned long)1;
	char sha1_buf[SHA_DIGEST_LENGTH], str_buf[104];
	sprintf(str_buf, "%s_%s_%1ld",
	node_inst_id, obj_type, (long)seq_no++);
	SHA1((unsigned char*)str_buf, strlen(str_buf), (unsigned char*)sha1_buf);
	memset(uoid_buf, 0, uoid_buf_sz);
	memcpy(uoid_buf, sha1_buf,
	min((unsigned int)uoid_buf_sz,sizeof(sha1_buf)));
	return uoid_buf;
}

/****************************************************************
Function to send Hello message. Does initial processing, spawns
a new thread to connect and send message.
****************************************************************/
void sendHello(struct MsgDetails msgSend, char* connect)
{
   pthread_t thrid;
   connData cd;
   msgSend.TTL = 1;
   msgSend.rsrv = 0;

   char *temp;
   temp = (char *)malloc(strlen("peer"));
   strcpy(temp, "peer");

   GetUOID(node.nodeInstance, temp, (char *)msgSend.UOID, sizeof msgSend.UOID);
   msgSend.dataLength = htonl(strlen(node.hostNme)+sizeof(short));
   msgSend.data = (char *)malloc(strlen(node.hostNme)+sizeof(short));

   short welKPort = htons((short)node.port);

   cd.sendBuf = (unsigned char *)malloc(3*sizeof(uint8_t)+sizeof msgSend.UOID+sizeof(int)+sizeof(short)+strlen(node.hostNme));
   cd.dataLen = 3*sizeof(uint8_t)+sizeof msgSend.UOID+sizeof(int)+sizeof(short)+strlen(node.hostNme);
   //Header
   memcpy(&cd.sendBuf[0], &msgSend.messageType, sizeof(uint8_t));
   memcpy(&cd.sendBuf[sizeof(uint8_t)], msgSend.UOID, sizeof msgSend.UOID);
   memcpy(&cd.sendBuf[sizeof(uint8_t)+sizeof msgSend.UOID], &msgSend.TTL, sizeof(uint8_t));
   memcpy(&cd.sendBuf[2*sizeof(uint8_t)+sizeof msgSend.UOID], &msgSend.rsrv, sizeof(uint8_t));
   memcpy(&cd.sendBuf[3*sizeof(uint8_t)+sizeof msgSend.UOID], &msgSend.dataLength, sizeof(int));

   //Data
   msgSend.data = (char *)malloc(sizeof(short)+strlen(node.hostNme));
   memcpy(&msgSend.data[0], &welKPort, sizeof(short));
   memcpy(&msgSend.data[sizeof(short)], node.hostNme, strlen(node.hostNme));
   memcpy(&cd.sendBuf[3*sizeof(uint8_t)+sizeof msgSend.UOID+sizeof(int)], &welKPort, sizeof(short));
   memcpy(&cd.sendBuf[3*sizeof(uint8_t)+sizeof msgSend.UOID+sizeof(int)+sizeof(short)], node.hostNme, strlen(node.hostNme));

	struct connData *cdParm;
	if(connect == NULL)//Hello to multiple nodes - initial
	{
	   if(IAMBEACON)
	   {
		   for(int j=0; (unsigned int)j < beaconList.size() ; j++)//Connect to other beacons
		   {

				cd.peerNode = (char *)malloc(strlen(beaconList[j]));
				strncpy(cd.peerNode, beaconList[j], strlen(beaconList[j]));

			   cdParm = NULL;
			   cdParm = (struct connData*)malloc(sizeof(struct connData));
			   cdParm->sendBuf = (unsigned char *)malloc(cd.dataLen);
			   cdParm->peerNode = (char *)malloc(strlen(beaconList[j]));

			   memcpy(cdParm->sendBuf, cd.sendBuf, cd.dataLen);
			   memcpy(cdParm->peerNode, cd.peerNode, strlen(beaconList[j]));
				cdParm->sockDesc = 0;
			   cdParm->dataLen = cd.dataLen;

				pthread_mutex_lock(&thrIDLock);
				pthread_create(&thrid, NULL, createConn, cdParm);
				threadID.push_back(thrid);
				pthread_mutex_unlock(&thrIDLock);


			   free(cd.peerNode);
			   cd.peerNode = NULL;

			   	GetUOID(node.nodeInstance, temp, (char *)msgSend.UOID, sizeof msgSend.UOID);
  			    memcpy(&cd.sendBuf[sizeof(uint8_t)], msgSend.UOID, sizeof msgSend.UOID);
		   }
	   }
	   else
	   {
		   for(int j=0; j< node.minNeighbors && myNeighbors[j] != NULL ; j++)//Connect to neighbors
		   {
			   GetUOID(node.nodeInstance, temp, (char *)msgSend.UOID, sizeof msgSend.UOID);
			   memcpy(&cd.sendBuf[sizeof(uint8_t)], msgSend.UOID, sizeof msgSend.UOID);
			   cd.peerNode = (char *)malloc(strlen(myNeighbors[j]));
			   strncpy(cd.peerNode, myNeighbors[j], strlen(myNeighbors[j]));

			   cdParm = NULL;
			   cdParm = (struct connData*)malloc(sizeof(struct connData));
			   cdParm->sendBuf = (unsigned char *)malloc(cd.dataLen);
			   cdParm->peerNode = (char *)malloc(strlen(myNeighbors[j]));

			   memcpy(cdParm->sendBuf, cd.sendBuf, cd.dataLen);
			   memcpy(cdParm->peerNode, cd.peerNode, strlen(myNeighbors[j]));
			   cdParm->sockDesc = 0;
			   cdParm->dataLen = cd.dataLen;

				pthread_mutex_lock(&thrIDLock);
				pthread_create(&thrid, NULL, createConn, cdParm);
				threadID.push_back(thrid);
				pthread_mutex_unlock(&thrIDLock);

			   free(cd.peerNode);
			   cd.peerNode = NULL;
		   }
	   }
   }
   else//Hello to specific node - when sending HLLO back on a link
   {
	   cd.peerNode = (char *)malloc(strlen(connect));
	   strncpy(cd.peerNode, connect, strlen(connect));

	   cdParm = NULL;
	   cdParm = (struct connData*)malloc(sizeof(struct connData));
	   cdParm->sendBuf = (unsigned char *)malloc(cd.dataLen);
	   cdParm->peerNode = (char *)malloc(strlen(connect));

	   memcpy(cdParm->sendBuf, cd.sendBuf, cd.dataLen);
	   memcpy(cdParm->peerNode, cd.peerNode, strlen(connect));
	   cdParm->sockDesc = 0;
	   cdParm->dataLen = cd.dataLen;

	   pthread_mutex_lock(&sendQLock);
	   if(msgSend.TTL > 0)
	   		sendQ.push_back(*cdParm);
	   pthread_mutex_unlock(&sendQLock);

		msgSend.kind = 3;
		msgSend.nodeID = (char *)malloc(strlen(connect));
		memcpy(msgSend.nodeID, connect, strlen(connect));
		pthread_mutex_lock(&logLock);
		logQueue.push(msgSend);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);
   }
}

/**************************************************************************
Function to send NTFY message. Formats message and pushes into sendQ.
**************************************************************************/
void sendNotify(string str, uint8_t errCode)
{
	char *ntfyNode = (char *)malloc(str.size());
	strcpy(ntfyNode, str.c_str());
	struct MsgDetails nMsg;
	struct connData* ntfyMsg = (struct connData *)malloc(sizeof(struct connData));
	char *temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");

	nMsg.messageType = NTFY;
	GetUOID(node.nodeInstance, temp, (char *)nMsg.UOID, sizeof nMsg.UOID);
	nMsg.TTL = 1;
	nMsg.rsrv = 0;
	nMsg.dataLength = sizeof(uint8_t);
	nMsg.dataLength = htonl(nMsg.dataLength);
	nMsg.data = (char *)malloc(sizeof(uint8_t));
	memcpy(&nMsg.data[0], &errCode, sizeof(uint8_t));

	ntfyMsg->dataLen = 3*sizeof(uint8_t)+sizeof nMsg.UOID+sizeof(int)+sizeof(uint8_t);
	ntfyMsg->sendBuf = (unsigned char *)malloc(ntfyMsg->dataLen);

   memcpy(&ntfyMsg->sendBuf[0], &nMsg.messageType, sizeof(uint8_t));
   memcpy(&ntfyMsg->sendBuf[sizeof(uint8_t)], nMsg.UOID, sizeof nMsg.UOID);
   memcpy(&ntfyMsg->sendBuf[sizeof(uint8_t)+sizeof nMsg.UOID], &nMsg.TTL, sizeof(uint8_t));
   memcpy(&ntfyMsg->sendBuf[2*sizeof(uint8_t)+sizeof nMsg.UOID], &nMsg.rsrv, sizeof(uint8_t));
   memcpy(&ntfyMsg->sendBuf[3*sizeof(uint8_t)+sizeof nMsg.UOID], &nMsg.dataLength, sizeof(int));

   memcpy(&ntfyMsg->sendBuf[3*sizeof(uint8_t)+sizeof nMsg.UOID+sizeof(int)], nMsg.data, ntohl(nMsg.dataLength));

    if(!strncmp(ntfyNode, "all", strlen("all")))//NTFY on all links - shutdown or restart
    {
	   pthread_mutex_lock(&existConnLock);
	   for(unsigned int j=0; j<existConn.size() ; j++)
	   {
			ntfyMsg->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo));
			memcpy(ntfyMsg->peerNode, existConn[j].nodeInfo, strlen(existConn[j].nodeInfo));

		   pthread_mutex_lock(&sendQLock);
		   if(nMsg.TTL > 0)
				sendQ.push_back(*ntfyMsg);
		   pthread_mutex_unlock(&sendQLock);


		   nMsg.kind = 3;
			nMsg.nodeID = (char *)malloc(strlen(ntfyMsg->peerNode));
			memcpy(nMsg.nodeID, ntfyMsg->peerNode, strlen(ntfyMsg->peerNode));
			pthread_mutex_lock(&logLock);
			logQueue.push(nMsg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);		   
   	   }
	   pthread_mutex_unlock(&existConnLock);
	}
	else//NTFY on a specific link
	{
		ntfyMsg->peerNode = (char *)malloc(strlen(ntfyNode));
		memcpy(ntfyMsg->peerNode, ntfyNode, strlen(ntfyNode));

	   pthread_mutex_lock(&sendQLock);
	   if(nMsg.TTL > 0)
			sendQ.push_back(*ntfyMsg);
	   pthread_mutex_unlock(&sendQLock);
		nMsg.kind = 3;
		nMsg.nodeID = (char *)malloc(strlen(ntfyNode));
		memcpy(nMsg.nodeID, ntfyNode, strlen(ntfyNode));
		pthread_mutex_lock(&logLock);
		logQueue.push(nMsg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);

	}
   free(ntfyMsg);
   free(temp);
}

/*******************************************************
Function to convert a double into host byte order
*******************************************************/
double convDoubleHByteOrder(char *op)
{
	unsigned char temp[8];
	double result;

	for(unsigned int i=0; i < sizeof(double) ; i++)
	{
		memcpy( &temp[(sizeof(char) * i)], &op[sizeof(char)*(sizeof(double)-1-i)], sizeof(char));
	}

	memcpy(&result, temp, sizeof(double));
	return result;
}

/**********************************************************
Function to process join Req. message is forwarded and
reply sent.
**********************************************************/
void prcsJoinReq(struct MsgDetails joinR)
{
	unsigned int peerLocation;
	char* peerName;
	short peerPort;
	struct connData* sendMsg = (struct connData *)malloc(sizeof(struct connData));

	memcpy(&peerLocation, &joinR.data[0], sizeof(unsigned int));
	memcpy(&peerPort, &joinR.data[sizeof(unsigned int)], sizeof(short));
	peerName = (char *)malloc(ntohl(joinR.dataLength)-sizeof(short)-sizeof(unsigned int));
	memcpy(peerName, &joinR.data[sizeof(unsigned int)+sizeof(short)], ntohl(joinR.dataLength)-sizeof(short)-sizeof(unsigned int));

	joinR.TTL = setTTL(joinR.TTL);

	if(IAMBEACON || !IAMBEACON)
	{
	   sendMsg->dataLen = (3*sizeof(uint8_t)+sizeof joinR.UOID+sizeof(int)+ntohl(joinR.dataLength));
	   sendMsg->sendBuf = (unsigned char*)malloc(sendMsg->dataLen);

	   memcpy(&sendMsg->sendBuf[0], &joinR.messageType, sizeof(uint8_t));
	   memcpy(&sendMsg->sendBuf[sizeof(uint8_t)], joinR.UOID, sizeof joinR.UOID);
	   memcpy(&sendMsg->sendBuf[sizeof(uint8_t)+sizeof joinR.UOID], &joinR.TTL, sizeof(uint8_t));
	   memcpy(&sendMsg->sendBuf[2*sizeof(uint8_t)+sizeof joinR.UOID], &joinR.rsrv, sizeof(uint8_t));
	   memcpy(&sendMsg->sendBuf[3*sizeof(uint8_t)+sizeof joinR.UOID], &joinR.dataLength, sizeof(int));

	   memcpy(&sendMsg->sendBuf[3*sizeof(uint8_t)+sizeof joinR.UOID+sizeof(int)], joinR.data, ntohl(joinR.dataLength));

		pthread_mutex_lock(&existConnLock);
		for(unsigned int i=0; i < existConn.size() ; i++)
		{
			if(existConn[i].peerSock != joinR.pSock)//Fwd on all sockets except on receving link
			{
				sendMsg->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(sendMsg->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;

				pthread_mutex_lock(&sendQLock);
				if(joinR.TTL > 0)
					sendQ.push_back(*sendMsg);
				pthread_mutex_unlock(&sendQLock);

				joinR.kind = 2;
				joinR.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(joinR.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
				pthread_mutex_lock(&logLock);
				logQueue.push(joinR);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
			}
			else
			{
				//Ignore , do not send.
			}
		}
		pthread_mutex_unlock(&existConnLock);
	}

	//JOIN response processing
	struct MsgDetails rspMsg;
	struct connData *sendRsp = (struct connData*)malloc(sizeof(struct connData));
	short welKPort = htons((short)node.port);
	unsigned int dist = abs((int)(node.location - ntohl(peerLocation)));
	char *temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");

	rspMsg.messageType = JNRS;
	GetUOID(node.nodeInstance, temp, (char *)rspMsg.UOID, sizeof rspMsg.UOID);
	rspMsg.TTL = (uint8_t)node.ttl;
	rspMsg.rsrv = 0;
	rspMsg.dataLength = sizeof joinR.UOID + sizeof(unsigned int) + sizeof(short) + strlen(node.hostNme);

	sendRsp->dataLen = (3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)+ rspMsg.dataLength);
	sendRsp->sendBuf = (unsigned char *)malloc(sendRsp->dataLen);

	rspMsg.dataLength = htonl(rspMsg.dataLength);
	dist = htonl(dist);

   memcpy(&sendRsp->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
   memcpy(&sendRsp->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof rspMsg.UOID);
   memcpy(&sendRsp->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
   memcpy(&sendRsp->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
   memcpy(&sendRsp->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));

   //Data
   rspMsg.data = (char *)malloc(sizeof joinR.UOID+sizeof(unsigned int)+sizeof(short)+strlen(node.hostNme));

   memcpy(&rspMsg.data[0], joinR.UOID, sizeof joinR.UOID);
   memcpy(&rspMsg.data[sizeof joinR.UOID], &dist, sizeof(unsigned int));
   memcpy(&rspMsg.data[sizeof joinR.UOID+sizeof(unsigned int)], &welKPort, sizeof(short));
   memcpy(&rspMsg.data[sizeof joinR.UOID+sizeof(unsigned int)+sizeof(short)], node.hostNme, strlen(node.hostNme));

   memcpy(&sendRsp->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)], joinR.UOID, sizeof joinR.UOID);
   memcpy(&sendRsp->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)+sizeof joinR.UOID], &dist, sizeof(unsigned int));
   memcpy(&sendRsp->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)+sizeof joinR.UOID+sizeof(unsigned int)], &welKPort, sizeof(short));
   memcpy(&sendRsp->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)+sizeof joinR.UOID+sizeof(unsigned int)+sizeof(short)], node.hostNme, strlen(node.hostNme));

   	pthread_mutex_lock(&timerLock);
   	for(unsigned int j=0 ; j<cacheUOID.size() ; j++)//Logic to send reply back to requesting node
   	{
   		if(!memcmp(joinR.UOID, cacheUOID[j].msgUOID, sizeof joinR.UOID))
   		{
   			sendRsp->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
   			memset(sendRsp->peerNode, '\0', strlen(cacheUOID[j].senderNode));
   			strcpy(sendRsp->peerNode, cacheUOID[j].senderNode);

 			pthread_mutex_lock(&sendQLock);
   			if(rspMsg.TTL > 0)
   				sendQ.push_back(*sendRsp);
   			pthread_mutex_unlock(&sendQLock);

			rspMsg.kind = 3;
			rspMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
			memcpy(rspMsg.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
			pthread_mutex_lock(&logLock);
			logQueue.push(rspMsg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);
   			break;
   		}
   	}
	pthread_mutex_unlock(&timerLock);

   free(sendRsp);
}

/**********************************************************************
Function to process join response messages.
Either forward or store responses for further processing
**********************************************************************/
void prcsJoinRsp(struct MsgDetails rspMsg)
{
	char repUOID[20];
	memcpy(repUOID, &rspMsg.data[0], sizeof rspMsg.UOID);

  if(IAMBEACON || !IAMBEACON)//Forward message
  {
	  rspMsg.TTL = setTTL(rspMsg.TTL);
	struct connData *fwdMsg = (struct connData *)malloc(sizeof(struct connData));

	   fwdMsg->dataLen = (3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)+ntohl(rspMsg.dataLength));
	   fwdMsg->sendBuf = (unsigned char*)malloc(fwdMsg->dataLen);

	   memcpy(&fwdMsg->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof rspMsg.UOID);
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));

	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)], rspMsg.data, ntohl(rspMsg.dataLength));

	pthread_mutex_lock(&timerLock);
	for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
	{
		if(!memcmp(repUOID, cacheUOID[j].msgUOID, sizeof repUOID))
		{
			fwdMsg->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
			strcpy(fwdMsg->peerNode, cacheUOID[j].senderNode);

			pthread_mutex_lock(&sendQLock);
			if(rspMsg.TTL > 0)
				sendQ.push_back(*fwdMsg);
			pthread_mutex_unlock(&sendQLock);

			rspMsg.kind = 2;
			rspMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
			memcpy(rspMsg.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
			pthread_mutex_lock(&logLock);
			logQueue.push(rspMsg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);
			break;
		}
	}
	pthread_mutex_unlock(&timerLock);

	free(fwdMsg);
   }

	if(!memcmp(myJoinUOID, repUOID, sizeof rspMsg.UOID))//Check UOID and store neighbor
	{
		pthread_mutex_lock(&joinPrcsSig);
		neighborNodes.push_back(rspMsg);
		pthread_mutex_unlock(&joinPrcsSig);
	}
}

/*****************************************************************
Function to format status req Message and push into sendQ
*****************************************************************/
void sendStatusReq(struct MsgDetails stMsg)
{
	struct connData *stData = (struct connData *)malloc(sizeof(struct connData));
	char *temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");

	GetUOID(node.nodeInstance, temp, (char *)stMsg.UOID, sizeof stMsg.UOID);


	struct timerSt *pTimer = (struct timerSt*)malloc(sizeof(struct timerSt));
	memcpy(pTimer->msgUOID, stMsg.UOID, sizeof stMsg.UOID);
	pTimer->timeout = node.msgLifeTime;
	pTimer->senderNode = NULL;	

	//New Msg, store UOID
	pthread_mutex_lock(&timerLock);
	cacheUOID.push_back(*pTimer);
	pthread_mutex_unlock(&timerLock);

	pthread_mutex_lock(&statLock);
	memcpy(myStatUOID, stMsg.UOID, sizeof myStatUOID);
	pthread_mutex_unlock(&statLock);

	stMsg.TTL = (uint8_t)node.ttl;
	stMsg.rsrv = 0;
	stMsg.dataLength = sizeof(uint8_t);
	stMsg.dataLength = htonl(stMsg.dataLength);

	stMsg.data = (char *)malloc(sizeof(uint8_t));

	if (stMsg.snof == 1)
	{
		memcpy(stMsg.data, &stNReq, sizeof(uint8_t));
		stData->snof = 1;
	}
	else if (stMsg.snof == 2)
	{
		memcpy(stMsg.data, &stFReq, sizeof(uint8_t));
		stData->snof = 2;
	}

	//create status file and Write known data
	ofstream fp;	

	statOp = (char *)malloc(1+strlen(statFile));
	memset(statOp, '\0', 1+strlen(statFile));
	strncpy(statOp, statFile, strlen(statFile));	

	if(stMsg.snof == 1)
	{
		fp.open(statOp, ios::in);
		if(!fp.is_open())
		{
			fp.close();
		}
		else
		{
			fp.close();
			remove(statOp);
		}
		fp.open(statOp, ios::out);
		if (stMsg.snof == 1)
		{
			fp << "V -t * -v 1.0a5" << endl;
			fp << "n -t * -s " ;
			fp << node.port ;
			fp << " -c red -i black" << endl;
			stPorts.push_back(node.port);
		}
		fp.close();
	}
	else
	{
		fp.open(statFile, ios::out);
		fp.close();
	}

	stData->sendBuf = (unsigned char *)malloc(4*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int));
	memcpy(&stData->sendBuf[0], &stMsg.messageType, sizeof(uint8_t));
	memcpy(&stData->sendBuf[sizeof(uint8_t)], stMsg.UOID, sizeof stMsg.UOID);
	memcpy(&stData->sendBuf[sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.TTL, sizeof(uint8_t));
	memcpy(&stData->sendBuf[2*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.rsrv, sizeof(uint8_t));
	memcpy(&stData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.dataLength, sizeof(int));

	memcpy(&stData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int)], stMsg.data, ntohl(stMsg.dataLength));
	stData->dataLen = 4*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int);

	pthread_mutex_lock(&existConnLock);
	for(unsigned int i=0; i < existConn.size() ; i++)
	{
		stData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
		memcpy(stData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)+1) ;
		stData->peerNode[strlen(existConn[i].nodeInfo)] = '\0';
		pthread_mutex_lock(&sendQLock);
		if(stMsg.TTL > 0)
			sendQ.push_back(*stData);
		pthread_mutex_unlock(&sendQLock);
		stMsg.kind = 3;
		stMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
		memcpy(stMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
		pthread_mutex_lock(&logLock);
		logQueue.push(stMsg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);			
		stMsg.nodeID = NULL;
	}
	pthread_mutex_unlock(&existConnLock);

	free(stData);

	if(!statusFiles)
		return;

	//Check for files in this node.
		char ***rFiles = (char ***)malloc(sizeof(char **));
		*rFiles = NULL;
		short *no = (short *)malloc(sizeof(short));
		*no = 0;

		pthread_mutex_lock(&treeLock);
		returnTree(nameTree, no, rFiles);
		pthread_mutex_unlock(&treeLock);
		
		struct stat forStat;

		int count = 0;
		ifstream ip;
		ofstream op;

		pthread_mutex_lock(&fileLock);
		if(*no == 0)
		{

			op.open(statFile, ios::app);
			op.write(node.hostNme, strlen(node.hostNme));
			op.write(":", 1);
			op << node.port ;
			op << " has no files \n";
			op.close();
			pthread_mutex_unlock(&fileLock);
			return;
		}

		int rCount = 0;
		char *prevFile = NULL;

		while(count < *no)
		{
			char *name = new char[strlen((*rFiles)[count])+1];
			strncpy(name, (*rFiles)[count], strlen((*rFiles)[count]));
			name[strlen((*rFiles)[count])] = '\0';

			if (strlen(name)!=0)
			{
				if(prevFile != NULL)
				{
					if(!strncmp(prevFile, name, strlen(name)))
					{
						count++;
						continue;
					}
				}

				name[strlen((*rFiles)[count])]='\0';
				name[strlen((*rFiles)[count])-3] = 'e';
				name[strlen((*rFiles)[count])-4] = 'm';

				if (stat(name,&forStat)==0)
				{
					char *Buf = new char[forStat.st_size];
					memset(Buf, '\0', forStat.st_size);					
					ip.open(name, ios::in);

					if(!ip.is_open())
					{
						rCount++;
						count++;
						continue;
					}

					op.open(statFile, ios::app);


					if(count == 0)
					{
						op.write(node.hostNme, strlen(node.hostNme));
						op.write(":", 1);
						op << node.port ;
						op << " has the following files \n";
					}

					ip.read(Buf, forStat.st_size);
					op.write(Buf, forStat.st_size);
					op << "\n";
					op.flush();

					op.close();
					ip.close();

					delete[] Buf;
				}
				prevFile = (char *)malloc(strlen(name)+1);
				strncpy(prevFile, name, strlen(name));
				prevFile[strlen(name)] = '\0';

			}

			delete[] name;
			count++;
		}

		if(rCount == *no)
		{
			op.open(statFile, ios::app);
			op.write(node.hostNme, strlen(node.hostNme));
			op.write(":", 1);
			op << node.port ;
			op << " has no files \n";
			op.close();
		}
		pthread_mutex_unlock(&fileLock);
}

/*******************************************************************************
Function to process received status message. forward the request and
reply to sending node.
*******************************************************************************/
void prcsStatusReq(struct MsgDetails reqMsg)
{
	struct connData* sendRply = (struct connData*)malloc(sizeof(struct connData));
	sendRply->peerNode = (char *)malloc(strlen(reqMsg.nodeID));
	strncpy(sendRply->peerNode, reqMsg.nodeID, strlen(reqMsg.nodeID));
	sendRply->peerNode[strlen(reqMsg.nodeID)] = '\0';

	reqMsg.TTL = setTTL(reqMsg.TTL);

	struct connData* fwdStat = (struct connData*)malloc(sizeof(struct connData));

	if (memcmp(reqMsg.data,&stNReq,sizeof(uint8_t))==0)
	{
		reqMsg.snof = 1;
		sendRply->snof = 1;
		fwdStat->snof = 1;
	}
	else if (memcmp(reqMsg.data,&stFReq,sizeof(uint8_t))==0)
	{
		reqMsg.snof = 2;
		sendRply->snof = 2;
		fwdStat->snof = 2;
	}



	fwdStat->dataLen = (3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)+ntohl(reqMsg.dataLength));
	fwdStat->sendBuf = (unsigned char*)malloc(fwdStat->dataLen);

	memcpy(&fwdStat->sendBuf[0], &reqMsg.messageType, sizeof(uint8_t));
	memcpy(&fwdStat->sendBuf[sizeof(uint8_t)], reqMsg.UOID, sizeof reqMsg.UOID);
	memcpy(&fwdStat->sendBuf[sizeof(uint8_t)+sizeof reqMsg.UOID], &reqMsg.TTL, sizeof(uint8_t));
	memcpy(&fwdStat->sendBuf[2*sizeof(uint8_t)+sizeof reqMsg.UOID], &reqMsg.rsrv, sizeof(uint8_t));
	memcpy(&fwdStat->sendBuf[3*sizeof(uint8_t)+sizeof reqMsg.UOID], &reqMsg.dataLength, sizeof(int));

	memcpy(&fwdStat->sendBuf[3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)], reqMsg.data, ntohl(reqMsg.dataLength));

	pthread_mutex_lock(&existConnLock);
	for(unsigned int i=0; i < existConn.size() ; i++)//Forwarding
	{
		if(existConn[i].peerSock != reqMsg.pSock)
		{
			fwdStat->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
			memcpy(fwdStat->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;
			pthread_mutex_lock(&sendQLock);
			if(reqMsg.TTL > 0)
				sendQ.push_back(*fwdStat);
			pthread_mutex_unlock(&sendQLock);
			reqMsg.kind = 2;
			reqMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
			memcpy(reqMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
			pthread_mutex_lock(&logLock);
			logQueue.push(reqMsg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);
			reqMsg.nodeID = NULL;
		}
	}
	pthread_mutex_unlock(&existConnLock);

	//Response Processing

	struct MsgDetails rspMsg;
	int totalData = 0;
	char reqUOID[20];

	char *temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");

	memcpy(reqUOID, reqMsg.UOID, sizeof reqMsg.UOID);

	rspMsg.messageType = STRS;
	GetUOID(node.nodeInstance, temp, (char *)rspMsg.UOID, sizeof rspMsg.UOID);
	rspMsg.TTL = (uint8_t)node.ttl;
	rspMsg.rsrv = 0;

	short hostInfoLength = sizeof(short)+strlen(node.hostNme);
	hostInfoLength = ntohs(hostInfoLength);

	short welKPort = ntohs(node.port);

	size_t constBlk = sizeof reqUOID+2*sizeof(short)+strlen(node.hostNme);
	rspMsg.data = (char *)malloc(constBlk);
	totalData = constBlk;
	memcpy(&rspMsg.data[0], reqUOID, sizeof reqMsg.UOID);
	memcpy(&rspMsg.data[sizeof reqMsg.UOID], &hostInfoLength, sizeof(short));
	memcpy(&rspMsg.data[sizeof reqMsg.UOID+sizeof(short)], &welKPort, sizeof(short));
	memcpy(&rspMsg.data[sizeof reqMsg.UOID+2*sizeof(short)], node.hostNme, strlen(node.hostNme));

	int recordLen = 0;
	size_t fPos;
	char *name;
	char *namePort;
	short nPort;
	int tPort;
	char *tempHol;

	if (reqMsg.snof == 1)
	{
		pthread_mutex_lock(&existConnLock);
		for(unsigned int i = 0 ; i < existConn.size() ; i++)//Check existing connections and build response
		{
			string strTemp(existConn[i].nodeInfo);
			fPos = strTemp.find_first_of(":");
			name = (char *)malloc((strTemp.substr(0, fPos)).size());
			strncpy(name, (strTemp.substr(0, fPos)).c_str(), (strTemp.substr(0, fPos)).size());

			namePort = (char *)malloc((strTemp.substr(fPos+1, strTemp.size())).size());
			strcpy(namePort, (strTemp.substr(fPos+1, strTemp.size())).c_str());
			sscanf(namePort, "%d", &tPort);
			nPort = (short)tPort;

			nPort = htons(nPort);

			if( i == existConn.size() - 1)
			{
				recordLen = 0;
			}
			else
			{
				recordLen = sizeof(short) + strlen(name);
			}

			recordLen= htonl(recordLen);

			tempHol = (char *)malloc(totalData+sizeof(int)+sizeof(short)+strlen(name));
			memcpy(&tempHol[0], rspMsg.data, totalData);
			free(rspMsg.data);
			rspMsg.data  = tempHol;
			tempHol = NULL;

			memcpy(&rspMsg.data[totalData], &recordLen, sizeof(int));
			memcpy(&rspMsg.data[totalData+sizeof(int)], &nPort, sizeof(short));
			memcpy(&rspMsg.data[totalData+sizeof(int)+sizeof(short)], name, strlen(name));
			totalData = totalData + sizeof(int) + sizeof(short)+strlen(name) ;

			free(name);
			free(namePort);
		}
		pthread_mutex_unlock(&existConnLock);
		rspMsg.dataLength = htonl(totalData);
		sendRply->dataLen = 3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)+totalData;
		sendRply->sendBuf = (unsigned char *)malloc(3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)+totalData);

		memcpy(&sendRply->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
		memcpy(&sendRply->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof reqMsg.UOID);
		memcpy(&sendRply->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
		memcpy(&sendRply->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
		memcpy(&sendRply->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));

		memcpy(&sendRply->sendBuf[3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)], rspMsg.data, totalData);

		pthread_mutex_lock(&sendQLock);
		if(rspMsg.TTL > 0)
			sendQ.push_back(*sendRply);
		pthread_mutex_unlock(&sendQLock);
	}
	else if (reqMsg.snof==2)
	{
		fstream tFP;
		char *tempFile;
		tempFile = (char *)malloc(strlen(myHome)+1+strlen("$Curr_Files"));
		strcpy(tempFile, myHome);
		strcat(tempFile, "/");
		strcat(tempFile, "$Curr_Files");		

		char ***rFiles = (char ***)malloc(sizeof(char **));
		*rFiles = NULL;
		short *no = (short *)malloc(sizeof(short));
		*no = 0;

		pthread_mutex_lock(&treeLock);
		returnTree(nameTree, no, rFiles);
		pthread_mutex_unlock(&treeLock);
		
		struct stat forStat;
		int total = 0;
		int count = 0;
		while(count < *no)
		{
			char *name = new char[strlen((*rFiles)[count])+1];
			strncpy(name, (*rFiles)[count], strlen((*rFiles)[count]));
			name[strlen((*rFiles)[count])] = '\0';

			if (strlen(name)!=0)
			{
				name[strlen((*rFiles)[count])]='\0';
				name[strlen((*rFiles)[count])-3] = 'e';
				name[strlen((*rFiles)[count])-4] = 'm';

				if (stat(name,&forStat)==0)
				{
					total += forStat.st_size;
				}
			}

			if (strlen(name)==0)
				break;

			delete[] name;
			count++;
		}
		
		
		int fileSize;

		int recordLength = 0;
		int rCount;
		total+=totalData+(count * sizeof(int));

		if (total<=MAXBUFFERSIZE)
		{			
			if (*no > 0)
			{				
				count = 0;
				rCount = 0;
				ifstream readMetaFile;
				char *prevFile = NULL;

				while(count < *no)
				{
					char *name = new char[strlen((*rFiles)[count])];
					strncpy(name, (*rFiles)[count], strlen((*rFiles)[count]));
					name[strlen((*rFiles)[count])] = '\0';

					if(prevFile != NULL)
					{
						if(!strncmp(name, prevFile, strlen(name)))
						{
							count++;
							continue;
						}
					}
					
					name[strlen((*rFiles)[count])]='\0';
					name[strlen((*rFiles)[count])-3] = 'e';
					name[strlen((*rFiles)[count])-4] = 'm';					

					readMetaFile.open(name, ios::in);
					if(!readMetaFile.is_open())
					{
						rCount++;
						readMetaFile.close();
						count++;
						continue;
					}
					readMetaFile.close();

					if (stat(name,&forStat)==0)
					{
						fileSize = forStat.st_size;
						if (count < *no-1)
							recordLength = fileSize;
						else if (count == *no-1)
							recordLength = 0;
					}

					readMetaFile.open(name,ios::in|ios::binary);
					while(!readMetaFile.eof() && readMetaFile.peek() != EOF)
					{
						char *metaFileData = new char[fileSize];
						readMetaFile.read(metaFileData,fileSize);

						tempHol = (char *)malloc(totalData+sizeof(int)+fileSize);
						memcpy(&tempHol[0], rspMsg.data, totalData);
						free(rspMsg.data);
						rspMsg.data  = tempHol;
						tempHol = NULL;

						memcpy(&rspMsg.data[totalData], &recordLength, sizeof(int));
						memcpy(&rspMsg.data[totalData+sizeof(int)], metaFileData, fileSize);
						totalData = totalData + sizeof(int) + fileSize;
						delete[] metaFileData;
					}
					readMetaFile.close();

					prevFile = (char *)malloc(strlen(name)+1);
					strncpy(prevFile, name, strlen(name));
					prevFile[strlen(name)] = '\0';

					delete[] name;
					count++;
				}				
			}

			if (*no == 0 || rCount == *no)
			{
				recordLength = -1;

				tempHol = (char *)malloc(totalData+sizeof(int));
				memcpy(&tempHol[0], rspMsg.data, totalData);
				free(rspMsg.data);
				rspMsg.data  = tempHol;
				tempHol = NULL;

				memcpy(&rspMsg.data[totalData], &recordLength, sizeof(int));
				totalData += sizeof(int);
			}
			
			rspMsg.dataLength = htonl(totalData);
			sendRply->dataLen = 3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)+totalData;
			sendRply->sendBuf = (unsigned char *)malloc(3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)+totalData);

			memcpy(&sendRply->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
			memcpy(&sendRply->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof reqMsg.UOID);
			memcpy(&sendRply->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
			memcpy(&sendRply->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
			memcpy(&sendRply->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));

			memcpy(&sendRply->sendBuf[3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)], rspMsg.data, totalData);

			pthread_mutex_lock(&sendQLock);
			if(rspMsg.TTL > 0)
				sendQ.push_back(*sendRply);
			pthread_mutex_unlock(&sendQLock);
		}
		else if (total>MAXBUFFERSIZE)
		{
			rspMsg.dataLength = htonl(total);


			sendRply->sendBuf = (unsigned char *)malloc(3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int));
			sendRply->dataLen = 3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int);
			memcpy(&sendRply->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
			memcpy(&sendRply->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof reqMsg.UOID);
			memcpy(&sendRply->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
			memcpy(&sendRply->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
			memcpy(&sendRply->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));
			memcpy(&sendRply->sendBuf[3*sizeof(uint8_t)+sizeof reqMsg.UOID+sizeof(int)], rspMsg.data, totalData);

			pthread_mutex_lock(&sendQLock);
			if(rspMsg.TTL > 0)
				sendQ.push_back(*sendRply);
			pthread_mutex_unlock(&sendQLock);

				int i=0;
				count = 0;				
				while(count < *no)
				{
					i++;
					char *name = new char[strlen((*rFiles)[count])];
					strncpy(name, (*rFiles)[count], strlen((*rFiles)[count]));
					name[strlen((*rFiles)[count])] = '\0';					
					name[strlen((*rFiles)[count])]='\0';
					name[strlen((*rFiles)[count])-3] = 'e';
					name[strlen((*rFiles)[count])-4] = 'm';

					ifstream readMetaFile;
					if (stat(name,&forStat)==0)
					{
						fileSize = forStat.st_size;
						if (count < *no-1)
							recordLength = fileSize;
						else if (count == *no-1)
							recordLength = 0;
					}
					
					readMetaFile.open(name,ios::in|ios::binary);
					while(!readMetaFile.eof() && readMetaFile.peek() != EOF)
					{
						char *metaFileData = new char[fileSize];
						readMetaFile.read(metaFileData,fileSize);
						sendRply->sendBuf = (unsigned char *)malloc(strlen(metaFileData)+sizeof(int));
						sendRply->dataLen = strlen(metaFileData)+sizeof(int);
						memcpy(&sendRply->sendBuf[0], &recordLength, sizeof(int));
						memcpy(&sendRply->sendBuf[sizeof(int)], metaFileData, strlen(metaFileData));
						pthread_mutex_lock(&sendQLock);
						if(rspMsg.TTL > 0)
							sendQ.push_back(*sendRply);
						pthread_mutex_unlock(&sendQLock);						
						delete[] metaFileData;
					}
					readMetaFile.close();
					delete[] name;
				}
				
		}
		free(rFiles);		
	}


	rspMsg.kind = 3;
	rspMsg.nodeID = (char *)malloc(strlen(sendRply->peerNode));
	memcpy(rspMsg.nodeID, sendRply->peerNode, strlen(sendRply->peerNode));
	pthread_mutex_lock(&logLock);
	logQueue.push(rspMsg);
	pthread_mutex_unlock(&logLock);
	pthread_cond_signal(&forLogging);

   free(sendRply);

}

/***************************************************************************
Function to process received status response.
***************************************************************************/
void prcsStatusRsp(struct MsgDetails rspMsg)
{	
	char rspUOID[20];
	memcpy(rspUOID, &rspMsg.data[0], sizeof rspUOID);

	if(!memcmp(rspUOID, myStatUOID, sizeof rspUOID))//Check if status response is for this node
	{
		if (!statusFiles)
		{
			ofstream fp;
			char *statOp;

			statOp = (char *)malloc(1+strlen(statFile));
			memset(statOp, '\0', 1+strlen(statFile));
			strncpy(statOp, statFile, strlen(statFile));

			fp.open(statOp, ios::in);

			if(!fp.is_open())
			{
				fp.close();
				fp.open(statOp, ios::out);
				fp << "V -t * -v 1.0a5" << endl;				
				fp << "n -t * -s " ;
				fp << node.port ;
				fp << " -c red -i black" << endl;

				stPorts.push_back(node.port);
			}

			fp.close();

			fp.open(statOp, ios::app);

			if(!fp.is_open())
			{				
				return;
			}

			short rpPort;
			memcpy(&rpPort, &rspMsg.data[sizeof rspUOID+sizeof(short)], sizeof(short));
			rpPort = ntohs(rpPort);

			short hostInfoLen;
			memcpy(&hostInfoLen, &rspMsg.data[sizeof rspUOID], sizeof(short));
			hostInfoLen = ntohs(hostInfoLen);

			bool present = false;
			multimap<short, short>::iterator it;

			for(unsigned int j=0; j<stPorts.size(); j++)
			{
				if(rpPort == stPorts[j])
				   present = true;
			}

			if(!present)
			{
				stPorts.push_back(ntohs(rpPort));
				fp << "n -t * -s " ;
				fp << rpPort ;
				fp << " -c red -i black" << endl;
			}

			int recLen = 1;
			short newRpPort;
			int k =0;
			int processed =0;
			memcpy(&recLen, &rspMsg.data[sizeof rspUOID + sizeof(short)+ hostInfoLen], sizeof(int));
			memcpy(&newRpPort, &rspMsg.data[sizeof rspUOID+sizeof(short)+hostInfoLen+sizeof(int)], sizeof(short));
			while(ntohl(recLen) !=0 )
			{

				present = false;
				processed = processed + sizeof(int) + ntohl(recLen);

				for(unsigned int j=0; j<stPorts.size(); j++)
				{
					if(ntohs(newRpPort) == stPorts[j])
					   present = true;
				}

				if(!present)
				{
					stPorts.push_back(ntohs(newRpPort));
					fp << "n -t * -s " ;
					fp << ntohs(newRpPort) ;
					fp << " -c red -i black" << endl;
				}

				it = namLinks.find(ntohs(newRpPort));
				bool exst = true;

				if(it == namLinks.end())
				{
					it = namLinks.find(ntohs(rpPort));

					if(it == namLinks.end())
					{
						namLinks.insert(pair<short, short>(ntohs(rpPort), ntohs(newRpPort)));
						fp << "l -t * -s "  ;
						fp << ntohs(rpPort) ;
						fp << " -d " << ntohs(newRpPort) ;
						fp << " -c blue" << endl;
					}
					else
					{
						for(it = namLinks.equal_range(ntohs(rpPort)).first ; it != namLinks.equal_range(ntohs(rpPort)).second ; ++it)
						{
							if(it->second == ntohs(newRpPort))
							{
								exst = false;
								break;
							}
						}

						if(exst)
						{
							namLinks.insert(pair<short, short>(ntohs(rpPort), ntohs(newRpPort)));
							fp << "l -t * -s "  ;
							fp << ntohs(rpPort) ;
							fp << " -d " << ntohs(newRpPort) ;
							fp << " -c blue" << endl;
						}
					}
				}
				else
				{
					for(it = namLinks.equal_range(ntohs(newRpPort)).first ; it != namLinks.equal_range(ntohs(newRpPort)).second ; ++it)
					{
						if(it->second == ntohs(rpPort))
						{
							exst = false;
							break;
						}
					}

					if(exst)
					{
						namLinks.insert(pair<short, short>(ntohs(newRpPort), ntohs(rpPort)));
						fp << "l -t * -s "  ;
						fp << ntohs(newRpPort) ;
						fp << " -d " << ntohs(rpPort) ;
						fp << " -c blue" << endl;
					}
				}

				k++;
				memcpy(&recLen, &rspMsg.data[sizeof rspUOID+sizeof(short)+hostInfoLen+processed], sizeof(int));
				memcpy(&newRpPort, &rspMsg.data[sizeof rspUOID+sizeof(short)+hostInfoLen+(k+1)*sizeof(int)+k*ntohl(recLen)], sizeof(short));
			}

			present = false;
			memcpy(&newRpPort, &rspMsg.data[sizeof rspUOID+sizeof(short)+hostInfoLen+processed+sizeof(int)], sizeof(short));

			for(unsigned int j=0; j<stPorts.size(); j++)
			{
				if(ntohs(newRpPort) == stPorts[j])
				   present = true;
			}

			if(!present)
			{
				stPorts.push_back(newRpPort);
				fp << "n -t * -s " ;
				fp << ntohs(newRpPort) ;
				fp << " -c red -i black" << endl;
			}
				it = namLinks.find(ntohs(newRpPort));
				bool exst = true;

				if(it == namLinks.end())
				{
					it = namLinks.find(ntohs(rpPort));

					if(it == namLinks.end())
					{
						namLinks.insert(pair<short, short>(ntohs(rpPort), ntohs(newRpPort)));
						fp << "l -t * -s "  ;
						fp << ntohs(rpPort) ;
						fp << " -d " << ntohs(newRpPort) ;
						fp << " -c blue" << endl;
					}
					else
					{
						for(it = namLinks.equal_range(ntohs(rpPort)).first ; it != namLinks.equal_range(ntohs(rpPort)).second ; ++it)
						{
							if(it->second == ntohs(newRpPort))
							{
								exst = false;
								break;
							}
						}

						if(exst)
						{
							namLinks.insert(pair<short, short>(ntohs(rpPort), ntohs(newRpPort)));
							fp << "l -t * -s "  ;
							fp << ntohs(rpPort) ;
							fp << " -d " << ntohs(newRpPort) ;
							fp << " -c blue" << endl;
						}
					}
				}
				else
				{
					for(it = namLinks.equal_range(ntohs(newRpPort)).first ; it != namLinks.equal_range(ntohs(newRpPort)).second ; ++it)
					{
						if(it->second == ntohs(rpPort))
						{
							exst = false;
							break;
						}
					}

					if(exst)
					{
						namLinks.insert(pair<short, short>(ntohs(newRpPort), ntohs(rpPort)));
						fp << "l -t * -s "  ;
						fp << ntohs(newRpPort) ;
						fp << " -d " << ntohs(rpPort) ;
						fp << " -c blue" << endl;
					}
				}
			free(statOp);
		}
		else
		{
			ofstream fp;

			

			pthread_mutex_lock(&fileLock);
			fp.open(statFile, ios::app);			

			if(!fp.is_open())
			{				
				pthread_mutex_unlock(&fileLock);
				return;
			}

			char *hn;
			short prt;
			short info;

			memcpy(&info, &rspMsg.data[20], sizeof(short));
			memcpy(&prt, &rspMsg.data[22], sizeof(short));
			hn = (char *)malloc(ntohs(info)-1);
			memcpy(hn, &rspMsg.data[24], ntohs(info)-2);
			hn[ntohs(info)-1] = '\0';

			int recordLen;
			memcpy(&recordLen, &rspMsg.data[22+ntohs(info)], sizeof(int));

			if (recordLen==-1)
			{				
				fp.write(hn, ntohs(info)-2);
				fp<<":"<<prt<<" has no files"<<endl;
				pthread_mutex_unlock(&fileLock);
				return;
			}
			else if (recordLen == 0)
			{				
				fp.write(hn,ntohs(info)-2);
				fp<<":"<<prt<<" has the following file"<<endl;
				char *metaD;
				int metaDLen = ntohl(rspMsg.dataLength)-22-ntohs(info)-sizeof(int);
				metaD = (char *)malloc(metaDLen);
				memcpy(metaD, &rspMsg.data[22+ntohs(info)+sizeof(int)], metaDLen);				
				fp.write(metaD,metaDLen);
				fp<<endl;
				free(metaD);
			}

			else
			{
				int totalD = 22+ntohs(info)+sizeof(int);				
				fp.write(hn,strlen(hn));
				fp<<":"<<prt<<" has the following files"<<endl;
				while(recordLen>0)
				{
					char *metaD = (char *)malloc(recordLen);
					memcpy(metaD, &rspMsg.data[totalD], recordLen);					
					fp.write(metaD,recordLen);
					fp<<endl;
					fp.flush();

					totalD += recordLen;
					memcpy(&recordLen, &rspMsg.data[totalD], sizeof(int));
					totalD += sizeof(int);
					free(metaD);

					if (recordLen == 0)
					{
						char *metaD = (char *)malloc(ntohl(rspMsg.dataLength)-totalD);
						int len = ntohl(rspMsg.dataLength)-totalD;
						memcpy(metaD, &rspMsg.data[totalD], ntohl(rspMsg.dataLength)-totalD);						
						fp.write(metaD,len);
						fp<<endl;
						fp.flush();
					}
				}
			}

			fp.close();
			pthread_mutex_unlock(&fileLock);			
		}
	}
	else //Message not for me, forward to original request sender
	{
		rspMsg.TTL = setTTL(rspMsg.TTL);
	   struct connData *fwdMsg = (struct connData *)malloc(sizeof(struct connData));

	   fwdMsg->dataLen = (3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)+ntohl(rspMsg.dataLength));
	   fwdMsg->sendBuf = (unsigned char*)malloc(fwdMsg->dataLen);

	   memcpy(&fwdMsg->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof rspMsg.UOID);
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));

	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)], rspMsg.data, ntohl(rspMsg.dataLength));

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
		{
			if(!memcmp(rspUOID, cacheUOID[j].msgUOID, sizeof rspUOID))
			{
				fwdMsg->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
				strcpy(fwdMsg->peerNode, cacheUOID[j].senderNode);

				pthread_mutex_lock(&sendQLock);
				if(rspMsg.TTL > 0)
					sendQ.push_back(*fwdMsg);
				pthread_mutex_unlock(&sendQLock);

				rspMsg.kind = 2;
				rspMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
				memcpy(rspMsg.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
				pthread_mutex_lock(&logLock);
				logQueue.push(rspMsg);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
				break;
			}
		}
		pthread_mutex_unlock(&timerLock);
		free(fwdMsg);
	 }
}

/************************************************************
Thread to wait for check response.
spawned after check request is sent.
************************************************************/
void *waitForCheck(void *arg)
{
	int a = 0;
	while(!terminateNode&&a<node.msgLifeTime)
	{
		sleep(1);
		a++;
	}

	pthread_mutex_lock(&statLock);
	if(waitingForCheck)
	{
		//Do Soft restart
		pthread_mutex_lock(&restart);
		softRestart = true;
		pthread_mutex_unlock(&restart);

	}
	pthread_mutex_unlock(&statLock);
	pthread_exit(NULL);
}

/****************************************************************************
Function to format check request and push in sendQ
****************************************************************************/
void sendChkMsg(struct MsgDetails chkMsg)
{
	pthread_mutex_lock(&statLock);
	if(IAMBEACON || waitingForCheck)//Do not send if beacon or if already sent
	{
		pthread_mutex_unlock(&statLock);
		return;
	}
	pthread_mutex_unlock(&statLock);

	char* temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");

	struct connData *chkData = (struct connData *)malloc(sizeof(struct connData));
	GetUOID(node.nodeInstance, temp, (char *)chkMsg.UOID, sizeof chkMsg.UOID);
	chkMsg.TTL = (uint8_t)node.ttl;
	chkMsg.rsrv = 0;
	chkMsg.dataLength = 0;

	chkData->dataLen = sizeof chkMsg.UOID + 3*sizeof(uint8_t) + sizeof(int);
   	chkData->sendBuf = (unsigned char *)malloc(chkData->dataLen);

	memcpy(myChkUOID, chkMsg.UOID, sizeof chkMsg.UOID);

   memcpy(&chkData->sendBuf[0], &chkMsg.messageType, sizeof(uint8_t));
   memcpy(&chkData->sendBuf[sizeof(uint8_t)], chkMsg.UOID, sizeof chkMsg.UOID);
   memcpy(&chkData->sendBuf[sizeof(uint8_t)+sizeof chkMsg.UOID], &chkMsg.TTL, sizeof(uint8_t));
   memcpy(&chkData->sendBuf[2*sizeof(uint8_t)+sizeof chkMsg.UOID], &chkMsg.rsrv, sizeof(uint8_t));
   memcpy(&chkData->sendBuf[3*sizeof(uint8_t)+sizeof chkMsg.UOID], &chkMsg.dataLength, sizeof(int));

	if(terminateNode)
		return;

   pthread_mutex_lock(&existConnLock);
   for(unsigned int i=0; i < existConn.size() ; i++)//Send to all existing connections.
   {
		chkData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
		memcpy(chkData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;

		pthread_mutex_lock(&sendQLock);
		if(chkMsg.TTL > 0)
			sendQ.push_back(*chkData);
		pthread_mutex_unlock(&sendQLock);

		chkMsg.kind = 3;
		chkMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
		memcpy(chkMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
		pthread_mutex_lock(&logLock);
		logQueue.push(chkMsg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);
		
		chkMsg.nodeID = NULL;
   }
   pthread_mutex_unlock(&existConnLock);

	pthread_t thrid;
	pthread_mutex_lock(&thrIDLock);
	pthread_create(&thrid, NULL, waitForCheck, NULL);
	threadID.push_back(thrid);
	pthread_mutex_unlock(&thrIDLock);

	pthread_mutex_lock(&statLock);
   waitingForCheck = true;
   	pthread_mutex_unlock(&statLock);

   free(temp);

}

/********************************************************************************
Function to process received check request message.
********************************************************************************/
void prcsChkMsg(struct MsgDetails pChk)
{
	struct connData *rData = (struct connData *)malloc(sizeof(struct connData));
	if(!IAMBEACON)
	{
		pChk.TTL = setTTL(pChk.TTL);
		//Forward
		rData->dataLen = 3*sizeof(uint8_t)+sizeof pChk.UOID+ sizeof(int);
		rData->sendBuf = (unsigned char *)malloc(rData->dataLen);

	   memcpy(&rData->sendBuf[0], &pChk.messageType, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[sizeof(uint8_t)], pChk.UOID, sizeof pChk.UOID);
	   memcpy(&rData->sendBuf[sizeof(uint8_t)+sizeof pChk.UOID], &pChk.TTL, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[2*sizeof(uint8_t)+sizeof pChk.UOID], &pChk.rsrv, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof pChk.UOID], &pChk.dataLength, sizeof(int));

		pthread_mutex_lock(&existConnLock);
		for(unsigned int i=0; i < existConn.size() ; i++)
		{
			if(existConn[i].peerSock != pChk.pSock)
			{
				rData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(rData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;

				pthread_mutex_lock(&sendQLock);
				if(pChk.TTL > 0)
					sendQ.push_back(*rData);
				pthread_mutex_unlock(&sendQLock);

				pChk.kind = 2;
				pChk.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(pChk.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
				pthread_mutex_lock(&logLock);
				logQueue.push(pChk);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
			}
		}
		pthread_mutex_unlock(&existConnLock);

		free(rData);
	}
	else//Send response - I am beacon :)
	{
		char reqUOID[20];
		memcpy(reqUOID, pChk.UOID, sizeof reqUOID);
		char* temp = (char *)malloc(strlen("peer"));
		strcpy(temp, "peer");
		GetUOID(node.nodeInstance, temp, (char *)pChk.UOID, sizeof reqUOID);
		pChk.messageType = CKRS;
		pChk.TTL = setTTL(pChk.TTL);
		pChk.dataLength = sizeof pChk.UOID;
		pChk.dataLength = htonl(pChk.dataLength);

		rData->dataLen = 3*sizeof(uint8_t)+2*sizeof pChk.UOID+ sizeof(int);
		rData->sendBuf = (unsigned char *)malloc(rData->dataLen);

	   memcpy(&rData->sendBuf[0], &pChk.messageType, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[sizeof(uint8_t)], pChk.UOID, sizeof reqUOID);
	   memcpy(&rData->sendBuf[sizeof(uint8_t)+sizeof pChk.UOID], &pChk.TTL, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[2*sizeof(uint8_t)+sizeof pChk.UOID], &pChk.rsrv, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof pChk.UOID], &pChk.dataLength, sizeof(int));

	   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof pChk.UOID+sizeof(int)], reqUOID, sizeof pChk.UOID);

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
		{
			if(!memcmp(reqUOID, cacheUOID[j].msgUOID, sizeof pChk.UOID))
			{
				rData->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
				strcpy(rData->peerNode, cacheUOID[j].senderNode);

				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*rData);
				pthread_mutex_unlock(&sendQLock);

				pChk.kind = 3;
				pChk.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
				memcpy(pChk.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
				pthread_mutex_lock(&logLock);
				logQueue.push(pChk);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
				break;
			}
		}
		pthread_mutex_unlock(&timerLock);
		free(temp);
	}
	free(rData);
}


/*************************************************************************
Function to format delete msg and push in sendQ
**************************************************************************/
void sendDelMsg(struct MsgDetails delMsg)
{

	struct connData *delData = (struct connData *)malloc(sizeof(struct connData));

	delData->dataLen = ntohl(delMsg.dataLength)+3*sizeof(uint8_t)+sizeof delMsg.UOID+sizeof(int);


	pthread_mutex_lock(&existConnLock);
	for(unsigned int i=0; i < existConn.size() ; i++)//Send to all existing connections.
	{
		delData->sendBuf = (unsigned char *)malloc(delData->dataLen);

		memcpy(&delData->sendBuf[0], &delMsg.messageType, sizeof(uint8_t));
		memcpy(&delData->sendBuf[sizeof(uint8_t)], delMsg.UOID, sizeof delMsg.UOID);
		memcpy(&delData->sendBuf[sizeof(uint8_t)+sizeof delMsg.UOID], &delMsg.TTL, sizeof(uint8_t));
		memcpy(&delData->sendBuf[2*sizeof(uint8_t)+sizeof delMsg.UOID], &delMsg.rsrv, sizeof(uint8_t));
		memcpy(&delData->sendBuf[3*sizeof(uint8_t)+sizeof delMsg.UOID], &delMsg.dataLength, sizeof(int));
		memcpy(&delData->sendBuf[3*sizeof(uint8_t)+sizeof delMsg.UOID+sizeof(int)],delMsg.data, ntohl(delMsg.dataLength));

		delData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo)+1);
		memcpy(delData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;
		delData->peerNode[strlen(existConn[i].nodeInfo)] = '\0';
		
		pthread_mutex_lock(&sendQLock);
		if(delMsg.TTL > 0)
			sendQ.push_back(*delData);
		pthread_mutex_unlock(&sendQLock);

		delMsg.kind = 3;
		delMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
		memcpy(delMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
		pthread_mutex_lock(&logLock);
		logQueue.push(delMsg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);		
	}
	pthread_mutex_unlock(&existConnLock);

}

/*************************************************************************
Function to process received delete message
**************************************************************************/
void prcsDelMsg(struct MsgDetails delMsg)
{	
	delMsg.TTL = setTTL(delMsg.TTL);
	delMsg.kind = 1;

	struct connData *rcvdDel = (struct connData *)malloc(sizeof(struct connData));

	rcvdDel->dataLen = ntohl(delMsg.dataLength)+3*sizeof(uint8_t)+sizeof delMsg.UOID+sizeof(int);
   	rcvdDel->sendBuf = (unsigned char *)malloc(rcvdDel->dataLen);

	memcpy(&rcvdDel->sendBuf[0], &delMsg.messageType, sizeof(uint8_t));
	memcpy(&rcvdDel->sendBuf[sizeof(uint8_t)], delMsg.UOID, sizeof delMsg.UOID);
	memcpy(&rcvdDel->sendBuf[sizeof(uint8_t)+sizeof delMsg.UOID], &delMsg.TTL, sizeof(uint8_t));
	memcpy(&rcvdDel->sendBuf[2*sizeof(uint8_t)+sizeof delMsg.UOID], &delMsg.rsrv, sizeof(uint8_t));
	memcpy(&rcvdDel->sendBuf[3*sizeof(uint8_t)+sizeof delMsg.UOID], &delMsg.dataLength, sizeof(int));
	memcpy(&rcvdDel->sendBuf[3*sizeof(uint8_t)+sizeof delMsg.UOID+sizeof(int)],delMsg.data, ntohl(delMsg.dataLength));

	char *rcvdSHA = (char *)malloc(41);
	char *rcvdNonce = (char *)malloc(41);
	char rcvdFileName[256];
	char rcvdPassword[20];
	memset(rcvdFileName,'\0',256);
	int i;
	for (i=9;delMsg.data[i]!='\r';i++)
		rcvdFileName[i-9]=delMsg.data[i];

	rcvdFileName[i]='\0';
	memcpy(rcvdSHA,&delMsg.data[i+7],40);
	memcpy(rcvdNonce,&delMsg.data[i+55],40);
	memcpy(rcvdPassword,&delMsg.data[i+106],20);

	rcvdSHA[40]='\0';
	rcvdNonce[40]='\0';		

	bool foundFile = false;

	int noFiles = 0;
	struct searchData s;	
	strncpy(s.fileName, rcvdFileName, strlen(rcvdFileName));
	struct treeNode* t = NULL;

	pthread_mutex_lock(&treeLock);
	t = searchTree(nameTree, s, 1);


	char *tdataFile;
	char *tpassFile;
	char **matchFiles = (char **)malloc(sizeof(char *));
	if(t != NULL)
	{
		struct treeNode* curr = t;
		tpassFile = (char *)malloc(strlen(curr->data.realName)+1);
		tdataFile = (char *)malloc(strlen(curr->data.realName)+1);		
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

	}
	else if (foundFile == true)
	{
		char fromMetaFName[256];
		memset(fromMetaFName,'\0',256);
		char *fromMetaSHA = (char *)malloc(40);
		char *fromMetaNonce = (char *)malloc(40);
		for (int i=0;i<noFiles;i++)
		{
			parseMetaData(matchFiles[i],NULL,fromMetaFName,fromMetaSHA,fromMetaNonce,NULL,NULL);									
			if (!strcmp(rcvdFileName,fromMetaFName))
			{				
				if (!strcmp(rcvdSHA,fromMetaSHA))
				{					
					char *hexNonce = (char *)malloc(40);

					SHA_CTX shaPointer;
					SHA1_Init(&shaPointer);
					char *nonce = (char *)malloc(SHA_DIGEST_LENGTH);
					SHA1_Init(&shaPointer);
					SHA1_Update(&shaPointer, rcvdPassword, sizeof rcvdPassword);
					SHA1_Final((unsigned char *)nonce, &shaPointer);
					hexNonce = (char *)convToHex((unsigned char *)nonce, SHA_DIGEST_LENGTH);					

					if (!strncmp(hexNonce,rcvdNonce, 40))
					{
						
						remove(matchFiles[i]);
						strcpy(tdataFile,matchFiles[i]);
						tdataFile[strlen(tdataFile)]='\0';
						tdataFile[strlen(tdataFile)-4]='d';
						tdataFile[strlen(tdataFile)-3]='a';

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
						remove(tdataFile);
						break;
					}

				}
			}
		}
	}
	pthread_mutex_lock(&logLock);
	logQueue.push(delMsg);
	pthread_mutex_unlock(&logLock);
	pthread_cond_signal(&forLogging);

	if(delMsg.TTL <= 0)
			return;

	pthread_mutex_lock(&existConnLock);
	for(unsigned int i=0; i < existConn.size() ; i++)//Forwarding
	{
		if(existConn[i].peerSock != delMsg.pSock)
		{			

			rcvdDel->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
			memcpy(rcvdDel->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));

			pthread_mutex_lock(&sendQLock);
			if(delMsg.TTL > 0)
				sendQ.push_back(*rcvdDel);
			pthread_mutex_unlock(&sendQLock);

			delMsg.kind = 2;
			delMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
			memcpy(delMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
			pthread_mutex_lock(&logLock);
			logQueue.push(delMsg);
			pthread_mutex_unlock(&logLock);
			pthread_cond_signal(&forLogging);

			delMsg.nodeID = NULL;			
		}
	}
	pthread_mutex_unlock(&existConnLock);
}

/*************************************************************************
Function to process received check response.
*************************************************************************/
void prcsChkRsp(struct MsgDetails chkRsp)
{
	pthread_mutex_lock(&statLock);
	if(waitingForCheck && !memcmp(myChkUOID, &chkRsp.data[0], sizeof myChkUOID))//If my check message, reset flag, return
	{
		waitingForCheck = false;
	}
	else //Forward message to original sender
	{
		chkRsp.TTL = setTTL(chkRsp.TTL);
		char rspUOID[20];
		memcpy(rspUOID, chkRsp.data, sizeof rspUOID);

	   struct connData *fwdMsg = (struct connData *)malloc(sizeof(struct connData));

	   fwdMsg->dataLen = (3*sizeof(uint8_t)+sizeof chkRsp.UOID+sizeof(int)+ntohl(chkRsp.dataLength));
	   fwdMsg->sendBuf = (unsigned char*)malloc(fwdMsg->dataLen);

	   memcpy(&fwdMsg->sendBuf[0], &chkRsp.messageType, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)], chkRsp.UOID, sizeof chkRsp.UOID);
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)+sizeof chkRsp.UOID], &chkRsp.TTL, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[2*sizeof(uint8_t)+sizeof chkRsp.UOID], &chkRsp.rsrv, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof chkRsp.UOID], &chkRsp.dataLength, sizeof(int));

	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof chkRsp.UOID+sizeof(int)], chkRsp.data, ntohl(chkRsp.dataLength));

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
		{
			if(!memcmp(rspUOID, cacheUOID[j].msgUOID, sizeof rspUOID))
			{
				fwdMsg->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
				strcpy(fwdMsg->peerNode, cacheUOID[j].senderNode);

				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*fwdMsg);
				pthread_mutex_unlock(&sendQLock);

				chkRsp.kind = 2;
				chkRsp.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
				memcpy(chkRsp.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
				pthread_mutex_lock(&logLock);
				logQueue.push(chkRsp);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
				break;
			}
		}
		pthread_mutex_unlock(&timerLock);
		free(fwdMsg);
	}
	pthread_mutex_unlock(&statLock);

}

/***************************************************************
Function to probabilistically flood a store message
***************************************************************/
void sendStore(struct MsgDetails stMsg)
{

	int metaFileSize,storeFileSize;

	struct connData *strData = (struct connData *)malloc(sizeof(struct connData));

	char *storeFileNme = (char *)malloc(strlen(myHomeFiles)+strlen(stMsg.data)+strlen(".data")+2);
	char *metaFileNme = (char *)malloc(strlen(myHomeFiles)+strlen(stMsg.data)+strlen(".meta")+2);

	strcpy(storeFileNme, myHomeFiles);
	strcat(storeFileNme, "/");
	strcat(storeFileNme,stMsg.data);
	strcat(storeFileNme,".data");
	storeFileNme[strlen(myHomeFiles)+strlen(stMsg.data)+strlen(".data")+1] = '\0';

	strcpy(metaFileNme, myHomeFiles);
	strcat(metaFileNme, "/");
	strcat(metaFileNme,stMsg.data);
	strcat(metaFileNme,".meta");
	metaFileNme[strlen(myHomeFiles)+strlen(stMsg.data)+strlen(".meta")+1] = '\0';

	free(stMsg.data);
	stMsg.rsrv = 0;
	char* temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");
	
	GetUOID(node.nodeInstance, temp, (char *)stMsg.UOID, sizeof stMsg.UOID);

	struct timerSt *pTimer = (struct timerSt*)malloc(sizeof(struct timerSt));
	memcpy(pTimer->msgUOID, stMsg.UOID, sizeof stMsg.UOID);
	pTimer->timeout = node.msgLifeTime;
	pTimer->senderNode = NULL;
	
	pthread_mutex_lock(&timerLock);
	cacheUOID.push_back(*pTimer);
	pthread_mutex_unlock(&timerLock);

	struct stat fs;
	if (stat(metaFileNme,&fs)==0)
		metaFileSize = fs.st_size;
	if (stat(storeFileNme,&fs)==0)
		storeFileSize = fs.st_size;

	stMsg.dataLength = sizeof(int)+metaFileSize+storeFileSize;
	ifstream fp;
	metaFileSize = htonl(metaFileSize);

	stMsg.dataLength = htonl(stMsg.dataLength);	

	if (ntohl(stMsg.dataLength) < MAXBUFFERSIZE)
	{
		stMsg.data = (char *)malloc(ntohl(stMsg.dataLength));
		memcpy(&stMsg.data[0],&metaFileSize,sizeof(int));
		fp.open(metaFileNme,ios::in|ios::binary);
		fp.read(&stMsg.data[sizeof(int)],ntohl(metaFileSize));
		fp.close();
		fp.open(storeFileNme,ios::in|ios::binary);
		fp.read(&stMsg.data[sizeof(int)+ntohl(metaFileSize)],storeFileSize);
		fp.close();

		strData->dataLen = ntohl(stMsg.dataLength)+headerLen;

		stMsg.kind = 3;
		pthread_mutex_lock(&existConnLock);
		for (unsigned int j=0; j<existConn.size(); j++)
		{
				strData->sendBuf = (unsigned char *)malloc(ntohl(stMsg.dataLength)+headerLen);
				memcpy(&strData->sendBuf[0], &stMsg.messageType, sizeof(uint8_t));
				memcpy(&strData->sendBuf[sizeof(uint8_t)], stMsg.UOID, sizeof stMsg.UOID);
				memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.TTL, sizeof(uint8_t));
				memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.rsrv, sizeof(uint8_t));
				memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.dataLength, sizeof(int));
				memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int)], stMsg.data, ntohl(stMsg.dataLength));

			if (drand48()<node.neighborStoreProb)
			{					
				strData->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
				strcpy(strData->peerNode,existConn[j].nodeInfo);
				strData->peerNode[strlen(existConn[j].nodeInfo)]='\0';
				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*strData);
				pthread_mutex_unlock(&sendQLock);

				stMsg.nodeID = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
				strcpy(stMsg.nodeID,existConn[j].nodeInfo);
				pthread_mutex_lock(&logLock);
				logQueue.push(stMsg);
				pthread_mutex_unlock(&logLock);
			}
		}
		pthread_mutex_unlock(&existConnLock);
	}
	else
	{
		int Qt = (int)storeFileSize/MAXBUFFERSIZE;
		int Rmn = storeFileSize%MAXBUFFERSIZE;


		strData->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
		memcpy(&strData->sendBuf[0], &stMsg.messageType, sizeof(uint8_t));
		memcpy(&strData->sendBuf[sizeof(uint8_t)], stMsg.UOID, sizeof stMsg.UOID);
		memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.TTL, sizeof(uint8_t));
		memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.rsrv, sizeof(uint8_t));
		memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.dataLength, sizeof(int));
		stMsg.kind = 3;
		strData->dataLen = headerLen+sizeof(int)+ntohl(metaFileSize);

		memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int)], &metaFileSize, sizeof(int));

		fp.open(metaFileNme,ios::in|ios::binary);
		fp.read((char *)&strData->sendBuf[2*sizeof(int)+3*sizeof(uint8_t)+sizeof stMsg.UOID],ntohl(metaFileSize));
		fp.close();



		pthread_mutex_lock(&existConnLock);
		for(unsigned int j=0; j<existConn.size() ; j++)
		{
			if (drand48()<node.neighborStoreProb)
			{
				fp.open(storeFileNme,ios::in|ios::binary);				
				strData->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
				strcpy(strData->peerNode,existConn[j].nodeInfo);
				strData->peerNode[strlen(existConn[j].nodeInfo)]='\0';

				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*strData);
				pthread_mutex_unlock(&sendQLock);

				struct connData *tempS = (struct connData*)malloc(sizeof(struct connData));				

				tempS->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
				strcpy(tempS->peerNode,existConn[j].nodeInfo);
				tempS->peerNode[strlen(existConn[j].nodeInfo)]='\0';

				for(int k=0; k < Qt ; k++)
				{
					tempS->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
					memset(tempS->sendBuf, '\0', MAXBUFFERSIZE);
					fp.read((char *)&tempS->sendBuf[0],MAXBUFFERSIZE);
					tempS->dataLen = MAXBUFFERSIZE;

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*tempS);
					pthread_mutex_unlock(&sendQLock);
				}

				if(Rmn != 0)
				{
					tempS->sendBuf = (unsigned char *)malloc(Rmn);
					memset(tempS->sendBuf, '\0', Rmn);
					fp.read((char *)&tempS->sendBuf[0], Rmn);
					tempS->dataLen = Rmn;

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*tempS);
					pthread_mutex_unlock(&sendQLock);
				}

				stMsg.nodeID = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
				strcpy(stMsg.nodeID,existConn[j].nodeInfo);
				pthread_mutex_lock(&logLock);
				logQueue.push(stMsg);
				pthread_mutex_unlock(&logLock);

				free(tempS);
				fp.close();
			}
		}
		pthread_mutex_unlock(&existConnLock);

	}
}

/**************************************************************
Function to format Keep Alive messages. Logging not done here,
message not pushed into sendQ here
**************************************************************/
struct connData * sendKeepAlive(struct MsgDetails kMsg)
{
	struct connData *kData = (struct connData *)malloc(sizeof(struct connData));

	char* temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");
	GetUOID(node.nodeInstance, temp, (char *)kMsg.UOID, sizeof kMsg.UOID);
	kMsg.TTL = 1;
	kMsg.rsrv = 0;
	kMsg.dataLength = 0;

	kData->dataLen = sizeof kMsg.UOID + 3*sizeof(uint8_t) + sizeof(int);
   	kData->sendBuf = (unsigned char *)malloc(kData->dataLen);

   memcpy(&kData->sendBuf[0], &kMsg.messageType, sizeof(uint8_t));
   memcpy(&kData->sendBuf[sizeof(uint8_t)], kMsg.UOID, sizeof kMsg.UOID);
   memcpy(&kData->sendBuf[sizeof(uint8_t)+sizeof kMsg.UOID], &kMsg.TTL, sizeof(uint8_t));
   memcpy(&kData->sendBuf[2*sizeof(uint8_t)+sizeof kMsg.UOID], &kMsg.rsrv, sizeof(uint8_t));
   memcpy(&kData->sendBuf[3*sizeof(uint8_t)+sizeof kMsg.UOID], &kMsg.dataLength, sizeof(int));

   kData->peerNode = (char *)malloc(strlen(kMsg.nodeID));
   strncpy(kData->peerNode, kMsg.nodeID, strlen(kMsg.nodeID));

   return kData;

	kMsg.kind = 3;
	pthread_mutex_lock(&logLock);
	logQueue.push(kMsg);
	pthread_mutex_unlock(&logLock);
	pthread_cond_signal(&forLogging);
}


/*******************************************************************************
Function to process received store message. Probabilistically
Forward the store msg and probabilistically store the received file
*******************************************************************************/
void prcsStore(struct MsgDetails sMsg)
{
	struct connData *strData = (struct connData *)malloc(sizeof(struct connData));
	int storeFileSize;
	ifstream fp;
	
	char *storeFileNme = (char *)malloc(strlen(myHomeFiles)+strlen(sMsg.data)+strlen(".data")+2);
	char *metaFileNme = (char *)malloc(strlen(myHomeFiles)+strlen(sMsg.data)+strlen(".meta")+2);

	strcpy(storeFileNme, myHomeFiles);
	strcat(storeFileNme, "/");
	strcat(storeFileNme,sMsg.data);
	strcat(storeFileNme,".data");
	storeFileNme[strlen(myHomeFiles)+strlen(sMsg.data)+strlen(".data")+1] = '\0';

	strcpy(metaFileNme, myHomeFiles);
	strcat(metaFileNme, "/");
	strcat(metaFileNme,sMsg.data);
	strcat(metaFileNme,".meta");
	metaFileNme[strlen(myHomeFiles)+strlen(sMsg.data)+strlen(".meta")+1] = '\0';

	struct stat fs;
	int metaFileSize;
	if (stat(metaFileNme,&fs)==0)
		metaFileSize = fs.st_size;
	if (stat(storeFileNme,&fs)==0)
		storeFileSize = fs.st_size;
	

	if(drand48() < node.storeProb)
	{

		pthread_mutex_lock(&cacheLock);
		if(storeFileSize+currCacheSize > node.cacheSize)
		{

			if(storeFileSize < node.cacheSize)
			{
			
			char *tdataFile = (char *)malloc(strlen(fileCache[0].fileName));
			strncpy(tdataFile, fileCache[0].fileName, strlen(fileCache[0].fileName));
			tdataFile[strlen(fileCache[0].fileName)] = '\0';

			pthread_mutex_lock(&treeLock);
			struct treeNode *t = NULL;
			struct searchData s;
			strncpy(s.realName, tdataFile, strlen(tdataFile));

			t = searchTree(nameTree, s, 2);

			if(t != NULL)
			{
				memset(t->data.fileName, '\0', sizeof t->data.fileName);
				memset(t->data.sha_val, '\0', sizeof t->data.sha_val);
			}
			pthread_mutex_unlock(&treeLock);

			pthread_mutex_lock(&bitVStoreLock);
			for(unsigned int j=0 ; j < bitVStorage.size() ; j++)
			{
				if(!strncmp(bitVStorage[j].fName, tdataFile,strlen(tdataFile)))
				{
					bitVStorage.erase(bitVStorage.begin() + j);
				}
			}
			pthread_mutex_unlock(&bitVStoreLock);

		  while((storeFileSize+currCacheSize) > node.cacheSize)
		  {
			if(fileCache.size() > 0)
			{
				currCacheSize -= fileCache[0].fileSz;
				fileCache.erase(fileCache.begin());
			}
			else
			{
				remove(metaFileNme);
				remove(storeFileNme);
				pthread_mutex_unlock(&cacheLock);
				return;
			}
		  }


		}
		else
		{
			remove(metaFileNme);
			remove(storeFileNme);
			pthread_mutex_unlock(&cacheLock);
			return;
		}
	 }

		currCacheSize += storeFileSize;
		pthread_mutex_unlock(&cacheLock);

		struct searchData newS;
		strcpy(newS.realName, storeFileNme);
		newS.realName[strlen(storeFileNme)] = '\0';

		struct bitVect cVect;
		parseMetaData(metaFileNme,cVect.vArr, newS.fileName, newS.sha_val, NULL, NULL, NULL);
		strcpy(cVect.fName, storeFileNme);
		cVect.fName[strlen(storeFileNme)] = '\0';

		pthread_mutex_lock(&treeLock);
		insertInTree(newS);
		pthread_mutex_unlock(&treeLock);

		pthread_mutex_lock(&bitVStoreLock);
		bitVStorage.push_back(cVect);
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

		struct cacheStruct newEntry;
		newEntry.fileSz = ntohl(sMsg.dataLength) - (sizeof(int)+metaFileSize);
		strcpy(newEntry.fileName, storeFileNme);
		newEntry.fileName[strlen(storeFileNme)] = '\0';

		//push into file cache
		pthread_mutex_lock(&cacheLock);
		fileCache.push_back(newEntry);
		pthread_mutex_unlock(&cacheLock);

		sMsg.TTL = setTTL(sMsg.TTL);

		if(sMsg.TTL <= (uint8_t)0)
			return;

		//Fwd Message probabilistically
		if (ntohl(sMsg.dataLength) < MAXBUFFERSIZE)
		{
			sMsg.data = (char *)malloc(ntohl(sMsg.dataLength));
			memcpy(&sMsg.data[0],&metaFileSize,sizeof(int));
			fp.open(metaFileNme,ios::in|ios::binary);
			fp.read(&sMsg.data[sizeof(int)],ntohl(metaFileSize));
			fp.close();
			fp.open(storeFileNme,ios::in|ios::binary);
			fp.read(&sMsg.data[sizeof(int)+ntohl(metaFileSize)],storeFileSize);
			fp.close();

			sMsg.kind = 2;
			strData->dataLen = headerLen + ntohl(sMsg.dataLength);
			pthread_mutex_lock(&existConnLock);
			for (unsigned int j=0; j<existConn.size(); j++)
			{
				if(existConn[j].peerSock == sMsg.pSock)
					continue;


				if (drand48() < node.neighborStoreProb)
				{
					strData->sendBuf = (unsigned char *)malloc(ntohl(sMsg.dataLength)+headerLen);
					memcpy(&strData->sendBuf[0], &sMsg.messageType, sizeof(uint8_t));
					memcpy(&strData->sendBuf[sizeof(uint8_t)], sMsg.UOID, sizeof sMsg.UOID);
					memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.TTL, sizeof(uint8_t));
					memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.rsrv, sizeof(uint8_t));
					memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.dataLength, sizeof(int));
					memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)], sMsg.data, ntohl(sMsg.dataLength));

					strData->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
					strcpy(strData->peerNode,existConn[j].nodeInfo);
					strData->peerNode[strlen(existConn[j].nodeInfo)]='\0';
					
					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);

					sMsg.nodeID = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
					strcpy(sMsg.nodeID,existConn[j].nodeInfo);
					pthread_mutex_lock(&logLock);
					logQueue.push(sMsg);
					pthread_mutex_unlock(&logLock);
				}
			}
			pthread_mutex_unlock(&existConnLock);
		}
		else
		{
			int Qt = (int)storeFileSize/MAXBUFFERSIZE;
			int Rmn = storeFileSize%MAXBUFFERSIZE;

			strData->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
			memcpy(&strData->sendBuf[0], &sMsg.messageType, sizeof(uint8_t));
			memcpy(&strData->sendBuf[sizeof(uint8_t)], sMsg.UOID, sizeof sMsg.UOID);
			memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.TTL, sizeof(uint8_t));
			memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.rsrv, sizeof(uint8_t));
			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.dataLength, sizeof(int));
			sMsg.kind = 2;
			strData->dataLen = headerLen+sizeof(int)+ntohl(metaFileSize);

			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)], &metaFileSize, sizeof(int));

			fp.open(metaFileNme,ios::in|ios::binary);
			fp.read((char *)&strData->sendBuf[2*sizeof(int)+3*sizeof(uint8_t)+sizeof sMsg.UOID],ntohl(metaFileSize));
			fp.close();

			fp.open(storeFileNme,ios::in|ios::binary);
			pthread_mutex_lock(&existConnLock);
			for(unsigned int j=0; j<existConn.size() ; j++)
			{
				if(existConn[j].peerSock == sMsg.pSock)
					continue;
				
				if (drand48()<node.neighborStoreProb)
				{
					strData->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
					strcpy(strData->peerNode,existConn[j].nodeInfo);
					strData->peerNode[strlen(existConn[j].nodeInfo)]='\0';

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);

					struct connData *tempS = (struct connData*)malloc(sizeof(struct connData));					
					tempS->peerNode = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
					strcpy(tempS->peerNode,existConn[j].nodeInfo);
					tempS->peerNode[strlen(existConn[j].nodeInfo)]='\0';

					for(int k=0; k < Qt ; k++)
					{
						tempS->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
						memset(tempS->sendBuf, '\0', MAXBUFFERSIZE);
						fp.read((char *)&tempS->sendBuf[0],MAXBUFFERSIZE);
						tempS->dataLen = MAXBUFFERSIZE;

						pthread_mutex_lock(&sendQLock);
						sendQ.push_back(*tempS);
						pthread_mutex_unlock(&sendQLock);
					}

					if(Rmn != 0)
					{
						tempS->sendBuf = (unsigned char *)malloc(Rmn);
						memset(tempS->sendBuf, '\0', Rmn);
						fp.read((char *)&tempS->sendBuf[0], Rmn);
						tempS->dataLen = Rmn;

						pthread_mutex_lock(&sendQLock);
						sendQ.push_back(*tempS);
						pthread_mutex_unlock(&sendQLock);
					}

					sMsg.nodeID = (char *)malloc(strlen(existConn[j].nodeInfo)+1);
					strcpy(sMsg.nodeID,existConn[j].nodeInfo);
					pthread_mutex_lock(&logLock);
					logQueue.push(sMsg);
					pthread_mutex_unlock(&logLock);

					free(tempS);
				}
			}
			pthread_mutex_unlock(&existConnLock);
			fp.close();
		}
	}
	else
	{
		remove(metaFileNme);
		remove(storeFileNme);
	}
}


/*******************************************************************************
Function to process received search response. If the search response is not
for this node, then forward it.
*******************************************************************************/
void prcsSearchRsp(struct MsgDetails rMsg)
{
	if(!memcmp(mySrcUOID, rMsg.data, sizeof mySrcUOID))
	{
		int recLen = 0;
		int processed = 0;
		int noFiles = mySrcResult;
		ofstream op;
		char fileName[256];
		memset(fileName, '\0', 256);
		int* fileSz = new int[1];
		char sha[41];
		char nonce[41];
		char keyWrd[256];
		memset(keyWrd, '\0', 256);

		char *tName = (char *)malloc(strlen(".meta")+1+sizeof(int));
		memset(tName, '\0', strlen(".meta")+1+sizeof(int));
		char *tempFile = (char *)malloc(strlen(myHomeFiles)+1+strlen("$")+strlen(".meta")+sizeof(int)+1);
		strcpy(tempFile, myHomeFiles);
		strcat(tempFile, "/");
		strcat(tempFile, "$");

		memcpy(&recLen, &rMsg.data[sizeof mySrcUOID], sizeof(int));
		processed = sizeof mySrcUOID;

		while(ntohl(recLen) != 0)
		{
			memset(tName, '\0', strlen(".meta")+1+sizeof(int));
			sprintf(tName, "%d", noFiles);
			strcat(tName, ".meta");

			strcpy(&tempFile[strlen(myHomeFiles)+2], tName);
			cout << "[" << noFiles << "]" << "	FileID=" << convToHex((unsigned char*)&rMsg.data[processed+sizeof(int)], sizeof mySrcUOID) << endl;

			getFileId = (char **)realloc(getFileId, sizeof(char *)*noFiles);
			getFileId[noFiles-1] = (char *)malloc(sizeof mySrcUOID);
			memcpy(getFileId[noFiles-1], &rMsg.data[processed+sizeof(int)], sizeof mySrcUOID);

			tempFile[strlen(tempFile)]='\0';
			op.open(tempFile, ios::out);
			if(op.is_open())
			{
				op.write(&rMsg.data[processed+sizeof(int)+sizeof mySrcUOID], ntohl(recLen));
			}
			op.close();

			parseMetaData(tempFile, NULL, fileName, sha, nonce, keyWrd, fileSz);
			fileName[strlen(fileName)]='\0';
			keyWrd[strlen(keyWrd)]='\0';
			sha[40] = '\0';
			nonce[40] = '\0';
			cout << "   	FileName=" << fileName << endl;
			cout << "	SHA1=" << sha << endl;
			cout << "	Nonce=" << nonce << endl;
			cout << "	Keywords=" << keyWrd << endl;

			getFileSha = (char **)realloc(getFileSha, sizeof(char *)*noFiles);
			getFileSha[noFiles-1] = (char *)malloc(sizeof mySrcUOID);

			for(unsigned int l=0; l < (sizeof nonce)/2 ; l++)
			{
				(getFileSha[noFiles-1])[l] = (uint8_t)returnDecValue(&sha[2*l]);
			}

			defGetFileName = (char **)realloc(defGetFileName, sizeof(char *)*noFiles);
			defGetFileName[noFiles-1] = (char *)malloc(strlen(fileName)+1);
			memcpy(defGetFileName[noFiles-1], fileName, strlen(fileName));
			(defGetFileName[noFiles-1])[strlen(fileName)] = '\0';

			processed = processed + sizeof(int) + sizeof mySrcUOID + ntohl(recLen);
			memcpy(&recLen, &rMsg.data[processed], sizeof(int));
			noFiles++;
		}

		memset(tName, '\0', strlen(".meta")+1+sizeof(int));
		sprintf(tName, "%d", noFiles);
		strcat(tName, ".meta");

		strcpy(&tempFile[strlen(myHomeFiles)+2], tName);
		cout << "[" << noFiles << "]" << "	FileID=" << convToHex((unsigned char*)&rMsg.data[processed+sizeof(int)], sizeof mySrcUOID) << endl;

		getFileId = (char **)realloc(getFileId, sizeof(char *)*noFiles);
		getFileId[noFiles-1] = (char *)malloc(sizeof mySrcUOID);
		memcpy(getFileId[noFiles-1], &rMsg.data[processed+sizeof(int)], sizeof mySrcUOID);

		op.open(tempFile, ios::out);
		if(op.is_open())
		{
			op.write(&rMsg.data[processed+sizeof(int)+sizeof mySrcUOID], ntohl(rMsg.dataLength)-(processed+sizeof(int)+sizeof mySrcUOID));
		}
		op.close();

		parseMetaData(tempFile, NULL, fileName, sha, nonce, keyWrd, fileSz);
		fileName[strlen(fileName)]='\0';
		keyWrd[strlen(keyWrd)]='\0';
		sha[40] = '\0';
		nonce[40] = '\0';
		cout << "   	FileName=" << fileName << endl;
		cout << "	SHA1=" << sha << endl;
		cout << "	Nonce=" << nonce << endl;
		cout << "	Keywords=" << keyWrd << endl;

		getFileSha = (char **)realloc(getFileSha, sizeof(char *)*noFiles);
		getFileSha[noFiles-1] = (char *)malloc(sizeof mySrcUOID);

		for(unsigned int l=0; l < (sizeof nonce)/2 ; l++)
		{
			(getFileSha[noFiles-1])[l] = (uint8_t)returnDecValue(&sha[2*l]);
		}


		defGetFileName = (char **)realloc(defGetFileName, sizeof(char *)*noFiles);
		defGetFileName[noFiles-1] = (char *)malloc(strlen(fileName));
		memcpy(defGetFileName[noFiles-1], fileName, strlen(fileName));
		(defGetFileName[noFiles-1])[strlen(fileName)] = '\0';

		mySrcResult = noFiles+1;

		pthread_mutex_lock(&statusRqsLock);
		allowGetCmd = true;
		pthread_mutex_unlock(&statusRqsLock);
	}
	else
	{
		//Forward reply to original sender
		rMsg.TTL = setTTL(rMsg.TTL);

		if(rMsg.TTL <= 0)
			return;

		char rspUOID[20];
		memcpy(rspUOID, rMsg.data, sizeof rspUOID);

	   struct connData *fwdMsg = (struct connData *)malloc(sizeof(struct connData));

	   fwdMsg->dataLen = (3*sizeof(uint8_t)+sizeof rMsg.UOID+sizeof(int)+ntohl(rMsg.dataLength));
	   fwdMsg->sendBuf = (unsigned char*)malloc(fwdMsg->dataLen);
	   memcpy(&fwdMsg->sendBuf[0], &rMsg.messageType, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)], rMsg.UOID, sizeof rMsg.UOID);
	   memcpy(&fwdMsg->sendBuf[sizeof(uint8_t)+sizeof rMsg.UOID], &rMsg.TTL, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[2*sizeof(uint8_t)+sizeof rMsg.UOID], &rMsg.rsrv, sizeof(uint8_t));
	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof rMsg.UOID], &rMsg.dataLength, sizeof(int));

	   memcpy(&fwdMsg->sendBuf[3*sizeof(uint8_t)+sizeof rMsg.UOID+sizeof(int)], rMsg.data, ntohl(rMsg.dataLength));

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
		{
			if(!memcmp(rspUOID, cacheUOID[j].msgUOID, sizeof rspUOID))
			{
				fwdMsg->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode));
				strncpy(fwdMsg->peerNode, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
				fwdMsg->peerNode[strlen(cacheUOID[j].senderNode)] = '\0';				

				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*fwdMsg);
				pthread_mutex_unlock(&sendQLock);

				rMsg.kind = 2;
				rMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
				memcpy(rMsg.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
				pthread_mutex_lock(&logLock);
				logQueue.push(rMsg);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
				break;
			}
		}
		pthread_mutex_unlock(&timerLock);
		free(fwdMsg);
	}
}

/*******************************************************************************
Function to process received search message. forward the request and
reply to sending node.
*******************************************************************************/
void prcsSearchRq(struct MsgDetails pMsg, bool mySearch)
{
	char **keywords;
	char *fileParam = (char *)malloc(ntohl(pMsg.dataLength));
	int type = 0;
	char *msgData = NULL;
	char reqUOID[20];
	char **matchFiles = (char **)malloc(sizeof(char *));
	int noFiles = 0;
	string tStr;
	bool foundFile = false;	

	if(!memcmp(pMsg.data, &type1, sizeof(uint8_t)))
		type = 1;
	else if(!memcmp(pMsg.data, &type2, sizeof(uint8_t)))
		type = 2;
	else if(!memcmp(pMsg.data, &type3, sizeof(uint8_t)))
		type = 3;

	memcpy(reqUOID, pMsg.UOID, sizeof reqUOID);
	struct connData *rData;

	if(!mySearch)
	{
		rData = (struct connData *)malloc(sizeof(struct connData));
		pMsg.TTL = setTTL(pMsg.TTL);
		//Forward
		rData->dataLen = 3*sizeof(uint8_t)+sizeof pMsg.UOID+ sizeof(int)+ntohl(pMsg.dataLength);
		rData->sendBuf = (unsigned char *)malloc(rData->dataLen);

		pthread_mutex_lock(&existConnLock);
		for(unsigned int i=0; i < existConn.size() ; i++)
		{
		   memcpy(&rData->sendBuf[0], &pMsg.messageType, sizeof(uint8_t));
		   memcpy(&rData->sendBuf[sizeof(uint8_t)], pMsg.UOID, sizeof pMsg.UOID);
		   memcpy(&rData->sendBuf[sizeof(uint8_t)+sizeof pMsg.UOID], &pMsg.TTL, sizeof(uint8_t));
		   memcpy(&rData->sendBuf[2*sizeof(uint8_t)+sizeof pMsg.UOID], &pMsg.rsrv, sizeof(uint8_t));
		   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof pMsg.UOID], &pMsg.dataLength, sizeof(int));

		   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof pMsg.UOID+sizeof(int)], pMsg.data, ntohl(pMsg.dataLength));

			if(existConn[i].peerSock != pMsg.pSock)
			{
				rData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo)+1);
				memcpy(rData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;
				rData->peerNode[strlen(existConn[i].nodeInfo)]='\0';

				pthread_mutex_lock(&sendQLock);
				if(pMsg.TTL > 0)
					sendQ.push_back(*rData);
				pthread_mutex_unlock(&sendQLock);

				pMsg.kind = 2;
				pMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(pMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
				pthread_mutex_lock(&logLock);
				logQueue.push(pMsg);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
			}
		}
		pthread_mutex_unlock(&existConnLock);
		free(rData);
	}



	if(type == 3)
	{
		char *tmp;
		memcpy(fileParam, &pMsg.data[sizeof(uint8_t)], ntohl(pMsg.dataLength)-sizeof(uint8_t));
		fileParam[ntohl(pMsg.dataLength)-sizeof(uint8_t)] = '\0';

		tmp = strtok(fileParam," ");
		keywords=(char **)malloc(sizeof(char *));
		int i = 0;

		while (tmp!=NULL)
		{
			keywords=(char **)realloc(keywords,(i+1)*sizeof(char *));
			keywords[i] = (char *)malloc(strlen(tmp));
			strcpy(keywords[i],tmp);
			tmp = strtok(NULL," ");			
			i++;
		}

		char bitVectArr[256];
		generateBitVector(bitVectArr, keywords, i);
		bool match = false;
		char *metaFile;
		char fileKey[256];

		size_t fPos;

		int pMatches = 0;
		int maxMatches = 0;
		char *transferFile;
		uint8_t *n1;


		pthread_mutex_lock(&bitVStoreLock);
		pthread_mutex_lock(&nodeLock);
		for(unsigned int j=0; j<bitVStorage.size() ; j++)
		{
			node.intArray = generateIntFromHex(bitVectArr);			
			n1 = generateIntFromHex(bitVStorage[j].vArr);
			match = node & n1;

			if(match)
			{
				metaFile = (char *)malloc(strlen(bitVStorage[j].fName)+1);
				strcpy(metaFile, bitVStorage[j].fName);
				metaFile[strlen(bitVStorage[j].fName)-3] = 'e';
				metaFile[strlen(bitVStorage[j].fName)-4] = 'm';
				metaFile[strlen(bitVStorage[j].fName)] = '\0';

				parseMetaData(metaFile, NULL, NULL, NULL, NULL, fileKey, NULL);
				tStr.assign(fileKey);

				for(int k=0; k < i ; k++)
				{

					fPos = tStr.find_first_of(keywords[k], 0);

					if(fPos != string::npos)
					{
						maxMatches = pMatches;
						transferFile = (char *)malloc(strlen(metaFile)+1);
						strcpy(transferFile, metaFile);
						transferFile[strlen(metaFile)] ='\0';

						matchFiles = (char **)realloc(matchFiles, sizeof(char *)*(noFiles+1));
						matchFiles[noFiles] = (char *)malloc(strlen(transferFile)+1);
						strcpy(matchFiles[noFiles], transferFile);
						(matchFiles[noFiles])[strlen(transferFile)] = '\0';
						foundFile = true;
						
						noFiles++;
						break;
					}
				}
				match = false;
			}
		}
		pthread_mutex_unlock(&nodeLock);
		pthread_mutex_unlock(&bitVStoreLock);
	}
	else if(type == 1)
	{		
		struct searchData s;		
		strncpy(s.fileName, &pMsg.data[sizeof(uint8_t)], ntohl(pMsg.dataLength)-sizeof(uint8_t));
		struct treeNode* t;

		pthread_mutex_lock(&treeLock);
		t = searchTree(nameTree, s, 1);

		if(t != NULL)
		{
			struct treeNode* curr = t;
			
				char *metaFile = (char *)malloc(strlen(curr->data.realName)+1);
				strcpy(metaFile, curr->data.realName);

				metaFile[strlen(curr->data.realName)-3] = 'e';
				metaFile[strlen(curr->data.realName)-4] = 'm';
				metaFile[strlen(curr->data.realName)] = '\0';

				matchFiles = (char **)realloc(matchFiles, sizeof(char *)*(noFiles+1));
				matchFiles[noFiles] = (char *)malloc(strlen(metaFile)+1);
				strncpy(matchFiles[noFiles], metaFile, strlen(metaFile));
				(matchFiles[noFiles])[strlen(metaFile)] = '\0';

				foundFile = true;
				noFiles++;				
		}
		pthread_mutex_unlock(&treeLock);
	}
	else if(type == 2)
	{		
		struct searchData s;
		
		strncpy(s.sha_val, &pMsg.data[sizeof(uint8_t)], ntohl(pMsg.dataLength)-sizeof(uint8_t));
		struct treeNode* t;

		pthread_mutex_lock(&treeLock);
		t = searchTree(shaTree, s, 0);


		if(t != NULL)
		{
			struct treeNode* curr = t;

				char *metaFile = (char *)malloc(strlen(curr->data.realName)+1);
				strcpy(metaFile, curr->data.realName);

				metaFile[strlen(curr->data.realName)-3] = 'e';
				metaFile[strlen(curr->data.realName)-4] = 'm';
				metaFile[strlen(curr->data.realName)] = '\0';

				matchFiles = (char **)realloc(matchFiles, sizeof(char *)*(noFiles+1));
				matchFiles[noFiles] = (char *)malloc(strlen(metaFile)+1);
				strncpy(matchFiles[noFiles], metaFile, strlen(metaFile));
				(matchFiles[noFiles])[strlen(metaFile)] = '\0';

				foundFile = true;
				noFiles++;				
		}
		pthread_mutex_unlock(&treeLock);
	}

		//Send actual Message
		MsgDetails rspMsg;
		rspMsg.messageType = SHRS;
		rspMsg.TTL = (uint8_t)node.ttl;
		rspMsg.rsrv = 0;
		char* temp = (char *)malloc(strlen("peer"));
		strcpy(temp, "peer");
		GetUOID(node.nodeInstance, temp, (char *)rspMsg.UOID, sizeof rspMsg.UOID);
		ifstream ip;

	struct stat st;
	int recordLen = 0;
	char *tempHol;
	int totalData = sizeof reqUOID;
	msgData = (char *)malloc(sizeof reqUOID);
	memcpy(&msgData[0], reqUOID, sizeof reqUOID);
	char nonce[40];
	uint8_t fileID[20];

	rspMsg.dataLength = sizeof reqUOID;

	for(int k =0; k < noFiles; k++)
	{
		if(stat(matchFiles[k], &st) == 0)
			rspMsg.dataLength = rspMsg.dataLength + sizeof(int) + st.st_size + sizeof fileID;
	}


	if(rspMsg.dataLength < MAXBUFFERSIZE)
	{
		for(int k=0; k<noFiles ; k++)
		{
			if(stat(matchFiles[k], &st) == 0)
			{
				if(k == noFiles-1)
					recordLen = 0;
				else
					recordLen = st.st_size;
			}

			recordLen = htonl(recordLen);
			tempHol = (char *)malloc(totalData+sizeof(int)+sizeof fileID+st.st_size);
			memcpy(&tempHol[0], msgData, totalData);
			free(msgData);
			msgData  = tempHol;
			tempHol = NULL;

			parseMetaData(matchFiles[k], NULL, NULL, NULL, nonce, NULL, NULL);			

			char* temp = (char *)malloc(strlen("peer"));
			strcpy(temp, "peer");
			GetUOID(node.nodeInstance, temp, (char *)fileID, sizeof fileID);
			free(temp);

			struct getFile g;
			strncpy(g.fileName, matchFiles[k], strlen(matchFiles[k]));
			memcpy(g.fileID, fileID, sizeof g.fileID);

			pthread_mutex_lock(&fileIdLock);
			fileIdStore.push_back(g);
			pthread_mutex_unlock(&fileIdLock);

			pthread_mutex_lock(&cacheLock);
			for(unsigned int j=0 ; j<fileCache.size() ; j++)
			{
				if(!strncmp(fileCache[j].fileName, matchFiles[k], strlen(matchFiles[k])))
				{
					struct cacheStruct c;
					c = fileCache[j];
					fileCache.erase(fileCache.begin()+j);
					fileCache.push_back(c);
				}
			}
			pthread_mutex_unlock(&cacheLock);

			memcpy(&msgData[totalData], &recordLen, sizeof(int));
			memcpy(&msgData[totalData+sizeof(int)], fileID, sizeof fileID);

			ip.open(matchFiles[k], ios::in|ios::binary);
			if(ip.is_open())
			{
				ip.read(&msgData[totalData+sizeof(int)+sizeof fileID], st.st_size);
			}
			ip.close();

			totalData = totalData + sizeof(int) + sizeof fileID + st.st_size ;
		}
	}

	if(foundFile)
	{
		rspMsg.dataLength = htonl(rspMsg.dataLength);
		rspMsg.data = msgData;

		if(mySearch)
		{
			prcsSearchRsp(rspMsg);
			return;
		}

		rData = (struct connData *)malloc(sizeof(struct connData));
		rData->dataLen = headerLen + totalData;

		rData->sendBuf = (unsigned char *)malloc(totalData+headerLen);
		memcpy(&rData->sendBuf[0], &rspMsg.messageType, sizeof(uint8_t));
		memcpy(&rData->sendBuf[sizeof(uint8_t)], rspMsg.UOID, sizeof rspMsg.UOID);
		memcpy(&rData->sendBuf[sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.TTL, sizeof(uint8_t));
		memcpy(&rData->sendBuf[2*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.rsrv, sizeof(uint8_t));
		memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID], &rspMsg.dataLength, sizeof(int));

		memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof rspMsg.UOID+sizeof(int)], rspMsg.data, totalData);

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
		{
			if(!memcmp(reqUOID, cacheUOID[j].msgUOID, sizeof reqUOID))
			{
				rData->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(rData->peerNode, cacheUOID[j].senderNode);
				rData->peerNode[strlen(cacheUOID[j].senderNode)]='\0';
				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*rData);
				pthread_mutex_unlock(&sendQLock);

				rspMsg.kind = 3;
				rspMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode));
				memcpy(rspMsg.nodeID, cacheUOID[j].senderNode, strlen(cacheUOID[j].senderNode));
				pthread_mutex_lock(&logLock);
				logQueue.push(rspMsg);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);
				break;
			}
		}
		pthread_mutex_unlock(&timerLock);
		free(rData);
	}
}

/*******************************************************************************
Function to send a search request
*******************************************************************************/
void sendSearchRq(struct MsgDetails sMsg)
{
	struct connData *sData = (struct connData *)malloc(sizeof(struct connData));

	char* temp = (char *)malloc(strlen("peer"));
	strcpy(temp, "peer");
	GetUOID(node.nodeInstance, temp, (char *)sMsg.UOID, sizeof sMsg.UOID);
	free(temp);

	struct timerSt *pTimer = (struct timerSt*)malloc(sizeof(struct timerSt));
	memcpy(pTimer->msgUOID, sMsg.UOID, sizeof sMsg.UOID);
	pTimer->timeout = node.msgLifeTime;
	pTimer->senderNode = NULL;
	
	//New Msg, store UOID
	pthread_mutex_lock(&timerLock);
	cacheUOID.push_back(*pTimer);
	pthread_mutex_unlock(&timerLock);

	sMsg.TTL = (uint8_t)node.ttl;

	memcpy(mySrcUOID, sMsg.UOID, sizeof mySrcUOID);


	if(getFileId != NULL)
	{
		for(int j=0; j < mySrcResult-1 ; j++)
			free(getFileId[j]);
	}

	if(getFileSha != NULL)
	{
		for(int j=0; j < mySrcResult-1 ; j++)
			free(getFileSha[j]);
	}

	if(defGetFileName != NULL)
	{
		for(int j=0; j < mySrcResult-1 ; j++)
			free(defGetFileName[j]);
	}

	getFileId = NULL;
	getFileSha = NULL;
	defGetFileName = NULL;

	mySrcResult = 1;

	sData->dataLen = sizeof sMsg.UOID + 3*sizeof(uint8_t) + sizeof(int) + ntohl(sMsg.dataLength);


   pthread_mutex_lock(&existConnLock);
   for(unsigned int i=0; i < existConn.size() ; i++)//Send to all existing connections.
   {
		sData->sendBuf = (unsigned char *)malloc(sData->dataLen);

	   memcpy(&sData->sendBuf[0], &sMsg.messageType, sizeof(uint8_t));
	   memcpy(&sData->sendBuf[sizeof(uint8_t)], sMsg.UOID, sizeof sMsg.UOID);
	   memcpy(&sData->sendBuf[sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.TTL, sizeof(uint8_t));
	   memcpy(&sData->sendBuf[2*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.rsrv, sizeof(uint8_t));
	   memcpy(&sData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.dataLength, sizeof(int));

	   memcpy(&sData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)], sMsg.data, ntohl(sMsg.dataLength));
	   
		sData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo)+1);
		memcpy(sData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
		sData->peerNode[strlen(existConn[i].nodeInfo)]='\0';

		pthread_mutex_lock(&sendQLock);
		if(sMsg.TTL > 0)
			sendQ.push_back(*sData);
		pthread_mutex_unlock(&sendQLock);

		sMsg.kind = 3;
		sMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo)+1);
		memcpy(sMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
		sMsg.nodeID[strlen(existConn[i].nodeInfo)] = '\0';
		pthread_mutex_lock(&logLock);
		logQueue.push(sMsg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);

		
   }
   pthread_mutex_unlock(&existConnLock);

   prcsSearchRq(sMsg, true);
}

/*******************************************************************************
Function to send a get request
*******************************************************************************/
void sendGetRq(struct MsgDetails sMsg)
{

	char fileID[20];
	int fileSz;
	bool fileExists = false;
	char *fName;

	memcpy(fileID, &sMsg.data[0], sizeof fileID);

	pthread_mutex_lock(&fileIdLock);
	for(unsigned int i=0; i < fileIdStore.size() ; i++)
	{
		if(!memcmp(fileIdStore[i].fileID, fileID, sizeof fileID))
		{
			fName = (char *)malloc(strlen(fileIdStore[i].fileName)+1);
			strncpy(fName, fileIdStore[i].fileName, strlen(fileIdStore[i].fileName));
			fName[strlen(fileIdStore[i].fileName)] = '\0';
			fileExists = true;

			fName[strlen(fileIdStore[i].fileName)-3] = 'a';
			fName[strlen(fileIdStore[i].fileName)-4] = 'd';
			break;
		}
	}
	pthread_mutex_unlock(&fileIdLock);

	if(fileExists)
	{
		pthread_mutex_lock(&cacheLock);
		for(unsigned int j=0; j < fileCache.size() ; j++)
		{
			if(!strncmp(fName, fileCache[j].fileName, strlen(fName)))
			{
				currCacheSize -= fileCache[j].fileSz;
				fileCache.erase(fileCache.begin() + j); //Move file to permanent storage
			}
		}
		pthread_mutex_unlock(&cacheLock);

		struct stat fs;

		if(stat(fName, &fs) == 0)
			fileSz = fs.st_size;

		int Qt = (int) fileSz/MAXBUFFERSIZE ;
		int Rmn = fileSz%MAXBUFFERSIZE;
		char *buffer = (char *)malloc(MAXBUFFERSIZE);

		ifstream ip;
		ofstream op;

		ip.open(fName, ios::in|ios::binary);
		op.open(getFileName, ios::out|ios::binary);

		if(ip.is_open())
		{
			for(int i=0; i<Qt; i++)
			{
				memset(buffer, '\0', MAXBUFFERSIZE);
				ip.read(buffer, MAXBUFFERSIZE);
				op.write(buffer, MAXBUFFERSIZE);
				op.flush();
			}

			if(Rmn > 0)
			{
				ip.read(buffer, Rmn);
				op.write(buffer, Rmn);
				op.flush();
			}
		}

		ip.close();
		op.close();
		return;
	}

   struct connData *sData = (struct connData *)malloc(sizeof(struct connData));

   sData->dataLen = (3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)+ntohl(sMsg.dataLength));
	
	pthread_mutex_lock(&existConnLock);
	for(unsigned int i=0; i < existConn.size() ; i++)//Send to all existing connections.
	{
		sData->sendBuf = (unsigned char*)malloc(sData->dataLen);
	   memcpy(&sData->sendBuf[0], &sMsg.messageType, sizeof(uint8_t));
	   memcpy(&sData->sendBuf[sizeof(uint8_t)], sMsg.UOID, sizeof sMsg.UOID);
	   memcpy(&sData->sendBuf[sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.TTL, sizeof(uint8_t));
	   memcpy(&sData->sendBuf[2*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.rsrv, sizeof(uint8_t));
	   memcpy(&sData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.dataLength, sizeof(int));

	   memcpy(&sData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)], sMsg.data, ntohl(sMsg.dataLength));

		sData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo)+1);
		memcpy(sData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;
		sData->peerNode[strlen(existConn[i].nodeInfo)] = '\0';

		pthread_mutex_lock(&sendQLock);
		if(sMsg.TTL > 0)
			sendQ.push_back(*sData);
		pthread_mutex_unlock(&sendQLock);

		sMsg.kind = 3;
		sMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
		memcpy(sMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
		pthread_mutex_lock(&logLock);
		logQueue.push(sMsg);
		pthread_mutex_unlock(&logLock);
		pthread_cond_signal(&forLogging);
		
	}
	pthread_mutex_unlock(&existConnLock);

}

/*******************************************************************************
Function to process received get request. Forward the request and reply to the
sending node
*******************************************************************************/
void prcsGetRq(struct MsgDetails rMsg)
{
	char fileID[20];

	bool fileExists = false;
	char *fName;

	memcpy(fileID, &rMsg.data[0], sizeof fileID);
	
	pthread_mutex_lock(&fileIdLock);
	for(unsigned int i=0; i < fileIdStore.size() ; i++)
	{
		if(!memcmp(fileIdStore[i].fileID, fileID, sizeof fileID))
		{
			fName = (char *)malloc(strlen(fileIdStore[i].fileName)+1);
			strncpy(fName, fileIdStore[i].fileName, strlen(fileIdStore[i].fileName));
			fName[strlen(fileIdStore[i].fileName)] = '\0';
			fileExists = true;
			break;
		}
	}
	pthread_mutex_unlock(&fileIdLock);

	if(fileExists)
	{		
		struct MsgDetails stMsg;
		int metaFileSize,storeFileSize;

		struct connData *strData = (struct connData *)malloc(sizeof(struct connData));

		char *storeFileNme;
		char *metaFileNme ;

		metaFileNme = (char *)malloc(strlen(fName)+1);
		strncpy(metaFileNme, fName, strlen(fName));
		metaFileNme[strlen(fName)] = '\0';

		storeFileNme = (char *)malloc(strlen(fName)+1);
		strncpy(storeFileNme, fName, strlen(fName));
		storeFileNme[strlen(fName)] = '\0';

		storeFileNme[strlen(fName)-3] = 'a';
		storeFileNme[strlen(fName)-4] = 'd';

		stMsg.rsrv = 0;
		char* temp = (char *)malloc(strlen("peer"));
		strcpy(temp, "peer");
		GetUOID(node.nodeInstance, temp, (char *)stMsg.UOID, sizeof stMsg.UOID);

		stMsg.TTL = (uint8_t)node.ttl;
		stMsg.messageType = GTRS;

		struct stat fs;
		if (stat(metaFileNme,&fs)==0)
			metaFileSize = fs.st_size;
		if (stat(storeFileNme,&fs)==0)
			storeFileSize = fs.st_size;

		stMsg.dataLength = sizeof(int)+metaFileSize+storeFileSize+sizeof rMsg.UOID;
		ifstream fp;
		metaFileSize = htonl(metaFileSize);
		strData->dataLen = stMsg.dataLength+headerLen;
		stMsg.dataLength = htonl(stMsg.dataLength);

		struct MsgDetails logMsg = stMsg;
		logMsg.data = (char *)malloc(sizeof(stMsg.UOID));
		memcpy(logMsg.data, rMsg.UOID, sizeof rMsg.UOID);
		logMsg.kind = 3;
		
		if (ntohl(stMsg.dataLength) < MAXBUFFERSIZE)
		{
			stMsg.data = (char *)malloc(ntohl(stMsg.dataLength));
			memcpy(&stMsg.data[0], rMsg.UOID, sizeof rMsg.UOID);
			memcpy(&stMsg.data[sizeof rMsg.UOID],&metaFileSize,sizeof(int));
			fp.open(metaFileNme,ios::in|ios::binary);
			fp.read(&stMsg.data[sizeof(int)+sizeof rMsg.UOID],ntohl(metaFileSize));
			fp.close();
			fp.open(storeFileNme,ios::in|ios::binary);
			fp.read(&stMsg.data[sizeof(int)+ntohl(metaFileSize)+sizeof rMsg.UOID],storeFileSize);
			fp.close();

			stMsg.kind = 3;
			pthread_mutex_lock(&timerLock);
			for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
			{
				if(!memcmp(rMsg.UOID, cacheUOID[j].msgUOID, sizeof rMsg.UOID))
				{
					strData->sendBuf = (unsigned char *)malloc(ntohl(stMsg.dataLength)+headerLen);
					memcpy(&strData->sendBuf[0], &stMsg.messageType, sizeof(uint8_t));
					memcpy(&strData->sendBuf[sizeof(uint8_t)], stMsg.UOID, sizeof stMsg.UOID);
					memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.TTL, sizeof(uint8_t));
					memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.rsrv, sizeof(uint8_t));
					memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.dataLength, sizeof(int));
					memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int)], stMsg.data, ntohl(stMsg.dataLength));				

					strData->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
					strcpy(strData->peerNode,cacheUOID[j].senderNode);
					strData->peerNode[strlen(cacheUOID[j].senderNode)]='\0';
					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);

					stMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
					strcpy(stMsg.nodeID,cacheUOID[j].senderNode);

					logMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
					strcpy(logMsg.nodeID,cacheUOID[j].senderNode);

					pthread_mutex_lock(&logLock);
					logQueue.push(logMsg);
					pthread_mutex_unlock(&logLock);
					break;
				}
			}
			pthread_mutex_unlock(&timerLock);
		}
		else
		{
			int Qt = (int)storeFileSize/MAXBUFFERSIZE;
			int Rmn = storeFileSize%MAXBUFFERSIZE;

			strData->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
			memcpy(&strData->sendBuf[0], &stMsg.messageType, sizeof(uint8_t));
			memcpy(&strData->sendBuf[sizeof(uint8_t)], stMsg.UOID, sizeof stMsg.UOID);
			memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.TTL, sizeof(uint8_t));
			memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.rsrv, sizeof(uint8_t));
			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID], &stMsg.dataLength, sizeof(int));
			stMsg.kind = 3;
			strData->dataLen = headerLen+sizeof(int)+ntohl(metaFileSize)+ sizeof rMsg.UOID;

			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof stMsg.UOID+sizeof(int)], rMsg.UOID, sizeof rMsg.UOID);
			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+2*sizeof stMsg.UOID+sizeof(int)], &metaFileSize, sizeof(int));

			fp.open(metaFileNme,ios::in|ios::binary);
			fp.read((char *)&strData->sendBuf[2*sizeof(int)+3*sizeof(uint8_t)+2*sizeof stMsg.UOID],ntohl(metaFileSize));
			fp.close();

			fp.open(storeFileNme,ios::in|ios::binary);		

		pthread_mutex_lock(&timerLock);
		for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
		{
			if(!memcmp(rMsg.UOID, cacheUOID[j].msgUOID, sizeof rMsg.UOID))
			{				

				strData->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(strData->peerNode,cacheUOID[j].senderNode);
				strData->peerNode[strlen(cacheUOID[j].senderNode)]='\0';

				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*strData);
				pthread_mutex_unlock(&sendQLock);

				for(int k=0; k < Qt ; k++)
				{
					strData->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
					memset(strData->sendBuf, '\0', MAXBUFFERSIZE);
					fp.read((char *)&strData->sendBuf[0],MAXBUFFERSIZE);
					strData->dataLen = MAXBUFFERSIZE;

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);
				}

				if(Rmn != 0)
				{
					strData->sendBuf = (unsigned char *)malloc(Rmn);
					memset(strData->sendBuf, '\0', Rmn);
					fp.read((char *)&strData->sendBuf[0], Rmn);
					strData->dataLen = Rmn;

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);
					
				}

				stMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(stMsg.nodeID,cacheUOID[j].senderNode);

				logMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(logMsg.nodeID,cacheUOID[j].senderNode);
				pthread_mutex_lock(&logLock);
				logQueue.push(logMsg);
				pthread_mutex_unlock(&logLock);

				break;
			 }
		 }
		 pthread_mutex_unlock(&timerLock);		
			fp.close();
		}
	}
	else
	{
		struct connData *rData = (struct connData *)malloc(sizeof(struct connData));
		rMsg.TTL = setTTL(rMsg.TTL);
		//Forward
		rData->dataLen = 3*sizeof(uint8_t)+sizeof rMsg.UOID+ sizeof(int)+ ntohl(rMsg.dataLength);
		rData->sendBuf = (unsigned char *)malloc(rData->dataLen);

	   memcpy(&rData->sendBuf[0], &rMsg.messageType, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[sizeof(uint8_t)], rMsg.UOID, sizeof rMsg.UOID);
	   memcpy(&rData->sendBuf[sizeof(uint8_t)+sizeof rMsg.UOID], &rMsg.TTL, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[2*sizeof(uint8_t)+sizeof rMsg.UOID], &rMsg.rsrv, sizeof(uint8_t));
	   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof rMsg.UOID], &rMsg.dataLength, sizeof(int));

	   memcpy(&rData->sendBuf[3*sizeof(uint8_t)+sizeof rMsg.UOID+sizeof(int)], rMsg.data, ntohl(rMsg.dataLength));

		pthread_mutex_lock(&existConnLock);
		for(unsigned int i=0; i < existConn.size() ; i++)
		{
			if(existConn[i].peerSock != rMsg.pSock)
			{
				rData->peerNode = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(rData->peerNode, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo)) ;

				pthread_mutex_lock(&sendQLock);
				if(rMsg.TTL > 0)
					sendQ.push_back(*rData);
				pthread_mutex_unlock(&sendQLock);

				rMsg.kind = 2;
				rMsg.nodeID = (char *)malloc(strlen(existConn[i].nodeInfo));
				memcpy(rMsg.nodeID, existConn[i].nodeInfo, strlen(existConn[i].nodeInfo));
				pthread_mutex_lock(&logLock);
				logQueue.push(rMsg);
				pthread_mutex_unlock(&logLock);
				pthread_cond_signal(&forLogging);

				break;
			}
		}
		pthread_mutex_unlock(&existConnLock);

		free(rData);
	}
}

/*******************************************************************************
Function to process received get response. Store the received file or if the 
response is not for the current node, then forward it
*******************************************************************************/
void prcsGetRsp(struct MsgDetails sMsg)
{
	struct connData *strData = (struct connData *)malloc(sizeof(struct connData));
	int storeFileSize;
	ifstream fp;
	bool store = false;

	char rspUOID[20];
	memcpy(rspUOID, sMsg.data, sizeof sMsg.UOID);
	
	char *storeFileNme = (char *)malloc(strlen(&sMsg.data[sizeof rspUOID])+1);
	char *metaFileNme = (char *)malloc(strlen(&sMsg.data[sizeof rspUOID])+1);

	strncpy(storeFileNme, &sMsg.data[sizeof rspUOID], strlen(&sMsg.data[sizeof rspUOID]));
	storeFileNme[strlen(&sMsg.data[sizeof rspUOID])] = '\0';

	strncpy(metaFileNme, &sMsg.data[sizeof rspUOID], strlen(&sMsg.data[sizeof rspUOID]));
	metaFileNme[strlen(&sMsg.data[sizeof rspUOID])] = '\0';
	metaFileNme[strlen(&sMsg.data[sizeof rspUOID])-3] = 'e';
	metaFileNme[strlen(&sMsg.data[sizeof rspUOID])-4] = 'm';	

	struct stat fs;
	int metaFileSize;
	if (stat(metaFileNme,&fs)==0)
		metaFileSize = fs.st_size;
	if (stat(storeFileNme,&fs)==0)
		storeFileSize = fs.st_size;


	if(!memcmp(myGetUOID, rspUOID, sizeof myGetUOID))
	{		

		ifstream ip;
		ofstream op;
		char *newBuf = new char[MAXBUFFERSIZE];

		ip.open(storeFileNme, ios::in|ios::binary);
		op.open(getFileName, ios::out|ios::binary);

		if(!ip.is_open() || !op.is_open())
		{			
			return;
		}

		int Qt = (int)storeFileSize/MAXBUFFERSIZE;
		int Rmn = storeFileSize%MAXBUFFERSIZE;

		for(int k=0; k<Qt; k++)
		{
			memset(newBuf, '\0', MAXBUFFERSIZE);
			ip.read(newBuf, MAXBUFFERSIZE);
			op.write(newBuf, MAXBUFFERSIZE);
			op.flush();
		}

		delete[] newBuf;

		if(Rmn > 0)
		{
			newBuf = new char[Rmn];
			memset(newBuf, '\0', Rmn);
			ip.read(newBuf, Rmn);
			op.write(newBuf, Rmn);
			op.flush();
			delete[] newBuf;
		}

		if(op.fail())
		{
			remove(storeFileNme);
			remove(metaFileNme);
			return;
		}

		op.close();
		ip.close();

		//Need to maintain a copy at perm side (just move if file aready in cache)		

		struct searchData newS;
		strcpy(newS.realName, storeFileNme);
		newS.realName[strlen(storeFileNme)] = '\0';

		char nonceArr[40];
		struct bitVect cVect;
		parseMetaData(metaFileNme,cVect.vArr, newS.fileName, newS.sha_val, nonceArr, NULL, NULL);


		char ***rFiles = (char ***)malloc(sizeof(char **));
		*rFiles = NULL;
		short *no = (short *)malloc(sizeof(short));
		*no = 0;

		pthread_mutex_lock(&treeLock);
		returnTree(nameTree, no, rFiles);
		pthread_mutex_unlock(&treeLock);



		int count = 0;		

		while(count < *no)
		{
			char *name = new char[strlen((*rFiles)[count])+1];
			strncpy(name, (*rFiles)[count], strlen((*rFiles)[count]));
			name[strlen((*rFiles)[count])] = '\0';

			if (strlen(name)!=0)
			{
				name[strlen((*rFiles)[count])]='\0';
				name[strlen((*rFiles)[count])-3] = 'e';
				name[strlen((*rFiles)[count])-4] = 'm';

				struct searchData chkData;
				char *chknonce = new char[40];
				parseMetaData(metaFileNme, NULL, chkData.fileName, chkData.sha_val, chknonce, NULL, NULL);

				if(!strncmp(chkData.fileName, newS.fileName, strlen(newS.fileName)) &&
						!strncmp(chkData.sha_val, newS.sha_val, sizeof newS.sha_val) &&
							!strncmp(nonceArr, chknonce, sizeof nonceArr))
				{
					pthread_cond_signal(&cmdPromptWait);
					return;
				}

				delete[]chknonce;
			}

			delete[] name;
			count++;
		}


		strcpy(cVect.fName, storeFileNme);
		cVect.fName[strlen(storeFileNme)] = '\0';

		pthread_mutex_lock(&treeLock);
		insertInTree(newS);
		pthread_mutex_unlock(&treeLock);

		pthread_mutex_lock(&bitVStoreLock);
		bitVStorage.push_back(cVect);
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

		struct cacheStruct newEntry;
		newEntry.fileSz = ntohl(sMsg.dataLength) - (sizeof(int)+metaFileSize);
		strcpy(newEntry.fileName, storeFileNme);
		newEntry.fileName[strlen(storeFileNme)] = '\0';

		pthread_mutex_lock(&cacheLock);
		fileCache.push_back(newEntry);
		pthread_mutex_unlock(&cacheLock);

		pthread_cond_signal(&cmdPromptWait);
		return;
	}


	if(drand48() < node.cacheProb)
	{
			store = true;
			pthread_mutex_lock(&cacheLock);
			if(storeFileSize+currCacheSize > node.cacheSize)
			{
				if(storeFileSize < node.cacheSize)
				{
					
					char *tdataFile = (char *)malloc(strlen(fileCache[0].fileName));
					strncpy(tdataFile, fileCache[0].fileName, strlen(fileCache[0].fileName));
					tdataFile[strlen(fileCache[0].fileName)] = '\0';

					pthread_mutex_lock(&treeLock);
					struct treeNode *t = NULL;
					struct searchData s;
					strncpy(s.realName, tdataFile, strlen(tdataFile));

					t = searchTree(nameTree, s, 2);

					if(t != NULL)
					{
						memset(t->data.fileName, '\0', sizeof t->data.fileName);
						memset(t->data.sha_val, '\0', sizeof t->data.sha_val);
					}
					pthread_mutex_unlock(&treeLock);

					pthread_mutex_lock(&bitVStoreLock);
					for(unsigned int j=0 ; j < bitVStorage.size() ; j++)
					{
						if(!strncmp(bitVStorage[j].fName, tdataFile,strlen(tdataFile)))
						{
							bitVStorage.erase(bitVStorage.begin() + j);
						}
					}
					pthread_mutex_unlock(&bitVStoreLock);

				  while((storeFileSize+currCacheSize) > node.cacheSize)
				  {
					if(fileCache.size() > 0)
					{
						currCacheSize -= fileCache[0].fileSz;
						fileCache.erase(fileCache.begin());
					}
					else
					{						
						store = false;
					}
				 }


				}
				else
				{					
					store = false;
				}

			}

			currCacheSize += storeFileSize;
			pthread_mutex_unlock(&cacheLock);

		struct searchData newS;
		strcpy(newS.realName, storeFileNme);
		newS.realName[strlen(storeFileNme)] = '\0';

		struct bitVect cVect;
		parseMetaData(metaFileNme,cVect.vArr, newS.fileName, newS.sha_val, NULL, NULL, NULL);
		strcpy(cVect.fName, storeFileNme);
		cVect.fName[strlen(storeFileNme)] = '\0';

		pthread_mutex_lock(&treeLock);
		insertInTree(newS);
		pthread_mutex_unlock(&treeLock);

		pthread_mutex_lock(&bitVStoreLock);
		bitVStorage.push_back(cVect);
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

		struct cacheStruct newEntry;
		newEntry.fileSz = ntohl(sMsg.dataLength) - (sizeof(int)+metaFileSize);
		strcpy(newEntry.fileName, storeFileNme);
		newEntry.fileName[strlen(storeFileNme)] = '\0';

		//push into file cache
		pthread_mutex_lock(&cacheLock);
		fileCache.push_back(newEntry);		
		pthread_mutex_unlock(&cacheLock);

	 }

		struct MsgDetails logMsg = sMsg;
		logMsg.data = (char *)malloc(sizeof(sMsg.UOID));
		memcpy(logMsg.data, rspUOID, sizeof rspUOID);
		logMsg.kind = 2;

		if (ntohl(sMsg.dataLength) < MAXBUFFERSIZE)
		{
			sMsg.data = (char *)malloc(ntohl(sMsg.dataLength));
			memcpy(&sMsg.data[0],&metaFileSize,sizeof(int));

			fp.open(metaFileNme,ios::in|ios::binary);
			fp.read(&sMsg.data[sizeof(int)],ntohl(metaFileSize));
			fp.close();

			fp.open(storeFileNme,ios::in|ios::binary);
			fp.read(&sMsg.data[sizeof(int)+ntohl(metaFileSize)],storeFileSize);
			fp.close();

			strData->dataLen = headerLen + ntohl(sMsg.dataLength);
			strData->sendBuf = (unsigned char *)malloc(ntohl(sMsg.dataLength)+headerLen);
			memcpy(&strData->sendBuf[0], &sMsg.messageType, sizeof(uint8_t));
			memcpy(&strData->sendBuf[sizeof(uint8_t)], sMsg.UOID, sizeof sMsg.UOID);
			memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.TTL, sizeof(uint8_t));
			memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.rsrv, sizeof(uint8_t));
			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.dataLength, sizeof(int));
			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)], sMsg.data, ntohl(sMsg.dataLength));
			sMsg.kind = 2;

			pthread_mutex_lock(&timerLock);
			for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
			{
				if(!memcmp(sMsg.UOID, cacheUOID[j].msgUOID, sizeof sMsg.UOID))
				{

					strData->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
					strcpy(strData->peerNode,cacheUOID[j].senderNode);
					strData->peerNode[strlen(cacheUOID[j].senderNode)]='\0';					
					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);

					sMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
					strcpy(sMsg.nodeID,cacheUOID[j].senderNode);

					logMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
					strcpy(logMsg.nodeID,cacheUOID[j].senderNode);

					pthread_mutex_lock(&logLock);
					logQueue.push(logMsg);
					pthread_mutex_unlock(&logLock);
					break;
				}
			}
			pthread_mutex_unlock(&timerLock);
		}
		else
		{
			int Qt = (int)storeFileSize/MAXBUFFERSIZE;
			int Rmn = storeFileSize%MAXBUFFERSIZE;

			strData->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
			memcpy(&strData->sendBuf[0], &sMsg.messageType, sizeof(uint8_t));
			memcpy(&strData->sendBuf[sizeof(uint8_t)], sMsg.UOID, sizeof sMsg.UOID);
			memcpy(&strData->sendBuf[sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.TTL, sizeof(uint8_t));
			memcpy(&strData->sendBuf[2*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.rsrv, sizeof(uint8_t));
			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID], &sMsg.dataLength, sizeof(int));
			sMsg.kind = 2;
			strData->dataLen = headerLen+sizeof(int)+ntohl(metaFileSize);

			memcpy(&strData->sendBuf[3*sizeof(uint8_t)+sizeof sMsg.UOID+sizeof(int)], &metaFileSize, sizeof(int));

			fp.open(metaFileNme,ios::in|ios::binary);
			fp.read((char *)&strData->sendBuf[2*sizeof(int)+3*sizeof(uint8_t)+sizeof sMsg.UOID],ntohl(metaFileSize));
			fp.close();

			fp.open(storeFileNme,ios::in|ios::binary);
			pthread_mutex_lock(&timerLock);
			for(unsigned int j=0 ; j<cacheUOID.size() ; j++)
			{
				if(!memcmp(sMsg.UOID, cacheUOID[j].msgUOID, sizeof sMsg.UOID))
				{
				
				strData->peerNode = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(strData->peerNode,cacheUOID[j].senderNode);
				strData->peerNode[strlen(cacheUOID[j].senderNode)]='\0';

				pthread_mutex_lock(&sendQLock);
				sendQ.push_back(*strData);
				pthread_mutex_unlock(&sendQLock);

				for(int k=0; k < Qt ; k++)
				{
					strData->sendBuf = (unsigned char *)malloc(MAXBUFFERSIZE);
					memset(strData->sendBuf, '\0', MAXBUFFERSIZE);
					fp.read((char *)&strData->sendBuf[0],MAXBUFFERSIZE);
					strData->dataLen = MAXBUFFERSIZE;

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);
				}

				if(Rmn != 0)
				{
					strData->sendBuf = (unsigned char *)malloc(Rmn);
					memset(strData->sendBuf, '\0', Rmn);
					fp.read((char *)&strData->sendBuf[0], Rmn);
					strData->dataLen = Rmn;

					pthread_mutex_lock(&sendQLock);
					sendQ.push_back(*strData);
					pthread_mutex_unlock(&sendQLock);
				}

				sMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(sMsg.nodeID,cacheUOID[j].senderNode);

				logMsg.nodeID = (char *)malloc(strlen(cacheUOID[j].senderNode)+1);
				strcpy(logMsg.nodeID,cacheUOID[j].senderNode);

				pthread_mutex_lock(&logLock);
				logQueue.push(logMsg);
				pthread_mutex_unlock(&logLock);
				}

			}
			pthread_mutex_unlock(&timerLock);
			fp.close();
		}
	
		if(!store)
		{
			remove(metaFileNme);
			remove(storeFileNme);
		}
}

/********************************************************************
Thread to process messages pushed into processQ. Calls
corresponding functions.
********************************************************************/
void* processData(void *arg)
{
	struct MsgDetails msg;
	while(1)
	{		
		pthread_mutex_lock(&processQLock);
		while( processQ.size() <= 0 && !terminateNode )
		{
			pthread_cond_wait(&processQWait, &processQLock);
		}
		if (!processQ.empty())
		{
		    msg = processQ.front();
		    processQ.pop();
		}
		pthread_mutex_unlock(&processQLock);

		if(terminateNode)
		   break;

		switch(msg.messageType)
		{
			case JNRQ:
			{
				prcsJoinReq(msg);
				break;
			}
			case JNRS:
			{
				prcsJoinRsp(msg);
				break;
			}
			case HLLO:
			{
				if(msg.dataLength <= 0)
				{
					sendHello(msg, NULL);
				}
				break;
			}
			case KPAV:
			{
				sendKeepAlive(msg);
				break;
			}
			case NTFY:
			case CKRQ:
			{
				//if(!strcmp(msg.nodeID, ""))
				//	sendChkMsg(msg);
				//else
				//	prcsChkMsg(msg);
				break;
			}
			case CKRS:
			{
				prcsChkRsp(msg);
				break;
			}
			case SHRQ:
			{
				if(msg.nodeID == NULL)
					sendSearchRq(msg);
				else
					prcsSearchRq(msg, false);
				break;
			}
			case SHRS:
			{
				prcsSearchRsp(msg);
				break;
			}
			case GTRQ:
			{
				if(msg.nodeID == NULL)
					sendGetRq(msg);
				else
					prcsGetRq(msg);
				break;
			}
			case GTRS:
			{
				prcsGetRsp(msg);
				break;
			}
			case STOR:
			{
				if (msg.nodeID == NULL)
					sendStore(msg);
				else
					prcsStore(msg);
				break;
			}
			case DELT:
			{
				if (msg.nodeID == NULL)
					sendDelMsg(msg);
				else
					prcsDelMsg(msg);
				break;
			}
			case STRQ:
			{
			   if(ntohl(msg.dataLength) == 0)
					sendStatusReq(msg);
			   else
					prcsStatusReq(msg);

				break;
			}
			case STRS:
			{
				prcsStatusRsp(msg);
				break;
			}
		}
	}
pthread_exit(NULL);
}

/****************************************************************
Thread for logging all messages. Processes messages
****************************************************************/
void *infoLogger(void *arg)
{
	struct timeval tme;
	char timeFormatted[14];
	int tSec;
	int tMsec;
	ofstream fOpen;
	struct MsgDetails readyToLog;
	fOpen.open(logFile, ios::app);

	while(!terminateNode)
	{
		pthread_mutex_lock(&logLock);

		while(logQueue.size() <= 0 && !terminateLogger)
		{
			pthread_cond_wait(&forLogging,&logLock);
		}

		if(logQueue.size() > 0)
		{
			readyToLog = logQueue.front();
			logQueue.pop();
		}
		pthread_mutex_unlock(&logLock);

	  if(terminateLogger)
		 break;

		if(!fOpen.is_open())
		{
			pthread_exit(NULL);
		}

		switch(readyToLog.kind)
		{
			case 1: fOpen<<"r ";
					break;
			case 2: fOpen<<"f ";
					break;
			case 3: fOpen<<"s ";
					break;
		}

   		gettimeofday(&tme,NULL);
		tSec = tme.tv_sec;
		tMsec = tme.tv_usec/1000;
		if (tMsec>1000)
		{
			tSec+=1;
			tMsec-=1000;
		}

		sprintf(timeFormatted,"%10d",tSec);
		sprintf(&timeFormatted[10],"%s",".");
		sprintf(&timeFormatted[11],"%3d",tMsec);

		fOpen<<timeFormatted<<" ";

		size_t fpos;
		string str(readyToLog.nodeID);
		fpos = str.find_first_of(":");
		str.replace(fpos,1,"_");
		fOpen<<str<<" ";

		unsigned char mID[8];
		unsigned char tempM[4];
		int k;
		switch(readyToLog.messageType)
		{
			case JNRQ:
			{
				fOpen << "JNRQ " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				short port;
				char *hostName;
				unsigned int hLoc;
				memcpy(&hLoc, &readyToLog.data[0], sizeof(unsigned int));
				fOpen << ntohl(hLoc) << " " ;
				memcpy(&port,&readyToLog.data[sizeof(unsigned int)],sizeof(short));
				fOpen<<ntohs(port)<<" ";
				hostName = (char *)malloc(ntohl(readyToLog.dataLength)-sizeof(short)-sizeof(unsigned int)+1);
				memset(hostName, '\0', ntohl(readyToLog.dataLength)-sizeof(short)-sizeof(unsigned int));
				memcpy(hostName,&readyToLog.data[sizeof(short)+sizeof(unsigned int)],ntohl(readyToLog.dataLength)-sizeof(short)-sizeof(unsigned int));
				hostName[ntohl(readyToLog.dataLength)-sizeof(short)-sizeof(unsigned int)] = '\0';
				fOpen<<hostName<<" "<<endl;

				break;
			}
			case JNRS:
			{
				fOpen << "JNRS " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

   				    short port;
					char *hostName;
					char jUOID[20];
					unsigned int dist;

					unsigned char tmID[8];
					unsigned char ttempM[4];

					memcpy(jUOID,&readyToLog.data[0], sizeof jUOID);
					memcpy(ttempM,&jUOID[sizeof jUOID-sizeof(int)],sizeof(int));

					int m=0;
					for (unsigned int l=0 ;l<sizeof(tmID) ;l+=2)
					{
							sprintf((char *)&tmID[l],"%02x",ttempM[m]);
							m++;
					}
					fOpen<<tmID<<" ";

					memcpy(&dist,&readyToLog.data[20],sizeof(unsigned int));
					memcpy(&port,&readyToLog.data[20+sizeof(unsigned int)],sizeof(short));
					hostName = (char *)malloc(readyToLog.dataLength-20-sizeof(short)-sizeof(int));
					memcpy(hostName,&readyToLog.data[20+sizeof(int)+sizeof(short)],readyToLog.dataLength-sizeof jUOID-sizeof(short)-sizeof(int));

					fOpen<<ntohl(dist)<<" "<<ntohs(port)<<" "<<hostName<<endl;
					break;
			}
			case HLLO:
			{
				fOpen << "HLLO " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				short port;
				char *hostName;
				memcpy(&port,&readyToLog.data[0],sizeof(short));
				port = ntohs(port);
				fOpen<<port<<" ";
				hostName = (char *)malloc(readyToLog.dataLength-sizeof(short));
				memcpy(hostName,&readyToLog.data[sizeof(short)],readyToLog.dataLength-sizeof(short));
				hostName[readyToLog.dataLength-sizeof(short)] = '\0';
				fOpen<<hostName<<" "<<endl;

				break;
			}
			case KPAV:
			{
				fOpen << "KPAV " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";
				fOpen << " " << endl;
				break;
			}
			case NTFY:
			{
				fOpen << "NTFY " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				uint8_t ercCode;

				memcpy(&ercCode, &readyToLog.data[0], sizeof(uint8_t));
				fOpen << ercCode << " " << endl;
				break;
			}
			case CKRQ:
			{
				fOpen << "CKRQ " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";
				fOpen << " " << endl;
				break;
			}
			case CKRS:
			{
				fOpen << "CKRS ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";
				memcpy(tempM,&readyToLog.data[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				int k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" " <<endl;
				break;
			}
			case SHRQ:
			{
				fOpen<<"SHRQ ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				char *query;
				uint8_t type;				
				query = (char *)malloc(ntohl(readyToLog.dataLength)-sizeof(uint8_t));
				memcpy(&type, &readyToLog.data[0], sizeof(uint8_t));
				memcpy(query, &readyToLog.data[sizeof(uint8_t)], ntohl(readyToLog.dataLength)-sizeof(uint8_t));

				if (type == 1)
					fOpen<<"filename ";
				else if (type == 2)
					fOpen<<"sha1hash ";
				else if (type == 3)
					fOpen<<"keywords ";

				fOpen.write(query, ntohl(readyToLog.dataLength)-sizeof(uint8_t));
				fOpen << endl;
				break;
			}
			case SHRS:
			{
				fOpen<<"SHRS ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				memcpy(tempM,&readyToLog.data[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				int k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" " <<endl;

				break;
			}
			case GTRQ:
			{
				fOpen<<"GTRQ ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				memcpy(tempM,&readyToLog.data[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				int k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" " <<endl;

				break;

			}
			case GTRS:
			{
				fOpen<<"GTRS ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";

				memcpy(tempM,&readyToLog.data[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				int k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" " <<endl;

				break;
			}
			case STOR:
			{
				fOpen << "STOR ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";
				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" "<<endl;
				break;

			}
			case DELT:
			{
				fOpen << "DELT ";
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";
				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" "<<endl;
				break;
			}
			case STRQ:
			{
				fOpen << "STRQ " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";
				uint8_t tpe;
				memcpy(&tpe, &readyToLog.data[0], sizeof(uint8_t));				
				if (readyToLog.snof == 1)
				{
					fOpen<< "neighbors" << " " <<endl;
				}
				else if (readyToLog.snof == 2)
				{
					fOpen<< "files" << " " <<endl;
				}
				break;
			}
			case STRS:
			{
				fOpen << "STRS " ;
				fOpen<<headerLen+readyToLog.dataLength<<" ";
				fOpen<<(short)readyToLog.TTL<<" ";

				memcpy(tempM,&readyToLog.UOID[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" ";
				memcpy(tempM,&readyToLog.data[sizeof(readyToLog.UOID)-sizeof(int)],sizeof(int));
				 k=0;
				for (unsigned int j=0 ;j<sizeof(mID) ;j+=2)
				{
					sprintf((char *)&mID[j],"%02x",tempM[k]);
					k++;
				}
				fOpen<<mID<<" "<<endl;
				break;
			}
		}
		fOpen.flush();
	}
	fOpen.close();
	pthread_exit(NULL);
}
