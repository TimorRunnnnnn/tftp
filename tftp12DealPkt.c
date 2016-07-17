//#if _MBCS
#pragma warning(disable:4996)
//#endif // _MBCS



#include <string.h>
#include <winsock.h>

#include "tftp12header.h"

/*
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |filename| 0 |  mode  | 0 |  opt1  | 0 | value1 | 0 |   optN  | 0 | valueN | 0 |
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
*/

#define DEBUG 0	/*�Ƿ����������,Ĭ��Ϊ�����*/
#define PAUSE 0 /*�Ƿ�ִ��һ��������ͣһ��,Ĭ��Ϊ����ͣ*/

typedef int INT32;
typedef short int INT16;


// typedef struct
// {
// 	INT16 writeOrRead;
// 	UINT8 *filename;	//Ŀ���ļ���
// 	FILE *openFile;	//�򿪵��ļ�
// 	UINT8 *mode;	//netascii/octet/mail
// 	TFTP12Option option;
// 	INT32 localPort;	//���س��˿�
// 	INT32 sock;
// 	INT32 transmitBytes;	//�Ѿ�����/���͵��ֽ���
// 	struct sockaddr_in peerAddr;		//�Զ˵�ַ
// 	void *sendBuffer;	//���պͷ��͹��û�����
// }TFTP12Description;

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




/*
Name:tftp12CreateREQPkt
function:create a request packet
input: pktDescriptor
output: packet length
packet format:
    2       n      1     n      1     n      1      n     1      n      1      n     1
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |filename| 0 |  mode  | 0 |  opt1  | 0 | value1 | 0 |   optN  | 0 | valueN | 0 |
+-------+---	printf("length = %d\n", strlen(currentString));~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
1-Read/2-Write   netascii/octet/mail  blockSize               timeout                  tsize
*/
INT32 tftp12CreateREQPkt(TFTP12Description *pktDescriptor)
{
    INT32 preStringLength = 0;  /*�����һ���ֶ�֮ǰ����õ��ֶγ���*/

    /*���opcode:1/2*/
    *((UINT8 *)pktDescriptor->controlPktBuffer) = (pktDescriptor->writeOrRead >> 8) & 0xFF;
    *((UINT8 *)pktDescriptor->controlPktBuffer+1) = pktDescriptor->writeOrRead & 0xFF;
    preStringLength = 2;

    /*����ļ���*/
    if (strlen(pktDescriptor->filename) > 256)
        return -1;  /*�ļ�������*/
    strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, pktDescriptor->filename, strlen(pktDescriptor->filename));   /*����ļ���*/
    *((UINT8 *)pktDescriptor->controlPktBuffer+2+strlen(pktDescriptor->filename)) = '\0';
	preStringLength = preStringLength + strlen(pktDescriptor->filename) + 1;	/*�����ֶγ���*/

	#if DEBUG
	printf("tftp12CreateREQPkt����ļ�����preStringLength = %d\n", preStringLength);
	#endif

    /*���ģʽ, netascii/octet/mail*/
    if (strlen(pktDescriptor->mode) > 8)
        return -1;  /*ģʽ����*/
    strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, pktDescriptor->mode, strlen(pktDescriptor->mode));   /*����ļ���*/
    *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(pktDescriptor->mode)) = '\0';
	preStringLength = preStringLength + strlen(pktDescriptor->mode) + 1;

	#if DEBUG
	printf("tftp12CreateREQPkt���ģʽ��preStringLength = %d\n", preStringLength);
	#endif

    /*���blockSize����ѡ��ֵΪ0ʱ���ǲ���仹����0���˴����õķ�ʽΪ�����*/
    if (pktDescriptor->option.blockSize > 0)
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, "blksize", strlen("blksize"));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen("blksize")) = '\0';
		preStringLength = preStringLength + strlen("blksize") + 1;

		UINT8 value[10] = {0};
		itoa(pktDescriptor->option.blockSize, value, 10);
		strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, value, strlen(value));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(value)) = '\0';
		preStringLength = preStringLength + strlen(value) + 1;

		#if DEBUG
		printf("tftp12CreateREQPkt���blocksize��preStringLength = %d\n", preStringLength);
		#endif
    }

	/*���timeout����ѡ��ֵΪ0ʱ���ǲ���仹����0���˴����õķ�ʽΪ�����*/
    if (pktDescriptor->option.timeout > 0)
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, "timeout", strlen("timeout"));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen("timeout")) = '\0';
		preStringLength = preStringLength + strlen("timeout") + 1;

		UINT8 value[10] = {0};
		itoa(pktDescriptor->option.timeout, value, 10);
		strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, value, strlen(value));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(value)) = '\0';
		preStringLength = preStringLength + strlen(value) + 1;

		#if DEBUG
		printf("tftp12CreateREQPkt���timeout��preStringLength = %d\n", preStringLength);
		#endif
    }

	/*���tSize����ѡ��ֵΪ0ʱ���ǲ���仹����0���˴����õķ�ʽΪ�����*/
  //  if (pktDescriptor->option.tsize > 0)
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, "tSize", strlen("tSize"));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen("tSize")) = '\0';
		preStringLength = preStringLength + strlen("tSize") + 1;

		UINT8 value[10] = {0};
		itoa(pktDescriptor->option.tsize, value, 10);
		strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, value, strlen(value));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(value)) = '\0';
		preStringLength = preStringLength + strlen(value) + 1;

		#if DEBUG
		printf("tftp12CreateREQPkt���tsize��preStringLength = %d\n", preStringLength);
		#endif
    }

    return preStringLength;
}

