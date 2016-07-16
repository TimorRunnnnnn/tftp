#pragma comment(lib, "ws2_32.lib")
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "stdio.h"
#include "stdint.h"
#include "conio.h"
#include "time.h"
#include "io.h"

#include "tftp12Transmission.h"
#include "tftp12header.h"
#include "tftp12IObuffer.h"

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */

#define TFTP12_DEFAULT_BLOCKSIZE			(512)

#define TFTP12_SERVER_DEFAULT_TIMEOUT		(3)
#define TFTP12_SERVER_DEFAULT_RETRANSMIT	(3)
#define TFTP12_SERVER_DEFAULT_BIND_PORT		(69)

#define TFTP12_BIND_FAILED_TIMES			(20)
#define TFTP12_SERVER_OPTION_INIT			(-1)


#define FOEVER								for(;;)

#define FREE_Z(m)	{\
						if(m!=NULL)		\
						{				\
							free(m);	\
							m=NULL;		\
						}				\
					}				

#define FCLOSE_Z(f)	{\
						if(f!=NULL)		\
						{				\
							fclose(f);	\
							f=NULL;		\
						}				\
					}		

#define SCLOSE_Z(s)	{\
						if(s!=0)		\
						{				\
							closesocket(s);	\
							s=0;		\
						}				\
					}				

char * const tftp12ErrorMsg[] = {
	"Not Defined",
	"File Not Found",
	"Access Violation",
	"Disk Full Or Allocation Exceeded",
	"Illegal TFTP Operation",
	"Unknown Transfer Id",
	"File Already Exists",
	"No Such User",
};

typedef struct _tftp12node
{
	TFTP12Description clientDesc;
	struct _tftp12node *next;
}TFTP12ClientNode;

typedef struct
{
	INT32 count;
	TFTP12ClientNode *head;
}TFTP12ClientList;

typedef struct _tftp12serverinfo
{
	INT32 sock;
	struct sockaddr_in serverAddr;
	INT32 defaultTimeout;
	INT32 reTransmitTimes;
	TFTP12ClientList clientList;
}TFTP12ServerInfo;

TFTP12ServerInfo gTftp12ServerInfo;

#define  tftp12ServoTaskExit(node)	{\
			printf("\ntask exit, port:%d",node->clientDesc.localPort);\
				\
			FCLOSE_Z(node->clientDesc.openFile)\
			SCLOSE_Z(node->clientDesc.sock);\
			tftp12IOBufferFree(node->clientDesc.localPort);\
			tftp12ClientListDelete(node);\
			FREE_Z(node->clientDesc.filename);\
			FREE_Z(node->clientDesc.mode);\
			FREE_Z(node); \
			return;\
}

static void tftp12ClientListInsert(TFTP12ClientNode *node)
{
	if (gTftp12ServerInfo.clientList.head == NULL)
	{
		gTftp12ServerInfo.clientList.head = node;
		return;
	}

	TFTP12ClientNode *pwalk = gTftp12ServerInfo.clientList.head;
	while (pwalk->next != NULL)
	{
		pwalk = pwalk->next;
	}
	pwalk->next = node;
	return;
}
static void tftp12ClientListDelete(TFTP12ClientNode *node)
{
	if (node == gTftp12ServerInfo.clientList.head)
	{
		gTftp12ServerInfo.clientList.head = node->next;
		gTftp12ServerInfo.clientList.count--;
		return;
	}

	TFTP12ClientNode *pWalk = gTftp12ServerInfo.clientList.head;
	while (pWalk->next != NULL)
	{
		if (pWalk->next == node)
		{
			pWalk->next = pWalk->next->next;
			return;
		}
		pWalk = pWalk->next;
	}
}

