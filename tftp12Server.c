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
#include "tftp12Log.h"
#include "tftp12Server.h"

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */

#define TFTP12_DEFAULT_BLOCKSIZE			(512)		/*服务器默认的blksize*/
#define TFTP12_SERVER_DEFAULE_USERNUM		(5)			/*默认最大用户数量*/
#define TFTP12_SERVER_DEFAULT_TIMEOUT		(3)			/*默认的超时时间*/
#define TFTP12_SERVER_DEFAULT_RETRANSMIT	(3)			/*默认的重传次数*/
#define TFTP12_SERVER_DEFAULT_BIND_PORT		(69)		/*默认的监听端口*/
#define TFTP12_BIND_FAILED_TIMES			(20)		/*默认绑定端口次数*/
#define TFTP12_SERVER_OPTION_INIT			(-1)		/*初始值*/

#define  tftp12ServoTaskExit(node)	{\
			printf("\ntask exit, port:%d",node->clientDesc.localPort);\
				\
			tftp12WaitIOFinishById(node->clientDesc.localPort);\
			FCLOSE_Z(node->clientDesc.openFile);\
			SCLOSE_Z(node->clientDesc.sock);\
			tftp12IOBufferFree(node->clientDesc.localPort);\
			tftp12ClientListDelete(node);\
			FREE_Z(node->clientDesc.filename);\
			FREE_Z(node->clientDesc.mode);\
			FREE_Z(node); \
			return;\
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
	/*需要用互斥信号量保护起来*/
	INT32 serverTaskID;
	INT32 sock;
	struct sockaddr_in serverAddr;
	INT32 serverPort;
	INT32 tftpServerIsRunning;
	INT32 defaultTimeout;
	INT32 reTransmitTimes;
	INT32 maxUser;
	TFTP12ClientList clientList;
}TFTP12ServerInfo;

static TFTP12ServerInfo tftp12ServerInfo;


INT32 tftp12ServerEnable();
INT32 tftp12ServerDisable();

/*设置在没有option时候的超时时间*/
void tftp12ServerSetTimeout(INT32 time)
{
	tftp12ServerInfo.defaultTimeout = time;
}

/*设置重传次数*/
void tftp12ServerSetRetransmit(INT32 times)
{
	tftp12ServerInfo.reTransmitTimes = times;
}
/*设置服务器的绑定端口*/
void tftp12ServerSetPort(UINT16 port)
{
	tftp12ServerInfo.serverPort = port;
}

/*show当前的连接*/
void tftp12ServerShowStatus(void)
{
	TFTP12ClientNode *pWalk = NULL;
	char numBuf[11];/*用来保存itoa的数组*/
	char percentBuf[11];/*用来保存百分比的数组*/
	float percent = 0;
	printf("\nTFTP12 Status:%s", (tftp12ServerInfo.tftpServerIsRunning == TRUE ? "Enable" : "Disable"));
	printf("\nServer port: %d", tftp12ServerInfo.serverPort);
	printf("\nTimeout: %d", tftp12ServerInfo.defaultTimeout);
	printf("\nRetransmissions: %d", tftp12ServerInfo.reTransmitTimes);
	printf("\nMax user number: %d", tftp12ServerInfo.maxUser);

	if (tftp12ServerInfo.tftpServerIsRunning == TRUE)
	{
		printf("\nCurrent transmissions: %d", tftp12ServerInfo.clientList.count);
		printf("\n       IP           Port(L/R)        Progress(total)         blksize/timeout");
		pWalk = tftp12ServerInfo.clientList.head;
		while (pWalk != NULL)
		{
			if (pWalk->clientDesc.option.tsize > 0)
			{
				sprintf(percentBuf, "[%2.1f%%]", ((pWalk->clientDesc.transmitBytes*100.0) / (float)pWalk->clientDesc.option.tsize));
			}
			else
			{
				percentBuf[0] = '\0';
				percentBuf[1] = '\0';
			}
			printf("\n%s     %d/%d     	%dk(%sk)%s		    %d/%d", inet_ntoa(pWalk->clientDesc.peerAddr.sin_addr), \
				pWalk->clientDesc.localPort, pWalk->clientDesc.peerAddr.sin_port, pWalk->clientDesc.transmitBytes / 1000, \
				((pWalk->clientDesc.option.tsize > 0) ? (_itoa(pWalk->clientDesc.option.tsize / 1000, numBuf, 10)) : ("Unknow")), \
				percentBuf, pWalk->clientDesc.option.blockSize, pWalk->clientDesc.option.timeout);
			pWalk = pWalk->next;
		}
	}
}

