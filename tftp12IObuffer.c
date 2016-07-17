#include "stdio.h"
#include "stdlib.h"
#include "tftp12header.h"



#include "windows.h"


typedef struct _iobuffnode
{
	INT32 id;
	INT32 blockSize;
	INT32 endOfFile;
	INT32 fileSize;
	FILE *targetFile;
	enum TFTP12_ReadOrWrite rwFlag;
	INT32 firstRun;
	INT32 IOtaskIsRunning;

	INT32 bufferSize;
	char *nextPosition;
	char *currentReadBuffer;
	char *currentWriteBuffer;
	char *fileEndPosition;/*在缓冲区中，指向文件结束的位置*/
	char *pFree;/*Free的时候使用这个指针*/
	char temPktHead[4];/*用来保存写入的时候被破坏的四个字节*/

	struct _iobuffnode *next;
}TFTP12IOBufferNode_t;

TFTP12IOBufferNode_t *head = NULL;


static void tftp12IOBufferRequest(INT32 id);

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

static void tftp12WaitIOFinish(TFTP12IOBufferNode_t *node)
{			/*sleep太弱智，需要换*/
	if (node->IOtaskIsRunning == TRUE)
	{
		Sleep(5);
	}
}

char *tftp12ReadNextBlock(INT32 id, INT32 *size)
{
	static INT32 times111 = 0;
	times111++;
	if (times111 == 82)
	{
		times111++;
	}
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	char *ret = NULL;

	if (node == NULL)
	{
		*size = 0;
		return NULL;
	}
	if (node->firstRun == TRUE)
	{
		/*如果是第一次运行需要将另外一个buffer填充满*/
		node->firstRun = FALSE;
		tftp12IOBufferRequest(node->id);
	}
	INT32 diff = node->fileEndPosition - node->currentReadBuffer;

	/*这种情况属于文件末尾不在buffer的开头*/
	if (node->nextPosition == node->fileEndPosition)
	{
		/*说明到文件末尾了*/
		*size = 0;
		return node->nextPosition;
	}
	else if ((node->endOfFile == TRUE) && (diff >= 0) && (diff < node->bufferSize) && \
		((node->fileEndPosition - node->nextPosition) <= node->blockSize))
	{
		/*说明文件结束在本缓冲区内,且小于等于一个blocksize*/
		*size = node->fileEndPosition - node->nextPosition;
		ret = node->nextPosition;
		node->nextPosition += *size;
		return ret;
	}
	else
	{
		/*只要不是在文件末尾的地方，size都是这么大*/
		*size = node->blockSize;

		/*如果指针加了以后等于了buff结束*/
		if (node->nextPosition == (node->currentReadBuffer + node->bufferSize))
		{
			tftp12WaitIOFinish(node);
			node->IOtaskIsRunning = TRUE;
			/*交换缓冲区*/
			char *tem = node->currentReadBuffer;
			node->currentReadBuffer = node->currentWriteBuffer;
			node->currentWriteBuffer = tem;
			node->nextPosition = node->currentReadBuffer;
			tftp12IOBufferRequest(node->id);
		}

		/*有可能文件末尾在另外一个缓冲区的开头，如果是的话，就返回0*/
		if (node->nextPosition == node->fileEndPosition)
		{
			/*说明到文件末尾了*/
			*size = 0;
			return node->nextPosition;
		}
		else
		{
			ret = node->nextPosition;
			node->nextPosition += *size;
		}
	}
	// 	for (INT32 i = 0; i < *size; i++)
	// 	{
	// 		if (*(ret + i) != '1')
	// 		{
	// 			printf("%c", *(node->currentWriteBuffer + i));
	// 		}
	// 	}
	return ret;
}

