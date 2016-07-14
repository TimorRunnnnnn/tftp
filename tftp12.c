#include "tftp12packet.h"
#include "tftp12header.h"
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "tftp12IObuffer.h"

#define TFTP_PORT		(69)


const INT32 tranModeStrLen[2] = { 8,5 };
// TFTPRequestPacket_t *tftpBuildRequestPacket(INT32 opcode, char *filename, INT32 mode, INT32 blocksize, INT8 timeout, INT32 tsize)
// {
// 
// }


INT32 tftpReadFrom(FILE *file, TFTP12Description_t *request)
{

	// 	if (/*file==NULL||*/request==NULL||request->destFilename==NULL||request->sourceFilename==NULL)
	// 	{
	// 		return FALSE;
	// 	}
	// 	
	// 	if (request->timeout>TFTP12_TIMEOUT_MAX|| request->timeout<TFTP12_TIMEOUT_MIN)
	// 	{
	// 		return FALSE;
	// 	}
	// 	if (request->blksize > TFTP12_BLOCKSIZE_MAX || request->blksize < TFTP12_BLOCKSIZE_MIN)
	// 	{
	// 		return FALSE;
	// 	}
	// 
	// 	INT32 streamLen = 0;
	// 	char blksize_str[10];//int32最大值为10位数
	// 	char timeout_str[10];//不可能到10位数
	// 	INT32 filenameLen = strlen(request->sourceFilename);
	// 
	// 	_itoa(request->blksize, blksize_str, 10);
	// 	_itoa(request->timeout, timeout_str, 10);
	// 
	// 	INT32 blksizeStrLen = strlen(blksize_str);
	// 
	// 	streamLen += 2;//opcode
	// 	streamLen += filenameLen + 1;//filename
	// 	streamLen+= tranModeStrLen[request->transMode] + 1 ;//octet or netascii
	// 	streamLen += strlen("blksize") + 1 + blksizeStrLen + 1;//blksize
	// 	streamLen += strlen("timeout") + 1 + strlen(timeout_str) + 1;//timeout
	// 	streamLen += strlen("tsize") + 1 + 1 + 1;//tsize.0.
	// 
	// 	char *pSendStream = (char *)malloc(streamLen);
	// 	if (pSendStream ==NULL)
	// 	{
	// 		printf("\nmemory alloc failed.");
	// 		return FALSE;
	// 	}
	// 	memset(pSendStream, '\0', streamLen);
	// 
	// 	char *pWalk = pSendStream;
	// 
	// 	*pWalk = 0;
	// 	pWalk++;
	// 	*pWalk = 1;
	// 	pWalk++;
	// 	strcpy(pWalk, request->sourceFilename);
	// 	pWalk += filenameLen+1;
	// 	strcpy(pWalk, transMode[request->transMode]);
	// 	pWalk += tranModeStrLen[request->transMode] + 1;
	// 	strcpy(pWalk, "blksize");
	// 	pWalk += strlen("blksize") + 1;
	// 	strcpy(pWalk, blksize_str);
	// 	pWalk += blksizeStrLen + 1;
	// 	strcpy(pWalk, "timeout");
	// 	pWalk += strlen("timeout") + 1;
	// 	strcpy(pWalk, timeout_str);
	// 	pWalk += strlen(timeout_str) + 1;
	// 	strcpy(pWalk, "tsize");
	// 	pWalk += strlen("tsize") + 1;
	// 	*pWalk = 0;
	// 
	// 	SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	// 	SOCKADDR_IN destAddr;
	// 
	// 	memset(&sendSocket, 0, sizeof(sendSocket));
	// 	memset(&destAddr, 0, sizeof(destAddr));
	// 
	// 	destAddr.sin_addr.S_un.S_addr=inet_addr(request->hostIP);
	// 	destAddr.sin_family = AF_INET;
	// 	destAddr.sin_port = TFTP_PORT;
	return TRUE;
}

INT32 test()
{
	TFTP12Description_t testDesc;
// 
// 	FILE *fd = fopen("test.txt", "w+");
// 	char a[20];
// 	for (INT32 i = 0; i < (1000000 / 500); i++)
// 	{
// 		for (int j = 0; j < 500; j++)
// 		{
// 			// 			_itoa(i, a, 10);
// 			// 			fwrite(a, strlen(a), 1, fd);
// 			fprintf(fd, "1", i % 10);
// 		}
// 	}
// //	fprintf(fd, "123456");
// 	fclose(fd);


	memset(&testDesc, 0, sizeof(testDesc));

	testDesc.option.blockSize = 8192;
	testDesc.option.timeout = 10;
	testDesc.option.tsize = 10000;
	testDesc.filename = "testfilename";
	testDesc.recvBuffer = malloc(TFTP12_BUFFER_SIZE(&testDesc));
	testDesc.writeOrRead = TFTP12_OPCODE_READ_REQUEST;
	//testDesc.openFile = 123;
	testDesc.mode = "ocete";
	tftp12CreateRequestPkt(&testDesc);

	FILE *tFile = fopen("test.txt", "r");

// 	char *rea = malloc(5000000);
// 	INT32 x = fread(rea, 1, 5000000, tFile);
// 	for (INT32 i=0;i<x;i++)
// 	{
// 		if (rea[i]!='1')
// 		{
// 			printf("error");
// 		}
// 	}

	FILE *wFile = fopen("test_copy.txt", "w+");
#define BLKSIZE 500

	if (tFile != NULL)
	{
		fseek(tFile, 0L, SEEK_END);
		INT32 size = ftell(tFile);
		fseek(tFile, 0, SEEK_SET);
		tftp12IOBufferInit(1, BLKSIZE, tFile, size, TFTP12_READ);
		tftp12IOBufferInit(2, BLKSIZE, wFile, size, TFTP12_WRITE);
		INT32 rsize = 0;

		//while (1)
		{
			extern INT32 testNum, testNum2;
			testNum = 0;
			testNum2 = 0;
// 
// 			fseek(tFile, 0, SEEK_SET);
// 			tftp12IOBufferFree(1);
// 			tftp12IOBufferInit(1, 500, tFile, size, TFTP12_READ);
			while (1)
			{
				char *tem = tftp12ReadNextBlock(1, &rsize);
// 				for (INT32 i = 0; i < (rsize-1); i++)
// 				{
// // 					if ((*tem) != (*(tem + i))&&((*(tem+i-1)) != (*(tem + i))+1))
// // 					{
// // 						printf("\n错误,%c -- %c", *tem, *(tem + i));
// // 						//break;
// // 					}
// 				}
				tftp12WriteNextBlock(2, tem, rsize);
				for (INT32 i = 0; i < rsize; i++)
				{
					*(tem + i) = 0;
				}
				if (rsize < BLKSIZE)
				{
					printf("\nfinish\n\n\n");
					break;
				}
				
			}
			//Sleep(100);
		}
	}

// 	fclose(wFile);
// 	fclose(tFile);


	tftp12ExtractOption(testDesc.sendBuffer, 512, &testDesc.option);
	// 	tftp12ExtractOption("timeout\0123\0blksize\01x34\0tsize\0111\0", &testDesc.option);
	// 	tftp12ExtractOption("blksize\01234\0timeout\0123\0tsize\0111\0", &testDesc.option);
	return 1;
}

