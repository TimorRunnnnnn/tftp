#include "stdio.h"
#include "stdlib.h"
#include "tftp12header.h"
#include "tftp12IObuffer.h"
#include "tftp12Server.h"

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

	INT32 totalTransBytes;
	
	INT32 bufferSize;
	char *nextPosition;
	char *currentReadBuffer;
	char *currentWriteBuffer;
	char *fileEndPosition;/*�ڻ������У�ָ���ļ�������λ��*/
	char *pFree;/*Free��ʱ��ʹ�����ָ��*/
	char temPktHead[4];/*��������д���ʱ���ƻ����ĸ��ֽ�*/

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
{			/*sleep̫���ǣ���Ҫ��*/
	if (node->IOtaskIsRunning == TRUE)
	{
		Sleep(5);
	}
}

void tftp12WaitIOFinishById(INT32 id)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node==NULL)
	{
		return;
	}
	tftp12WaitIOFinish(node);
}


/*����data���ָ��(ָ����ǰ��4���ֽڿ����޸�)*/
char *tftp12ReadNextBlock(INT32 id, INT32 *size)
{
	INT32 diff = 0;
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	char *ret = NULL;

	if (node == NULL)
	{
		*size = 0;
		return NULL;
	}
	if (node->firstRun == TRUE)
	{
		/*����ǵ�һ��������Ҫ������һ��buffer�����*/
		node->firstRun = FALSE;
		node->IOtaskIsRunning = TRUE;
		tftp12IOBufferRequest(node->id);
	}
	 diff = node->fileEndPosition - node->currentReadBuffer;

	/*������������ļ�ĩβ����buffer�Ŀ�ͷ*/
	if (node->nextPosition == node->fileEndPosition)
	{
		/*˵�����ļ�ĩβ��*/
		*size = 0;
		return node->nextPosition;
	}
	else if ((node->endOfFile == TRUE) && (diff >= 0) && (diff < node->bufferSize) && \
		((node->fileEndPosition - node->nextPosition) <= node->blockSize))
	{
		/*˵���ļ������ڱ���������,��С�ڵ���һ��blocksize*/
		*size = node->fileEndPosition - node->nextPosition;
		ret = node->nextPosition;
		node->nextPosition += *size;
		return ret;
	}
	else
	{
		/*ֻҪ�������ļ�ĩβ�ĵط���size������ô��*/
		*size = node->blockSize;

		/*���ָ������Ժ������buff����*/
		if (node->nextPosition == (node->currentReadBuffer + node->bufferSize))
		{
			tftp12WaitIOFinish(node);
			node->IOtaskIsRunning = TRUE;
			/*����������*/
			char *tem = node->currentReadBuffer;
			node->currentReadBuffer = node->currentWriteBuffer;
			node->currentWriteBuffer = tem;
			node->nextPosition = node->currentReadBuffer;
			tftp12IOBufferRequest(node->id);
		}

		/*�п����ļ�ĩβ������һ���������Ŀ�ͷ������ǵĻ����ͷ���0*/
		if (node->nextPosition == node->fileEndPosition)
		{
			/*˵�����ļ�ĩβ��*/
			*size = 0;
			return node->nextPosition;
		}
		else
		{
			ret = node->nextPosition;
			node->nextPosition += *size;
		}
	}
	return ret;
}


/*���ص�ָ��������һ�ε�recvBuffer��buf������ָ�����ݿ�ģ�Ҫ����4���ֽ�*/
//desc->recvBuffer = tftp12WriteNextBlock(desc->localPort, desc->recvBuffer + 4, recvBytes - 4);
char *tftp12WriteNextBlock(INT32 id, char *buf, INT32 writeSize)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL)
	{
		return FALSE;
	}

	/*�ñ����������䱻tftp����ͷ�ƻ�����һƬ��4���ֽ�*/
	*(buf - 4) = node->temPktHead[0];
	*(buf - 3) = node->temPktHead[1];
	*(buf - 2) = node->temPktHead[2];
	*(buf - 1) = node->temPktHead[3];

	if (writeSize > node->blockSize)
	{
		/*��������²����ܳ������*/
		system("pause");
		return FALSE;
	}

	/*���С�ڿ��С��˵���������ļ���β*/
	if (writeSize < node->blockSize)
	{
		printf("\nrecevie final pkt:%d",writeSize);
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
		/*������ǵ�һ��д��buffer���ͱ���ȴ�io������һ�����в���Ҫ����Ϊ�ڶ���buffer�ǿյ�*/
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
		/*�¸�λ�ô�����buffer��ĩβ����������²����ܳ����������*/
		system("pause");
		//return FALSE;
	}

	/*������һƬ���ݵĺ��ĸ��ֽڣ�����һ�����ݱ���ͷ�������ռ�*/
	node->temPktHead[0] = *(node->nextPosition - 4);
	node->temPktHead[1] = *(node->nextPosition - 3);
	node->temPktHead[2] = *(node->nextPosition - 2);
	node->temPktHead[3] = *(node->nextPosition - 1);

	/*��Ϊread��writebuffer���������ڴ��ʱ�����������4���ֽڣ����Բ���Խ��S*/
	return (node->nextPosition - 4);
	//return TRUE;
}

