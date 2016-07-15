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

#define TFTP12_DEFAULT_BLOCKSIZE			(512)

#define TFTP12_SERVER_DEFAULT_TIMEOUT		(3)
#define TFTP12_SERVER_DEFAULT_RETRANSMIT	(3)
#define TFTP12_SERVER_DEFAULT_BIND_PORT		(69)

#define TFTP12_BIND_FAILED_TIMES			(20)
#define TFTP12_SERVER_OPTION_INIT			(-1)

#define TFTP12_GET_BLOCKNUM(buf)		htons((*(INT16*)(buf+2)))
#define FOEVER								for(;;)

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

void tftp12ServerServoTask(void *arg)
{
	TFTP12ClientNode *node = arg;

	if (node == NULL)
	{
		printf("\nserver servo task argument is NULL!");
		return;
	}

	node->clientDesc.sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (node->clientDesc.sock < 0)
	{
		printf("\nservo task socket create failed!");
		return;
	}

	struct sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));

	/*随机产生一个30000-60000的端口并绑定在socket上，如果绑定失败，再换一个*/
	INT32 failedTimes = 0;
	node->clientDesc.localPort = rand() % 30000 + 30000;
	localAddr.sin_port = htons(node->clientDesc.localPort);
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	while (bind(node->clientDesc.sock, (struct sockaddr *)&localAddr, sizeof(localAddr)) != 0)
	{
		/*限制最大失败次数，防止因为bind函数卡死在这里*/
		failedTimes++;
		if (failedTimes > TFTP12_BIND_FAILED_TIMES)
		{
			printf("\nserver servo bind failed:%d", GetLastError());
			return;
		}
		node->clientDesc.localPort = rand() % 30000 + 30000;
		localAddr.sin_port = htons(node->clientDesc.localPort);
	}

	/*检查是否带有blocksize,且在允许的范围内*/
	if (node->clientDesc.option.blockSize != TFTP12_SERVER_OPTION_INIT)
	{
		if (node->clientDesc.option.blockSize > TFTP12_BLOCKSIZE_MAX)
		{
			node->clientDesc.option.blockSize = TFTP12_BLOCKSIZE_MAX;
		}
		else if (node->clientDesc.option.blockSize < TFTP12_BLOCKSIZE_MIN)
		{
			node->clientDesc.option.blockSize = TFTP12_BLOCKSIZE_MIN;
		}
	}

	/*检查是否带有timeout,且在允许的范围内*/
	if (node->clientDesc.option.timeout != TFTP12_SERVER_OPTION_INIT)
	{
		if (node->clientDesc.option.timeout > TFTP12_TIMEOUT_MAX)
		{
			node->clientDesc.option.timeout = TFTP12_TIMEOUT_MAX;
		}
		else if (node->clientDesc.option.timeout < TFTP12_TIMEOUT_MIN)
		{
			node->clientDesc.option.timeout = TFTP12_TIMEOUT_MIN;
		}
	}

	/*如果客户端发过来的是PUT（WRITE）*/
	if (node->clientDesc.writeOrRead == TFTP12_WRITE)
	{
		/*测试文件是否存在*/
		if (_access(node->clientDesc.filename, 0) == TRUE)
		{
			//发送错误报文
		}

		node->clientDesc.openFile = fopen(node->clientDesc.filename, "wb+");
		if (node->clientDesc.openFile == NULL)
		{
			printf("\nfile create failed");
			return;
		}
		fseek(node->clientDesc.openFile, 0, SEEK_SET);

		/*如果没有解析到tsize*/
		if (node->clientDesc.option.tsize != TFTP12_SERVER_OPTION_INIT)
		{
			//先不管
		}

		/*以本地的端口号做ID，保证不重复*/
		if ((node->clientDesc.recvBuffer = tftp12IOBufferInit(node->clientDesc.localPort, \
			node->clientDesc.option.blockSize, \
			node->clientDesc.openFile, \
			node->clientDesc.option.tsize,
			node->clientDesc.writeOrRead)) == NULL)
		{
			printf("\nio buffer alloc failed!");
			return;
		}

		/*只要有一个option就回OACK*/
		INT32 sendBytes = 0;
		INT32 recvBytes = 0;
		INT32 nextBlockNumber = 0;
		if (node->clientDesc.option.blockSize != TFTP12_SERVER_OPTION_INIT\
			|| node->clientDesc.option.timeout != TFTP12_SERVER_OPTION_INIT\
			|| node->clientDesc.option.tsize != TFTP12_SERVER_OPTION_INIT)
		{
			sendBytes = tftp12CreateOACKPkt(&(node->clientDesc));
		}
		else /*如果没有option就回ACK*/
		{
			/*如果没有option，使用默认的值*/
			node->clientDesc.option.blockSize = TFTP12_DEFAULT_BLOCKSIZE;
			node->clientDesc.option.timeout = gTftp12ServerInfo.defaultTimeout;
			sendBytes = tftp12CreateACKPkt(&(node->clientDesc), 0);
		}

		static INT32 toalWrite = 0;
		FOEVER
		{
			if (tftp12SendAndRecv(&(node->clientDesc), sendBytes,&recvBytes,FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
			}

		INT32 realWrite = tftp12WriteNextBlock(node->clientDesc.localPort, node->clientDesc.recvBuffer + 4, recvBytes-4);
		toalWrite += realWrite;
		if (realWrite < (recvBytes - 4))
		{
			printf("\nwrite error:%d",realWrite);
		}
		printf("\nwrite:%d", realWrite);
			tftp12CreateACKPkt(&(node->clientDesc), nextBlockNumber);
			nextBlockNumber++;
			if (recvBytes < (node->clientDesc.option.blockSize + 4))
			{
				if (tftp12SendAndRecv(&(node->clientDesc), sendBytes, &recvBytes, TRUE) != TFTP12_OK)
				{
					printf("\nsend and receive error");
				}
				break;
			}
		}
		fclose(node->clientDesc.openFile);
		printf("\nreceive finish:%d",toalWrite);
	}
	else if (node->clientDesc.writeOrRead == TFTP12_READ)
	{

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
		TFTP12ClientNode *node = (TFTP12ClientNode*)malloc(sizeof(TFTP12ClientNode));
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
	//	node->clientDesc.maxRetransmit = gTftp12ServerInfo.reTransmitTimes;

		tftp12ParseREQPkt(&(node->clientDesc));
		useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12ServerServoTask, (LPVOID)node, 0, NULL);
		CloseHandle(useless);
	}
}

INT32 tftp12ServerEnable()
{
	HANDLE useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12ServerMainTask, NULL, 0, NULL);
	CloseHandle(useless);
	return TRUE;
}