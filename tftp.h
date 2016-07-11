#ifndef _TFTP_H_
#define _TFTP_H_

#include "windows.h"

#define TFTP12_BLOCKSIZE_MAX		(8192)
#define TFTP12_BLOCKSIZE_MIN		(512)

#define TFTP12_TIMEOUT_MAX		(99)
#define TFTP12_TIMEOUT_MIN		(1)


enum TFTP_TRANS_MODE
{
	TFTP12_NETASCII=0,
	TFTP12_OCTET,
};

enum TFTP_OPCODE
{
	TFTP12_READ_REQUEST = 1,
	TFTP12_WRITE_REQUEST,
	TFTP12_DATA,
	TFTP12_ACKNOWLEDGMENT,
	TFTP12_TFTP_ERROR,
};
enum TFTP_ERROR_CODE
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


typedef struct 
{
	char hostIP[16];
	char *sourceFilename;
	char *destFilename;
	INT32 transMode;
	INT32 blksize;
	INT32 timeout;
	INT32 tsize;
}TFTP12Request_t;

// typedef struct
// {
// 	INT16 opcode;
// 	INT16 blockNum;
// }TFTPAck_t;
// 
// typedef struct
// {
// 	INT16 opcode;
// 	char *option;
// }TFTPOAck_t;
// 
// typedef struct 
// {
// 
// };

// typedef struct _tftpRequestPacket
// {
// 	INT16 opcode;
// 	char *filename;
// 	char *mode;
// 	char *option;
// }TFTPRequestPacket_t;



INT32 test();


#endif