char *tftp12WriteNextBlock(INT32 id, char *buf, INT32 writeSize)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL)
	{
		return FALSE;
	}

	/*用保存的数据填充被tftp报文头破坏的上一片的4个字节*/
	*(buf - 4) = node->temPktHead[0];
	*(buf - 3) = node->temPktHead[1];
	*(buf - 2) = node->temPktHead[2];
	*(buf - 1) = node->temPktHead[3];

	if (writeSize > node->blockSize)
	{
		/*正常情况下不可能出现这个*/
		system("pause");
		return FALSE;
	}

	/*如果小于块大小，说明包括了文件结尾*/
	if (writeSize < node->blockSize)
	{
		node->endOfFile = TRUE;
		node->fileEndPosition = node->nextPosition + writeSize;
	}
	//memcpy(node->nextPosition, buf, writeSize);
	node->nextPosition += writeSize;

	/*
	if (node->endOfFile == TRUE)
	{
	tftp12IOBufferRequest(id);
	}
	else*/ if ((node->nextPosition == (node->currentWriteBuffer + node->bufferSize)) || (node->endOfFile == TRUE))
	{
		/*如果不是第一次写满buffer，就必须等待io结束第一次运行不需要，因为第二个buffer是空的*/
		// 			if (node->firstRun == FALSE)
		// 			{
		tftp12WaitIOFinish(node);
		node->IOtaskIsRunning = TRUE;
		//}
		// 			for (INT32 i = 0; i < node->bufferSize; i++)
		// 			{
		// 				if (*(node->currentWriteBuffer+i)!='1')
		// 				{
		// 					printf("%c", *(node->currentWriteBuffer + i));
		// 				}
		// 			}

		char *tem = node->currentReadBuffer;
		node->currentReadBuffer = node->currentWriteBuffer;
		node->currentWriteBuffer = tem;
		node->nextPosition = node->currentWriteBuffer;
		// 			if (node->firstRun == TRUE)
		// 			{
		// 				node->firstRun = FALSE;
		// 			}
		// 			else
		// 			{
		tftp12IOBufferRequest(id);
		//}
	}
	else if (node->nextPosition > (node->currentWriteBuffer + node->bufferSize))
	{
		/*下个位置大于了buffer的末尾，正常情况下不可能出现这种情况*/
		system("pause");
		//return FALSE;
	}

	/*保存上一片数据的后四个字节，给下一个数据报文头部留出空间*/
	node->temPktHead[0] = *(node->nextPosition - 4);
	node->temPktHead[1] = *(node->nextPosition - 3);
	node->temPktHead[2] = *(node->nextPosition - 2);
	node->temPktHead[3] = *(node->nextPosition - 1);

	/*因为read和writebuffer都在申请内存的时候多留出来了4个字节，所以不会越界S*/
	return (node->nextPosition - 4);
	//return TRUE;
}

INT32 testNum = 0, testNum2 = 0;;

static  void *  WINAPI tftp12IObufferHandleTask(void *arg)
{
	static INT32 timess = 0;
	timess++;
	static INT32 erro11r = 0;
	TFTP12IOBufferNode_t *node = (TFTP12IOBufferNode_t*)arg;
	if (node == NULL)
	{
		return NULL;
	}
	if (node->endOfFile == TRUE&&node->rwFlag == TFTP12_READ)
	{
		return NULL;
	}
	if (node->rwFlag == TFTP12_READ)
	{
		INT32 realRead = fread(node->currentWriteBuffer, 1, node->bufferSize, node->targetFile);
		if (realRead < node->bufferSize)
		{
			node->endOfFile = TRUE;
			node->fileEndPosition = node->currentWriteBuffer + realRead;
		}

		// 		for (INT32 i = 0; i < realRead; i++)
		// 		{
		// 			if (*(node->currentWriteBuffer + i) != '1')
		// 			{
		// 				printf("%c", *(node->currentWriteBuffer + i));
		// 				
		// 				erro11r++;
		// 			}
		// 		}
		static INT32 total = 0;
		total += realRead;
		printf("\nread:%d,%d,total:%d", realRead, testNum, total);
		//testNum++;
		//ReleaseMutex(node->IOMutex);
	}
	else if (node->rwFlag == TFTP12_WRITE)
	{
		INT32 writeSize = 0;
		INT32 diff = node->fileEndPosition - node->currentReadBuffer;
		static INT32 totalWrite = 0;
		/*如果文件到了末尾，且在本次写的缓冲内*/
		if (node->endOfFile == TRUE)
		{
			node->endOfFile = TRUE;
		}
		if ((node->endOfFile == TRUE) && (diff > 0) && (diff < node->bufferSize))
		{
			writeSize = diff;
		}
		else
		{
			writeSize = node->bufferSize;
		}
		INT32 realWrite = fwrite(node->currentReadBuffer, 1, writeSize, node->targetFile);
		totalWrite += realWrite;
		printf("\nwrite:%d,%d,total:%d", realWrite, testNum, totalWrite);
		if (realWrite < writeSize)
		{
			printf("\n写入错误");
		}
	}
	else
	{
		return NULL;
	}

	node->IOtaskIsRunning = FALSE;
	return NULL;
}