/*
Name:tftp12CreateACKPkt
function:create a ACK packet
input: pktDescriptor, blockNum
output: packet length
packet format:
    2       2
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |blockNum|                             NULL                                    |
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
 4-ACK
*/
INT32 tftp12CreateACKPkt(TFTP12Description *pktDescriptor, INT32 blockNumber)
{
    UINT8 preStringLength = 0;  /*�����һ���ֶ�֮ǰ����õ��ֶγ���*/

	/*���opcode:4*/
    *((UINT8 *)pktDescriptor->controlPktBuffer) = 0x00;
    *((UINT8 *)pktDescriptor->controlPktBuffer+1) = 0x04;
    preStringLength = 2;

	*((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength) = (blockNumber >> 8)&0xFF;
    *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+1) = blockNumber & 0xFF;
    preStringLength = 4;

	return preStringLength;
}

/*
Name:tftp12CreateOACKPkt
function:create a OACK packet
input: pktDescriptor
output: packet length
packet format:
    2       n      1     n      1                                n      1      n     1
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |  opt1  | 0 | value1 | 0 |          ......             optN  | 0 | valueN | 0 |
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
 6-OACK
*/
INT32 tftp12CreateOACKPkt(TFTP12Description *pktDescriptor)
{
    UINT8 preStringLength = 0;  /*�����һ���ֶ�֮ǰ����õ��ֶγ���*/

	/*���opcode:6*/
    *((UINT8 *)pktDescriptor->controlPktBuffer) = 0x00;
    *((UINT8 *)pktDescriptor->controlPktBuffer+1) = 0x06;
    preStringLength = 2;

    /*���blockSize����ѡ��ֵΪ0ʱ���ǲ���仹����0���˴����õķ�ʽΪ�����*/
    if (pktDescriptor->option.blockSize > 0)
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, "blksize", strlen("blksize"));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen("blksize")) = '\0';
		preStringLength = preStringLength + strlen("blksize") + 1;

		UINT8 value[10] = {0};
		itoa(pktDescriptor->option.blockSize, value, 10);
		strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, value, strlen(value));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(value)) = '\0';
		preStringLength = preStringLength + strlen(value) + 1;

		#if DEBUG
		printf("tftp12CreateOACKPkt���blocksize��preStringLength = %d\n", preStringLength);
		#endif
    }

	/*���timeout����ѡ��ֵΪ0ʱ���ǲ���仹����0���˴����õķ�ʽΪ�����*/
    if (pktDescriptor->option.timeout > 0)
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, "timeout", strlen("timeout"));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen("timeout")) = '\0';
		preStringLength = preStringLength + strlen("timeout") + 1;

		UINT8 value[10] = {0};
		itoa(pktDescriptor->option.timeout, value, 10);
		strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, value, strlen(value));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(value)) = '\0';
		preStringLength = preStringLength + strlen(value) + 1;

		#if DEBUG
		printf("tftp12CreateOACKPkt���timeout��preStringLength = %d\n", preStringLength);
		#endif
    }

	/*���tSize����ѡ��ֵΪ0ʱ���ǲ���仹����0���˴����õķ�ʽΪ�����*/
    if (pktDescriptor->option.tsize > 0)
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, "tSize", strlen("tSize"));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen("tSize")) = '\0';
		preStringLength = preStringLength + strlen("tSize") + 1;

		UINT8 value[10] = {0};
		itoa(pktDescriptor->option.tsize, value, 10);
		strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, value, strlen(value));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(value)) = '\0';
		preStringLength = preStringLength + strlen(value) + 1;

		#if DEBUG
		printf("tftp12CreateOACKPkt���tsize��preStringLength = %d\n", preStringLength);
		#endif
    }


	return preStringLength;
}