INT32 testNum = 0, testNum2 = 0;;

static  void *  WINAPI tftp12IObufferHandleTask(void *arg)
{
	INT32 writeSize = 0;
	INT32 realWrite = 0;
	INT32 realRead = 0;
	INT32 diff = 0;
	TFTP12IOBufferNode_t *node = (TFTP12IOBufferNode_t*)arg;
	if (node == NULL)
	{
		return NULL;
	}
	if (node->endOfFile == TRUE&&node->rwFlag == TFTP12_READ)
	{
		return NULL;
	}
	if (node->targetFile==NULL)
	{
		return NULL;
	}

	if (node->rwFlag == TFTP12_READ)
	{
		realRead = fread(node->currentWriteBuffer, 1, node->bufferSize, node->targetFile);
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
// 		static INT32 total = 0;
// 		total += realRead;
// 		printf("\nread:%d,%d,total:%d", realRead, testNum, total);
		//testNum++;
		//ReleaseMutex(node->IOMutex);
	}
	else if (node->rwFlag == TFTP12_WRITE)
	{
		
		diff = node->fileEndPosition - node->currentReadBuffer;

		/*����ļ�����ĩβ�����ڱ���д�Ļ�����*/
		if ((node->endOfFile == TRUE) && (diff > 0) && (diff < node->bufferSize))
		{
			writeSize = diff;
		}
		else
		{
			writeSize = node->bufferSize;
		}
		realWrite = fwrite(node->currentReadBuffer, 1, writeSize, node->targetFile);

		node->totalTransBytes += realWrite;
		printf("\nwrite:%d,%d,total:%d", realWrite, testNum, node->totalTransBytes);
		if (realWrite < writeSize)
		{
			printf("\nд�����");
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

	/*���Ѿ���ȡ���ļ�ĩβ��ʱ���ֹ�ٴζ�ȡ*/
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
	/*���ܻ�ʧ�ܣ�Windows�²���*/
	HANDLE useless = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tftp12IObufferHandleTask, node, 0, NULL);
	if (useless == NULL)
	{
		printf("\ncreate io buffer handle task failed");
		node->IOtaskIsRunning = FALSE;
	}
	CloseHandle(useless);
}

/*����ʱ�򷵻����ݿ�ָ�룬д��ʱ�򷵻�recvBuffer*/
char *tftp12IOBufferInit(INT32 id, INT32 blocksize, FILE *file, UNUSED(INT32 fileSize), enum TFTP12_ReadOrWrite rwFlag)
{
	/*Ӧ����ʣ���ڴ��С���ļ���С�����ڴ�ռ�*/
	//����ʱ��1M
	/*���黺����*/
	//INT32 bufSize = ((1*1024*1024) / blocksize)*blocksize + 4;
	char *tem = NULL;
	char *buf = NULL;
	INT32 bufSize = TFTP12_IO_BUFFERSIZE(blocksize);
	TFTP12IOBufferNode_t *node=NULL;
	buf = (char *)malloc(bufSize * 2+8);

	if (buf == NULL)
	{
		return NULL;
	}

	node = (TFTP12IOBufferNode_t*)malloc(sizeof(TFTP12IOBufferNode_t));
	if (node == NULL)
	{
		return NULL;
	}
	memset(node, 0, sizeof(TFTP12IOBufferNode_t));
	node->blockSize = blocksize;
	node->fileSize = fileSize;
	node->id = id;
	node->bufferSize = bufSize;
	node->rwFlag = rwFlag;
	node->pFree = buf;
	node->targetFile = file;

	node->currentReadBuffer = buf + 4;
	node->currentWriteBuffer = buf + bufSize + 8;
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

		/*����������,��Ϊ��һ�ζ�ȡ�������Ƿ���writeBuffer����*/
		tem = node->currentReadBuffer;
		node->currentReadBuffer = node->currentWriteBuffer;
		node->currentWriteBuffer = tem;
		node->nextPosition = node->currentReadBuffer;

		return node->nextPosition;
	}
	else
	{
		node->nextPosition = node->currentWriteBuffer;

		/*�����д���̣��򷵻�writebuffer-4*/
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
	FREE_Z(node->pFree);
	tftp12IOListDelete(node);
	FREE_Z(node);
	return TRUE;
}