/*设置最大并发数量*/
void tftp12ServerSetMaxuser(INT32 userNum)
{
	tftp12ServerInfo.maxUser = userNum;
}

/*强制断开一个用户的连接*/
void tftp12ServerForceStop(UINT32 ip)
{
	struct in_addr in;
	in.S_un.S_addr = ip;
	printf("\nforce stop: %s", inet_ntoa(in));
}


/*
tftp12Server Enable						//Enable TFTP12 Server
tftp12Server Disable					//Disable TFTP12 Server
tftp12Server SetMaxUser 1-20			//Set the maximum number of concurrent users
tftp12Server SetServerPort 1-65535		//Set the server listen port
tftp12Server SetRetransmitTimes			//Set the number of retransmissions
tftp12Server SetTimeout 1-255			//Set the timeout time
tftp12Server ForceStop					//Force disconnect by ip
*/
void tftp12SeverShellCallback(INT32 argc, INT8 *argv[])
{
	if (strcmp(argv[0], "tftp12Server") == 0)
	{
		if (strcmp(argv[1], "Enable") == 0)
		{
			tftp12ServerEnable();
			return;
		}
		else if (strcmp(argv[1], "Disable") == 0)
		{
			tftp12ServerDisable();
			return;
		}
		else if (strcmp(argv[1], "SetMaxUser") == 0)
		{
			tftp12ServerSetMaxuser(atoi(argv[2]));
			return;
		}
		else if (strcmp(argv[1], "SetServerPort") == 0)
		{
			tftp12ServerSetPort(atoi(argv[2]));
			return;
		}
		else if (strcmp(argv[1], "SetTimeout") == 0)
		{
			tftp12ServerSetTimeout(atoi(argv[2]));
			return;
		}
		else if (strcmp(argv[1], "ForceStop") == 0)
		{
			tftp12ServerForceStop(inet_addr(argv[2]));
			return;
		}
	}
	else if (strcmp(argv[0], "no") == 0)
	{
		if (strcmp(argv[1], "tftp12Server") == 0)
		{
			if (strcmp(argv[2], "Enable") == 0)
			{
				tftp12ServerDisable();
				return;
			}
		}
	}

	printf("\nTFTP12:undefined command");
}

void tftp12ServerInit(void)
{
	memset(&tftp12ServerInfo, 0, sizeof(tftp12ServerInfo));
	tftp12ServerInfo.serverPort = TFTP12_SERVER_DEFAULT_BIND_PORT;
	tftp12ServerInfo.defaultTimeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
	tftp12ServerInfo.reTransmitTimes = TFTP12_SERVER_DEFAULT_RETRANSMIT;
	tftp12ServerInfo.maxUser = TFTP12_SERVER_DEFAULE_USERNUM;
	return;
}

static void tftp12ClientListInsert(TFTP12ClientNode *node)
{
	tftp12ServerInfo.clientList.count++;
	if (tftp12ServerInfo.clientList.head == NULL)
	{
		tftp12ServerInfo.clientList.head = node;
		return;
	}

	TFTP12ClientNode *pwalk = tftp12ServerInfo.clientList.head;
	while (pwalk->next != NULL)
	{
		pwalk = pwalk->next;
	}
	pwalk->next = node;
	return;
}

static void tftp12ClientListDelete(TFTP12ClientNode *node)
{
	TFTP12ClientNode *pWalk = tftp12ServerInfo.clientList.head;

	if (node == tftp12ServerInfo.clientList.head)
	{
		tftp12ServerInfo.clientList.head = node->next;
		tftp12ServerInfo.clientList.count--;
		return;
	}

	while (pWalk->next != NULL)
	{
		if (pWalk->next == node)
		{
			pWalk->next = pWalk->next->next;
			tftp12ServerInfo.clientList.count--;
			return;
		}
		pWalk = pWalk->next;
	}
}

