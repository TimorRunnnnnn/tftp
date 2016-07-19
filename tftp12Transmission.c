#pragma comment(lib, "ws2_32.lib")
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "stdio.h"
#include "stdint.h"
#include "conio.h"
#include "time.h"

#include "tftp12header.h"

/*****************************************************************
* DESCRIPTION:
*	发送和接收一个字节,需要提前在desc->sendBuffer内填充发送报文
* INPUTS:
*	desc - tftp会话结构的指针，需要用到里面的sendBuffer和recvBuffer，
			用tftp12IOBufferInit的返回值赋给recvBuffer
*	sendBuf - 发送缓冲区
*	SendPktSize - 要发送的字节数
*	lastACK	- 可选TRUE和FALSE，接收文件，最后一个ACK不需要接收data报文
			  需要为TRUE
* OUTPUTS:
*	recvBytes - 接收字节数存放的指针
* RETURNS:
*	TFTP12_OK - 正确
*	其余返回不正常
		↑
	这里需要修改
*****************************************************************/
INT32 tftp12SendAndRecv(TFTP12Description *desc, char *sendBuf, INT32 SendPktSize, INT32 *recvBytes, INT32 lastPkt)
{
	struct timeval timeout = { desc->option.timeout,0 };
	struct fd_set fd;
	INT32 recvOPCode = 0;
	INT32 sendOPCode = 0;
	INT32 sendTimes = 0;
	INT32 len = 0;
	INT32 selectRet = 0;
	INT32 sendLen = sendto(desc->sock, sendBuf, SendPktSize, \
		0, (struct sockaddr *)&desc->peerAddr, sizeof(desc->peerAddr));
	if (sendLen != SendPktSize)
	{
		printf("\nsendError,Expect:%d,Send:%d", SendPktSize, sendLen);
		if (sendLen == -1)
		{
			printf("\nsendError:%d", GetLastError());
		}
		return ERROR;
	}
	if (lastPkt == TRUE)
	{
		return TFTP12_OK;
	}


	memset(&fd, 0, sizeof(fd));
	//INT32 sendTimes = 0;

	//struct timeval currentTime = { 0,0 };
	while (1)
	{
		FD_ZERO(&fd);
		FD_SET(desc->sock, &fd);
		selectRet = select(desc->sock + 1, &fd, NULL, NULL, &timeout);

		//memset(&recvPeerAddr, 0, sizeof(recvPeerAddr));

		if (selectRet == 0)
		{
			printf("\nselect超时");
			sendTimes++;
			if (sendTimes >= desc->maxRetransmit)
			{
				return TFTP12_TIMEOUT;
			}

			/*超时才重置时间*/
			timeout.tv_sec = desc->option.timeout;
			timeout.tv_usec = 0;
			sendLen = sendto(desc->sock, sendBuf, SendPktSize, \
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
			len = sizeof(desc->peerAddr);
			*recvBytes = recvfrom(desc->sock, desc->recvBuffer, desc->recvBufferSize, 0, (struct sockaddr*)&(desc->peerAddr), &len);
			if (*recvBytes < 0)
			{
				if (GetLastError() == 10054)
				{
					return TFTP12_NOCONNECT;
				}
			}
			else if (*recvBytes > (desc->option.blockSize + 4))
			{
				/*如果大于了正常报文的最大值，继续*/
				continue;
			}
			//printf("\nselect time:%d,%d", timeout.tv_sec, timeout.tv_usec);
			recvOPCode = TFTP12_GET_OPCODE(desc->recvBuffer);
			sendOPCode = TFTP12_GET_OPCODE(sendBuf);

			/*如果出现错误报文，除了errorcode=5不中断意外，其余返回错误*/
			if (recvOPCode == TFTP12_OPCODE_ERROR)
			{
				if (TFTP12_GET_ERRORCODE(desc->recvBuffer) == TFTP12_UNKNOWN_TRANSFER_ID)
				{
					return TFTP12_OK;
				}
				else
				{
					return TFTP12_RECV_A_ERROR_PKT;
				}
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
				/*如果发送的是数据报文，那么接收到的ACK应该要和发送的块编号一致*/
				if (recvOPCode == TFTP12_OPCODE_ACK && (TFTP12_GET_BLOCKNUM(desc->recvBuffer) == TFTP12_GET_BLOCKNUM(sendBuf)))
				{
					return TFTP12_OK;
				}
				continue;
			}
			else if (sendOPCode == TFTP12_OPCODE_ACK)
			{
				/*如果大于65535，则为0，否则加1*/
				if (recvOPCode == TFTP12_OPCODE_DATA && (TFTP12_GET_BLOCKNUM(desc->recvBuffer) == (((TFTP12_GET_BLOCKNUM(sendBuf)) + 1) > 65535) ? (0) : (TFTP12_GET_BLOCKNUM(sendBuf) + 1)))
				{
					return TFTP12_OK;
				}
				continue;
			}
			else if (sendOPCode == TFTP12_OPCODE_OACK)
			{
				if ((recvOPCode == TFTP12_OPCODE_ACK&&TFTP12_GET_BLOCKNUM(desc->recvBuffer) == 0) || recvOPCode == TFTP12_OPCODE_DATA)
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

