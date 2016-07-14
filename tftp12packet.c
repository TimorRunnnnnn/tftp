#include "windows.h"
#include "tftp12packet.h"
#include "tftp12header.h"
#include "stdarg.h"
#include "stdio.h"

#define MAX_DIGITS		(11)

#define STR_BLKSIZE		"blksize"
#define STR_TIMEOUT		"timeout"
#define STR_TSIZE		"tsize"

const char *tftp12ConstStrTransMode[2] = {
	"netascii",
	"octet",
};
const char *tftp12ConstStrOption[3] = {
	STR_BLKSIZE,STR_TIMEOUT,STR_TSIZE
};

static inline INT32 tftp12IntPow(INT32 x, INT32 y) {
	INT32 ret = 1;
	for (INT32 i = 0; i < y; i++)
	{
		ret *= x;
	}
	return ret;
}

static inline INT32 tftp12StrToNum(char *buf)
{
	INT32 cnt = 0;
	char *pBuffer = buf;
	char tem[MAX_DIGITS];
	while (*pBuffer != '\0')
	{
		if ((*pBuffer >= '0') && (*pBuffer <= '9'))
		{
			tem[cnt] = *pBuffer;
			cnt++;
			pBuffer++;
			if (cnt >= MAX_DIGITS)
			{
				/*���ظ�������ʾ����*/
				return -1;
			}
		}
		else
		{
			return -1;
		}

	}

	INT32 num = 0;
	for (INT32 i = 0; i < cnt; i++)
	{
		num += (tem[i] - '0') * tftp12IntPow(10, (cnt - i - 1));
	}
	return num;
}

static inline tftp12StrReplace(char *buf, INT32 len) 
{

	for (INT32 i = 0; i < (len-1); i++)
	{
		if (buf[i] == '\0')
		{
			/*����滻Ϊһ�������ַ�*/
			buf[i] = 1;
		}
	}
}


INT32 tftp12ExtractOption(char *buffer, INT32 len, TFTP12Option *option)
{
	char *position = NULL;
	INT32 num = 0;
	tftp12StrReplace(buffer, len);
	for (INT32 i = 0; i < TFTP12_NUMBER_OF_OPTIONS; i++)
	{
		position = strstr(buffer, tftp12ConstStrOption[i]);
		if (position == NULL)
		{
			continue;
		}
		position++;
		num = tftp12StrToNum(position);
		if (num < 0)
		{
			return ERROR;
		}

		/*���뱣֤TFTP12Option����˳��,��tftp12ConstStrOption�����˳��һ��*/
		((INT32 *)option)[i] = num;
	}
	return TRUE;
}

INT32 tftp12CreateRequestPkt(TFTP12Description_t *desc)
{
	if (desc == NULL)
	{
		return ERROR;
	}

	/*������ָ���Ƿ�Ϊ��*/
	if (desc->filename == NULL || desc->sendBuffer == NULL || desc->mode == NULL)
	{
		return ERROR;
	}

	/*ֻ����opcode = 1��2*/
	if (!(desc->writeOrRead == TFTP12_OPCODE_READ_REQUEST || desc->writeOrRead == TFTP12_OPCODE_WRITE_REQUEST))
	{
		return ERROR;
	}

	memset(desc->sendBuffer, '\0', TFTP12_CONTROL_PACKET_MAX_SIZE);

	char *pWalk = desc->sendBuffer;
	*pWalk = 0;
	pWalk++;
	*pWalk = (INT8)(desc->writeOrRead);
	pWalk++;
	strcpy(pWalk, desc->filename);
	pWalk += strlen(desc->filename) + 1;
	strcpy(pWalk, desc->mode);
	pWalk += strlen(desc->mode) + 1;


	/*INT32�Ϊ10���ַ�*/
	char num[11];

	/*���blksize�ֶ�*/
	strcpy(pWalk, STR_BLKSIZE);
	pWalk += strlen(STR_BLKSIZE) + 1;
	_itoa(desc->option.blockSize, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;

	/*���timeout�ֶ�*/
	strcpy(pWalk, STR_TIMEOUT);
	pWalk += strlen(STR_TIMEOUT) + 1;
	_itoa(desc->option.timeout, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;

	/*���tsize�ֶ�*/
	strcpy(pWalk, STR_TSIZE);
	pWalk += strlen(STR_TSIZE) + 1;
	_itoa(desc->option.tsize, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;
	*pWalk = 0;
	pWalk++;

	/*���ر��ĳ���*/
	return  pWalk - desc->sendBuffer;
}

INT32 tftp12CreateOACKPkt(TFTP12Description_t *desc)
{
	if (desc == NULL)
	{
		return ERROR;
	}

	/*������ָ���Ƿ�Ϊ��*/
	if (desc->sendBuffer == NULL || desc->mode == NULL)
	{
		return ERROR;
	}

	memset(desc->sendBuffer, '\0', TFTP12_BUFFER_SIZE(desc));

	char *pWalk = desc->sendBuffer;
	*pWalk = 0;
	pWalk++;
	*pWalk = (INT8)(TFTP12_OPCODE_OACK);
	pWalk++;

	strcpy(pWalk, desc->filename);
	pWalk += strlen(desc->filename) + 1;
	strcpy(pWalk, desc->mode);
	pWalk += strlen(desc->mode) + 1;


	/*INT32�Ϊ10���ַ�*/
	char num[10];

	/*���blksize�ֶ�*/
	strcpy(pWalk, STR_BLKSIZE);
	pWalk += strlen(STR_BLKSIZE) + 1;
	_itoa(desc->option.blockSize, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;

	/*���timeout�ֶ�*/
	strcpy(pWalk, STR_TIMEOUT);
	pWalk += strlen(STR_TIMEOUT) + 1;
	_itoa(desc->option.timeout, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;

	/*���tsize�ֶ�*/
	strcpy(pWalk, STR_TSIZE);
	pWalk += strlen(STR_TSIZE) + 1;
	_itoa(desc->option.tsize, num, 10);
	strcpy(pWalk, num);
	pWalk += strlen(num) + 1;
	*pWalk = 0;
	pWalk++;

	/*���ر��ĳ���*/
	return  pWalk - desc->sendBuffer;
}