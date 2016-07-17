#pragma once

#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "tftp12Log.h"

/*Э�̹��̱�����󳤶�*/
#define TFTP12_CONTROL_PACKET_MAX_SIZE	(512)
#define TFTP12_BUFFER_SIZE(desc)		(((TFTP12Description *)desc)->option.blockSize+1)
#define TFTP12_GET_OPCODE(buf)			(htons(*(INT16*)buf))
#define TFTP12_GET_BLOCKNUM(buf)		htons((*(INT16*)(buf+2)))
#define TFTP12_GET_ERRORCODE(buf)		TFTP12_GET_BLOCKNUM(buf)

#define TFTP12_BLOCKSIZE_MAX		(65464)
#define TFTP12_BLOCKSIZE_MIN		(512)

#define TFTP12_TIMEOUT_MAX			(256)
#define TFTP12_TIMEOUT_MIN			(1)

#define TFTP12_NUMBER_OF_OPTIONS	(3)


enum TFTP12_ReadOrWrite {
	TFTP12_READ=1,
	TFTP12_WRITE,
};

enum TFTP12_TRANS_MODE
{
	TFTP12_NETASCII = 0,
	TFTP12_OCTET,
};

enum TFTP12_OPCODE
{
	TFTP12_OPCODE_READ_REQUEST = 1,
	TFTP12_OPCODE_WRITE_REQUEST,
	TFTP12_OPCODE_DATA,
	TFTP12_OPCODE_ACK,
	TFTP12_OPCODE_ERROR,
	TFTP12_OPCODE_OACK,
};
enum TFTP12_ERROR_CODE
{
	TFTP12_NOT_DEFINED = 0,
	TFTP12_FILE_NOT_FOUND,
	TFTP12_ACCESS_VIOLATION,
	TFTP12_DISK_FULL_OR_ALLOCATION_EXCEEDED,
	TFTP12_ILLEGAL_TFTP_OPERATION,
	TFTP12_UNKNOWN_TRANSFER_ID,
	TFTP12_FILE_ALREADY_EXISTS,
	TFTP12_NO_SUCH_USER,
};

enum TFTP12_SEND_RECV_STAUTS
{
	TFTP12_OK = 0,
	TFTP12_TIMEOUT,
	TFTP12_SELECT_ERROR,
	TFTP12_SEND_FAILED,
	TFTP12_NOCONNECT,
	TFTP12_RECV_A_ERROR_PKT,
};

typedef struct
{
	INT32 blockSize;
	INT32 timeout;
	INT32 tsize;
}TFTP12Option;


typedef struct
{
	INT16 writeOrRead;
	INT8 *filename;	//Ŀ���ļ���
	FILE *openFile;	//�򿪵��ļ�
	INT8 *mode;	//netascii/octet/mail
	TFTP12Option option;
	INT32 localPort;	//���س��˿�
	INT32 sock;
	INT32 transmitBytes;	//�Ѿ�����/���͵��ֽ���
	struct sockaddr_in peerAddr;		//�Զ˵�ַ
	INT8 *recvBuffer;	//���ջ�����
	INT32 recvBufferSize;/*���ջ������Ĵ�С���ڸ�recvBuffer��ֵ��ʱ������������ֵ*/
	INT8 *dataPktBuffer;//�ڷ������ݱ��ĵ�ʱ���õĻ�����ָ��
	//INT32 recvBytes;
	INT8 controlPktBuffer[TFTP12_CONTROL_PACKET_MAX_SIZE];/*���Ʊ��ĵĻ�������������512�ֽ�*/

	INT32 maxRetransmit;
}TFTP12Description;

// typedef struct _tftp12node
// {
// 	TFTP12Description clientInfo;
// 	struct _tftp12node *next;
// }TFTP12ClientNode;
// 
// 
// typedef struct
// {
// 	INT32 count;
// 	TFTP12ClientNode clientNode;
// } TFTP12ClientInfoList;



/******************************��������**********************************************/
/*����һ��Request���ģ����ر��ĳ���*/
INT32 tftp12CreateREQPkt(TFTP12Description *pktDescriptor);

//��δʵ��
/*����һ��Data���ģ����ر��ĳ���*/
//INT32 tftp12CreateDataPkt(TFTP12Description *pktDescriptor);

/*����һ��ACK���ģ����ر��ĳ���*/
INT32 tftp12CreateACKPkt(TFTP12Description *pktDescriptor, INT32 blockNum);

/*����һ��OACK���ģ����ر��ĳ���*/
INT32 tftp12CreateOACKPkt(TFTP12Description *pktDescriptor);

/*����һ��Error���ģ����ر��ĳ���*/
INT32 tftp12CreateERRPkt(TFTP12Description *pktDescriptor, INT16 errorCode, UINT8 *errorMsg);
/****************************************************************************/

/*****************************��������***********************************************/
/*����Request���ģ����ر��ĳ���*/
INT32 tftp12ParseREQPkt(TFTP12Description *pktDescriptor);

//��δʵ��
/*����һ��Data���ģ����ر��ĳ���*/
//INT32 tftp12ParseDataPkt(TFTP12Description *pktDescriptor);

/*����һ��ACK���ģ����ر��ĳ���*/
INT16 tftp12ParseACKPkt(TFTP12Description *pktDescriptor);

/*����һ��OACK���ģ����ر��ĳ���*/
INT32 tftp12ParseOACKPkt(TFTP12Description *pktDescriptor);

/*����һ��Error���ģ����ر��ĳ���*/
INT32 tftp12ParseERRPkt(TFTP12Description *pktDescriptor, INT16 errorCode, UINT8 *errorMsg);
/****************************************************************************/

char *tftp12CreateDataPkt(char *buf, INT16 blockNum);

INT16 tftp12ParseDataPkt(char *buf, INT32 blockSize);

INT32 test();