static inline INT32 tftp12GetTransMode(TFTP12ClientNode *node)
{
	if (_strnicmp(node->clientDesc.mode, "netascii", sizeof("netascii")) == 0)
	{
		return TFTP12_NETASCII;
	}
	else
	{
		return TFTP12_OCTET;
	}
}

void tftp12ServerServoTask(void *arg)
{
	char oackTemBuffer[TFTP12_DEFAULT_BLOCKSIZE];		/*临时用一个，size只需要大于512就可以了*/
	INT32 pktBytes = 0;				/*发送报文长度*/
	INT32 realWriteBytes = 0;		/*单次写入的字节数*/
	INT32 realReadBytes = 0;		/*单次读出的字节数*/
	INT32 recvBytes = 0;			/*单次报文接收的字节数*/
	UINT16 nextBlockNumber = 0;		/*下一块数据的编号*/
	INT32 totalTransBytes = 0;			/*一共传输的字节数*/
	enum TFTP12_TRANS_MODE transMode = TFTP12_OCTET;
	TFTP12ClientNode *node = arg;
	TFTP12Description *desc = NULL;
	INT32 fileSize = 0;

	if (node == NULL)
	{
		printf("\nserver servo task argument is NULL!");
		return;
	}
	transMode = tftp12GetTransMode(node);
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
		// 			tftp12SendAndRecv(desc, desc->controlPktBuffer, pktBytes, &recvBytes, TRUE);
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
		else/*如果没有option就回ACK*/
		{
			desc->option.timeout = tftp12ServerInfo.defaultTimeout;
			pktBytes = tftp12CreateACKPkt(&(node->clientDesc), 0);
		}

		/*接收的时候第一个数据报文的number是1*/
		/*有的第一个data报文从0开始，有的又是从1开始*/
		nextBlockNumber = 1;
		//nextBlockNumber = TFTP12_GET_BLOCKNUM(desc->recvBuffer);

		/*如果没有option，使用默认的值*/
		if (desc->option.blockSize == TFTP12_SERVER_OPTION_INIT)
		{
			desc->option.blockSize = TFTP12_DEFAULT_BLOCKSIZE;
		}
		if (desc->option.timeout == TFTP12_SERVER_OPTION_INIT)
		{
			desc->option.timeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
		}

		/*以本地的端口号做ID，保证不重复*/
		if ((desc->recvBuffer = tftp12IOBufferInit(
			desc->localPort,
			desc->option.blockSize,
			desc->openFile,
			desc->option.tsize,
			transMode,
			desc->writeOrRead)) == NULL)
		{
			printf("\nio buffer alloc failed!");
			//	FCLOSE_Z(desc->openFile);
			tftp12ServoTaskExit(node);
		}
		desc->recvBufferSize = TFTP12_IO_BUFFERSIZE(desc->option.blockSize);
		while (1)
		{
			if (tftp12SendAndRecv(desc, desc->controlPktBuffer, pktBytes, &recvBytes, FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
				//	FCLOSE_Z(desc->openFile);
				tftp12ServoTaskExit(node);
			}

			/*获得下一次的接收缓冲区，赋给recvBuffer*/
			desc->recvBuffer = tftp12WriteNextBlock(desc->localPort, desc->recvBuffer + 4, recvBytes - 4);

			/*保存传输了多少字节*/
			desc->transmitBytes += recvBytes - 4;

			pktBytes = tftp12CreateACKPkt(&(node->clientDesc), nextBlockNumber);
			nextBlockNumber++;
			if (recvBytes < (desc->option.blockSize + 4))
			{
				if (tftp12SendAndRecv(desc, desc->controlPktBuffer, pktBytes, &recvBytes, TRUE) != TFTP12_OK)
				{
					printf("\nsend and receive error");
				}

				/*如果带的有tsize字段，则检查接收到的数据和tsize是否一致*/
				if (desc->option.tsize != TFTP12_SERVER_OPTION_INIT)
				{
					if (desc->transmitBytes != desc->option.tsize)
					{
						printf("\ntransmit size error");
					}
				}
				printf("\nreceive finish");
				break;
			}
		}
		/*这里需要等待iobuffer的写入操作结束*/
		//tftp12WaitIOFinishById(desc->localPort);
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
			tftp12SendAndRecv(desc, desc->controlPktBuffer, pktBytes, &recvBytes, TRUE);
			tftp12ServoTaskExit(node);
		}

		/*测试文件是否可读*/
		if (_access(desc->filename, R_OK) != 0)
		{
			pktBytes = tftp12CreateERRPkt(desc, TFTP12_ACCESS_VIOLATION, tftp12ErrorMsg[TFTP12_ACCESS_VIOLATION]);
			tftp12SendAndRecv(desc, desc->controlPktBuffer, pktBytes, &recvBytes, TRUE);
			tftp12ServoTaskExit(node);
		}

		/*这个需要根据模式选择打开方式*/
		desc->openFile = fopen(node->clientDesc.filename, "rb");
		if (node->clientDesc.openFile == NULL)
		{
			printf("\nfile open failed");
			return;
		}


		/*读出文件大小*/
		fileSize = 0;
		fseek(desc->openFile, 0, SEEK_END);
		fileSize = ftell(desc->openFile);

		/*如果客户端带有tsize*/
		if (desc->option.tsize == 0)
		{
			desc->option.tsize = fileSize;
		}
		fseek(desc->openFile, 0, SEEK_SET);

		/*只要有一个option就回OACK*/
		if (desc->option.blockSize != TFTP12_SERVER_OPTION_INIT\
			|| desc->option.timeout != TFTP12_SERVER_OPTION_INIT\
			|| desc->option.tsize != TFTP12_SERVER_OPTION_INIT)
		{
			/*如果timeout*/
			if (desc->option.timeout = TFTP12_SERVER_OPTION_INIT)
			{
				desc->option.timeout = TFTP12_SERVER_DEFAULT_TIMEOUT;
			}
			desc->dataPktBuffer = tftp12IOBufferInit(
				desc->localPort,
				desc->option.blockSize,
				desc->openFile,
				fileSize,
				transMode,
				TFTP12_READ);
			if (desc->dataPktBuffer == NULL)
			{
				printf("\nIObuffer Init Failed!");
				tftp12ServoTaskExit(node);
			}

			desc->recvBuffer = oackTemBuffer;
			desc->recvBufferSize = TFTP12_DEFAULT_BLOCKSIZE;
			pktBytes = tftp12CreateOACKPkt(desc);
			if (tftp12SendAndRecv(desc, desc->controlPktBuffer, pktBytes, &recvBytes, FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
				tftp12ServoTaskExit(node);
			}

			/*改回控制报文的buffe*/
			desc->recvBuffer = desc->controlPktBuffer;
			desc->recvBufferSize = sizeof(desc->controlPktBuffer);

			/*如果收到了ACk，那发送的数据块编号应该为1*/
		}
		else
		{
			desc->option.blockSize = TFTP12_BLOCKSIZE_MIN;
			desc->option.timeout = TFTP12_SERVER_DEFAULT_TIMEOUT;

			/*如果没有option*/
			desc->dataPktBuffer = tftp12IOBufferInit(
										desc->localPort, 
										desc->option.blockSize, 
										desc->openFile, 
										fileSize,
										transMode,
										TFTP12_READ);
			desc->recvBuffer = desc->controlPktBuffer;
			desc->recvBufferSize = sizeof(desc->controlPktBuffer);
			//desc->option.tsize=
		}
		//tftp12ReadNextBlock(desc->localPort,)
		nextBlockNumber = 1;
		if (desc->recvBuffer == NULL)
		{
			printf("\nIO buffer init failed");
			//FCLOSE_Z(node->clientDesc.openFile);
			tftp12ServoTaskExit(node);
		}

		char *sendData = NULL;
		while (1)
		{
			/*返回了数据块的位置*/
			sendData = tftp12ReadNextBlock(desc->localPort, &realReadBytes);
			if (sendData == NULL)
			{
				printf("\nfile read failed");
				//FCLOSE_Z(node->clientDesc.openFile);
				tftp12ServoTaskExit(node);
			}
			desc->transmitBytes += realReadBytes;

			/*数据包的大小为读取的字节数加上data报文头部4个字节*/
			pktBytes = realReadBytes + 4;

			/*用返回的数据块位置创建datapkt*/
			sendData = tftp12CreateDataPkt(sendData, nextBlockNumber);
			if (tftp12SendAndRecv(desc, sendData, pktBytes, &recvBytes, FALSE) != TFTP12_OK)
			{
				printf("\nsend and receive error");
				//FCLOSE_Z(node->clientDesc.openFile);
				tftp12ServoTaskExit(node);
			}
			nextBlockNumber++;
			// 			if (nextBlockNumber==0)
			// 			{
			// 				nextBlockNumber = 1;
			// 			}

			/*如果读出的字节数小于一个blocksiz，表示发送完成，退出*/
			if (realReadBytes < desc->option.blockSize)
			{
				/*如果带的有tsize字段，则检查接收到的数据和tsize是否一致*/
				if (desc->option.tsize != TFTP12_SERVER_OPTION_INIT)
				{
					if (desc->transmitBytes != desc->option.tsize)
					{
						printf("\ntransmit size error");
					}
				}
				printf("\nsend finish");
				//FCLOSE_Z(node->clientDesc.openFile);
				tftp12ServoTaskExit(node);
			}
		}
	}
}

