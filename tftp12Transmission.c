#pragma comment(lib, "ws2_32.lib")
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "stdio.h"
#include "stdint.h"
#include "conio.h"
#include "time.h"

#include "tftp12header.h"


extern const char *tftp12ConstStrTransMode[2];


void testtrans()
{
	srand((UINT32)time(0));


	TFTP12Description test;


	struct sockaddr_in localAddr;

	memset(&test, 0, sizeof(test));
	memset(&localAddr, 0, sizeof(localAddr));

	test.filename = "test.txt";
	test.sock = socket(AF_INET, SOCK_DGRAM, 0);
	test.localPort = rand() % 30000 + 30000;

	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
	localAddr.sin_port = htons(test.localPort);

	test.peerAddr.sin_addr.S_un.S_addr = inet_addr("192.168.139.128");
	test.peerAddr.sin_port = htons(69);
	test.peerAddr.sin_family = AF_INET;
	while (bind(test.sock, (struct sockaddr*)&localAddr, sizeof(localAddr)) != 0)
	{
		test.localPort = rand() % 30000 + 30000;
		localAddr.sin_port = htons(test.localPort);
	}

	test.mode = tftp12ConstStrTransMode[1];
	test.option.blockSize = 1024;
	test.option.timeout = 1;
	test.option.tsize = 10000;
	test.writeOrRead = TFTP12_OPCODE_WRITE_REQUEST;
	test.maxRetransmit = 5;

	INT32 sendSize = tftp12CreateREQPkt(&test);
	static char recvbuf[600000];
	test.recvBuffer = recvbuf;
	tftp12SendAndRecv(&test, sendSize);
}

INT32 tftp12SendAndRecv(TFTP12Description *desc, INT32 SendPktSize)
{
	INT32 selectRet = 0;
	INT32 sendLen = sendto(desc->sock, desc->sendBuffer, SendPktSize, \
		0, (struct sockaddr *)&desc->peerAddr, sizeof(desc->peerAddr));
	if (sendLen != SendPktSize)
	{
		printf("\nsendError,Expect:%d,Send:%d", SendPktSize, sendLen);
		if (sendLen==-1)
		{
			printf("\nsendError:%d", GetLastError());
		}
		return ERROR;
	}
	struct timeval timeout = { desc->option.timeout,0 };
	struct fd_set fd;
	memset(&fd, 0, sizeof(fd));
	INT32 sendTimes = 0;

	struct timeval currentTime = { 0,0 };
	while(1)
	{
		timeout.tv_sec = desc->option.timeout;
		timeout.tv_usec = 0;
		FD_ZERO(&fd);
		FD_SET(desc->sock, &fd);
		selectRet = select(desc->sock + 1, &fd, NULL, NULL, &timeout);

		SOCKADDR_IN recvPeerAddr;
		memset(&recvPeerAddr, 0, sizeof(recvPeerAddr));

		if (selectRet == 0)
		{
			printf("\nselect超时");
			sendTimes++;
			if (sendTimes >= desc->maxRetransmit)
			{
				return TFTP12_TIMEOUT;
			}
			sendLen = sendto(desc->sock, desc->sendBuffer, SendPktSize, \
				0, (struct sockaddr *)&desc->peerAddr, sizeof(desc->peerAddr));
			if (sendLen != SendPktSize)
			{
				printf("\nsendError,Expect:%d,Send:%d", SendPktSize, sendLen);
				return ERROR;
			}
			continue;
		}
		else if (selectRet > 0)
		{
			if (FD_ISSET(desc->sock, &fd) != TRUE)
			{
				continue;
			}
			INT32 len = sizeof(recvPeerAddr);
			desc->recvBytes = recvfrom(desc->sock, desc->recvBuffer, TFTP12_BUFFER_SIZE(desc), 0, (struct sockaddr*)&recvPeerAddr, &len);
			
			if (desc->recvBytes < 0)
			{
				if (GetLastError()==10054)
				{
					return TFTP12_NOCONNECT;
				}
			}

			printf("\n");
			for (INT32 i=0;i<desc->recvBytes;i++)
			{
				printf("%c", desc->recvBuffer[i]);
			}
			INT32 recvOPCode = TFTP12_GET_OPCODE(desc->recvBuffer);
			INT32 sendOPCode = TFTP12_GET_OPCODE(desc->sendBuffer);

			/*如果出现错误报文，同样视为收到了正确的报文，在外部处理*/
			if (recvOPCode == TFTP12_OPCODE_ERROR)
			{
				return TFTP12_OK;
			}
			if (sendOPCode == TFTP12_OPCODE_READ_REQUEST)
			{
				if (recvOPCode == TFTP12_OPCODE_OACK || recvOPCode == TFTP12_OPCODE_DATA)
				{
					return TFTP12_OK;
				}
				continue;
			}
			else if (sendOPCode == TFTP12_OPCODE_WRITE_REQUEST)
			{
				if (recvOPCode == TFTP12_OPCODE_OACK || recvOPCode == TFTP12_OPCODE_ACK)
				{
					return TFTP12_OK;
				}
				continue;
			}
			else if (sendOPCode == TFTP12_OPCODE_DATA)
			{
				if (recvOPCode == TFTP12_OPCODE_ACK)
				{
					return TFTP12_OK;
				}
				continue;
			}
			else if (sendOPCode == TFTP12_OPCODE_ACK)
			{
				if (recvOPCode == TFTP12_OPCODE_DATA)
				{
					return TFTP12_OK;
				}
				continue;
			}
		}
		else if (selectRet == -1)
		{
			printf("\nselect Error");
			return TFTP12_SELECT_ERROR;
		}
	}
}