void tftp12ServerServoTask(void *arg)
{
	INT32 pktBytes = 0;/*发送报文长度*/
	INT32 recvBytes = 0;/*单次报文接收的字节数*/
	INT32 nextBlockNumber = 0;/*下一块数据的编号*/
	TFTP12ClientNode *node = arg;
	TFTP12Description *desc = NULL;
	if (node == NULL)
	{
		printf("\nserver servo task argument is NULL!");
		return;
	}

	desc = &(node->clientDesc);
	desc->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (desc->sock < 0)
	{
		printf("\nservo task socket create failed!");
		return;
	}

	struct sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));

	/*随机产生一个30000-60000的端口并绑定在socket上，如果绑定失败，再换一个*/
	INT32 failedTimes = 0;
	desc->localPort = rand() % 30000 + 30000;
	localAddr.sin_port = htons(desc->localPort);
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	while (bind(desc->sock, (struct sockaddr *)&localAddr, sizeof(localAddr)) != 0)
	{
		/*限制最大失败次数，防止因为bind函数卡死在这里*/
		failedTimes++;
		if (failedTimes > TFTP12_BIND_FAILED_TIMES)
		{
			printf("\nserver servo bind failed:%d", GetLastError());
			return;
		}
		desc->localPort = rand() % 30000 + 30000;
		localAddr.sin_port = htons(desc->localPort);
	}

	/*检查是否带有blocksize,且在允许的范围内*/
	if (desc->option.blockSize != TFTP12_SERVER_OPTION_INIT)
	{
		if (desc->option.blockSize > TFTP12_BLOCKSIZE_MAX)
		{
			desc->option.blockSize = TFTP12_BLOCKSIZE_MAX;
		}
		else if (desc->option.blockSize < TFTP12_BLOCKSIZE_MIN)
		{
			desc->option.blockSize = TFTP12_BLOCKSIZE_MIN;
		}
	}

	/*检查是否带有timeout,且在允许的范围内*/
	if (desc->option.timeout != TFTP12_SERVER_OPTION_INIT)
	{
		if (desc->option.timeout > TFTP12_TIMEOUT_MAX)
		{
			desc->option.timeout = TFTP12_TIMEOUT_MAX;
		}
		else if (desc->option.timeout < TFTP12_TIMEOUT_MIN)
		{
			desc->option.timeout = TFTP12_TIMEOUT_MIN;
		}
	}

	/*如果客户端发过来的是PUT（WRITE）*/
	if (desc->writeOrRead == TFTP12_WRITE)
	{
// 		/*测试文件是否存在*/
// 		if (_access(desc->filename, 0) == 0)
// 		{
// 			//发送错误报文
// 			pktBytes = tftp12CreateERRPkt(desc, TFTP12_FILE_ALREADY_EXISTS, tftp12ErrorMsg[TFTP12_FILE_ALREADY_EXISTS]);
// 			tftp12SendAndRecv(desc, desc->sendBuffer, pktBytes, &recvBytes, TRUE);
// 			tftp12ServoTaskExit(node);
// 		}

		desc->openFile = fopen(desc->filename, "wb+");
		if (desc->openFile == NULL)
		{
			printf("\nfile create failed");
			return;
		}
		fseek(desc->openFile, 0, SEEK_SET);

		/*只要有一个option就回OACK*/
		if (desc->option.blockSize != TFTP12_SERVER_OPTION_INIT\
			|| desc->option.timeout != TFTP12_SERVER_OPTION_INIT\
			|| desc->option.tsize != TFTP12_SERVER_OPTION_INIT)
		{
			pktBytes = tftp12CreateOACKPkt(&(node->clientDesc));
		}
		else /*如果没有option就回ACK*/
		{
			/*如果没有option，使用默认的值*/
			desc->option.blockSize = TFTP12_DEFAULT_BLOCKSIZE;
			desc->option.timeout = gTftp12ServerInfo.defaultTimeout;
			pktBytes = tftp12CreateACKPkt(&(node->clientDesc), 0);
			nextBlockNumber = 1;
		}

		if (desc->option.blockSize == TFTP12_SERVER_OPTION_INIT)
		{
			desc->option.blockSize = TFTP12_DEFAULT_BLOCKSIZE;
		}
		if (desc->option.timeout == TFTP12_SERVER_OPTION_INIT)
		{
			desc->option.timeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
		}

		/*以本地的端口号做ID，保证不重复*/
		if ((desc->recvBuffer = tftp12IOBufferInit(desc->localPort, \
			desc->option.blockSize, \
			desc->openFile, \
			desc->option.tsize,
			desc->writeOrRead)) == NULL)
		{
			printf("\nio buffer alloc failed!");
			return;
		}

		static INT32 toalWrite = 0;
		while (1)
		{
			if (tftp12SendAndRecv(desc, desc->sendBuffer, pktBytes, &recvBytes, FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
				break;
			}

			INT32 realWrite = tftp12WriteNextBlock(desc->localPort, desc->recvBuffer + 4, recvBytes - 4);
			toalWrite += realWrite;
			if (realWrite < (recvBytes - 4))
			{
				printf("\nwrite error:%d", realWrite);
			}
			tftp12CreateACKPkt(&(node->clientDesc), nextBlockNumber);
			nextBlockNumber++;
			if (recvBytes < (desc->option.blockSize + 4))
			{
				if (tftp12SendAndRecv(desc, desc->sendBuffer, pktBytes, &recvBytes, TRUE) != TFTP12_OK)
				{
					printf("\nsend and receive error");
				}
				printf("\nreceive finish");
				break;
			}
		}
		//FCLOSE_Z(desc->openFile);
		tftp12ServoTaskExit(node);
		//	printf("\nreceive finish:%d", toalWrite);
	}
	else if (desc->writeOrRead == TFTP12_READ)
	{
		/*测试文件是否存在且具有读权限*/
		if (_access(desc->filename, F_OK) != 0)
		{
			/*没有不存在，回复error*/
			pktBytes = tftp12CreateERRPkt(desc, TFTP12_FILE_NOT_FOUND, tftp12ErrorMsg[TFTP12_FILE_NOT_FOUND]);
			tftp12SendAndRecv(desc, desc->sendBuffer, pktBytes, &recvBytes, TRUE);
			tftp12ServoTaskExit(node);
		}

		/*测试文件是否可读*/
		if (_access(desc->filename, R_OK) != 0)
		{
			pktBytes = tftp12CreateERRPkt(desc, TFTP12_ACCESS_VIOLATION, tftp12ErrorMsg[TFTP12_ACCESS_VIOLATION]);
			tftp12SendAndRecv(desc, desc->sendBuffer, pktBytes, &recvBytes, TRUE);
			tftp12ServoTaskExit(node);
		}

		/*这个需要根据模式选择打开方式*/
		desc->openFile = fopen(node->clientDesc.filename, "rb");
		if (node->clientDesc.openFile == NULL)
		{
			printf("\nfile open failed");
			return;
		}

		/*只要有一个option就回OACK*/
		if (desc->option.blockSize != TFTP12_SERVER_OPTION_INIT\
			|| desc->option.timeout != TFTP12_SERVER_OPTION_INIT\
			|| desc->option.tsize != TFTP12_SERVER_OPTION_INIT)
		{
			/*如果客户端带有tsize*/
			if (desc->option.tsize == 0)
			{
				INT32 fileSize = 0;
				fseek(desc->openFile, 0, SEEK_END);
				fileSize = ftell(desc->openFile);
				desc->option.tsize = fileSize;
				fseek(desc->openFile, 0, SEEK_SET);
			}
			if (desc->option.timeout = TFTP12_SERVER_OPTION_INIT)
			{
				desc->option.timeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
			}
			desc->recvBuffer = tftp12IOBufferInit(desc->localPort, desc->option.blockSize, desc->openFile, 0, TFTP12_READ);
			pktBytes = tftp12CreateOACKPkt(desc);
			if (tftp12SendAndRecv(desc, desc->sendBuffer, pktBytes, &recvBytes, FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
				fclose(desc->openFile);
				tftp12ServoTaskExit(node);
			}

			/*如果收到了ACk，那发送的数据块编号应该为1*/
			nextBlockNumber = 1;
		}
		else
		{
			desc->option.blockSize = TFTP12_BLOCKSIZE_MIN;
			desc->option.timeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
			desc->recvBuffer = tftp12IOBufferInit(desc->localPort, desc->option.blockSize, desc->openFile, 0, TFTP12_READ);
			//desc->option.tsize=
		}
		//tftp12ReadNextBlock(desc->localPort,)

		if (desc->recvBuffer == NULL)
		{
			printf("\nIO buffer init failed");
			fclose(desc->openFile);
			tftp12ServoTaskExit(node);
		}

		INT32 realReadBytes = 0;
		while (1)
		{
			char *sendData = tftp12ReadNextBlock(desc->localPort, &realReadBytes);
			if (sendData == NULL)
			{
				printf("\nfile read failed");
				fclose(desc->openFile);
				tftp12ServoTaskExit(node);
			}
			/*数据包的大小为读取的字节数加上data报文头部4个字节*/
			pktBytes = realReadBytes + 4;
			sendData = tftp12CreateDataPkt(sendData, nextBlockNumber);
			if (tftp12SendAndRecv(desc, sendData, pktBytes, &recvBytes, FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
				fclose(desc->openFile);
				tftp12ServoTaskExit(node);
			}
			nextBlockNumber++;
			/*如果读出的字节数小于一个blocksiz，表示发送完成，退出*/
			if (realReadBytes < desc->option.blockSize)
			{
				printf("\nsend finish");
				fclose(desc->openFile);
				tftp12ServoTaskExit(node);
			}
		}
		
	}
}

void tftp12ServerMainTask(void *arg)
{
	char recvBuff[TFTP12_CONTROL_PACKET_MAX_SIZE];
	struct sockaddr_in peerAddr;

	memset(&peerAddr, 0, sizeof(peerAddr));
	memset(recvBuff, 0, sizeof(recvBuff));
	memset(&gTftp12ServerInfo, 0, sizeof(gTftp12ServerInfo));
	gTftp12ServerInfo.defaultTimeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
	gTftp12ServerInfo.reTransmitTimes = TFTP12_SERVER_DEFAULT_RETRANSMIT;
	gTftp12ServerInfo.sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (gTftp12ServerInfo.sock < 0)
	{
		printf("\nserver create socket failed!");
		return;
	}
	gTftp12ServerInfo.serverAddr.sin_addr.S_un.S_addr = htons(INADDR_ANY);
	gTftp12ServerInfo.serverAddr.sin_port = htons(TFTP12_SERVER_DEFAULT_BIND_PORT);
	gTftp12ServerInfo.serverAddr.sin_family = AF_INET;
	if (bind(gTftp12ServerInfo.sock, \
		(struct sockaddr*)&gTftp12ServerInfo.serverAddr, \
		sizeof(gTftp12ServerInfo.serverAddr)) != 0)
	{
		printf("\nserver bind port failed!");
		return;
	}
	printf("\nserver start!");
	INT32 fromLen = sizeof(peerAddr);
	INT32 recvBytes = 0;
	HANDLE useless = NULL;
	TFTP12ClientNode *node;
	while (1)
	{
		recvBytes = recvfrom(gTftp12ServerInfo.sock, recvBuff, TFTP12_CONTROL_PACKET_MAX_SIZE, \
			0, (struct sockaddr*)&peerAddr, &fromLen);
		if (recvBytes < 0)
		{
			printf("\nserver receive error");
			return;
		}
		printf("\nrecieved request from :%s", inet_ntoa(peerAddr.sin_addr));
		if (TFTP12_GET_OPCODE(recvBuff) != TFTP12_OPCODE_READ_REQUEST\
			&&TFTP12_GET_OPCODE(recvBuff) != TFTP12_OPCODE_WRITE_REQUEST)
		{
			continue;
		}
		recvBuff[recvBytes] = '\0';
		node = (TFTP12ClientNode*)malloc(sizeof(TFTP12ClientNode));
		if (node == NULL)
		{
			printf("\nmemory alloc failed!");
			return;
		}
		memset(node, 0, sizeof(TFTP12ClientNode));
		tftp12ClientListInsert(node);
		memcpy(node->clientDesc.sendBuffer, recvBuff, recvBytes);
		memcpy(&node->clientDesc.peerAddr, &peerAddr, sizeof(peerAddr));

		/*先让他等于-1，用来区分对方报文是否携带option*/
		node->clientDesc.option.tsize = TFTP12_SERVER_OPTION_INIT;
		node->clientDesc.option.timeout = TFTP12_SERVER_OPTION_INIT;
		node->clientDesc.option.blockSize = TFTP12_SERVER_OPTION_INIT;
		node->clientDesc.maxRetransmit = gTftp12ServerInfo.reTransmitTimes;

		tftp12ParseREQPkt(&(node->clientDesc));
		useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12ServerServoTask, (LPVOID)node, 0, NULL);
		printf("\ncreate servo task");
		CloseHandle(useless);
	}
}

INT32 tftp12ServerEnable()
{
	HANDLE useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12ServerMainTask, NULL, 0, NULL);
	CloseHandle(useless);
	return TRUE;
}