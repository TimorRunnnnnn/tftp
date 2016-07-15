#include "stdio.h"
#include "stdlib.h"
#include "tftp12header.h"



#include "windows.h"


typedef struct _iobuffnode
{
	INT32 id;
	INT32 blockSize;
	// 	INT32 endOfFile;
	// 	INT32 fileSize;
	FILE *targetFile;
	// 	enum TFTP12_ReadOrWrite rwFlag;
	// 	INT32 firstRun;
	// 	INT32 IOtaskIsRunning;
	// 
	// 	INT32 bufferSize;
	// 	char *nextPosition;
	char *currentReadBuffer;
	char *currentWriteBuffer;
	// 	char *fileEndPosition;/*在缓冲区中，指向文件结束的位置*/
	char *pFree;/*Free的时候使用这个指针*/


	struct _iobuffnode *next;
}TFTP12IOBufferNode_t;

TFTP12IOBufferNode_t *head = NULL;


/*static void tftp12IOBufferRequest(INT32 id);*/

static void tftp12IOListInsert(TFTP12IOBufferNode_t *node)
{
	if (head == NULL)
	{
		head = node;
		return;
	}
	TFTP12IOBufferNode_t *pWalk = head;
	while (pWalk->next != NULL)
	{
		pWalk = pWalk->next;
	}
	pWalk->next = node;
}

static INT32 tftp12IOListDelete(TFTP12IOBufferNode_t *node)
{
	TFTP12IOBufferNode_t *pWalk = head;
	if (head == node)
	{
		head = head->next;
		return TRUE;
	}
	while (pWalk->next != NULL)
	{
		if (pWalk->next == node)
		{
			pWalk->next = pWalk->next->next;
			return TRUE;
		}
		pWalk = pWalk->next;
	}
	return FALSE;
}


static TFTP12IOBufferNode_t *tftp12FindNodeByid(INT32 id)
{
	TFTP12IOBufferNode_t *pWalk = head;
	while (pWalk != NULL)
	{
		if (pWalk->id == id)
		{
			return pWalk;
		}
		pWalk = pWalk->next;
	}
	return NULL;
}



char *tftp12ReadNextBlock(INT32 id, INT32 *size)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL)
	{
		return NULL;
	}
	*size = fread(node->currentReadBuffer, 1, node->blockSize, node->targetFile);

	for (INT32 i = 0; i < *size; i++)
	{
		if (node->currentReadBuffer[i] != '1')
		{
			printf("\nerror");
		}
	}

	return node->currentReadBuffer;
}

INT32 tftp12WriteNextBlock(INT32 id, char *buf, INT32 writeSize)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL || writeSize == 0)
	{
		return 0;
	}
	for (INT32 i = 0; i < writeSize; i++)
	{
		if (buf[i] != '1')
		{
			printf("\nerror");
		}
	}

	return fwrite(buf, 1, writeSize, node->targetFile);
}

char *tftp12IOBufferInit(INT32 id, INT32 blocksize, FILE *file, INT32 fileSize, enum TFTP12_ReadOrWrite rwFlag)
{
	/*应根据剩余内存大小和文件大小分配内存空间*/
	//测试时用1M
	/*两块缓冲区*/
	//INT32 bufSize = ((2500) / blocksize)*blocksize + 4;
	INT32 bufSize = (blocksize + 4);

	char *buf = (char *)malloc(bufSize);
	if (buf == NULL)
	{
		return NULL;
	}

	TFTP12IOBufferNode_t *node = (TFTP12IOBufferNode_t*)malloc(sizeof(TFTP12IOBufferNode_t));
	if (node == NULL)
	{
		return NULL;
	}
	memset(node, 0, sizeof(TFTP12IOBufferNode_t));
	node->blockSize = blocksize;
	//node->fileSize = fileSize;
	node->id = id;
	//node->bufferSize = bufSize - 4;
	//node->rwFlag = rwFlag;
	node->pFree = buf;
	node->targetFile = file;

	node->currentReadBuffer = buf + 4;
	node->currentWriteBuffer = buf + 4;

	// 	node->rwStart = node->pBuf + 4;
	// 	node->currentRead = node->rwStart;
	// 	node->currentWrite = node->rwStart;

/*	node->IOMutex = CreateMutex(NULL, FALSE, "");*/
	tftp12IOListInsert(node);

	// 	if (rwFlag == TFTP12_READ)
	// 	{
	// 		node->IOtaskIsRunning = TRUE;
	// 		tftp12IOBufferRequest(id);
	// 		while (node->IOtaskIsRunning == TRUE)
	// 		{
	// 			Sleep(5);
	// 		}
	// 		/*交换缓冲区,因为第一次读取的数据是放在writeBuffer里面*/
	// 		char *tem = node->currentReadBuffer;
	// 		node->currentReadBuffer = node->currentWriteBuffer;
	// 		node->currentWriteBuffer = tem;
	// 		node->nextPosition = node->currentReadBuffer;
	// 	}
	// 	else
	// 	{
	// 		node->nextPosition = node->currentWriteBuffer;
	// 	}
	// 	node->firstRun = TRUE;
		/*
			WaitForSingleObject(node->IOMutex, INFINITE);
			ReleaseMutex(node->IOMutex);*/
	return node->currentWriteBuffer;
}

INT32 tftp12IOBufferFree(INT32 id)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL)
	{
		return FALSE;
	}
	free(node->pFree);
	tftp12IOListDelete(node);
	free(node);
	return TRUE;
}