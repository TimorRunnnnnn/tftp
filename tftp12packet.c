#include "windows.h"
#include "tftp12packet.h"
#include "tftp.h"
#include "stdarg.h"
#include "stdio.h"

const char *transMode[2] = {
	"netascii",
	"octet",
};

#define STR_BLKSIZE		"blksize"
#define STR_TIMEOUT		"timeout"
#define STR_TSIZE		"tsize"


INT32 tftp12CreateRequestPkt(TFTP12Description_t *desc)
{
	if (desc == NULL)
	{
		return ERROR;
	}

	/*检查各个指针是否为空*/
	if (desc->filename == NULL || desc->buffer == NULL || desc->openFile == NULL||desc->mode==NULL)
	{
		return ERROR;
	}

	/*只接受opcode = 1或2*/
	if (!(desc->opCode == TFTP12_READ_REQUEST || desc->opCode == TFTP12_WRITE_REQUEST))
	{
		return ERROR;
	}

	memset(desc->buffer, '\0', TFTP12_BUFFER_SIZE(desc));

	char *pWalk = desc->buffer;

	*pWalk = 0;
	pWalk++;
	*pWalk = (INT8 )(desc->opCode);
	pWalk ++;
	strcpy(pWalk, desc->filename);
	pWalk += strlen(desc->filename) + 1;
	strcpy(pWalk, desc->mode);
	pWalk += strlen(desc->mode)+ 1;
	strcpy(pWalk, STR_BLKSIZE);
	pWalk += strlen(STR_BLKSIZE) + 1;

	/*INT32最长为10个字符*/
	char num[10];
	_itoa(desc->option.blockSize, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;

	strcpy(pWalk, STR_TIMEOUT);
	pWalk += strlen(STR_TIMEOUT) + 1;
	_itoa(desc->option.timeout, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;

	strcpy(pWalk, STR_TSIZE);
	pWalk += strlen(STR_TSIZE) + 1;
	_itoa(desc->option.tsize, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;
	*pWalk = 0;
	pWalk++;
	
	INT32 ret = pWalk - desc->buffer;
	return ret;
}