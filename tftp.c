#include "tftp.h"
#include "stdlib.h"
#include "stdio.h"
#include "time.h"

#define TFTP_PORT		(69)

const char *transMode[2] = {
	"netascii",
	"octet",
};
const INT32 tranModeStrLen[2] = {8,5};
// TFTPRequestPacket_t *tftpBuildRequestPacket(INT32 opcode, char *filename, INT32 mode, INT32 blocksize, INT8 timeout, INT32 tsize)
// {
// 
// }



INT32 tftpReadFrom(FILE *file, TFTP12Request_t *request)
{
	if (/*file==NULL||*/request==NULL||request->destFilename==NULL||request->sourceFilename==NULL)
	{
		return FALSE;
	}
	
	if (request->timeout>TFTP12_TIMEOUT_MAX|| request->timeout<TFTP12_TIMEOUT_MIN)
	{
		return FALSE;
	}
	if (request->blksize > TFTP12_BLOCKSIZE_MAX || request->blksize < TFTP12_BLOCKSIZE_MIN)
	{
		return FALSE;
	}

	INT32 streamLen = 0;
	char blksize_str[10];//int32最大值为10位数
	char timeout_str[10];//不可能到10位数
	INT32 filenameLen = strlen(request->sourceFilename);

	_itoa(request->blksize, blksize_str, 10);
	_itoa(request->timeout, timeout_str, 10);

	INT32 blksizeStrLen = strlen(blksize_str);

	streamLen += 2;//opcode
	streamLen += filenameLen + 1;//filename
	streamLen+= tranModeStrLen[request->transMode] + 1 ;//octet or netascii
	streamLen += strlen("blksize") + 1 + blksizeStrLen + 1;//blksize
	streamLen += strlen("timeout") + 1 + strlen(timeout_str) + 1;//timeout
	streamLen += strlen("tsize") + 1 + 1 + 1;//tsize.0.

	char *pSendStream = (char *)malloc(streamLen);
	if (pSendStream ==NULL)
	{
		printf("\nmemory alloc failed.");
		return FALSE;
	}
	memset(pSendStream, '\0', streamLen);

	char *pWalk = pSendStream;

	*pWalk = 0;
	pWalk++;
	*pWalk = 1;
	pWalk++;
	strcpy(pWalk, request->sourceFilename);
	pWalk += filenameLen+1;
	strcpy(pWalk, transMode[request->transMode]);
	pWalk += tranModeStrLen[request->transMode] + 1;
	strcpy(pWalk, "blksize");
	pWalk += strlen("blksize") + 1;
	strcpy(pWalk, blksize_str);
	pWalk += blksizeStrLen + 1;
	strcpy(pWalk, "timeout");
	pWalk += strlen("timeout") + 1;
	strcpy(pWalk, timeout_str);
	pWalk += strlen(timeout_str) + 1;
	strcpy(pWalk, "tsize");
	pWalk += strlen("tsize") + 1;
	*pWalk = 0;

	SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN destAddr;

	memset(&sendSocket, 0, sizeof(sendSocket));
	memset(&destAddr, 0, sizeof(destAddr));

	destAddr.sin_addr.S_un.S_addr=inet_addr(request->hostIP);
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = TFTP_PORT;

	

	return TRUE;
}

INT32 test()
{
	TFTP12Request_t xx;
	xx.destFilename = "1.exe";
	xx.sourceFilename = "1.exe";
	strcpy_s(xx.hostIP, 16, "127.0.0.1");
	xx.transMode = TFTP12_OCTET;
	xx.timeout = 10;
	xx.tsize = 0;
	xx.blksize = 512;
	FILE *xxx = NULL;
	tftpReadFrom(xxx, &xx);
}



INT32 tftpWriteTo(FILE *file)
{}