void tftp12ServerMainTask(void *arg)
{
	char recvBuff[TFTP12_CONTROL_PACKET_MAX_SIZE];
	char rejectPkt[] = { 0,TFTP12_OPCODE_ERROR,0,TFTP12_NOT_DEFINED,"Server busy!" };
	struct sockaddr_in peerAddr;
	INT32 fromLen = sizeof(peerAddr);
	INT32 recvBytes = 0;
	HANDLE useless = NULL;
	TFTP12ClientNode *node;
	SOCKET rejectSock = 0;/*发送拒绝连接报文的socket*/

	memset(&peerAddr, 0, sizeof(peerAddr));
	memset(recvBuff, 0, sizeof(recvBuff));

	rejectSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (rejectSock <= 0)
	{
		printf("\nServer Socket create failed!");
		return;
	}

	tftp12ServerInfo.sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (tftp12ServerInfo.sock < 0)
	{
		printf("\nserver create socket failed!");
		return;
	}
	tftp12ServerInfo.serverAddr.sin_addr.S_un.S_addr = htons(INADDR_ANY);
	tftp12ServerInfo.serverAddr.sin_port = htons(tftp12ServerInfo.serverPort);
	tftp12ServerInfo.serverAddr.sin_family = AF_INET;

	if (bind(tftp12ServerInfo.sock, \
		(struct sockaddr*)&tftp12ServerInfo.serverAddr, \
		sizeof(tftp12ServerInfo.serverAddr)) != 0)
	{
		printf("\nserver bind port failed!");
		return;
	}
	printf("\nserver start!");


	while (1)
	{
		recvBytes = recvfrom(tftp12ServerInfo.sock, recvBuff, TFTP12_CONTROL_PACKET_MAX_SIZE, \
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

		/*如果大于了最大用户数量，不在允许新的连接*/
		if (tftp12ServerInfo.clientList.count >= tftp12ServerInfo.maxUser)
		{
			sendto(rejectSock, rejectPkt, sizeof(rejectPkt), 0, (struct sockaddr*)&peerAddr, sizeof(peerAddr));
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
		memcpy(node->clientDesc.controlPktBuffer, recvBuff, recvBytes);
		memcpy(&node->clientDesc.peerAddr, &peerAddr, sizeof(peerAddr));

		/*先让他等于-1，用来区分对方报文是否携带option*/
		node->clientDesc.option.tsize = TFTP12_SERVER_OPTION_INIT;
		node->clientDesc.option.timeout = TFTP12_SERVER_OPTION_INIT;
		node->clientDesc.option.blockSize = TFTP12_SERVER_OPTION_INIT;
		node->clientDesc.maxRetransmit = tftp12ServerInfo.reTransmitTimes;

		tftp12ParseREQPkt(&(node->clientDesc));
		useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12ServerServoTask, (LPVOID)node, 0, NULL);
		printf("\ncreate servo task");
		CloseHandle(useless);
	}
}

INT32 tftp12ServerEnable()
{
	tftp12ServerInfo.tftpServerIsRunning = TRUE;
	HANDLE useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12ServerMainTask, NULL, 0, NULL);
	CloseHandle(useless);
	return TRUE;
}

INT32 tftp12ServerDisable()
{
	return TRUE;
}