/*
Name:tftp12CreateERRPkt
function:create an Error packet
input: pktDescriptor, errorMsg
output: packet length
packet format:
    2       2        n      1
+-------+---~~---+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |errCode |errorMsg| 0 |                          NULL                                   |
+-------+---~~---+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
 5-ERR
*/
INT32 tftp12CreateERRPkt(TFTP12Description *pktDescriptor, INT16 errorCode, UINT8 *errorMsg)
{
    UINT8 preStringLength = 0;  /*�����һ���ֶ�֮ǰ����õ��ֶγ���*/

	/*���opcode:5*/
    *((UINT8 *)pktDescriptor->controlPktBuffer) = 0x00;
    *((UINT8 *)pktDescriptor->controlPktBuffer+1) = 0x05;
    preStringLength = 2;

    /*���errCode:0-7*/
    {
		*((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength) = (errorCode >> 8) & 0xFF;
		*((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+1) = errorCode & 0xFF;
		preStringLength = 4;

		#if DEBUG
		printf("tftp12CreateERRPkt���errorCode��preStringLength = %d\n", preStringLength);
		#endif
    }

	/*���errorMsg*/
    {
        strncpy((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength, errorMsg, strlen(errorMsg));   /*����ļ���*/
        *((UINT8 *)pktDescriptor->controlPktBuffer+preStringLength+strlen(errorMsg)) = '\0';
		preStringLength = preStringLength + strlen(errorMsg) + 1;

		#if DEBUG
		printf("tftp12CreateOACKPkt���errorMsg��preStringLength = %d\n", preStringLength);
		#endif
    }

	return preStringLength;
}





/************************************************************************/

void getNextSegment(UINT8 **currentString, INT32 *preStringLength)
{
    *preStringLength += strlen(*currentString) + 1;
    *currentString = (UINT8 *)(*currentString) + strlen(*currentString) + 1; /*�˴�ע�⣬��ҪŪ����*/

    /*�����⣬����Ѿ�����ĩβ�����һֱ����ȥ*/
    #if 0
    while(**currentString == '\0')
    {
        *preStringLength += 1;
        *currentString = (UINT8 *)(currentString) + 1;
    }
    #endif
}

/*
Name:tftp12ParseREQPkt
function:Parse a request packet
input: pktDescriptor
output: packet length
packet format:
    2       n      1     n      1     n      1      n     1      n      1      n     1
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |filename| 0 |  mode  | 0 |  opt1  | 0 | value1 | 0 |   optN  | 0 | valueN | 0 |
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
1-Read/2-Write   netascii/octet/mail  blockSize               timeout                  tsize
*/
INT32 tftp12ParseREQPkt(TFTP12Description *pktDescriptor)
//INT32 tftp12ParseREQPkt(char *recvBuf,INT32 recvBytes,char *filename,INT32 *mode,TFTP12Option *option)
{
	UINT8 *currentString = 0;
	INT32 preStringLength = 2;

    /*��ȡopcode,���������ݴ�ֵȡ��,R->W,W->R*/
    pktDescriptor->writeOrRead = *((UINT8 *)pktDescriptor->controlPktBuffer);
    pktDescriptor->writeOrRead = (pktDescriptor->writeOrRead << 8) ^ *((UINT8 *)pktDescriptor->controlPktBuffer+1);

	#if DEBUG
	printf("tftp12ParseREQPkt��ȡ����opCode=%d\n", pktDescriptor->writeOrRead);
	#endif

	/*��ȡfileName,���ǵ�����ϵͳ�Դ�Сд������,�ļ���δ���д�Сдת��*/
	currentString = (UINT8 *)pktDescriptor->controlPktBuffer + preStringLength;

    #if DEBUG
	printf("tftp12ParseREQPkt��ȡ����currentString=%s\n", currentString);
	printf("length = %d\n", strlen(currentString));
	#endif
	

	pktDescriptor->filename = (UINT8 *)malloc(strlen(currentString) + 1);
	memset(pktDescriptor->filename, 0, strlen(currentString) + 1);			/*���*/
	strncpy(pktDescriptor->filename, currentString, strlen(currentString));		/*ע��filename�洢�ռ��������ͷ�*/

	#if DEBUG
	printf("tftp12ParseREQPkt��ȡ����filename=%s\n", pktDescriptor->filename);
	#endif

	/*��ȡmode*/

    getNextSegment(&currentString, &preStringLength);       /*ָ����һ���ֶ�*/
	pktDescriptor->mode = (UINT8 *)malloc(currentString + 1);
	memset(pktDescriptor->mode, 0, strlen(currentString) + 1);
	strncpy(pktDescriptor->mode, currentString, strlen(currentString));
	strlwr(pktDescriptor->mode);	/*ת��ΪСд�ַ�*/

	#if DEBUG
	printf("tftp12ParseREQPkt��ȡ����mode=%s\n", pktDescriptor->mode);
	#endif

    /*ѭ����ȡoptionѡ��*/
    getNextSegment(&currentString, &preStringLength);     /*ָ����һ���ֶ�,�ֶ�֮��ֻ��һ��\0�ָ����ж������ֽ�������*/
	while (preStringLength < 511)   /*����λ��*/
	{
		strlwr(currentString);

		#if DEBUG
		if (strlen(currentString) >0)
            printf("tftp12ParseREQPkt��ȡ����option=%s\n", currentString);
		#endif

		if (strnicmp(currentString, "blksize", strlen("blksize")) == 0)
		{
		    /*
			preStringLength += strlen(currentString) + 1;
			currentString = (UINT8 *)pktDescriptor->buffer + preStringLength;
			*/
            getNextSegment(&currentString, &preStringLength);   /*ʹ��getNextSegment����������������*/
			pktDescriptor->option.blockSize = atoi(currentString);	/*���ΪINT32,�����⣬���������Ϊ�ա�������0��ô����*/


			#if DEBUG
			printf("tftp12ParseREQPkt��ȡ����blocksize=%s\n", currentString);
			printf("tftp12ParseREQPkt��ȡ����pktDescriptor->option.blocksize=%d\n", pktDescriptor->option.blockSize);
			#endif
		}
		else if (strnicmp(currentString, "timeout", strlen("timeout")) == 0)
		{
			getNextSegment(&currentString, &preStringLength);

			pktDescriptor->option.timeout = atoi(currentString);	/*���ΪINT32*/

			#if DEBUG
			printf("tftp12ParseREQPkt��ȡ����timeout=%s\n", currentString);
			printf("tftp12ParseREQPkt��ȡ����pktDescriptor->option.timeout=%d\n", pktDescriptor->option.timeout);
			#endif
		}
		else if (strnicmp(currentString, "tsize", strlen("tsize")) == 0)
		{
			getNextSegment(&currentString, &preStringLength);

			pktDescriptor->option.tsize = atoi(currentString);	/*���ΪINT32*/

			#if DEBUG
			printf("tftp12ParseREQPkt��ȡ����tsize=%s\n", currentString);
			printf("tftp12ParseREQPkt��ȡ����pktDescriptor->option.tsize=%d\n", pktDescriptor->option.tsize);
			#endif
		}
		else
			;	/*�����ַ���δ������*/
        getNextSegment(&currentString, &preStringLength);
	}

    return 0;	/*��ʱδ���巵��ֵ*/
}

/*
Name:tftp12ParseACKPkt
function:Parse a ACK packet
input: pktDescriptor
output: packet blockNumber
packet format:
    2       2
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |blockNum|                             NULL                                    |
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
 4-ACK
*/
INT16 tftp12ParseACKPkt(TFTP12Description *pktDescriptor)
{
    INT16 blockNumber = 0;  /*�����һ���ֶ�֮ǰ����õ��ֶγ���*/

	/*��ȡopcode*/
    blockNumber = *((UINT8 *)pktDescriptor->controlPktBuffer+2);
    blockNumber = (blockNumber << 8) ^ *((UINT8 *)pktDescriptor->controlPktBuffer+3);

	#if DEBUG
	printf("tftp12ParseACKPkt�յ��Ĵ��ʮ������blockNumber=%s\n", (UINT8 *)pktDescriptor->controlPktBuffer+2);
	#endif

	return blockNumber;
}

/*
Name:tftp12ParseOACKPkt
function:Parse a OACK packet
input: pktDescriptor
output: option number
packet format:
    2       n      1     n      1                                n      1      n     1
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |  opt1  | 0 | value1 | 0 |          ......             optN  | 0 | valueN | 0 |
+-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
 6-OACK
*/
INT32 tftp12ParseOACKPkt(TFTP12Description *pktDescriptor)
{
	/*�����õ���optionֵ�����ԭʼֵ������Զ˲�֧��option�������Ӧ�ֶε�ֵ*/
	INT32 preStringLength = 2;
    UINT8 argNum = 0;  /*�ڽ���һ���ֶ�֮ǰ�ѽ������ֶγ���*/
	UINT8 *currentString = 0;	/*ָ��ǰ���ָ�����ַ���*/

	/*��ȡopcode:6,û��Ҫ�ٽ��д˲���,���˺���������֪��opcode*/
	#if 0
    *((UINT8 *)pktDescriptor->buffer);
    *((UINT8 *)pktDescriptor->buffer+1);
	#endif

	currentString = (UINT8 *)pktDescriptor->controlPktBuffer;
	currentString = currentString + 2;	/*����opcode*/

	/*Э��option������,�յ��Է���OACK���ݰ������ֵ,����ǰ����0*/
	pktDescriptor->option.blockSize = 0;
	pktDescriptor->option.timeout = 0;
	pktDescriptor->option.tsize = 0;

	while (preStringLength < 511)
	{
		argNum++;
		strlwr(currentString);		/*ת��ΪСд*/

		#if DEBUG
		printf("tftp12ParseOACKPkt��������optionΪ:%s\n", currentString);
		#endif

		if (strnicmp(currentString, "blksize", strlen("blksize") == 0))
		{
			getNextSegment(&currentString, &preStringLength);   /*�ڶ���������δʹ��*/
			pktDescriptor->option.blockSize = atoi(currentString);	/*���ΪINT32,�����⣬���������Ϊ�ա�������0��ô����*/

			#if DEBUG
			printf("tftp12ParseOACKPkt��������blocksize=%s\n", currentString);
			printf("tftp12ParseOACKPkt��������pktDescriptor->option.blocksize=%d\n", pktDescriptor->option.blockSize);
			#endif
		}
		else if (strnicmp(currentString, "timeout", strlen("timeout") == 0))
		{
			getNextSegment(&currentString, &preStringLength);
			pktDescriptor->option.timeout = atoi(currentString);	/*���ΪINT32*/

			#if DEBUG
			printf("tftp12ParseOACKPkt��������timeout=%s\n", currentString);
			printf("tftp12ParseOACKPkt��������pktDescriptor->option.timeout=%d\n", pktDescriptor->option.timeout);
			#endif
		}
		else if (strnicmp(currentString, "tsize", strlen("tsize") == 0))
		{
			getNextSegment(&currentString, &preStringLength);
			pktDescriptor->option.tsize = atoi(currentString);	/*���ΪINT32*/

			#if DEBUG
			printf("tftp12ParseOACKPkt��������tsize=%s\n", currentString);
			printf("tftp12ParseOACKPkt��������pktDescriptor->option.tsize=%d\n", pktDescriptor->option.tsize);
			#endif
		}
		else
			;	/*�����ַ���δ������*/

		getNextSegment(&currentString, &preStringLength);
	}

	return argNum;
}

/*
Name:tftp12ParseERRPkt
function:Parse an Error packet
input: pktDescriptor,errorCode,errorMsg buffer
output: packet length
packet format:
    2       2        n      1
+-------+---~~---+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
|  opc  |errCode |errorMsg| 0 |                          NULL                                   |
+-------+---~~---+---~~---+---+---~~---+---+---~~---+---+---~~---+---+---------+---+---~~---+---+
 5-ERR
*/
INT32 tftp12ParseERRPkt(TFTP12Description *pktDescriptor, INT16 errorCode, UINT8 *errorMsg)
{
	/*��ȡerrCode*/
    errorCode = *((UINT8 *)pktDescriptor->controlPktBuffer+2);
    errorCode = (errorCode << 8) ^ *((UINT8 *)pktDescriptor->controlPktBuffer+3);

	/*��ȡerrorMsg*/
    {
        strncpy((UINT8 *)errorMsg, (UINT8 *)pktDescriptor->controlPktBuffer+4, strlen((UINT8 *)pktDescriptor->controlPktBuffer+4));   /*������Բ��ÿ����ģ�*/
        *(errorMsg+strlen((UINT8 *)pktDescriptor->controlPktBuffer+4)) = '\0';

		#if DEBUG
		printf("tftp12ParseERRPkt��ȡ��(UINT8 *)pktDescriptor->buffer+4Ϊ��%s\n", (UINT8 *)pktDescriptor->controlPktBuffer+4);
		printf("tftp12ParseERRPkt��ȡ��errorMsgΪ��%s\n", errorMsg);
		#endif
    }

	return errorCode;
}

char *tftp12CreateDataPkt(char *buf, INT16 blockNum)
{
	char *pktHead = buf - 4;
	*pktHead = 0;
	*(pktHead+1) = TFTP12_OPCODE_DATA;
	*(INT16 *)(pktHead+2) = htons(blockNum);
	return pktHead;
}

INT16 tftp12ParseDataPkt(char *buf, INT32 blockSize)
{
	INT16 blockNum = 0;
	char *tem = buf;
	tem++;
	if (*tem!=TFTP12_OPCODE_DATA)
	{
		//����
		return -1;
	}
	tem++;
	blockNum = *(INT16*)tem;
	blockNum = htons(blockNum);
	return blockNum;
}

/*
typedef struct
{
	INT16 opCode;
	UINT8 *filename;	//Ŀ���ļ���
	INT32 *openFile;	//�򿪵��ļ�
	UINT8 *mode;	//netascii/octet/mail
	TFTP12Option option;
	INT32 localPort;	//���س��˿�
	INT32 sock;
	INT32 transmitBytes;	//�Ѿ�����/���͵��ֽ���
	struct sockaddr_in peerAddr;		//�Զ˵�ַ
	void *buffer;	//���պͷ��͹��û�����
}TFTP12Description;

*/

int main1()
{
    TFTP12Description newSession;
    TFTP12Description session;
  //  newSession.buffer = malloc(600);
  //  session.buffer = malloc(1000);
    memset(session.controlPktBuffer, 0, 256);
    newSession.filename = (UINT8 *)malloc(256);
    memset(newSession.filename, 0, 256);
    strcpy(newSession.filename, "test_parse.a");
    newSession.writeOrRead = 2;
    newSession.option.blockSize = 255;
    newSession.option.timeout = 10;
    newSession.option.tsize = 512;
    newSession.mode = "octet";

    INT32 count = tftp12CreateREQPkt(&newSession);

    printf("count=%d\n", count);
    memcpy(session.controlPktBuffer, newSession.controlPktBuffer, 600);
    //session.buffer = newSession.buffer;

    printf("session.buffer=%s\n", (UINT8 *)session.controlPktBuffer+2);
    tftp12ParseREQPkt(&session);
    printf("session.opcode=%d\n", session.writeOrRead);
    printf("session.filename=%s\n", session.filename);
        printf("session.mode=%s\n", session.mode);
            printf("session.blockSize=%d\n", session.option.blockSize);
                printf("session.timeout=%d\n", session.option.timeout);
                    printf("session.tsize=%d\n", session.option.tsize);

    printf("***************ACK***************************8\n");

    tftp12CreateACKPkt(&newSession, 511);
    printf("blockNumber = %d\n", tftp12ParseACKPkt(&newSession));
    printf("***************OACK***************************8\n");
    printf("OACK length = %d\n", tftp12CreateOACKPkt(&newSession));
    printf("***************errorMsg***************************8\n");
    tftp12CreateERRPkt(&newSession, 32, "This is an error message!");
    UINT8 *error = (UINT8 *)malloc(256);
    memset(error, 0, 256);
    INT16 code=0;
    printf("������Ϣ���!\n");
    printf("errorCode = %d\n", tftp12ParseERRPkt(&newSession, code, error));
    return 0;
}