static void tftp12IOBufferRequest(INT32 id)
{

	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);

	/*在已经读取到文件末尾的时候禁止再次读取*/
	if (node->endOfFile == TRUE&&node->rwFlag == TFTP12_READ)
	{
		return;
	}

	if (node->rwFlag == TFTP12_READ)
	{
	}

	//printf("\nrequest id=%d:%d", id, testNum);
	//testNum++;

	/*	printf("%d", WaitForSingleObject(node->IOMutex, INFINITE));*/
	/*可能会失败，Windows下不管*/
	HANDLE useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12IObufferHandleTask, node, 0, NULL);
	if (useless == NULL)
	{
		node->IOtaskIsRunning = FALSE;
	}
	CloseHandle(useless);
}

char *tftp12IOBufferInit(INT32 id, INT32 blocksize, FILE *file, INT32 fileSize, enum TFTP12_ReadOrWrite rwFlag)
{
	/*应根据剩余内存大小和文件大小分配内存空间*/
	//测试时用1M
	/*两块缓冲区*/
	INT32 bufSize = ((1*1024*1024) / blocksize)*blocksize + 4;

	char *buf = (char *)malloc(bufSize * 2);
	if (buf == NULL)
	{
		return FALSE;
	}

	TFTP12IOBufferNode_t *node = (TFTP12IOBufferNode_t*)malloc(sizeof(TFTP12IOBufferNode_t));
	if (node == NULL)
	{
		return FALSE;
	}
	memset(node, 0, sizeof(TFTP12IOBufferNode_t));
	node->blockSize = blocksize;
	node->fileSize = fileSize;
	node->id = id;
	node->bufferSize = bufSize - 4;
	node->rwFlag = rwFlag;
	node->pFree = buf;
	node->targetFile = file;

	node->currentReadBuffer = buf + 4;
	node->currentWriteBuffer = buf + bufSize + 4;

	// 	node->rwStart = node->pBuf + 4;
	// 	node->currentRead = node->rwStart;
	// 	node->currentWrite = node->rwStart;

	/*	node->IOMutex = CreateMutex(NULL, FALSE, "");*/
	tftp12IOListInsert(node);

	node->firstRun = TRUE;
	if (rwFlag == TFTP12_READ)
	{
		node->IOtaskIsRunning = TRUE;
		tftp12IOBufferRequest(id);
		while (node->IOtaskIsRunning == TRUE)
		{
			Sleep(5);
		}
		/*交换缓冲区,因为第一次读取的数据是放在writeBuffer里面*/
		char *tem = node->currentReadBuffer;
		node->currentReadBuffer = node->currentWriteBuffer;
		node->currentWriteBuffer = tem;
		node->nextPosition = node->currentReadBuffer;

		return node->nextPosition;
	}
	else
	{
		node->nextPosition = node->currentWriteBuffer;

		/*如果是写磁盘，则返回writebuffer-4*/
		return node->nextPosition - 4;
	}


	//return 1;
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