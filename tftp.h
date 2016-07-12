#ifndef _TFTP_H_
#define _TFTP_H_

#include "windows.h"

#define TFTP12_BLOCKSIZE_MAX		(8192*2)
#define TFTP12_BLOCKSIZE_MIN		(512)

#define TFTP12_TIMEOUT_MAX		(256)
#define TFTP12_TIMEOUT_MIN		(1)


enum TFTP12_TRANS_MODE
{
	TFTP12_NETASCII = 0,
	TFTP12_OCTET,
};

enum TFTP12_OPCODE
{
	TFTP12_READ_REQUEST = 1,
	TFTP12_WRITE_REQUEST,
	TFTP12_DATA,
	TFTP12_ACKNOWLEDGMENT,
	TFTP12_TFTP_ERROR,
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
};

typedef struct
{
	INT32 blockSize;
	INT32 timeout;
	INT32 tsize;
}TFTP12Option;


typedef struct
{
	INT16 opCode;
	INT8 *filename;	//目的文件名
	FILE *openFile;	//打开的文件
	INT8 *mode;	//netascii/octet/mail
	TFTP12Option option;
	INT32 localPort;	//本地出端口
	INT32 sock;
	INT32 transmitBytes;	//已经接收/发送的字节数
	struct sockaddr_in peerAddr;		//对端地址
	INT8 *buffer;	//接收和发送共用缓冲区
}TFTP12Description;

typedef struct _tftp12node
{
	TFTP12Description clientInfo;
	struct _tftp12node *next;
}TFTP12ClientNode;


typedef struct
{
	INT32 count;
	TFTP12ClientNode clientNode;
} TFTP12ClientInfoList;


INT32 test();


#endif