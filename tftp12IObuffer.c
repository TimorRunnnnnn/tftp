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
	// 	char *fileEndPosition;/*�ڻ������У�ָ���ļ�������λ��*/
	char *pFree;/*Free��ʱ��ʹ�����ָ��*/


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

/*****************************************************************
* DESCRIPTION:
*	���ļ�����һ��
* INPUTS:
*	id - ÿ��io������������Ψһ��id
* OUTPUTS:
*	size - ��ȡ�˶����ֽ�
* RETURNS:
*	���ػ�������
*****************************************************************/
char *tftp12ReadNextBlock(INT32 id, INT32 *size)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL)
	{
		return NULL;
	}
	*size = fread(node->currentReadBuffer, 1, node->blockSize, node->targetFile);
	return node->currentReadBuffer;
}

/*****************************************************************
* DESCRIPTION:
*	���ļ�����һ��
* INPUTS:
*	id - ÿ��io������������Ψһ��id
*	buf - д����̵Ļ�����
*	writeSize - ��Ҫд������ֽ�
* OUTPUTS:
*	none
* RETURNS:
*	д���˶����ֽ�
*****************************************************************/
INT32 tftp12WriteNextBlock(INT32 id, char *buf, INT32 writeSize)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL || writeSize == 0)
	{
		return 0;
	}
	return fwrite(buf, 1, writeSize, node->targetFile);
}


/*****************************************************************
* DESCRIPTION:
*	��ʼ������IO����������optionЭ�����֮�����
* INPUTS:
*	id - ÿ��io������������Ψһ��id,�ñ��صķ��Ͷ˿���id
*	blocksize - ���С
*	file - �Ѿ��򿪵��ļ����壨��ʱû�õ���
*	fileSize - ��㴫
*	rwFlag - ������д�ı�־����(TFTP12_OPCODE_READ_REQUEST)����(TFTP12_READ)������
* OUTPUTS:
*	none
* RETURNS:
*	NULL - ���ִ���
*	��������·��ؽ��ջ�������ַ(��ʱ,Ҫ�ģ�
*****************************************************************/
char *tftp12IOBufferInit(INT32 id, INT32 blocksize, FILE *file, INT32 fileSize, enum TFTP12_ReadOrWrite rwFlag)
{
	/*Ӧ����ʣ���ڴ��С���ļ���С�����ڴ�ռ�*/
	//����ʱ��1M
	/*���黺����*/
	//INT32 bufSize = ((2500) / blocksize)*blocksize + 4;

	/*���id�Ѿ�����*/
	if (tftp12FindNodeByid(id)!=NULL)
	{
		return NULL;
	}

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
	// 		/*����������,��Ϊ��һ�ζ�ȡ�������Ƿ���writeBuffer����*/
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

/*����,��ʱ��Ҫ��*/
INT32 tftp12IOBufferFree(INT32 id)
{
	TFTP12IOBufferNode_t *node = tftp12FindNodeByid(id);
	if (node == NULL)
	{
		return FALSE;
	}
	//free(node->pFree);
	tftp12IOListDelete(node);
	free(node);
	return TRUE;
}