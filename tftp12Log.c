#include "tftp12log.h"

INT32 tftp12ClinetLogEnable = FALSE;
INT32 tftp12ServerLogEnable = FALSE;
FILE *clientLogFile = NULL;
FILE *serverLogFile = NULL;


#define TFTP12_MAX_LOGMSG_SIZE		(512)

void logtest()
{
	tftp12LogInit();
}


void tftp12LogInit(void)
{
	clientLogFile = fopen("tftp12ClinetLog.txt", "a+");
	serverLogFile = fopen("tftp12ServerLog.txt", "a+");

	if (clientLogFile == NULL)
	{
		printf("\nopen client logfile failed");
	}
	else
	{
		tftp12ClinetLogEnable = TRUE;
	}
	if (serverLogFile==NULL)
	{
		printf("\nopen server logfile failed");
	}
	else
	{
		tftp12ServerLogEnable = TRUE;
	}
}


static void tftp12LogMsgMain(FILE *file, char *format, ...)
{
	char msg[TFTP12_MAX_LOGMSG_SIZE];
	INT32 msgLen = 0;
	va_list ap;

	/*添加一个终止符，防止访问越界，比内存清零快*/
	msg[TFTP12_MAX_LOGMSG_SIZE - 1] = '\0';
	va_start(ap, format);
	_vsnprintf(msg, TFTP12_MAX_LOGMSG_SIZE, format, ap);
	msgLen = strlen(msg);
	msg[msgLen] = '\n';
	msg[msgLen + 1] = '\0';

	//添加时间戳
	fputs(msg, file);
	fflush(file);
	va_end(ap);
	return;
}

void tftp12ClientLogMsg(char *format, ...)
{
	if (tftp12ClinetLogEnable == TRUE)
	{
		//tftp12LogMsgMain()
	}
}



void tftp12ServerLogMsg(char *format, ...)
{
	char msg[TFTP12_MAX_LOGMSG_SIZE];
	INT32 msgLen = 0;
	va_list ap;

	/*添加一个终止符，防止访问越界，比内存清零快*/
	msg[TFTP12_MAX_LOGMSG_SIZE - 1] = '\0';
	va_start(ap, format);
	_vsnprintf(msg, TFTP12_MAX_LOGMSG_SIZE, format, ap);
	msgLen = strlen(msg);
	msg[msgLen] = '\n';
	msg[msgLen + 1] = '\0';
	if (tftp12ServerLogEnable == TRUE)
	{
		//添加时间戳
		fputs(msg, serverLogFile);
		fflush(serverLogFile);
	}
	va_end(ap);
	return;
}
void tftp12ServerLogTask(void *arg)
{

}
